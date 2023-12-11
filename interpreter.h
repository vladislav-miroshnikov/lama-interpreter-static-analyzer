#pragma once

#include "interpreter_state.h"
#include "bytecode_decoder.h"
#include "byte_file.h"
#include <stdbool.h>

extern int Lread();

extern int Lwrite(int);

extern int Llength(void *);

extern void *Lstring(void *p);

extern void *Bstring(void *);

extern void *Belem(void *p, int i);

extern void *Bsta(void *v, int i, void *x);

extern void *Barray_my(int bn, int *data_);

extern void *Bsexp_my(int bn, int tag, int *data_);

extern int LtagHash(char *s);

extern int Btag(void *d, int t, int n);

extern int Barray_patt(void *d, int n);

extern void *Bclosure_my(int bn, void *entry, int *values);

extern void *Belem_link(void *p, int i);

extern int Bstring_patt(void *x, void *y);

extern int Bstring_tag_patt(void *x);

extern int Barray_tag_patt(void *x);

extern int Bsexp_tag_patt(void *x);

extern int Bunboxed_patt(void *x);

extern int Bboxed_patt(void *x);

extern int Bclosure_tag_patt(void *x);

// redefined from runtime.c for performance using the preprocessor
# define UNBOXED(x)  (((int) (x)) &  0x0001)
# define UNBOX(x)    (((int) (x)) >> 1)
# define BOX(x)      ((((int) (x)) << 1) | 0x0001)


int32_t *get_by_loc(interpreter_state *interpreterState, u_int8_t bytecode, int32_t value) {
    switch (low_bits(bytecode)) {
        case GLOBAL:
            return interpreterState->byteFile->global_ptr + value;
        case LOCAL:
            return interpreterState->vstack->fp - value - 1;
        case ARGUMENT:
            return interpreterState->vstack->fp + value + 3;
        case CLOJURE : {
            int32_t n_args = *(interpreterState->vstack->fp + 1);
            int32_t *argument = interpreterState->vstack->fp + n_args + 2;
            int32_t *closure = (int32_t *) *argument;
            return (int32_t *) Belem_link(closure, BOX(value + 1));
        }
        default:
            failure("Severity ERROR: Invalid bytecode for loc.\n");
    }
}

void exec_binop(u_int8_t bytecode, interpreter_state *interpreterState) {
#define BINOP_CODE(OP) \
        {              \
        virt_stack *vstack = interpreterState->vstack; \
        int b = UNBOX(vstack_pop(vstack)); \
        int a = UNBOX(vstack_pop(vstack)); \
        vstack_push(vstack, BOX(a OP b));              \
        return; \
        }
    switch (low_bits(bytecode)) {
        case PLUS: BINOP_CODE(+)
        case MINUS: BINOP_CODE(-)
        case MULTIPLY: BINOP_CODE(*)
        case DIVIDE: BINOP_CODE(/)
        case REMAINDER: BINOP_CODE(%)
        case LESS: BINOP_CODE(<)
        case LESS_EQUAL: BINOP_CODE(<=)
        case GREATER: BINOP_CODE(>)
        case GREATER_EQUAL: BINOP_CODE(>=)
        case EQUAL: BINOP_CODE(==)
        case NOT_EQUAL: BINOP_CODE(!=)
        case AND: BINOP_CODE(&&)
        case OR: BINOP_CODE(||)
        default:
            failure("Severity ERROR: Unknown binop bytecode.\n");
    }
#undef BINOP_CODE
}

void exec_ld(u_int8_t bytecode, interpreter_state *interpreterState) {
    int32_t index = get_next_int(interpreterState);
    int32_t value = *get_by_loc(interpreterState, bytecode, index);
    vstack_push(interpreterState->vstack, value);
}

void exec_lda(u_int8_t bytecode, interpreter_state *interpreterState) {
    int32_t index = get_next_int(interpreterState);
    int32_t value = (int32_t) get_by_loc(interpreterState, bytecode, index);
    vstack_push(interpreterState->vstack, value);
}

