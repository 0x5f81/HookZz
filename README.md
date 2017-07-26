## HookZz

hook framwork. ref with: [frida-gum](https://github.com/frida/frida-gum) and [minhook](https://github.com/TsudaKageyu/minhook) and [substrate](https://github.com/jevinskie/substrate)

**still developing, for arm64 now!**

```
SAME: 效果相同的另一种写法
TODO: 有些没有考虑到, 或需要改进的.
NOUSE: 没有使用
```

## 通用Hook结构设计

1. 内存分配 模块
2. 指令写 模块
3. 指令读 模块
4. 指令修复 模块
5. 跳板 模块
6. 调度器 模块

#### 内存分配 模块

需要分配部分内存用于写入指令, 这里需要关注两个函数都是关于内存属性相关的. 1. 如何使内存 `可写` 2. 如何使内存 `可执行`

这一部分与具体的操作系统有关. 比如 `darwin` 使用 `mach_vm_allocate`

在 lldb 中可以通过 `memory region address` 查看地址的内存属性.

#### 指令写 模块

由于现在大部分的 `assembler` 需要 llvm 的支持. 所以并没有现成的框架支持. 其实这里的指令写有种简单的方法, 就是在本地生成指令的16进制串, 之后直接写即可. 但这种应该是属于 hardcode.

这里使用 `frida` 和 `CydiaSubstrace` 都用的方法, 把需要用到的指令都写成一个小函数.

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

其实有另外一个小思路, 唯一不足的就是属于部分 hardcode.

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
void zz_build_enter_thunk(ZZWriter *writer) {
  writer_put_bytes(writer, (void *)ctx_save, 26 * 4);

  // call `function_context_begin_invocation`
  writer_put_bytes(writer, (void *)pass_enter_func_args, 4 * 4);
  writer_put_ldr_reg_address(
      writer, ARM64_REG_X16,
      (zaddr)(zpointer)function_context_begin_invocation);
  writer_put_blr_reg(writer, ARM64_REG_X16);

  writer_put_bytes(writer, (void *)ctx_restore, 23 * 4);

}
```

#### 指令读 模块

这一部分实际上就是 `disassembler`, 这一部分可以直接使用 `capstone`, 这里需要把 `capstone` 编译成多种架构.

#### 指令修复 模块

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

  gum_arm64_writer_put_ldr_reg_address(ctx->output, ARM64_REG_X16, target->imm);
  gum_arm64_writer_put_br_reg(ctx->output, ARM64_REG_X16);

  return TRUE;
}
```

#### 跳板 模块

跳板模块的设计是希望各个模块的实现更浅的耦合, 跳板函数主要作用就是进行跳转, 并准备 `跳转目标` 需要的参数. 举个例子, 被 hook 的函数经过入口跳板(`enter_trampoline`), 跳转到调度函数(`enter_chunk`), 需要被 hook 的函数相关信息等, 这个就需要在构造跳板是完成

#### 调度 模块

可以理解为所有被 hook 的函数都必须经过的函数, 类似于 `objc_msgSend`, 在这里通过栈来函数(`pre_call`, `replace_call`, `post_call`)调用顺序.

本质有些类似于 `objc_msgSend` 所有的被 hook 的函数都在经过 `enter_trampoline` 跳板后, 跳转到 `enter_thunk`, 在此进行下一步的跳转判断决定, 并不是直接跳转到 `replace_call`.

## 编译 & 使用

#### export 3 func:

```
// initialize the interceptor and so on.
ZZSTATUS ZZInitialize(void);

// build hook with `replace_call`, `pre_call`, `post_call`, but not enable.
ZZSTATUS ZZBuildHook(zpointer target_ptr, zpointer replace_ptr, zpointer *origin_ptr, zpointer pre_call_ptr, zpointer post_call_ptr);

// enable hook, with `code patch`
ZZSTATUS ZZEnableHook(zpointer target_ptr);
```

#### export 1 datastruct:

```
// current all cpu register state, read `zzdefs.h` for detail.
struct RegState_
```

#### arm64 & ios 架构

```
jmpews at localhost in ~/Desktop/SpiderZz/project/evilHOOK/HookZz (master●) (normal)
λ : >>> make -f darwin.ios.mk test
generate [src/allocator.o]!
generate [src/interceptor.o]!
generate [src/trampoline.o]!
generate [src/platforms/darwin/memory-darwin.o]!
generate [src/platforms/arm64/reader.o]!
generate [src/platforms/arm64/relocator.o]!
generate [src/platforms/arm64/thunker.o]!
generate [src/platforms/arm64/writer.o]!
build [test] success for arm64(IOS)!
```

#### 测试

已经生成编译测试的 `test_hook.dylib` 与 `test_ios.dylib` 请自行测试


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
  ZZInitialize();
  ZZBuildHook((void *)recvmsg, NULL, (void **)(&orig_recvmsg),
              (zpointer)recvmsg_pre_call, (zpointer)recvmsg_post_call);
  ZZEnableHook((void *)recvmsg);
}
```

#### 使用 `replace_call`

```
#include <sys/socket.h>
void *orig_func;
__attribute__((constructor)) void test_hook_recvmsg() {
  ZZInitialize();
  ZZBuildHook((void *)func, (void *)fake_func, (void **)(&orig_func), NULL,
              NULL);
  ZZEnableHook((void *)func);
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
    printf("call -[ViewController %s]", (zpointer) (rs->general.regs.x1));
}

void *oriObjcMethod;
+(void)zzMethodSwizzlingHook {
    Class hookClass = objc_getClass("UIViewController");
    SEL oriSEL = @selector(viewWillAppear:);
    Method oriMethod = class_getInstanceMethod(hookClass, oriSEL);
    IMP oriImp = method_getImplementation(oriMethod);

    ZZBuildHook((void *)oriImp, NULL, (void **) (&oriObjcMethod),
                (zpointer) objcMethod_pre_call, NULL);
    ZZEnableHook((void *) oriImp);
}
```

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