#include "reader-arm64.h"
ARM64AssemblyReader *arm64_reader_new(zz_ptr_t insn_address) {
    ARM64AssemblyReader *reader = (ARM64AssemblyReader *)malloc0(sizeof(ARM64AssemblyReader));

    reader->start_pc          = (zz_addr_t)insn_address;
    reader->insns_buffer   = (zz_addr_t)insn_address;
    reader->insns_size              = 0;
    reader->insnCTXs_count         = 0;
    return reader;
}

void arm64_reader_init(ARM64AssemblyReader *self, zz_ptr_t insn_address) { arm64_reader_reset(self, insn_address); }

void arm64_reader_reset(ARM64AssemblyReader *self, zz_ptr_t insn_address) {
    self->start_pc          = (zz_addr_t)insn_address;
    self->insns_buffer   = (zz_addr_t)insn_address;
    self->insns_size              = 0;
    self->insnCTXs_count         = 0;
}

void arm64_reader_free(ARM64AssemblyReader *self) {
    if (self->insnCTXs_count) {
        for (int i = 0; i < self->insnCTXs_count; i++) {
            free(self->insnCTXs[i]);
        }
    }
    free(self);
}

ARM64InstructionCTX *arm64_reader_read_one_instruction(ARM64AssemblyReader *self) {
    ARM64InstructionCTX *insn_ctx = (ARM64InstructionCTX *)malloc0(sizeof(ARM64InstructionCTX));
    zz_addr_t next_insn_address = (zz_addr_t)self->insns_buffer + self->insns_size;
    insn_ctx->pc      = next_insn_address;
    insn_ctx->address = next_insn_address;
    insn_ctx->insn    = *(uint32_t *)next_insn_address;
    insn_ctx->size               = 4;
    self->insnCTXs[self->insnCTXs_count++] = insn_ctx;
    self->insns_size += insn_ctx->size;
    return insn_ctx;
}

ARM64InsnType GetARM64InsnType(uint32_t insn) {
    // PAGE: C6-673
    if (insn_equal(insn, "01011000xxxxxxxxxxxxxxxxxxxxxxxx")) {
        return ARM64_INS_LDR_literal;
    }

    // PAGE: C6-535
    if (insn_equal(insn, "0xx10000xxxxxxxxxxxxxxxxxxxxxxxx")) {
        return ARM64_INS_ADR;
    }

    // PAGE: C6-536
    if (insn_equal(insn, "1xx10000xxxxxxxxxxxxxxxxxxxxxxxx")) {
        return ARM64_INS_ADRP;
    }

    // PAGE: C6-550
    if (insn_equal(insn, "000101xxxxxxxxxxxxxxxxxxxxxxxxxx")) {
        return ARM64_INS_B;
    }

    // PAGE: C6-560
    if (insn_equal(insn, "100101xxxxxxxxxxxxxxxxxxxxxxxxxx")) {
        return ARM64_INS_BL;
    }

    // PAGE: C6-549
    if (insn_equal(insn, "01010100xxxxxxxxxxxxxxxxxxx0xxxx")) {
        return ARM64_INS_B_cond;
    }

    return ARM64_UNDEF;
}