void exec_st(u_int8_t bytecode, interpreter_state *interpreterState) {
    int32_t index = get_next_int(interpreterState);
    int32_t value = vstack_pop(interpreterState->vstack);
    *get_by_loc(interpreterState, bytecode, index) = value;
    vstack_push(interpreterState->vstack, value);
}

void exec_patt(u_int8_t bytecode, interpreter_state *interpreterState) {
    int32_t *element = (int32_t *) vstack_pop(interpreterState->vstack);
    int32_t result = -1;
    switch (low_bits(bytecode)) {
        case PATT_STR: {
            result = Bstring_patt(element, (int32_t *) vstack_pop(interpreterState->vstack));
            break;
        }
        case PATT_TAG_STR: {
            result = Bstring_tag_patt(element);
            break;
        }
        case PATT_TAG_ARR: {
            result = Barray_tag_patt(element);
            break;
        }
        case PATT_TAG_SEXP: {
            result = Bsexp_tag_patt(element);
            break;
        }
        case PATT_BOXED: {
            result = Bboxed_patt(element);
            break;
        }
        case PATT_UNBOXED: {
            result = Bunboxed_patt(element);
            break;
        }
        case PATT_TAG_CLOSURE: {
            result = Bclosure_tag_patt(element);
            break;
        }
        default: {
            failure("Severity RUNTIME: Unknown pattern type.\n");
        }
    }
    vstack_push(interpreterState->vstack, result);
}

void exec_const(interpreter_state *interpreterState) {
    int32_t const_v = BOX(get_next_int(interpreterState));
    vstack_push(interpreterState->vstack, const_v);
}

void exec_string(interpreter_state *interpreterState) {
    char *string = get_next_string(interpreterState);
    vstack_push(interpreterState->vstack, (int32_t) Bstring(string));
}

void exec_sexp(interpreter_state *interpreterState) {
    char *sexp_name = get_next_string(interpreterState);
    int32_t sexp_tag = LtagHash(sexp_name);
    int32_t sexp_arity = get_next_int(interpreterState);
    reverse_on_stack(interpreterState->vstack, sexp_arity);
    int32_t bsexp = (int32_t) Bsexp_my(BOX(sexp_arity + 1), sexp_tag, interpreterState->vstack->sp_bottom);
    interpreterState->vstack->sp_bottom += sexp_arity;
    vstack_push(interpreterState->vstack, bsexp);
}

void exec_sta(interpreter_state *interpreterState) {
    virt_stack *vstack = interpreterState->vstack;
    int32_t value = vstack_pop(vstack);
    int32_t index = vstack_pop(vstack);
    int32_t bsta;
    if (UNBOXED(index)) {
        bsta = (int32_t) Bsta((void *) value, index, (void *) vstack_pop(vstack));
    } else {
        bsta = (int32_t) Bsta((void *) value, index, 0);
    }
    vstack_push(vstack, bsta);
}

void jump(interpreter_state *interpreterState, int32_t ip_offset) {
    interpreterState->ip = interpreterState->byteFile->code_ptr + ip_offset;
}

void exec_jmp(interpreter_state *interpreterState) {
    int32_t ip_offset = get_next_int(interpreterState);
    jump(interpreterState, ip_offset);
}

void exec_cjmp_z(interpreter_state *interpreterState) {
    int32_t ip_offset = get_next_int(interpreterState);
    int cmp_value = UNBOX(vstack_pop(interpreterState->vstack));
    if (cmp_value == 0) {
        jump(interpreterState, ip_offset);
    }
}

void exec_cjmp_nz(interpreter_state *interpreterState) {
    int32_t ip_offset = get_next_int(interpreterState);
    int cmp_value = UNBOX(vstack_pop(interpreterState->vstack));
    if (cmp_value != 0) {
        jump(interpreterState, ip_offset);
    }
}

