#ifndef zzdeps_posix_memory_utils_h
#define zzdeps_posix_memory_utils_h

#include <err.h>
#include <stdio.h>
#include <stdlib.h>

#include "../zz.h"
#include "../common/memory-utils-common.h"

zsize zz_vm_get_page_size();
bool zz_vm_check_address_valid_via_msync(const zpointer p);
bool zz_vm_check_address_valid_via_signal(zpointer p);
#endif