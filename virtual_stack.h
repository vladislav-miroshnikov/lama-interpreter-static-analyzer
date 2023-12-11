#pragma once

#include <stdio.h>
#include "runtime/runtime.h"

typedef struct {
    u_int32_t *fp;
    u_int32_t *sp_bottom;
    u_int32_t *sp_top;
} virt_stack;

static size_t RUNTIME_VSTACK_SIZE = 1024 * 1024;

virt_stack *create_virtual_stack() {
    virt_stack *vstack = malloc(sizeof(virt_stack));
    vstack->sp_top = malloc(RUNTIME_VSTACK_SIZE * sizeof(u_int32_t)) + RUNTIME_VSTACK_SIZE;
    if (vstack->sp_top == NULL) {
        failure("Severity ERROR: Failed to allocate memory for virtual stack.\n");
    }
    vstack->sp_bottom = vstack->sp_top;
    vstack->fp = vstack->sp_bottom;
    return vstack;
}

void vstack_push(virt_stack *vstack, u_int32_t value) {
    if (vstack->sp_top == vstack->sp_bottom - RUNTIME_VSTACK_SIZE) {
        failure("Severity ERROR: Virtual stack limit exceeded.\n");
    }
    *(--vstack->sp_bottom) = value;
}

u_int32_t vstack_pop(virt_stack *vstack) {
    if (vstack->sp_bottom >= vstack->fp) {
        failure("Severity ERROR: Illegal pop.\n");
    }
    return *(vstack->sp_bottom++);
}

void copy_on_stack(virt_stack *vstack, u_int32_t value, int count) {
    for (int i = 0; i < count; ++i) {
        vstack_push(vstack, value);
    }
}

void reverse_on_stack(virt_stack *vstack, int count) {
    u_int32_t *st_bottom = vstack->sp_bottom;
    u_int32_t *arg = st_bottom + count - 1;
    while (st_bottom < arg) {
        u_int32_t tmp = *st_bottom;
        *st_bottom = *arg;
        *arg = tmp;
        st_bottom++;
        arg--;
    }
}