void exec_call_read(interpreter_state *interpreterState) {
    int r = Lread();
    vstack_push(interpreterState->vstack, r);
}

void exec_call_write(interpreter_state *interpreterState) {
    int w = Lwrite(vstack_pop(interpreterState->vstack));
    vstack_push(interpreterState->vstack, w);
}

void exec_call_string(interpreter_state *interpreterState) {
    int32_t s = (int32_t) Lstring((void *) vstack_pop(interpreterState->vstack));
    vstack_push(interpreterState->vstack, s);
}

void exec_call_length(interpreter_state *interpreterState) {
    int32_t l = (int32_t) Llength((void *) vstack_pop(interpreterState->vstack));
    vstack_push(interpreterState->vstack, l);
}

void exec_call_array(interpreter_state *interpreterState) {
    int32_t len = get_next_int(interpreterState);
    reverse_on_stack(interpreterState->vstack, len);
    int32_t result = (int32_t) Barray_my(BOX(len), (int *) interpreterState->vstack->sp_bottom);
    interpreterState->vstack->sp_bottom += len;
    vstack_push(interpreterState->vstack, result);
}

void exec_closure(interpreter_state *interpreterState) {
    int32_t ip = get_next_int(interpreterState);
    int32_t bn = get_next_int(interpreterState);
    int32_t values[bn];
    for (int i = 0; i < bn; ++i) {
        u_int8_t b = (u_int8_t) get_next_byte(interpreterState);
        int32_t value = (int32_t) get_next_int(interpreterState);
        values[i] = *get_by_loc(interpreterState, b, value);
    }
    int32_t blosure = (int32_t) Bclosure_my(BOX(bn), interpreterState->byteFile->code_ptr + ip, values);
    vstack_push(interpreterState->vstack, blosure);
}

void exec_elem(interpreter_state *interpreterState) {
    virt_stack *vstack = interpreterState->vstack;
    int32_t index = vstack_pop(vstack);
    void *p = (void *) vstack_pop(vstack);
    int32_t belem = (int32_t) Belem(p, index);
    vstack_push(vstack, belem);
}

void exec_begin(interpreter_state *interpreterState) {
    int32_t n_args = get_next_int(interpreterState);
    int32_t n_locals = get_next_int(interpreterState);
    vstack_push(interpreterState->vstack, (int32_t) interpreterState->vstack->fp);
    interpreterState->vstack->fp = interpreterState->vstack->sp_bottom;
    copy_on_stack(interpreterState->vstack, BOX(0), n_locals);
}


void exec_end(interpreter_state *interpreterState) {
    virt_stack *vstack = interpreterState->vstack;
    int32_t return_value = vstack_pop(vstack);
    vstack->sp_bottom = vstack->fp;
    int32_t value_on_stack = *(vstack->sp_bottom++);
    vstack->fp = (int32_t *) value_on_stack;
    int32_t n_args = vstack_pop(vstack);
    char *addr = (char *) vstack_pop(vstack);
    vstack->sp_bottom += n_args;
    vstack_push(vstack, return_value);
    interpreterState->ip = addr;
}

void exec_drop(interpreter_state *interpreterState) {
    vstack_pop(interpreterState->vstack);
}

void exec_dup(interpreter_state *interpreterState) {
    copy_on_stack(interpreterState->vstack, vstack_pop(interpreterState->vstack), 2);
}

void exec_tag(interpreter_state *interpreterState) {
    char *tag_name = get_next_string(interpreterState);
    int32_t n = get_next_int(interpreterState);
    int32_t t = LtagHash(tag_name);
    void *d = (void *) vstack_pop(interpreterState->vstack);
    vstack_push(interpreterState->vstack, Btag(d, t, BOX(n)));
}

void exec_array(interpreter_state *interpreterState) {
    virt_stack *vstack = interpreterState->vstack;
    int32_t len = get_next_int(interpreterState);
    int32_t array = Barray_patt((int32_t *) vstack_pop(vstack), BOX(len));
    vstack_push(vstack, array);
}

