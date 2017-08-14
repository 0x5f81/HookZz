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

#ifndef interceptor_h
#define interceptor_h

#include "../include/zz.h"
#include "../include/hookzz.h"

#include "allocator.h"

typedef struct _FunctionBackup {
    zpointer address;
    zsize size;
    zbyte data[32];
} FunctionBackup;

struct _ZzInterceptor;

/*
 * hook entry
 */
typedef struct _ZzHookFunctionEntry {
    unsigned long id;
    bool isEnabled;

    zpointer target_ptr;
    zpointer caller_ret_addr;

    zpointer pre_call;
    zpointer post_call;
    zpointer replace_call;

    FunctionBackup origin_prologue;

    zpointer on_enter_trampoline;
    zpointer on_invoke_trampoline;
    zpointer on_leave_trampoline;

    struct _ZzInterceptor *interceptor;
} ZzHookFunctionEntry;

typedef struct {
    ZzHookFunctionEntry **entries;
    zsize size;
    zsize capacity;
} ZzHookFunctionEntrySet;

// NOUSE!
typedef struct _ZzInterceptorCenter {
    ZzCodeSlice enter_thunk;
    ZzCodeSlice leave_thunk;
} ZzInterceptorCenter;

typedef struct _ZzInterceptor {
    ZzHookFunctionEntrySet *hook_func_entries;
    ZzInterceptorCenter *intercepter_center;
    zpointer enter_thunk;
    zpointer leave_thunk;
} ZzInterceptor;

ZZSTATUS ZzInitialize(void);

ZZSTATUS ZzBuildThunk(void);

ZzHookFunctionEntry *ZzNewHookFunctionEntry(zpointer target_ptr);

ZZSTATUS ZzActiveEnterTrampoline(ZzHookFunctionEntry *entry);

#endif