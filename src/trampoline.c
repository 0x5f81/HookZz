//    Copyright 2017 jmpews
//
//    Licensed under the Apache License, Version 2.0 (the "License");
//    you may not use this file except in compliance with the License.
//    You may obtain a copy of the License at
//
//        http://www.apache.org/licenses/LICENSE-2.0
//
//    Unless required by applicable law or agreed to in writing, software
//    distributed under the License is distributed on an "AS IS" BASIS,
//    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//    See the License for the specific language governing permissions and
//    limitations under the License.

#include <assert.h>
#include <stdlib.h>

#include "relocator.h"
#include "thunker.h"
#include "trampoline.h"

// ZZSTATUS ZzBuildEnterTrampoline(ZzHookFunctionEntry *entry) {
//     zbyte temp_code_slice_data[256] = {0};
//     ZzWriter *writer;
//     ZzCodeSlice *code_slice;
//     ZzInterceptor *interceptor;
//     ZZSTATUS status;

//     interceptor = entry->interceptor;
//     writer = ZzWriterNewWriter(temp_code_slice_data);

//     ZzThunkerBuildJumpToEnterThunk(writer, (zpointer) entry,
//                                    (zpointer) interceptor->enter_thunk); //
//                                    @common-function

//     status = ZZ_FAILED;
//     do {
//         code_slice = ZzNewNearCodeSlice(entry->interceptor->allocator,
//         (zaddr) entry->target_ptr,
//                                        ZzWriterNearJumpRangeSize(),
//                                        writer->size); // @common-function
//         if (!code_slice) {
//             code_slice = ZzNewCodeSlice(entry->interceptor->allocator,
//             writer->size); // @common-funciton entry->isNearJump = false;
//         } else {
//             entry->isNearJump = true;
//         }

//         if (!code_slice || !code_slice->data || !code_slice->size)
//             break;
//         if (!ZzMemoryPatchCode((zaddr) code_slice->data,
//         temp_code_slice_data, writer->size))
//             break;
//         entry->on_enter_trampoline = code_slice->data;
//         status = ZZ_SUCCESS;
//     } while (0);

//     free(writer);
//     return status;
// }

// ZZSTATUS ZzBuildInvokeTrampoline(ZzHookFunctionEntry *entry) {
//     zbyte temp_code_slice_data[256] = {0};
//     ZzWriter *backup_writer, *relocate_writer;
//     ZzCodeSlice *code_slice;
//     ZzInterceptor *interceptor;
//     ZZSTATUS status;

//     backup_writer = ZzWriterNewWriter(entry->origin_prologue.data);
//     relocate_writer = ZzWriterNewWriter(temp_code_slice_data);

//     ZzRelocatorBuildInvokeTrampoline(entry, backup_writer,
//                                      relocate_writer);

//     ZzWriterPutAbsJump(relocate_writer,
//                        entry->target_ptr +
//                        (zuint)(backup_writer->pc - backup_writer->base));

//     status = ZZ_FAILED;
//     do {
//         code_slice = ZzNewCodeSlice(entry->interceptor->allocator,
//         relocate_writer->size); // @common-function if (!code_slice ||
//         !code_slice->data || !code_slice->size)
//             break;

//         if (entry->hook_type == HOOK_ADDRESS_TYPE && entry->target_end_ptr) {
//             // update target_half_ret_addr
//             entry->target_half_ret_addr += (zaddr) code_slice->data;
//         }

//         if (!ZzMemoryPatchCode((zaddr) code_slice->data,
//         temp_code_slice_data, relocate_writer->size))
//             break;
//         entry->on_invoke_trampoline = code_slice->data;

//         entry->origin_prologue.size = backup_writer->pc -
//         backup_writer->base; assert(entry->origin_prologue.size ==
//         backup_writer->size); status = ZZ_SUCCESS;
//     } while (0);

//     free(backup_writer);
//     free(relocate_writer);
//     return status;
// }

// ZZSTATUS ZzBuildLeaveTrampoline(ZzHookFunctionEntry *entry) {
//     zbyte temp_code_slice_data[256] = {0};
//     ZzWriter *writer;
//     ZzCodeSlice *code_slice;
//     ZzInterceptor *interceptor;
//     ZZSTATUS status;

//     interceptor = entry->interceptor;
//     writer = ZzWriterNewWriter(temp_code_slice_data);

//     ZzThunkerBuildJumpToLeaveThunk(writer, (zpointer) entry,
//                                    (zpointer) interceptor->leave_thunk);

//     status = ZZ_FAILED;
//     do {
//         code_slice = ZzNewCodeSlice(entry->interceptor->allocator,
//         writer->size); // @common-function if (!code_slice ||
//         !code_slice->data || !code_slice->size)
//             break;
//         if (!ZzMemoryPatchCode((zaddr) code_slice->data,
//         temp_code_slice_data, writer->size))
//             break;
//         entry->on_leave_trampoline = code_slice->data;
//         status = ZZ_SUCCESS;
//     } while (0);

//     free(writer);
//     return ZZ_DONE;
// }

// ZZSTATUS ZzBuildHalfTrampoline(ZzHookFunctionEntry *entry) {
//     zbyte temp_code_slice_data[256] = {0};
//     ZzWriter *writer;
//     ZzCodeSlice *code_slice;
//     ZzInterceptor *interceptor;
//     ZZSTATUS status;

//     interceptor = entry->interceptor;
//     writer = ZzWriterNewWriter(temp_code_slice_data);

//     ZzThunkerBuildJumpToHalfThunk(writer, (zpointer) entry,
//                                   (zpointer) interceptor->half_thunk);

//     status = ZZ_FAILED;
//     do {
//         code_slice = ZzNewCodeSlice(entry->interceptor->allocator,
//         writer->size); // @common-function if (!code_slice ||
//         !code_slice->data || !code_slice->size)
//             break;
//         if (!ZzMemoryPatchCode((zaddr) code_slice->data,
//         temp_code_slice_data, writer->size))
//             break;
//         entry->on_half_trampoline = code_slice->data;
//         status = ZZ_SUCCESS;
//     } while (0);

//     free(writer);
//     return ZZ_DONE;
// }

ZZSTATUS ZzBuildTrampoline(struct _ZzInterceptorBackend *self,
                           ZzHookFunctionEntry *entry) {

    ZzBuildEnterTrampoline(self, entry);

    if (entry->hook_type == HOOK_ADDRESS_TYPE) {
        ZzBuildHalfTrampoline(self, entry);
        ZzBuildInvokeTrampoline(self, entry);
    } else {
        ZzBuildInvokeTrampoline(self, entry);
        ZzBuildLeaveTrampoline(self, entry);
    }

    return ZZ_DONE;
}
