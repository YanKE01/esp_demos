#include "dl_calloc.h"
#include "stdio.h"
#include "string.h"

void *dl_lib_calloc(int cnt, int size, int align)
{
    int total_size = cnt * size + align + sizeof(void *);
    void *res = malloc(total_size);
    // void *res = NULL;
    if (NULL == res)
    {
        printf("Item alloc failed. Size: %d = %d x %d + %d + %d\n", total_size, cnt, size, align, sizeof(void *));
        return NULL;
    }
    bzero(res, total_size);
    void **data = (void **)res + 1;
    void **aligned;
    if (align)
        aligned = (void **)(((size_t)data + (align - 1)) & -align);
    else
        aligned = data;

    aligned[-1] = res;
    return (void *)aligned;
}

void dl_lib_free(void *d)
{
    if (NULL == d)
        return;

    free(((void **)d)[-1]);
}