void exec_fail(interpreter_state *interpreterState) {
    int32_t a = get_next_int(interpreterState);
    int32_t b = get_next_int(interpreterState);
    failure("Severity RUNTIME: Failed executing FAIL %d %d.\n", a, b);
}

void exec_line(interpreter_state *interpreterState) {
    get_next_int(interpreterState);
}

void exec_swap(interpreter_state *interpreterState) {
    reverse_on_stack(interpreterState->vstack, 2);
}

void exec_call(interpreter_state *interpreterState) {
    int32_t call_offset = get_next_int(interpreterState);
    int32_t n_args = get_next_int(interpreterState);
    reverse_on_stack(interpreterState->vstack, n_args);
    vstack_push(interpreterState->vstack, (int32_t) interpreterState->ip);
    vstack_push(interpreterState->vstack, n_args);
    interpreterState->ip = interpreterState->byteFile->code_ptr + call_offset;
}

void exec_callc(interpreter_state *interpreterState) {
    int32_t n_args = get_next_int(interpreterState);
    char *callee = (char *) Belem((int32_t *) interpreterState->vstack->sp_bottom[n_args], BOX(0));
    reverse_on_stack(interpreterState->vstack, n_args);
    vstack_push(interpreterState->vstack, (int32_t) interpreterState->ip);
    vstack_push(interpreterState->vstack, n_args + 1);
    interpreterState->ip = callee;
}


// simple iterative bytecode interpreter
void interpret(interpreter_state *interpreterState) {
    do {
        u_int8_t bytecode = get_next_byte(interpreterState);
        bytecode_type bc_type = get_bytecode_type(bytecode);
#define EXEC_WITH_LOWER_BITS(BC_NAME, EXEC_SUFFIX) \
        case BC_NAME:              \
            exec_##EXEC_SUFFIX(bytecode, interpreterState);  \
            break;
#define EXEC(BC_NAME, EXEC_SUFFIX) \
        case BC_NAME:              \
            exec_##EXEC_SUFFIX(interpreterState);  \
            break;
        switch (bc_type) {
            /** interpret bytecodes with meaningful lower bits */
            EXEC_WITH_LOWER_BITS(BINOP, binop)
            EXEC_WITH_LOWER_BITS(LD, ld)
            EXEC_WITH_LOWER_BITS(LDA, lda)
            EXEC_WITH_LOWER_BITS(ST, st)
            EXEC_WITH_LOWER_BITS(PATT, patt)
            /** interpret other bytecodes  */
            EXEC(CONST, const)
            EXEC(STRING, string)
            EXEC(SEXP, sexp)
            EXEC(STA, sta)
            EXEC(JMP, jmp)
            EXEC(CJMP_Z, cjmp_z)
            EXEC(CJMP_NZ, cjmp_nz)
            EXEC(ELEM, elem)
            EXEC(BEGIN, begin)
            EXEC(CALL, call)
            EXEC(CALLC, callc)
            EXEC(CALL_READ, call_read)
            EXEC(CALL_WRITE, call_write)
            EXEC(CALL_STRING, call_string)
            EXEC(CALL_LENGTH, call_length)
            EXEC(CALL_ARRAY, call_array)
            EXEC(END, end)
            EXEC(DROP, drop)
            EXEC(DUP, dup)
            EXEC(TAG, tag)
            EXEC(ARRAY, array)
            EXEC(FAIL, fail)
            EXEC(LINE, line)
            EXEC(CLOSURE, closure)
            EXEC(SWAP, swap)
            case STI:
                failure("Severity RUNTIME: STI bytecode is deprecated.\n");
                break;
            case RET:
                failure("Severity RUNTIME: RET bytecode has UB.\n");
                break;
            default:
                failure("Severity ERROR: Unknown bytecode type.\n");
        }
    } while (interpreterState->ip != 0);
#undef EXEC_WITH_LOWER_BITS
#undef EXEC
}