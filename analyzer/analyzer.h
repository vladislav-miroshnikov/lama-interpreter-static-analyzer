#pragma once

#include <stdlib.h>
#include "string.h"
#include "stdio.h"
#include "malloc.h"
#include "../byte_file.h"
#include "../set/set.h"

typedef struct {
    const char *ip;
    int frequency;
    int length;
} bytecode;

int bytecode_comparator(void *ut1, void *ut2) {
    bytecode *bytecode1 = (bytecode *) ut1;
    bytecode *bytecode2 = (bytecode *) ut2;
    return !memcmp(bytecode1->ip, bytecode2->ip, bytecode1->length) && bytecode1->length == bytecode2->length;
}

int frequency_comparator(const bytecode *bytecode1, const bytecode *bytecode2) {
    return bytecode2->frequency - bytecode1->frequency;
}

int empty_printer(FILE *f, const char *value, ...) {
    return 0;
}

const char *analyze_bytecode(FILE *f, byte_file *byteFile, const char *ip, int (*printer)(FILE *, const char *, ...)) {
#define NEXT_BYTE  (*ip++)
#define NEXT_INT (ip += sizeof(int), *(int*)(ip - sizeof(int)))
#define NEXT_STRING (byteFile->string_ptr + NEXT_INT)
    static const char *const binop_types[] = {"+", "-", "*", "/", "%", "<", "<=", ">", ">=", "==", "!=", "&&", "!!"};
    static const char *const patt_types[] = {"=str", "#string", "#array", "#sexp", "#ref", "#val", "#fun"};
    static const char *const load_types[] = {"LD", "LDA", "ST"};

    u_int8_t bytecode = NEXT_BYTE;
    bytecode_type bc_type = get_bytecode_type(bytecode);
    switch (high_bits(bytecode)) {
        case BINOP_HIGH_BITS:
            printer(f, "BINOP\t%s", binop_types[low_bits(bytecode)]);
            return ip;
        case LD_HIGH_BITS:
        case LDA_HIGH_BITS:
        case ST_HIGH_BITS:
            printer(f, "%s\t", load_types[high_bits(bytecode) - 2]);
            switch (low_bits(bytecode)) {
                case GLOBAL:
                    printer(f, "G(%d)", NEXT_INT);
                    break;
                case LOCAL:
                    printer(f, "L(%d)", NEXT_INT);
                    break;
                case ARGUMENT:
                    printer(f, "A(%d)", NEXT_INT);
                    break;
                case CLOJURE:
                    printer(f, "C(%d)", NEXT_INT);
                    break;
                default:
                    failure("Severity ERROR: Invalid code for loc.\n");
            }
            return ip;
        case PATT_HIGH_BITS:
            printer(f, "PATT\t%s\n", patt_types[low_bits(bytecode)]);
            return ip;
        case 0xF:
            printer(f, "STOP");
            return ip;
    }

    switch (bc_type) {
        case CONST: {
            printer(f, "CONST\t%d", NEXT_INT);
            break;
        }
        case XSTRING: {
            printer(f, "STRING\t%s", NEXT_STRING);
            break;
        }
        case SEXP: {
            printer(f, "SEXP\t%s ", NEXT_STRING);
            printer(f, "%d", NEXT_INT);
            break;
        }
        case ST: {
            printer(f, "STI");
            break;
        }
        case STA: {
            printer(f, "STA");
            break;
        }
        case JMP: {
            printer(f, "JMP\t0x%.8x", NEXT_INT);
            break;
        }
        case CJMP_Z: {
            printer(f, "CJMPz\t0x%.8x", NEXT_INT);
            break;
        }
        case CJMP_NZ: {
            printer(f, "CJMPnz\t0x%.8x", NEXT_INT);
            break;
        }
        case CLOSURE: {
            printer(f, "CLOSURE\t0x%.8x", NEXT_INT);
            int n = NEXT_INT;
            for (int i = 0; i < n; ++i) {
                switch (NEXT_BYTE) {
                    case GLOBAL:
                        printer(f, "G(%d)", NEXT_INT);
                        break;
                    case LOCAL:
                        printer(f, "L(%d)", NEXT_INT);
                        break;
                    case ARGUMENT:
                        printer(f, "A(%d)", NEXT_INT);
                        break;
                    case CLOJURE:
                        printer(f, "C(%d)", NEXT_INT);
                        break;
                    default:
                        failure("Severity ERROR: Invalid code for loc.\n");
                }
            }
            break;
        }
        case ELEM: {
            printer(f, "ELEM");
            break;
        }
        case BEGIN: {
            printer(f, "BEGIN\t%d ", NEXT_INT);
            printer(f, "%d", NEXT_INT);
            break;
        }
        case CALL: {
            printer(f, "CALL\t0x%.8x ", NEXT_INT);
            printer(f, "%d", NEXT_INT);
            break;
        }
        case CALLC: {
            printer(f, "CALLC\t%d", NEXT_INT);
            break;
        }
        case CALL_READ: {
            printer(f, "CALL\tLread");
            break;
        }
        case CALL_WRITE: {
            printer(f, "CALL\tLwrite");
            break;
        };
        case CALL_STRING: {
            printer(f, "CALL\tLstring");
            break;
        }
        case CALL_LENGTH: {
            printer(f, "CALL\tLlength");
            break;
        }
        case CALL_ARRAY: {
            printer(f, "CALL\tBarray\t%d", NEXT_INT);
            break;
        }
        case END: {
            printer(f, "END");
            break;
        }
        case DROP: {
            printer(f, "DROP");
            break;
        }
        case DUP: {
            printer(f, "DUP");
            break;
        }
        case TAG: {
            printer(f, "TAG\t%s ", NEXT_STRING);
            printer(f, "%d", NEXT_INT);
            break;
        }
        case SWAP: {
            printer(f, "SWAP");
            break;
        }
        case ARRAY: {
            printer(f, "ARRAY\t%d", NEXT_INT);
            break;
        }
        case FAIL: {
            printer(f, "FAIL\t%d", NEXT_INT);
            printer(f, "%d", NEXT_INT);
            break;
        }
        case LINE: {
            printer(f, "LINE\t%d", NEXT_INT);
            break;
        }
        case RET: {
            printer(f, "RET");
            break;
        }
        default:
            failure("Severity ERROR: Unknown bytecode type.\n");
    }
    return ip;
#undef NEXT_BYTE
#undef NEXT_INT
#undef NEXT_STRING
}

