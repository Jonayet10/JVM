#include "jvm.h"

#include <assert.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "heap.h"
#include "read_class.h"

int const OFFSET_ICONST = 0x03;
int const OFFSET_ILOAD = 0x1a;
int const OFFSET_ALOAD = 0x2a;
int const OFFSET_ASTORE = 0x4b;
int const OFFSET_ISTORE = 0x3b;

/** The name of the method to invoke to run the class file */
const char MAIN_METHOD[] = "main";
/**
 * The "descriptor" string for main(). The descriptor encodes main()'s signature,
 * i.e. main() takes a String[] and returns void.
 * If you're interested, the descriptor string is explained at
 * https://docs.oracle.com/javase/specs/jvms/se12/html/jvms-4.html#jvms-4.3.2.
 */
const char MAIN_DESCRIPTOR[] = "([Ljava/lang/String;)V";

/**
 * Represents the return value of a Java method: either void or an int or a reference.
 * For simplification, we represent a reference as an index into a heap-allocated array.
 * (In a real JVM, methods could also return object references or other primitives.)
 */
typedef struct {
    /** Whether this returned value is an int */
    bool has_value;
    /** The returned value (only valid if `has_value` is true) */
    int32_t value;
} optional_value_t;

void push(int32_t *operand_stack, int32_t *stack_pointer, int32_t value) {
    operand_stack[++(*stack_pointer)] = value;
}

int32_t pop(int32_t *operand_stack, int32_t *stack_pointer) {
    return operand_stack[(*stack_pointer)--];
}

int32_t perform_binary_operation(int32_t value1, int32_t value2, u1 instruction) {
    switch (instruction) {
        case i_iadd:
            return value1 + value2;
        case i_isub:
            return value1 - value2;
        case i_imul:
            return value1 * value2;
        case i_idiv:
            return value1 / value2;
        case i_irem:
            return value1 % value2;
        case i_iand:
            return value1 & value2;
        case i_ior:
            return value1 | value2;
        case i_ixor:
            return value1 ^ value2;
        default:
            return 0; // or handle error
    }
}

bool should_jump_based_on_instruction(u1 instruction, int32_t *operand_stack,
                                      int32_t *stack_pointer) {
    switch (instruction) {
        case i_ifeq:
            return pop(operand_stack, stack_pointer) == 0;
        case i_ifne:
            return pop(operand_stack, stack_pointer) != 0;
        case i_iflt:
            return pop(operand_stack, stack_pointer) < 0;
        case i_ifge:
            return pop(operand_stack, stack_pointer) >= 0;
        case i_ifgt:
            return pop(operand_stack, stack_pointer) > 0;
        case i_ifle:
            return pop(operand_stack, stack_pointer) <= 0;
        case i_if_icmpeq: {
            int32_t value2 = pop(operand_stack, stack_pointer);
            int32_t value1 = pop(operand_stack, stack_pointer);
            return value1 == value2;
        }
        case i_if_icmpne: {
            int32_t value2 = pop(operand_stack, stack_pointer);
            int32_t value1 = pop(operand_stack, stack_pointer);
            return value1 != value2;
        }
        case i_if_icmplt: {
            int32_t value2 = pop(operand_stack, stack_pointer);
            int32_t value1 = pop(operand_stack, stack_pointer);
            return value1 < value2;
        }
        case i_if_icmpge: {
            int32_t value2 = pop(operand_stack, stack_pointer);
            int32_t value1 = pop(operand_stack, stack_pointer);
            return value1 >= value2;
        }
        case i_if_icmpgt: {
            int32_t value2 = pop(operand_stack, stack_pointer);
            int32_t value1 = pop(operand_stack, stack_pointer);
            return value1 > value2;
        }
        case i_if_icmple: {
            int32_t value2 = pop(operand_stack, stack_pointer);
            int32_t value1 = pop(operand_stack, stack_pointer);
            return value1 <= value2;
        }
        default:
            return false;
    }
}

/**
 * Runs a method's instructions until the method returns.
 *
 * @param method the method to run
 * @param locals the array of local variables, including the method parameters.
 *   Except for parameters, the locals are uninitialized.
 * @param class the class file the method belongs to
 * @param heap an array of heap-allocated pointers, useful for references
 * @return an optional int containing the method's return value
 */
