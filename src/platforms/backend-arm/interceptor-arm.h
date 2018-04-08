#ifndef platforms_backend_arm_intercetor_arm
#define platforms_backend_arm_intercetor_arm

#include "hookzz.h"
#include "zkit.h"

#include "emm.h"
#include "interceptor.h"
#include "bridge.h"
#include "tools.h"

#include "platforms/arch-arm/relocator-arm.h"
#include "platforms/arch-arm/relocator-thumb.h"
#include "platforms/arch-arm/writer-arm.h"
#include "platforms/arch-arm/writer-thumb.h"

// (nextHop + general_regs + sp)
#define CTX_SAVE_STACK_OFFSET (4 * 14)

typedef struct _InterceptorBackend {
    ExecuteMemoryManager *emm;
    ZzARMRelocator arm_relocator;
    ZzThumbRelocator thumb_relocator;
    ZzARMAssemblerWriter arm_writer;
    ZzThumbAssemblerWriter thumb_writer;
    ZzARMReader arm_reader;
    ZzARMReader thumb_reader;

    zz_ptr_t enter_bridge;
    zz_ptr_t leave_bridge;
    zz_ptr_t dynamic_binary_instrumentation_bridge;

} InterceptorBackend;

typedef struct _ZzARMHookFuntionEntryBackend {
    bool is_thumb;
    zz_size_t redirect_code_size;
} ZzARMHookEntryBackend;

#endif