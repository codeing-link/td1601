/*
 * Copyright (C) 2016 YunOS Project. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <csi_config.h>
#include <string.h>
#ifndef CONFIG_KERNEL_NONE
#include <csi_kernel.h>
#include <drv/list.h>
#else
#include <drv/list.h>
#include <umm_heap.h>
#endif

#ifndef MALLOC_WEAK
#define MALLOC_WEAK __attribute__((weak))
#endif

typedef struct {
    slist_t next;
    void *point_real;
    void *point_used;
} align_heap_t;

slist_t align_heap_head;

MALLOC_WEAK void *malloc(size_t size)
{
    void *ret;

#ifdef CONFIG_KERNEL_NONE
    ret = mm_malloc(USR_HEAP, size, __builtin_return_address(0U));
#else
    ret = csi_kernel_malloc(size, __builtin_return_address(0U));
#endif

    return ret;
}
MALLOC_WEAK void free(void *ptr) __attribute__((optimize("O0")));
MALLOC_WEAK void free(void *ptr)
{
#ifdef CONFIG_KERNEL_NONE
    uint8_t match = 0;
    align_heap_t *node;
    slist_t *tmp;

    slist_for_each_entry_safe(&align_heap_head, tmp, node, align_heap_t, next) {
        if (ptr == node->point_used) {
            match = 1;
            mm_free(USR_HEAP, node->point_real, __builtin_return_address(0U));
            slist_del(&node->next, &align_heap_head);
            break;
        }
    }
    if (!match) {
        mm_free(USR_HEAP, ptr, __builtin_return_address(0U));
    }
#else
    csi_kernel_free(ptr, __builtin_return_address(0));
#endif
}

MALLOC_WEAK void *realloc(void *ptr, size_t size)
{
    void *new_ptr;

#ifdef CONFIG_KERNEL_NONE
    new_ptr = mm_malloc(USR_HEAP, size, __builtin_return_address(0U));
#else
    new_ptr = csi_kernel_malloc(size, __builtin_return_address(0U));
#endif

    if (new_ptr == NULL) {
        return new_ptr;
    }

    if (ptr) {
        memcpy(new_ptr, ptr, size);

#ifdef CONFIG_KERNEL_NONE
        mm_free(USR_HEAP, ptr, __builtin_return_address(0U));
#else
        csi_kernel_free(ptr, __builtin_return_address(0U));
#endif
    }

    return new_ptr;
}

MALLOC_WEAK void *calloc(size_t nmemb, size_t size)
{
    void *ptr = NULL;

#ifdef CONFIG_KERNEL_NONE
    ptr = mm_malloc(USR_HEAP, size * nmemb, __builtin_return_address(0U));
#else
    ptr = csi_kernel_malloc(size * nmemb, __builtin_return_address(0U));
#endif

    if (ptr) {
        memset(ptr, 0, size * nmemb);
    }

    return ptr;
}

MALLOC_WEAK void *memalign(size_t alignment, size_t size)
{
    void *ret;

#ifdef CONFIG_KERNEL_NONE
    align_heap_t *node = mm_malloc(USR_HEAP, sizeof(align_heap_t), __builtin_return_address(0U));

    node->point_real = mm_malloc(USR_HEAP, size + (alignment << 1), __builtin_return_address(0U));
    if (((unsigned long)node->point_real & (alignment - 1)) != 0) {
        node->point_used = (void *)(((unsigned long)node->point_real + alignment - 1) & ~(alignment -1));
    } else {
        node->point_used = node->point_real;
    }
    slist_add(&node->next, &align_heap_head);
    ret = node->point_used;
#else
    ret = csi_kernel_malloc(size, __builtin_return_address(0U));
#endif

    return ret;
}
