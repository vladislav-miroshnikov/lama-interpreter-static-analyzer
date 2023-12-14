#pragma once

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

extern u_int32_t *__gc_stack_top, *__gc_stack_bottom;
void *__start_custom_data, *__stop_custom_data;

extern void __gc_init(void);

static size_t RUNTIME_VSTACK_SIZE = 1024 * 1024;
static u_int32_t *stack_fp;
static u_int32_t *stack_start;

typedef struct {
    byte_file *byteFile;
    char *ip;
} interpreter_state;

interpreter_state interpreterState;

static inline u_int8_t get_next_byte() {
    return *interpreterState.ip++;
}

static inline u_int32_t get_next_int() {
    interpreterState.ip += sizeof(int);
    return *(u_int32_t *) (interpreterState.ip - sizeof(int));
}

static inline char *get_next_string() {
    return interpreterState.byteFile->string_ptr + get_next_int();
}

static inline void vstack_push(u_int32_t value) {
    if (stack_start == __gc_stack_top) {
        failure("Severity ERROR: Virtual stack limit exceeded.\n");
    }
    *(--__gc_stack_top) = value;
}

static inline u_int32_t vstack_pop() {
    if (__gc_stack_top >= stack_fp) {
        failure("Severity ERROR: Illegal pop.\n");
    }
    return *(__gc_stack_top++);
}

static inline void copy_on_stack(u_int32_t value, int count) {
    for (int i = 0; i < count; ++i) {
        vstack_push(value);
    }
}

static inline void reverse_on_stack(int count) {
    u_int32_t *st = __gc_stack_top;
    u_int32_t *arg = st + count - 1;
    while (st < arg) {
        u_int32_t tmp = *st;
        *st = *arg;
        *arg = tmp;
        st++;
        arg--;
    }
}

u_int32_t *get_by_loc(u_int8_t bytecode, u_int32_t value) {
    switch (low_bits(bytecode)) {
        case GLOBAL:
            return interpreterState.byteFile->global_ptr + value;
        case LOCAL:
            return stack_fp - value - 1;
        case ARGUMENT:
            return stack_fp + value + 3;
        case CLOJURE : {
            u_int32_t n_args = *(stack_fp + 1);
            u_int32_t *argument = stack_fp + n_args + 2;
            u_int32_t *closure = (u_int32_t *) *argument;
            return (u_int32_t *) Belem_link(closure, BOX(value + 1));
        }
        default:
            failure("Severity ERROR: Invalid bytecode for loc.\n");
    }
}

