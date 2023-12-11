#include "byte_file.h"
#include "string.h"
#include "assert.h"
#include "interpreter_state.h"
#include "interpreter.h"
#include "virtual_stack.h"

extern u_int32_t *__gc_stack_top, *__gc_stack_bottom;
void *__start_custom_data, *__stop_custom_data;

extern void __gc_init(void);


int main(int argc, char *argv[]) {
    assert(argc == 3);
    if (strcmp(argv[1], "interpret") == 0) {
        byte_file *bf = read_file(argv[2]);
        virt_stack *vstack = create_virtual_stack();
        // place vstack between __gc_stack_bottom and __gc_stack_top for detection of lama GC and call extern __gc__init
        __gc_stack_top = vstack->sp_top;
        __gc_stack_bottom = vstack->sp_bottom;
        __gc_init();
        interpreter_state *intrp_state = create_interpreter_state(bf, vstack);
        interpret(intrp_state);
    } else if (strcmp(argv[1], "analyze") == 0) {
        // TODO:
    }
    return 0;
}
