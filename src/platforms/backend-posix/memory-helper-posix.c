#include "memroy-helper-posix.h"
#include "memroy-manager-posix.h"

inline void set_page_memory_permission(void *address, int prot) {
    int page_size = memory_manger_class(get_page_size)();
    int r;
    r = mprotect((zz_ptr_t)address, page_size, prot);
    if (r == -1) {
        ERROR_LOG("r = %d, at (%p) error!", r, (zz_ptr_t)address);
        return false;
    }
    return true;
}

/*
  ref:
  substitute/lib/darwin/execmem.c:execmem_foreign_write_with_pc_patch
  frida-gum-master/gum/gummemory.c:gum_memory_patch_code

  frida-gum-master/gum/backend-darwin/gummemory-darwin.c:gum_alloc_n_pages

  mach mmap use __vm_allocate and __vm_map
  https://github.com/bminor/glibc/blob/master/sysdeps/mach/hurd/mmap.c
  https://github.com/bminor/glibc/blob/master/sysdeps/mach/munmap.c

  http://shakthimaan.com/downloads/hurd/A.Programmers.Guide.to.the.Mach.System.Calls.pdf
*/

void posix_memory_helper_cclass(patch_code)(void *dest, void *src, int count) {
    void *dest_page;
    int offset;

    int page_size = memory_manger_class(get_page_size)();

    // https://www.gnu.org/software/hurd/gnumach-doc/Memory-Attributes.html
    dest_page = (zz_addr_t)dest & ~(page_size - 1);
    offset    = (zz_addr_t)dest - dest_page;

    // another method, pelease read `REF`;
    // zz_ptr_t code_mmap = mmap(NULL, range_size, PROT_READ | PROT_WRITE,
    //                           MAP_ANON | MAP_SHARED, -1, 0);
    // if (code_mmap == MAP_FAILED) {
    //   return;
    // }

    void *copy_page = memory_manger_class(allocate_page)(3, 1);

    memcpy(copy_page, (void *)dest_page, page_size);
    memcpy((void *)((zz_addr_t)copy_page + offset), src, count);

    /* SAME: mprotect(code_mmap, range_size, prot); */
    zz_addr_t target = (zz_addr_t)start_page_addr;
    set_page_memory_permission(copy_page, PROT_WRITE | PROT_READ);
    memcpy(dest_page, copy_page, page_size);
    set_page_memory_permission(copy_page, PROT_EXEC | PROT_READ);
    // TODO
    munmap(copy_page, page_size);
    return true;
}
