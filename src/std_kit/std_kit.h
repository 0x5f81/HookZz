#ifndef std_kit_std_kit_h
#define std_kit_std_kit_h

/* malloc with 0 and NULL check */
void *safe_malloc(size_t size);

/* malloc macro */
#define SAFE_MALLOC_TYPE(TYPE) (TYPE *)safe_malloc(sizeof(TYPE));

#endif