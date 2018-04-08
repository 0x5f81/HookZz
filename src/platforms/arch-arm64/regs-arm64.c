#include "regs-arm64.h"

void arm64_register_describe(ARM64Reg reg, ARM64RegInfo *ri) {
    if (reg >= ZZ_ARM64_REG_X0 && reg <= ZZ_ARM64_REG_X28) {
        ri->is_integer = TRUE;
        ri->width      = 64;
        ri->meta       = ZZ_ARM64_REG_X0 + (reg - ZZ_ARM64_REG_X0);
    } else if (reg == ZZ_ARM64_REG_X29 || reg == ZZ_ARM64_REG_FP) {
        ri->is_integer = TRUE;
        ri->width      = 64;
        ri->meta       = ZZ_ARM64_REG_X29;
    } else if (reg == ZZ_ARM64_REG_X30 || reg == ZZ_ARM64_REG_LR) {
        ri->is_integer = TRUE;
        ri->width      = 64;
        ri->meta       = ZZ_ARM64_REG_X30;
    } else if (reg == ZZ_ARM64_REG_SP) {
        ri->is_integer = TRUE;
        ri->width      = 64;
        ri->meta       = ZZ_ARM64_REG_X31;
    } else {
        ri->index = 0;
        ZZ_ERROR_LOG_STR("arm64_register_describe error.");
    }
    ri->index = ri->meta - ZZ_ARM64_REG_X0;
}
