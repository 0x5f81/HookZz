## HookZz

**hook framework**. 

**ref to: [frida-gum](https://github.com/frida/frida-gum) and [minhook](https://github.com/TsudaKageyu/minhook) and [substrate](https://github.com/jevinskie/substrate). special thanks to `frida-gum's` perfect code and modular architecture**.

**still developing, for arm64/IOS now!**

## Hook架构设计

一般来说可以分为以下几个模块

1. 内存分配 模块
2. 指令写 模块
3. 指令读 模块
4. 指令修复 模块
5. 跳板 模块
6. 调度器 模块

#### 1. 内存分配 模块

需要分配部分内存用于写入指令, 这里需要关注两个函数都是关于内存属性相关的. 1. 如何使内存 `可写` 2. 如何使内存 `可执行`

这一部分与具体的操作系统有关. 比如 `darwin` 使用 `mach_vm_allocate`

在 lldb 中可以通过 `memory region address` 查看地址的内存属性.

当然这里也存在一个巨大的坑, IOS 下无法分配 `rwx` 属性的内存页. 这导致 inlinehook 无法在非越狱系统上使用, 并且只有 `MobileSafari` 才有 `VM_FLAGS_MAP_JIT` 权限. 具体解释请参下方 [坑-rwx 与 codesigning].

#### 2. 指令写 模块

先说坑,  非越狱状态下不允许设置 `rw-` 为 `r-x`, 或者  设置 `r-x` 为 `rx-`. 具体解释请参考下方坑 [坑-rwx 与 codesigning].

其实这里的指令写有种简单的方法, 就是在本地生成指令的16进制串, 之后直接写即可. 但这种应该是属于 hardcode.

这里使用 `frida-gum` 和 `CydiaSubstrace` 都用的方法, 把需要用到的指令都写成一个小函数.

例如:

```
void
gum_arm64_writer_put_ldr_reg_address (GumArm64Writer * self,
                                      arm64_reg reg,
                                      GumAddress address)
{
  gum_arm64_writer_put_ldr_reg_u64(self, reg, (guint64)address);
}

void
gum_arm64_writer_put_ldr_reg_u64 (GumArm64Writer * self,
                                  arm64_reg reg,
                                  guint64 val)
{
  GumArm64RegInfo ri;

  gum_arm64_writer_describe_reg(self, reg, &ri);

  g_assert_cmpuint(ri.width, ==, 64);

  gum_arm64_writer_add_literal_reference_here(self, val);
  gum_arm64_writer_put_instruction(
      self, (ri.is_integer ? 0x58000000 : 0x5c000000) | ri.index);
}

```

其实有另外一个小思路,  有一点小不足, 就是确定指令片段的长度, 但其实也有解决方法, **可以放几条特殊指令作为结尾标记**.

先使用内联汇编写一个函数.

```

__attribute__((__naked__)) static void ctx_save() {
  __asm__ volatile(

      /* reserve space for next_hop */
      "sub sp, sp, #(2*8)\n"

      /* save {q0-q7} */
      "sub sp, sp, #(8*16)\n"
      "stp q6, q7, [sp, #(6*16)]\n"
      "stp q4, q5, [sp, #(4*16)]\n"
      "stp q2, q3, [sp, #(2*16)]\n"
      "stp q0, q1, [sp, #(0*16)]\n"

      /* save {x1-x30} */
      "sub sp, sp, #(30*8)\n"
      "stp fp, lr, [sp, #(28*8)]\n"
      "stp x27, x28, [sp, #(26*8)]\n"
      "stp x25, x26, [sp, #(24*8)]\n"
      "stp x23, x24, [sp, #(22*8)]\n"
      "stp x21, x22, [sp, #(20*8)]\n"
      "stp x19, x20, [sp, #(18*8)]\n"
      "stp x17, x18, [sp, #(16*8)]\n"
      "stp x15, x16, [sp, #(14*8)]\n"
      "stp x13, x14, [sp, #(12*8)]\n"
      "stp x11, x12, [sp, #(10*8)]\n"
      "stp x9, x10, [sp, #(8*8)]\n"
      "stp x7, x8, [sp, #(6*8)]\n"
      "stp x5, x6, [sp, #(4*8)]\n"
      "stp x3, x4, [sp, #(2*8)]\n"
      "stp x1, x2, [sp, #(0*8)]\n"

      /* save sp, x0 */
      "sub sp, sp, #(2*8)\n"
      "add x1, sp, #(2*8 + 8*16 + 30*8 + 2*8)\n"
      "stp x1, x0, [sp, #(0*8)]\n"

      /* alignment padding + dummy PC */
      "sub sp, sp, #(2*8)\n");
}

```

之后直接复制这块函数内存数据即可, 这一般适合那种指令片段堆.

```
void zz_build_enter_thunk(ZzWriter *writer) {

    // pop x17
    writer_put_ldr_reg_reg_offset(writer, ARM64_REG_X17, ARM64_REG_SP, 0);
    writer_put_add_reg_reg_imm(writer, ARM64_REG_SP, ARM64_REG_SP, 16);

    // TODO:  is bad code ?
    writer_put_bytes(writer, (void *) ctx_save, 26 * 4);

    // call `function_context_begin_invocation`
    writer_put_bytes(writer, (void *) pass_enter_func_args, 4 * 4);
    writer_put_ldr_reg_address(
            writer, ARM64_REG_X17,
            (zaddr) (zpointer) function_context_begin_invocation);
    writer_put_blr_reg(writer, ARM64_REG_X17);

    // TOOD: is bad code ?
    writer_put_bytes(writer, (void *) ctx_restore, 23 * 4);
}
```

#### 3. 指令读 模块

这一部分实际上就是 `disassembler`, 这一部分可以直接使用 `capstone`, 这里需要把 `capstone` 编译成多种架构.

#### 4. 指令修复 模块

这里的指令修复主要是发生在 hook 函数头几条指令, 由于备份指令到另一个地址, 这就需要对所有 `PC(IP)` 相关指令进行修复.

大致的思路就是: 判断 `capstone` 读取到的指令 ID, 针对特定指令写一个小函数进行修复

例如在 `frida-gum` 中:

```
static gboolean
gum_arm64_relocator_rewrite_b (GumArm64Relocator * self,
                               GumCodeGenCtx * ctx)
{
  const cs_arm64_op *target = &ctx->detail->operands[0];

  (void)self;

  gum_arm64_writer_put_ldr_reg_address(ctx->output, ARM64_REG_X17, target->imm);
  gum_arm64_writer_put_br_reg(ctx->output, ARM64_REG_X17);

  return true;
}
```

#### 5. 跳板 模块

跳板模块的设计是希望各个模块的实现更浅的耦合, 跳板函数主要作用就是进行跳转, 并准备 `跳转目标` 需要的参数. 举个例子, 被 hook 的函数经过入口跳板(`enter_trampoline`), 跳转到调度函数(`enter_chunk`), 需要被 hook 的函数相关信息等, 这个就需要在构造跳板是完成

#### 6. 调度 模块

可以理解为所有被 hook 的函数都必须经过的函数, 类似于 `objc_msgSend`, 在这里通过栈来函数(`pre_call`, `replace_call`, `post_call`)调用顺序.

本质有些类似于 `objc_msgSend` 所有的被 hook 的函数都在经过 `enter_trampoline` 跳板后, 跳转到 `enter_thunk`, 在此进行下一步的跳转判断决定, 并不是直接跳转到 `replace_call`.

## 使用

**export 3 func**:

```
// initialize the interceptor and so on.
ZZSTATUS ZzInitialize(void);

// build hook with `replace_call`, `pre_call`, `post_call`, but not enable.
ZZSTATUS ZzBuildHook(zpointer target_ptr, zpointer replace_ptr, zpointer *origin_ptr, zpointer pre_call_ptr, zpointer post_call_ptr);

// enable hook, with `code patch`
ZZSTATUS ZzEnableHook(zpointer target_ptr);
```

**export 1 variable:


```
// current all cpu register state, read `zzdefs.h` for detail.
#if defined (__aarch64__)
typedef union FPReg_ {
    __int128_t q;
    struct {
        double d1; // Holds the double (LSB).
        double d2;
    } d;
    struct {
        float f1; // Holds the float (LSB).
        float f2;
        float f3;
        float f4;
    } f;
} FPReg;

// just ref how to backup/restore registers
struct RegState_ {
    uint64_t pc;
    uint64_t sp;

    union {
        uint64_t x[29];
        struct {
            uint64_t x0,x1,x2,x3,x4,x5,x6,x7,x8,x9,x10,x11,x12,x13,x14,x15,x16,x17,x18,x19,x20,x21,x22,x23,x24,x25,x26,x27,x28;
        } regs;
    } general;

    uint64_t fp;
    uint64_t lr;

    union {
        FPReg q[8];
        FPReg q0,q1,q2,q3,q4,q5,q6,q7;
    } floating;
};
```

在 `pre_call` 和 `post_call` 传递该变量.

#### 使用 `pre_call` 和 `post_call`

```
#include <sys/socket.h>
void *orig_recvmsg;
// bad code, should be `thread-local variable` and `lock`
zpointer recvmsg_data;
void recvmsg_pre_call(struct RegState_ *rs) {
  zpointer t = *((zpointer *)(rs->general.regs.x1) + 2);
  recvmsg_data = *(zpointer *)t;
}
void recvmsg_post_call(struct RegState_ *rs) {
  printf("@recvmsg@: %s\n", recvmsg_data);
}
__attribute__((constructor)) void test_hook_recvmsg() {
  ZzInitialize();
  ZzBuildHook((void *)recvmsg, NULL, (void **)(&orig_recvmsg),
              (zpointer)recvmsg_pre_call, (zpointer)recvmsg_post_call);
  ZzEnableHook((void *)recvmsg);
}
```

#### 使用 `replace_call`

```
#include <sys/socket.h>
void *orig_func;
__attribute__((constructor)) void test_hook_recvmsg() {
  ZzInitialize();
  ZzBuildHook((void *)func, (void *)fake_func, (void **)(&orig_func), NULL,
              NULL);
  ZzEnableHook((void *)func);
}
```

#### 对于 Objective-C 方法呢 ?

这里需要对 `<objc/runtime.h>` 里的函数比较了解. 也需要对 objc 的内存有一些了解.

```
+ (void)load
{
    [self zzMethodSwizzlingHook];
}

void objcMethod_pre_call(struct RegState_ *rs) {
  NSLog(@"hookzz OC-Method: -[ViewController %s]",
        (zpointer)(rs->general.regs.x1));
}

+ (void)zzMethodSwizzlingHook {
  Class hookClass = objc_getClass("UIViewController");
  SEL oriSEL = @selector(viewWillAppear:);
  Method oriMethod = class_getInstanceMethod(hookClass, oriSEL);
  IMP oriImp = method_getImplementation(oriMethod);

  ZzInitialize();
  ZzBuildHook((void *)oriImp, NULL, NULL, (zpointer)objcMethod_pre_call, NULL);
  ZzEnableHook((void *)oriImp);
}
```

## 编译 & 测试
#### arm64 & ios 架构

```
λ : >>> make -f darwin.ios.mk darwin.ios
generate [src/allocator.o]!
generate [src/interceptor.o]!
generate [src/trampoline.o]!
generate [src/platforms/darwin/memory-darwin.o]!
generate [src/platforms/arm64/reader.o]!
generate [src/platforms/arm64/relocator.o]!
generate [src/platforms/arm64/thunker.o]!
generate [src/platforms/arm64/writer.o]!
generate [src/zzdeps/darwin/memory-utils.o]!
build success for arm64(IOS)!
```

#### 测试

已经生成编译测试的 `test_hook.dylib` 与 `test_ios.dylib` 请自行测试

## 坑

#### 寄存器污染

在进行 inlinehook 需要进行各种跳转, 通常会以以下模板进行跳转.

```
0:  ldr x16, 8;
4:  br x16;
8:  0x12345678
12: 0x00000000
```

问题在于这会造成 x16 寄存器被污染. 所以这里有两种思路解决这个问题.

思路一:

在使用寄存器之前进行 `push`, 跳转后 `pop`, 这里存在一个问题就是在原地址的几条指令进行 `patch code` 时一定会污染一个寄存器(也不能说一定, 如果这时进行压栈, 在之后的 `invoke_trampline` 会导致函数栈发生改变, 此时有个解决方法可以 pop 出来, 由 hookentry 或者其他变量暂时保存, 但这时需要处理锁的问题. )

思路二:

挑选合适的寄存器, 不考虑污染问题. 这时可以参考, 下面的资料, 选择 x16 or x17, 或者自己做一个实验 `otool -tv ~/Downloads/DiSpecialDriver64 > ~/Downloads/DiSpecialDriver64.txt` 通过 dump 一个 arm64 程序的指令, 来判断哪个寄存器用的最少, 但是不要使用 x18 寄存器, 你对该寄存器的修改是无效的.

```
PAGE: 9-3
Programmer’s Guide for ARMv8-A
9.1 Register use in the AArch64 Procedure Call Standard 
9.1.1 Parameters in general-purpose registers
```

这里也有一个问题,  这也是 `frida-gum` 中遇到一个问题, 就是对于 svc 类系统调用, 系统调用号(syscall number)的传递是利用 x16 寄存器进行传递的, 所以本框架使用 x17 寄存器, 并且在传递参数时使用, `push` & `pop`, 在跳转后恢复 x17.

#### rwx 与 codesigning

对于非越狱, 不能分配可执行内存, 不能进行 `code patch`.

两篇原理讲解 codesign 的原理

```
https://papers.put.as/papers/ios/2011/syscan11_breaking_ios_code_signing.pdf
http://www.newosxbook.com/articles/CodeSigning.pdf
```

以及源码分析如下:

crash 异常如下, 其中 `0x0000000100714000` 是 mmap 分配的页.

```
Exception Type:  EXC_BAD_ACCESS (SIGKILL - CODESIGNING)
Exception Subtype: unknown at 0x0000000100714000
Termination Reason: Namespace CODESIGNING, Code 0x2
Triggered by Thread:  0
```

寻找对应的错误码

```
xnu-3789.41.3/bsd/sys/reason.h
/*
 * codesigning exit reasons
 */
#define CODESIGNING_EXIT_REASON_TASKGATED_INVALID_SIG 1
#define CODESIGNING_EXIT_REASON_INVALID_PAGE          2
#define CODESIGNING_EXIT_REASON_TASK_ACCESS_PORT      3
```

找到对应处理函数, 请仔细阅读注释里内容

```
# xnu-3789.41.3/osfmk/vm/vm_fault.c:2632

	/* If the map is switched, and is switch-protected, we must protect
	 * some pages from being write-faulted: immutable pages because by 
	 * definition they may not be written, and executable pages because that
	 * would provide a way to inject unsigned code.
	 * If the page is immutable, we can simply return. However, we can't
	 * immediately determine whether a page is executable anywhere. But,
	 * we can disconnect it everywhere and remove the executable protection
	 * from the current map. We do that below right before we do the 
	 * PMAP_ENTER.
	 */
	cs_enforcement_enabled = cs_enforcement(NULL);

	if(cs_enforcement_enabled && map_is_switched && 
	   map_is_switch_protected && page_immutable(m, prot) && 
	   (prot & VM_PROT_WRITE))
	{
		return KERN_CODESIGN_ERROR;
	}

	if (cs_enforcement_enabled && page_nx(m) && (prot & VM_PROT_EXECUTE)) {
		if (cs_debug)
			printf("page marked to be NX, not letting it be mapped EXEC\n");
		return KERN_CODESIGN_ERROR;
	}

	if (cs_enforcement_enabled &&
	    !m->cs_validated &&
	    (prot & VM_PROT_EXECUTE) &&
	    !(caller_prot & VM_PROT_EXECUTE)) {
		/*
		 * FOURK PAGER:
		 * This page has not been validated and will not be
		 * allowed to be mapped for "execute".
		 * But the caller did not request "execute" access for this
		 * fault, so we should not raise a code-signing violation
		 * (and possibly kill the process) below.
		 * Instead, let's just remove the "execute" access request.
		 * 
		 * This can happen on devices with a 4K page size if a 16K
		 * page contains a mix of signed&executable and
		 * unsigned&non-executable 4K pages, making the whole 16K
		 * mapping "executable".
		 */
		prot &= ~VM_PROT_EXECUTE;
	}

	/* A page could be tainted, or pose a risk of being tainted later.
	 * Check whether the receiving process wants it, and make it feel
	 * the consequences (that hapens in cs_invalid_page()).
	 * For CS Enforcement, two other conditions will 
	 * cause that page to be tainted as well: 
	 * - pmapping an unsigned page executable - this means unsigned code;
	 * - writeable mapping of a validated page - the content of that page
	 *   can be changed without the kernel noticing, therefore unsigned
	 *   code can be created
	 */
	if (!cs_bypass &&
	    (m->cs_tainted ||
	     (cs_enforcement_enabled &&
	      (/* The page is unsigned and wants to be executable */
	       (!m->cs_validated && (prot & VM_PROT_EXECUTE))  ||
	       /* The page should be immutable, but is in danger of being modified
		* This is the case where we want policy from the code directory -
		* is the page immutable or not? For now we have to assume that 
		* code pages will be immutable, data pages not.
		* We'll assume a page is a code page if it has a code directory 
		* and we fault for execution.
		* That is good enough since if we faulted the code page for
		* writing in another map before, it is wpmapped; if we fault
		* it for writing in this map later it will also be faulted for executing 
		* at the same time; and if we fault for writing in another map
		* later, we will disconnect it from this pmap so we'll notice
		* the change.
		*/
	      (page_immutable(m, prot) && ((prot & VM_PROT_WRITE) || m->wpmapped))
	      ))
		    )) 
	{
```

#### 其他文章:

http://ddeville.me/2014/04/dynamic-linking

> Later on, whenever a page fault occurs the vm_fault function in `vm_fault.c` is called. During the page fault the signature is validated if necessary. The signature will need to be validated if the page is mapped in user space, if the page belongs to a code-signed object, if the page will be writable or simply if it has not previously been validated. Validation happens in the `vm_page_validate_cs` function inside vm_fault.c (the validation process and how it is enforced continually and not only at load time is interesting, see Charlie Miller’s book for more details).

> If for some reason the page cannot be validated, the kernel checks whether the `CS_KILL` flag has been set and kills the process if necessary. There is a major distinction between iOS and OS X regarding this flag. All iOS processes have this flag set whereas on OS X, although code signing is checked it is not set and thus not enforced.

> In our case we can safely assume that the (missing) code signature couldn’t be verified leading to the kernel killing the process.

---