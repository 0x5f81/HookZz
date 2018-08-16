#ifndef ARCH_ARM64
#define ARCH_ARM64

#include "srcxx/platforms/code-buffer.h"

namespace zz {
namespace arm64 {

class Label {
public:
  Label() : location_(kLocationUnbound) {
  }
  ~Label() {
  }
}

class Assembler {
private:
  CodeBuffer *buffer_;

public:
  void b(int64_t imm26);
  void ldr();

  void Assembler::BranchLink(const StubEntry &stub_entry, Patchability patchable) {
    const Code &target   = Code::ZoneHandle(stub_entry.code());
    const int32_t offset = ObjectPool::element_offset(object_pool_wrapper_.FindObject(target, patchable));
    LoadWordFromPoolOffset(CODE_REG, offset);
    ldr(TMP, FieldAddress(CODE_REG, Code::entry_point_offset()));
    blr(TMP);
  }

  void EmitLoadStoreReg(LoadStoreRegOp op, Register rt, Address a, OperandSize sz) {
    const int32_t size     = Log2OperandSizeBytes(sz);
    const int32_t encoding = op | ((size & 0x3) << kSzShift) | Arm64Encode::Rt(rt) | a.encoding();
    Emit(encoding);
  }

  void EmitInst(uint32 inst);
};

} // namespace arm64
} // namespace zz

#endif