void analyze_bytecode_frequency(FILE *f, byte_file *byteFile) {
    const char *ip = byteFile->code_ptr;
    struct set *s = set_init();
    struct adt_funcs adt;
    adt.ptr_equality = bytecode_comparator;
    set_add_adt(s, &adt, USER_DEFINED);
    while (ip < byteFile->code_ptr + byteFile->bytecode_size) {
        bytecode *bc = malloc(sizeof(bytecode));
        if (bc == NULL) {
            failure("Severity ERROR: Can't allocate memory.\n");
        }
        const char *next_ip = analyze_bytecode(f, byteFile, ip, &empty_printer);
        bc->ip = ip;
        bc->length = next_ip - ip;
        ip = next_ip;
        struct node *n = find_node(s, (void *) bc, USER_DEFINED);
        if (n != NULL) {
            bytecode *existed_bc = (bytecode *) node_get_data(n);
            existed_bc->frequency++;
            free(bc);
        } else {
            bc->frequency = 1;
            set_add(s, (void *) bc, USER_DEFINED);
        }
    }

    bytecode *bytecodes = malloc(s->num * sizeof(bytecode));
    if (bytecodes == NULL) {
        failure("Severity ERROR: Can't allocate memory.\n");
    }

    struct node *n;
    int i = 0;
    for (n = set_first(s); set_done(s); n = set_next(s)) {
        bytecode *set_bc = (bytecode *) node_get_data(n);
        bytecode bc = {.ip = set_bc->ip, .frequency = set_bc->frequency, .length = set_bc->length};
        bytecodes[i++] = bc;
    }

    qsort(bytecodes, s->num, sizeof(bytecode), (__compar_fn_t) frequency_comparator);
    for (int i = 0; i < s->num; ++i) {
        fprintf(f, "%d occurrences of bytecode: \"", bytecodes[i].frequency);
        analyze_bytecode(f, byteFile, bytecodes[i].ip, &fprintf);
        fprintf(f, "\"\n");
    }
    free(bytecodes);
    set_free(s);
}
