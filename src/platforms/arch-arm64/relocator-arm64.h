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

#ifndef platforms_arch_arm64_relocator_h
#define platforms_arch_arm64_relocator_h

#include "hookzz.h"
#include "kitzz.h"

#include "memory.h"
#include "writer.h"

#include "instructions.h"
#include "reader-arm64.h"
#include "regs-arm64.h"
#include "writer-arm64.h"

typedef struct _ZzARM64Relocator {
    bool try_relocated_again;
    zz_size_t try_relocated_length;
    zz_ptr_t input_start;
    zz_ptr_t input_cur;
    zz_addr_t input_pc;
    int inpos;
    int outpos;
    ZzInstruction *input_insns;
    ZzRelocateInstruction *output_insns;
    ZzARM64AssemblerWriter *output;
    ZzLiteralInstruction **relocate_literal_insns;
    zz_size_t relocate_literal_insns_size;
} ZzARM64Relocator;

void zz_arm64_relocator_init(ZzARM64Relocator *relocator, zz_ptr_t input_code, ZzARM64AssemblerWriter *writer);

void zz_arm64_relocator_free(ZzARM64Relocator *relocator);

void zz_arm64_relocator_reset(ZzARM64Relocator *self, zz_ptr_t input_code, ZzARM64AssemblerWriter *output);

zz_size_t zz_arm64_relocator_read_one(ZzARM64Relocator *self, ZzInstruction *instruction);

bool zz_arm64_relocator_write_one(ZzARM64Relocator *self);

void zz_arm64_relocator_write_all(ZzARM64Relocator *self);

void zz_arm64_relocator_try_relocate(zz_ptr_t address, zz_size_t min_bytes, zz_size_t *max_bytes);

void zz_arm64_relocator_relocate_writer(ZzARM64Relocator *relocator, zz_addr_t code_address);

#endif