optional_value_t execute(method_t *method, int32_t *locals, class_file_t *class,
                         heap_t *heap) {
    /* You should remove these casts to void in your solution.
     * They are just here so the code compiles without warnings. */

    u1 *bytecode =
        method->code
            .code; // pointer to bytecode (this array has instruction and its arguments)
    u4 code_length = method->code.code_length; // length of the bytecode
    u2 max_stack = method->code.max_stack;     // max depth of operand stack

    // create and initialize operand stack
    int32_t operand_stack[max_stack]; // (this array is for work stack)
    int32_t stack_pointer = -1;       // initialized to -1 cb stack is currently empty

    // memset
    memset(operand_stack, 0, sizeof(operand_stack));

    // initialize pc, type is u4 so that it can represent the u4 code_length
    u4 pc = 0;

    while (pc < code_length) {
        u1 instruction = bytecode[pc]; // fetch instruction

        switch (instruction) {
            case i_bipush:
                push(operand_stack, &stack_pointer,
                     (int32_t)(int8_t)(
                         uint8_t) bytecode[pc + 1]); // push signed byte onto stack
                pc += 2;                             // move to next instruction
                break;

            case i_getstatic:
                pc += 3; //  move past the opcode and its two-byte operand
                break;

            case i_invokevirtual: {
                int32_t val = pop(operand_stack, &stack_pointer);
                printf("%d\n", val); // print popped value and newline
            }
                pc += 3; // move past the opcode and its two-byte operands
                break;

            case i_iconst_m1:
            case i_iconst_0:
            case i_iconst_1:
            case i_iconst_2:
            case i_iconst_3:
            case i_iconst_4:
            case i_iconst_5:
                push(operand_stack, &stack_pointer, instruction - OFFSET_ICONST);
                pc++;
                break;

            case i_iadd:
            case i_isub:
            case i_imul:
            case i_idiv:
            case i_irem:
            case i_iand:
            case i_ior:
            case i_ixor: {
                int32_t value2 = pop(operand_stack, &stack_pointer);
                int32_t value1 = pop(operand_stack, &stack_pointer);
                int32_t result = perform_binary_operation(value1, value2, instruction);
                push(operand_stack, &stack_pointer, result);
                pc++;
                break;
            }

            case i_sipush:
                pc++;
                {
                    int16_t value =
                        (bytecode[pc] << 8) |
                        bytecode[pc + 1]; // push signed short (int16_t) value onto stack
                    push(operand_stack, &stack_pointer, value);
                }
                pc += 2;
                break;

            case i_ineg: {
                int32_t value = pop(operand_stack, &stack_pointer);
                push(operand_stack, &stack_pointer, -value);
            }
                pc++;
                break;

            case i_ishl: {
                int32_t shift_amount = pop(operand_stack, &stack_pointer);
                int32_t value = pop(operand_stack, &stack_pointer);
                push(operand_stack, &stack_pointer, value << shift_amount);
            }
                pc++;
                break;

            case i_ishr: {
                int32_t shift_amount = pop(operand_stack, &stack_pointer);
                int32_t value = pop(operand_stack, &stack_pointer);
                push(operand_stack, &stack_pointer, value >> shift_amount);
            }
                pc++;
                break;

            case i_iushr: {
                int32_t shift_amount = pop(operand_stack, &stack_pointer);
                int32_t value = pop(operand_stack, &stack_pointer);
                uint32_t result = ((uint32_t) value) >> shift_amount;
                push(operand_stack, &stack_pointer, (int32_t) result);
            }
                pc++;
                break;

            case i_iload: {
                u1 index = bytecode[++pc];
                push(operand_stack, &stack_pointer, locals[index]);
            }
                pc++;
                break;

            case i_istore: {
                u1 index = bytecode[++pc];
                locals[index] = pop(operand_stack, &stack_pointer);
            }
                pc++;
                break;

            case i_iinc: {
                u1 index = bytecode[++pc];
                int8_t constant = (uint8_t) bytecode[++pc];
                locals[index] += constant;
            }
                pc++;
                break;

            case i_iload_0:
            case i_iload_1:
            case i_iload_2:
            case i_iload_3:
                operand_stack[++stack_pointer] = locals[instruction - OFFSET_ILOAD];
                pc++;
                break;

            case i_istore_0:
            case i_istore_1:
            case i_istore_2:
            case i_istore_3:
                locals[instruction - OFFSET_ISTORE] = pop(operand_stack, &stack_pointer);
                pc++;
                break;

            case i_ldc:
                pc++; // move to argument of ldc instruction
                {
                    u1 index = bytecode[pc]; // get unisgned byte argument
                    cp_info constant =
                        class->constant_pool[index - 1]; // adjust for 0 indexing

                    if (constant.tag == CONSTANT_Integer) {
                        CONSTANT_Integer_info *int_info =
                            (CONSTANT_Integer_info *) constant.info;
                        operand_stack[++stack_pointer] = int_info->bytes;
                    }
                }
                pc++;
                break;

            case i_ifeq:
            case i_ifne:
            case i_iflt:
            case i_ifge:
            case i_ifgt:
            case i_ifle:
            case i_if_icmpeq:
            case i_if_icmpne:
            case i_if_icmplt:
            case i_if_icmpge:
            case i_if_icmpgt:
            case i_if_icmple: {
                int16_t offset = (uint16_t)((bytecode[pc + 1] << 8) | bytecode[pc + 2]); // bytecode is u1* and u1 is uint8_t
                bool shouldJump = should_jump_based_on_instruction(
                    instruction, operand_stack, &stack_pointer);
                pc = shouldJump ? (pc + offset) : (pc + 3);
                break;
            }

            case i_goto: {
                int16_t offset = (uint16_t)((bytecode[pc + 1] << 8) | bytecode[pc + 2]);
                pc += offset;
            } break;

            case i_ireturn:
                pc++;
                {
                    int32_t ret_val = pop(operand_stack, &stack_pointer);
                    return (optional_value_t){.has_value = true, .value = ret_val};
                }
                break;

            case i_invokestatic: {
                pc++; // move past opcode
                u2 index = (bytecode[pc] << 8) | bytecode[pc + 1];
                method_t *called_method = find_method_from_index(index, class);

                int num_params = get_number_of_parameters(called_method);
                int32_t method_locals[called_method->code.max_locals];

                // pop arguments from operand stack in reverse order to go from stack to
                // queue
                memset(method_locals, 0, called_method->code.max_locals);
                for (int i = num_params - 1; i >= 0; i--) {
                    method_locals[i] = pop(operand_stack, &stack_pointer);
                }

                optional_value_t result =
                    execute(called_method, method_locals, class, heap);

                // if method has a return value, push it onto operand stack
                if (result.has_value) {
                    operand_stack[++stack_pointer] = result.value;
                }
                pc += 2;
            } break;

            case i_nop:
                pc++;
                break;

            case i_dup: {
                int32_t val = operand_stack[stack_pointer]; // skip past instruction to
                                                            // value to duplicate
                operand_stack[++stack_pointer] = val;
            }
                pc++;
                break;

            case i_newarray: {
                int32_t count = pop(operand_stack, &stack_pointer);
                int32_t *new_array;
                if (count <= 0) {
                    new_array = malloc(sizeof(int32_t));
                }
                else {
                    new_array = calloc(
                        (count + 1),
                        sizeof(
                            int32_t)); //  allocate for one more int32_t to include count
                }
                new_array[0] = count; // fill the first index of the array with count
                for (int32_t i = 1; i < count + 1; i++) {
                    new_array[i] = 0;
                }
                int32_t ref = heap_add(heap, new_array);
                operand_stack[++stack_pointer] = ref;
            }
                pc += 2;
                break;

            case i_arraylength: {
                int32_t ref = pop(operand_stack, &stack_pointer);
                int32_t *array = heap_get(heap, ref);
                int32_t count = array[0];
                operand_stack[++stack_pointer] = count;
            }
                pc++;
                break;

            case i_areturn:
                pc++;
                return (optional_value_t){.has_value = true,
                                          .value = operand_stack[stack_pointer--]};

            case i_iastore: {
                int32_t value = pop(operand_stack, &stack_pointer);
                int32_t index = pop(operand_stack, &stack_pointer);
                int32_t ref = pop(operand_stack, &stack_pointer);
                int32_t *array = heap_get(heap, ref);
                array[index + 1] = value;
            }
                pc++;
                break;

            case i_iaload: {
                int32_t index = pop(operand_stack, &stack_pointer);
                int32_t ref = pop(operand_stack, &stack_pointer);
                int32_t *array = heap_get(heap, ref);
                operand_stack[++stack_pointer] = array[index + 1];
            }
                pc++;
                break;

            case i_aload: {
                u1 index = bytecode[++pc];
                push(operand_stack, &stack_pointer, locals[index]);
            }
                pc++;
                break;

            case i_astore: {
                u1 index = bytecode[++pc];
                locals[index] = pop(operand_stack, &stack_pointer);
            }
                pc++;
                break;

            case i_aload_0:
            case i_aload_1:
            case i_aload_2:
            case i_aload_3:
                operand_stack[++stack_pointer] = locals[instruction - OFFSET_ALOAD];
                // using fact that 0x2a is the opcode for i_aload_0
                pc++;
                break;

            case i_astore_0:
            case i_astore_1:
            case i_astore_2:
            case i_astore_3:
                locals[instruction - OFFSET_ASTORE] =
                    pop(operand_stack, &stack_pointer); // using fact that 0x4b is the
                                                        // opcode for i_astore_0
                pc++;
                break;

            case i_return: // return
                // assume returns void for now
                return (optional_value_t){.has_value = false};
        }
    }

    // Return void
    optional_value_t result = {.has_value = false};
    return result;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "USAGE: %s <class file>\n", argv[0]);
        return 1;
    }

    // Open the class file for reading
    FILE *class_file = fopen(argv[1], "r");
    assert(class_file != NULL && "Failed to open file");

    // Parse the class file
    class_file_t *class = get_class(class_file);
    int error = fclose(class_file);
    assert(error == 0 && "Failed to close file");

    // The heap array is initially allocated to hold zero elements.
    heap_t *heap = heap_init();

    // Execute the main method
    method_t *main_method = find_method(MAIN_METHOD, MAIN_DESCRIPTOR, class);
    assert(main_method != NULL && "Missing main() method");
    /* In a real JVM, locals[0] would contain a reference to String[] args.
     * But since TeenyJVM doesn't support Objects, we leave it uninitialized. */
    int32_t locals[main_method->code.max_locals];
    // Initialize all local variables to 0
    memset(locals, 0, sizeof(locals));
    optional_value_t result = execute(main_method, locals, class, heap);
    assert(!result.has_value && "main() should return void");

    // Free the internal data structures
    free_class(class);

    // Free the heap
    heap_free(heap);
}
