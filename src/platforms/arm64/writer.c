/**
 *    Copyright 2017 jmpews
 * 
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 * 
 *        http://www.apache.org/licenses/LICENSE-2.0
 * 
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#include "writer.h"
#include <string.h>

ZZWriter *ZZNewWriter(zpointer addr) {
    ZZWriter *writer = (ZZWriter *) malloc(sizeof(ZZWriter));
    writer->codedata = addr;
    writer->base = addr;
    writer->pc = addr;
    writer->size = 0;
    return writer;
}

void WriterPutAbsJmp(ZZWriter *self, zpointer target_addr) {
    writer_put_ldr_reg_imm(self, ARM64_REG_X16, (zuint) 0x8);
    writer_put_br_reg(self, ARM64_REG_X16);
    writer_put_bytes(self, (zpointer) &target_addr, sizeof(target_addr));
}

// NOUSE:
void writer_put_ldr_br_b_reg_address(ZZWriter *self, arm64_reg reg, zaddr address) {
    writer_put_ldr_reg_imm(self, reg, (zuint) 0xc);
    writer_put_br_reg(self, reg);
    writer_put_b_imm(self, (zaddr) 0xc);
    writer_put_bytes(self, (zpointer) &address,
                     sizeof(address));

}

void writer_put_ldr_reg_address(ZZWriter *self, arm64_reg reg, zaddr address) {
    writer_put_ldr_reg_imm(self, reg, (zuint) 0x8);
    writer_put_b_imm(self, (zaddr) 0xc);
    writer_put_bytes(self, (zpointer) &address,
                     sizeof(address));

}

void writer_put_ldr_reg_imm(ZZWriter *self, arm64_reg reg, zuint imm) {
    ZZArm64RegInfo ri;
    uint32_t ins_bytes = 0;

    writer_describe_reg(reg, &ri);
    
    ins_bytes = 0x58000000 | ri.index;
    
    writer_put_instruction(self, ins_bytes | ((imm >> 2) << 5));
}

void writer_put_b_cond_imm(ZZWriter *self, arm64_cc cc, zuint imm) {
    uint32_t ins_bytes = 0;
    ins_bytes = ins_bytes | 0x54000000 | (cc - 1);
    ins_bytes = ins_bytes | (imm >> 2) << 5;
    writer_put_instruction(self, ins_bytes);
}

// TODO:
// br x16;
void writer_put_br_reg(ZZWriter *self, arm64_reg reg) {
    writer_put_instruction(self, 0xd61f0200);
}

//TODO:
// blr x16
void writer_put_blr_reg(ZZWriter *self, arm64_reg reg) {
    writer_put_instruction(self, 0xD63F0200);
}

void writer_put_b_imm(ZZWriter *self, zuint imm) {
    // zaddr offset = address - (zaddr)self->pc;
    writer_put_instruction(self, 0x14000000 | ((imm / 4) & 0x03ffffff));
}

void writer_put_bytes(ZZWriter *self, zbyte *data, zuint data_size) {
    memcpy(self->codedata, data, data_size);
    self->codedata = (zpointer) self->codedata + data_size;
    self->pc += data_size;
    self->size += data_size;

}

void writer_put_instruction(ZZWriter *self, uint32_t insn) {
    *(uint32_t *) (self->codedata) = insn;
    self->codedata = (zpointer) self->codedata + sizeof(uint32_t);
    self->pc += 4;
    self->size += 4;
}

// TODO:
void writer_describe_reg(arm64_reg reg, ZZArm64RegInfo *ri) {
  if (reg >= ARM64_REG_X0 && reg <= ARM64_REG_X28)
  {
    ri->index = reg - ARM64_REG_X0;
  } else {
      Serror("error at writer_describe_reg");
      ri->index = 0;
  }
}
