#pragma once

#include <stdlib.h>
#include "string.h"
#include "stdio.h"
#include "malloc.h"
#include "../byte_file.h"
#include "../hashtable/hashtable.h"
#include "../hashtable/hashtable_utility.h"
#include "../hashtable/hashtable_itr.h"

typedef struct {
    char *ip;
    int frequency;
    int length;
} bytecode;

int bytecode_comparator(bytecode *bytecode1, bytecode *bytecode2) {
    return !memcmp(bytecode1->ip, bytecode2->ip, bytecode1->length) && bytecode1->length == bytecode2->length;
}

int frequency_comparator(const bytecode *bytecode1, const bytecode *bytecode2) {
    return bytecode2->frequency - bytecode1->frequency;
}

unsigned int hash_bytecode(const bytecode *bc) {
    size_t hash = 0;
    for (int i = 0; i < bc->length; ++i) {
        hash = bc->ip[i] + (hash << 8);
    }
    return hash;
}

static const char *const binop_types[] = {"+", "-", "*", "/", "%", "<", "<=", ">", ">=", "==", "!=", "&&", "!!"};
static const char *const patt_types[] = {"=str", "#string", "#array", "#sexp", "#ref", "#val", "#fun"};
static const char *const load_types[] = {"LD", "LDA", "ST"};


int empty_printer(FILE *f, const char *value, ...) {
    return 0;
}

char *analyze_bytecode(FILE *f, byte_file *byteFile, char *ip, int (*printer)(FILE *, const char *, ...)) {
#define NEXT_BYTE  (*ip++)
#define NEXT_INT (ip += sizeof(int), *(int*)(ip - sizeof(int)))
#define NEXT_STRING (byteFile->string_ptr + NEXT_INT)
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
        case STRING: {
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
    char *ip = byteFile->code_ptr;
    struct hashtable *ht = create_hashtable(1000, (unsigned int (*)(void *)) hash_bytecode,
                                            (int (*)(void *, void *)) bytecode_comparator);
    int total_different_bytecodes = 0;
    while (ip < byteFile->code_ptr + byteFile->bytecode_size) {
        bytecode *bc = malloc(sizeof(bytecode));
        if (bc == NULL) {
            failure("Severity ERROR: Can't allocate memory.\n");
        }
        char *next_ip = analyze_bytecode(f, byteFile, ip, &empty_printer);
        bc->ip = ip;
        bc->length = next_ip - ip;
        ip = next_ip;
        int *curr_freq = (int *) (hashtable_search(ht, bc));
        if (curr_freq != NULL) {
            int *new_freq = malloc(sizeof(int));
            if (new_freq == NULL) {
                failure("Severity ERROR: Can't allocate memory.\n");
            }
            new_freq[0] = *curr_freq + 1;
            bc->frequency = *curr_freq + 1;
            hashtable_change(ht, bc, new_freq);
        } else {
            int *new_freq = malloc(sizeof(int));
            if (new_freq == NULL) {
                failure("Severity ERROR: Can't allocate memory.\n");
            }
            new_freq[0] = 1;
            bc->frequency = 1;
            hashtable_insert(ht, bc, new_freq);
            total_different_bytecodes++;
        }
    }

    bytecode *different_bytecodes = malloc(total_different_bytecodes * sizeof(bytecode));
    if (different_bytecodes == NULL) {
        failure("Severity ERROR: Can't allocate memory.\n");
    }
    struct hashtable_itr *ht_iter = hashtable_iterator(ht);
    int i = 0;
    do {
        bytecode *key = hashtable_iterator_key(ht_iter);
        int *freq = hashtable_iterator_value(ht_iter);
        bytecode bc = {.ip = key->ip, .frequency = *freq, .length = key->length};
        different_bytecodes[i++] = bc;
    } while (hashtable_iterator_advance(ht_iter));

    qsort(different_bytecodes, total_different_bytecodes, sizeof(bytecode), (__compar_fn_t) frequency_comparator);
    for (int i = 0; i < total_different_bytecodes; ++i) {
        fprintf(f, "%d occurrences of bytecode: \"", different_bytecodes[i].frequency);
        analyze_bytecode(f, byteFile, different_bytecodes[i].ip, &fprintf);
        fprintf(f, "\"\n");
    }
    hashtable_destroy(ht, 1);
}