void exec_binop(u_int8_t bytecode) {
#define BINOP_CODE(OP) \
        {              \
        int b = UNBOX(vstack_pop()); \
        int a = UNBOX(vstack_pop()); \
        vstack_push(BOX(a OP b)); \
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

void exec_ld(u_int8_t bytecode) {
    u_int32_t index = get_next_int();
    u_int32_t value = *get_by_loc(bytecode, index);
    vstack_push(value);
}

void exec_lda(u_int8_t bytecode) {
    u_int32_t index = get_next_int();
    u_int32_t value = (u_int32_t) get_by_loc(bytecode, index);
    vstack_push(value);
}

void exec_st(u_int8_t bytecode) {
    u_int32_t index = get_next_int();
    u_int32_t value = vstack_pop();
    *get_by_loc(bytecode, index) = value;
    vstack_push(value);
}

void exec_patt(u_int8_t bytecode) {
    u_int32_t *element = (u_int32_t *) vstack_pop();
    u_int32_t result = -1;
    switch (low_bits(bytecode)) {
        case PATT_STR: {
            result = Bstring_patt(element, (u_int32_t *) vstack_pop());
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
    vstack_push(result);
}

void exec_const() {
    u_int32_t const_v = BOX(get_next_int());
    vstack_push(const_v);
}

void exec_string() {
    char *string = get_next_string();
    vstack_push((u_int32_t) Bstring(string));
}

void exec_sexp() {
    char *sexp_name = get_next_string();
    u_int32_t sexp_tag = LtagHash(sexp_name);
    u_int32_t sexp_arity = get_next_int();
    reverse_on_stack(sexp_arity);
    u_int32_t bsexp = (u_int32_t) Bsexp_my(BOX(sexp_arity + 1), sexp_tag, __gc_stack_top);
    __gc_stack_top += sexp_arity;
    vstack_push(bsexp);
}

void exec_sta() {
    u_int32_t value = vstack_pop();
    u_int32_t index = vstack_pop();
    u_int32_t bsta;
    if (UNBOXED(index)) {
        bsta = (u_int32_t) Bsta((void *) value, index, (void *) vstack_pop());
    } else {
        bsta = (u_int32_t) Bsta((void *) value, index, 0);
    }
    vstack_push(bsta);
}

void jump(u_int32_t ip_offset) {
    interpreterState.ip = interpreterState.byteFile->code_ptr + ip_offset;
}

void exec_jmp() {
    u_int32_t ip_offset = get_next_int();
    jump(ip_offset);
}

void exec_cjmp_z() {
    u_int32_t ip_offset = get_next_int();
    int cmp_value = UNBOX(vstack_pop());
    if (cmp_value == 0) {
        jump(ip_offset);
    }
}

void exec_cjmp_nz() {
    u_int32_t ip_offset = get_next_int();
    int cmp_value = UNBOX(vstack_pop());
    if (cmp_value != 0) {
        jump(ip_offset);
    }
}

void exec_call_read() {
    int r = Lread();
    vstack_push(r);
}

void exec_call_write() {
    int w = Lwrite(vstack_pop());
    vstack_push(w);
}

void exec_call_string() {
    u_int32_t s = (u_int32_t) Lstring((void *) vstack_pop());
    vstack_push(s);
}

void exec_call_length() {
    u_int32_t l = (u_int32_t) Llength((void *) vstack_pop());
    vstack_push(l);
}

void exec_call_array() {
    u_int32_t len = get_next_int();
    reverse_on_stack(len);
    u_int32_t result = (u_int32_t) Barray_my(BOX(len), (int *) __gc_stack_top);
    __gc_stack_top += len;
    vstack_push(result);
}

void exec_closure() {
    u_int32_t ip = get_next_int();
    u_int32_t bn = get_next_int();
    u_int32_t values[bn];
    for (int i = 0; i < bn; ++i) {
        u_int8_t b = (u_int8_t) get_next_byte();
        u_int32_t value = (u_int32_t) get_next_int();
        values[i] = *get_by_loc(b, value);
    }
    u_int32_t blosure = (u_int32_t) Bclosure_my(BOX(bn), interpreterState.byteFile->code_ptr + ip, values);
    vstack_push(blosure);
}

void exec_elem() {
    u_int32_t index = vstack_pop();
    void *p = (void *) vstack_pop();
    u_int32_t belem = (u_int32_t) Belem(p, index);
    vstack_push(belem);
}

void exec_begin() {
    u_int32_t n_args = get_next_int();
    u_int32_t n_locals = get_next_int();
    vstack_push((u_int32_t) stack_fp);
    stack_fp = __gc_stack_top;
    copy_on_stack(BOX(0), n_locals);
}


void exec_end() {
    u_int32_t return_value = vstack_pop();
    __gc_stack_top = stack_fp;
    u_int32_t value_on_stack = *(__gc_stack_top++);
    stack_fp = (u_int32_t *) value_on_stack;
    u_int32_t n_args = vstack_pop();
    char *addr = (char *) vstack_pop();
    __gc_stack_top += n_args;
    vstack_push(return_value);
    interpreterState.ip = addr;
}

void exec_drop() {
    vstack_pop();
}

void exec_dup() {
    copy_on_stack(vstack_pop(), 2);
}

void exec_tag() {
    char *tag_name = get_next_string();
    u_int32_t n = get_next_int();
    u_int32_t t = LtagHash(tag_name);
    void *d = (void *) vstack_pop();
    vstack_push(Btag(d, t, BOX(n)));
}

void exec_array() {
    u_int32_t len = get_next_int();
    u_int32_t array = Barray_patt((u_int32_t *) vstack_pop(), BOX(len));
    vstack_push(array);
}

void exec_fail() {
    u_int32_t a = get_next_int();
    u_int32_t b = get_next_int();
    failure("Severity RUNTIME: Failed executing FAIL %d %d.\n", a, b);
}

void exec_line() {
    get_next_int();
}

void exec_swap() {
    reverse_on_stack(2);
}

void exec_call() {
    u_int32_t call_offset = get_next_int();
    u_int32_t n_args = get_next_int();
    reverse_on_stack(n_args);
    vstack_push((u_int32_t) interpreterState.ip);
    vstack_push(n_args);
    interpreterState.ip = interpreterState.byteFile->code_ptr + call_offset;
}

void exec_callc() {
    u_int32_t n_args = get_next_int();
    char *callee = (char *) Belem((u_int32_t *) __gc_stack_top[n_args], BOX(0));
    reverse_on_stack(n_args);
    vstack_push((u_int32_t) interpreterState.ip);
    vstack_push(n_args + 1);
    interpreterState.ip = callee;
}


void init_interpreter(byte_file *bf) {
    stack_start = malloc(RUNTIME_VSTACK_SIZE * sizeof(u_int32_t));
    if (stack_start == NULL) {
        failure("Severity ERROR: Failed to allocate memory for virtual stack.\n");
    }
    // init __gc_stack_bottom and __gc_stack_top for detection of lama GC and call extern __gc__init
    __gc_stack_bottom = __gc_stack_top = stack_start + RUNTIME_VSTACK_SIZE;
    __gc_init();

    stack_fp = __gc_stack_top;
    vstack_push(0); // argv
    vstack_push(0); // argc
    vstack_push(2);

    interpreterState.byteFile = bf;
    interpreterState.ip = bf->code_ptr;
}


// simple iterative bytecode interpreter
void interpret() {
    do {
        u_int8_t bytecode = get_next_byte();
        bytecode_type bc_type = get_bytecode_type(bytecode);
#define EXEC_WITH_LOWER_BITS(BC_NAME, EXEC_SUFFIX) \
        case BC_NAME:              \
            exec_##EXEC_SUFFIX(bytecode);  \
            break;
#define EXEC(BC_NAME, EXEC_SUFFIX) \
        case BC_NAME:              \
            exec_##EXEC_SUFFIX();  \
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
            EXEC(XSTRING, string)
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
    } while (interpreterState.ip != 0);
#undef EXEC_WITH_LOWER_BITS
#undef EXEC
}