#pragma once

#include "byte_file.h"
#include "virtual_stack.h"

typedef struct {
    byte_file *byteFile;
    virt_stack *vstack;
    char *ip;
} interpreter_state;

interpreter_state *create_interpreter_state(byte_file *byteFile, virt_stack *vstack) {
    interpreter_state *interpreterState = malloc(sizeof(interpreter_state));
    interpreterState->byteFile = byteFile;
    interpreterState->vstack = vstack;
    interpreterState->ip = byteFile->code_ptr;
    vstack_push(interpreterState->vstack, 0); // argv
    vstack_push(interpreterState->vstack, 0); // argc
    vstack_push(interpreterState->vstack, 2);
    return interpreterState;
}

u_int8_t get_next_byte(interpreter_state *interpreterState) {
    return *interpreterState->ip++;
}

u_int32_t get_next_int(interpreter_state *interpreterState) {
    interpreterState->ip += sizeof(int);
    return *(u_int32_t *) (interpreterState->ip - sizeof(int));
}

char *get_next_string(interpreter_state *interpreterState) {
    return interpreterState->byteFile->string_ptr + get_next_int(interpreterState);
}
