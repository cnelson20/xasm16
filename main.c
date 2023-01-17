#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "main.h"
#include "functions.h"
#include "instructions.h"
#include "dyn_list.h"

#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

char *fileloc;
char *fileloc_term;

struct dyn_list command_list;
struct dyn_list sym_table;

char scratch[2 + 32];
char *filename = scratch + 2;
unsigned char filename_size = 32;
char output_filename[32];
unsigned char verbose_mode = 1;

unsigned short program_start_addr;

char *lastgloballabel;

void main() {
    printf("enter filename: ");
    fgets(filename, filename_size, stdin);
    *strchr(filename, '\n') = '\0';

    loadfile(filename);
    fileloc_term = strlen(fileloc) + fileloc;
    if (fileloc_term == fileloc) {
        puts("no such file exists");
        return;
    }
    printf("file length: %lu\n", fileloc_term - fileloc);
    printf("output filename? (blank=use file as basis):");
    fgets(output_filename, sizeof(output_filename), stdin);
    *strchr(output_filename, '\n') = '\0';

    char *temp = malloc(16);
    printf("be verbose mode? (y/n):");
    fgets(temp, 16, stdin);
    verbose_mode = (*temp == 'y' || *temp == 'Y');

    printf("program start address? (blank=default)\n");
    printf("entering an addr removes basic header: ");
    fgets(temp, 16, stdin);
    if (*temp == '$') {
        if (sscanf(temp + 1, "%hx\n", &program_start_addr) == 0) {
            program_start_addr = cx16_startaddr;
        }
    } else {
        if (sscanf(temp, "%hd\n", &program_start_addr) == 0) {
            program_start_addr = cx16_startaddr;
        }
    }
    if (program_start_addr == 0) {
        program_start_addr = cx16_startaddr;
    }

    free(temp);

    puts("first-pass()");
    first_pass();
    /*{
        unsigned char i;
        for (i = 0; i < command_list.length; i++) {
            struct command *curr_command = command_list.list[i];
            printf("label: '%s' label2: '%s'\n", curr_command->label ? curr_command->label : "null", curr_command->label2 ? curr_command->label2 : "null");
        }
    }*/
    puts("second-pass()");
    second_pass();
    puts("third-pass()");
    third_pass();
    puts("fourth-pass()");
    fourth_pass();

    unsigned short i;
    for (i = 0; i < sym_table.length; ++i) {
        struct string_short_pair *sp = sym_table.list[i];
        free(sp->key);
        free(sp);
    }
    free(sym_table.list);

    free(fileloc);

}

#define MAX_FILE_SIZE 16384

/* 
	Loads file into location.
*/
void loadfile(char *filename) {
    int fd = open(filename, O_RDONLY);
    fileloc = malloc(MAX_FILE_SIZE);

    *(fileloc + read(fd, fileloc, MAX_FILE_SIZE)) = '\0';
    fileloc_term = fileloc + strlen(fileloc);
}

unsigned char is_letter(char c) {
    return ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'));
}

void print_chars(char *string) {
    while (*string) {
        printf("%hhx ", *string);
        ++string;
    }
    puts("");
}

void first_pass() {
    char *temp = fileloc;

    init_dyn_list(&command_list);
    /* makes life easier */
    strtolower(temp);

    lastgloballabel = NULL;

    while (*temp) {
        /* setup line */
        char *line = temp;
        char *line_first = temp;
        unsigned char ins_num;
        struct command *comm = NULL;
        char *semicolon_temp;
        unsigned char string_index;

        /* find next \r and replace with null terminator */
        temp = strchr(line,'\r');
        if (temp == NULL) {
            temp = strchr(line, '\n');
        }
        //printf("line: %p temp: %p\n", line, temp);
        if (temp == NULL && line + strlen(line) < fileloc_term) {
            temp = line + strlen(line);
        }
        /*printf("line: %p temp: %p\n", line, temp);
        puts("--------");
        for (ins_num = 0; ins_num < 16; ins_num++) {
            printf("%hx ", line[ins_num]);
        }
        puts("");
        puts("--------");*/

        if (temp == line) {
            temp++;
            continue;
        }
        /* if not found default to end of doc */
        if (temp == NULL) {
            temp = fileloc_term;
        }
        *temp = '\0';
        /*
            for CRLF files
            redunant due to the next line
        */
        if (*line == '\n') {line++;}
        /* remove preceding whitespace */
        while (*line <= 0x20) {
            line++;
        }

        semicolon_temp = findwhitespacerev(line);
        if (semicolon_temp != NULL) {
            *(semicolon_temp + 1) = '\0';
        }
        semicolon_temp = strchr(line,';');
        if (semicolon_temp != NULL) {
            *semicolon_temp = '\0';
        }
        if (strlen(line) == 0 || strchr(line,'\r') || strchr(line, '\n')) {
            if (semicolon_temp != NULL) {
                *semicolon_temp = ';';
            }
            *temp = '\n';
            ++temp;
            continue;
        }

        if (verbose_mode) {putchar('\n'); }

        for (string_index = 0; string_index < 4; ++string_index) {
            if (string_index < 3 && !is_letter(line[string_index])) { break; }
            if (string_index == 3 && line[3] > 0x20) { break; }
        }
        if (string_index < 4) {
            ins_num = 0xFF;
        } else {
            ins_num = get_instruction_from_string(line);
        }
        if (verbose_mode) { printf("line: \"%s\" in #: %hhx\n", line, ins_num); }

        comm = malloc(sizeof(struct command));

        if (ins_num != 0xFF) {
            char *space_temp;

            comm->label = NULL;
            comm->label2 = NULL;
            comm->is_directive = 0;

            comm->inst.mode = 0xFF;

            comm->inst.type = ins_num;

            space_temp = strchr(line, 0x20); // Find space
            /*
                If no space present, assume implied / accumulator mode
                ASL, LSR, ROL, ROR are the only instructions to use accumulator mode
            */
            if (space_temp == NULL) {
                actually_no_args:
                if (ins_num == IN_ASL || ins_num == IN_LSR || ins_num == IN_ROL || ins_num == IN_ROR) {
                    comm->inst.mode = MODE_ACC;
                } else {
                    comm->inst.mode = MODE_IMP;
                }
                comm->inst.label = NULL;
                if (ins_num != IN_BRK) {
                    comm->inst.mem_size = 1;
                } else {
                    comm->inst.mem_size = 2;
                    comm->inst.val = 0xEA; // a nop instruction, since BRKs are two-byte instructions
                }
            } else {
                /* Now start line at one character past the space, then move forward to ignore whitespace */
                line = findwhitespace(space_temp);
                if (verbose_mode) { printf("line args: '%s'\n", line); }
                /* Figure out addressing mode */

                /* All branches use relative adressing */
                if (ins_num == IN_BCC || ins_num == IN_BCS || ins_num == IN_BEQ || ins_num == IN_BMI || ins_num == IN_BNE || ins_num == IN_BPL || ins_num == IN_BVC || ins_num == IN_BVS) {
                    char char_temp;
                    char *temp = findwhitespacerev(line) + 1;
                    char_temp = *temp;

                    *temp = '\0';
                    comm->inst.label = strdup(line);
                    *temp = char_temp;

                    comm->inst.mode = MODE_REL;
                    comm->inst.mem_size = 2;
                    /* hash or # indicates a immediate load */
                } else if (*line == '#') {
                    char char_temp;
                    char *temp = findwhitespacerev(line) + 1;
                    char_temp = *temp;

                    *temp = '\0';
                    line++; // go past the hash
                    comm->inst.label = strdup(line);
                    *temp = char_temp;

                    comm->inst.mode = MODE_IMM;
                    comm->inst.mem_size = 2;
                } else {
                    /* Check for comma, left parenthesis, right parenthesis */
                    char *comma_pos = strchr(line, ',');
                    char *left_paren_pos = strchr(line, '(');
                    char *right_paren_pos = strchr(line, ')');
                    char *inst_xy_pos;

                    /* Check for non matching parenthesis */
                    if ((left_paren_pos != NULL) != (right_paren_pos != NULL) ) {
                        printf("non-matching parenthesis: '%s'\n", line_first);
                        exit(0);
                        /* if ) is before ( something's wrong */
                    } else if (left_paren_pos > right_paren_pos) {
                        printf("illegal addressing error (parenthesis): '%s'\n", line_first);
                        exit(0);
                        /* indirect addressing */
                    } else if (left_paren_pos != NULL && comma_pos != NULL) {
                        line = findwhitespace(left_paren_pos + 1);
                        inst_xy_pos = strchr(line, 0x58); // Find X
                        if (inst_xy_pos != NULL) {
                            /* (zp,X) */
                            char char_temp;
                            char *temp;
                            if (comma_pos >= right_paren_pos) {
                                printf("illegal addressing mode: '%s'\n", line_first);
                            }

                            /* (ADDR, */
                            temp = searchwhitespacerev(comma_pos - 1) + 1;
                            char_temp = *temp;
                            *temp = '\0';

                            line = findwhitespace(left_paren_pos + 1);
                            comm->inst.label = strdup(line);

                            *temp = char_temp;

                            comm->inst.mode = MODE_IND_X;
                            comm->inst.mem_size = 2;
                        } else {
                            char char_temp;
                            char *temp;
                            inst_xy_pos = strchr(line, 0x59); // Find Y
                            /* (zp),Y */
                            if (inst_xy_pos == NULL || right_paren_pos >= comma_pos) {
                                printf("illegal addressing mode: '%s'\n", line_first);
                                exit(1);
                            }

                            /* (ADDR) */
                            temp = searchwhitespacerev(right_paren_pos - 1) + 1;
                            char_temp = *temp;
                            *temp = '\0';

                            line = findwhitespace(left_paren_pos + 1);
                            comm->inst.label = strdup(line);

                            *temp = char_temp;

                            comm->inst.mode = MODE_IND_Y;
                            comm->inst.mem_size = 2;
                        }
                        /* indirect addressing for JMP */
                    } else if (left_paren_pos != NULL) {
                        char char_temp;
                        char *temp;

                        *right_paren_pos = '\0';
                        line = findwhitespace(left_paren_pos + 1);
                        temp = findwhitespacerev(line) + 1;
                        char_temp = *temp;
                        *temp = '\0';

                        comm->inst.label = strdup(line);

                        *temp = char_temp;
                        *right_paren_pos = ')';

                        comm->inst.mode = MODE_IND;
                        comm->inst.mem_size = 3;
                        /* absolute x,y addressing  */
                    } else if (comma_pos != NULL) {
                        char char_temp;
                        char *temp;
                        /* Find X */
                        inst_xy_pos = strchr(line, 'x');
                        if (inst_xy_pos != NULL) {
                            comm->inst.mode = MODE_ABS_X;
                        } else {
                            /* Else check for Y */
                            inst_xy_pos = strchr(line, 'y');
                            if (inst_xy_pos == NULL) {
                                printf("illegal addressing mode: '%s'\n", line_first);
                                exit(1);
                            }
                            comm->inst.mode = MODE_ABS_Y;
                        }
                        /* Find correct label */
                        temp = searchwhitespacerev(comma_pos - 1) + 1;
                        char_temp = *temp;
                        *temp = '\0';

                        comm->inst.label = strdup(line);
                        comm->inst.mem_size = 3;

                        *temp = char_temp;
                        /* absolute addressing  */
                    } else {
                        char *temp = findwhitespacerev(line) + 1;
                        char char_temp;

                        char_temp = *temp;
                        *temp = '\0';

                        if (strlen(line) == 0) {
                            goto actually_no_args;
                        }

                        if ((ins_num == IN_INC || ins_num == IN_DEC || ins_num == IN_ASL || ins_num == IN_LSR || ins_num == IN_ROL || ins_num == IN_ROR) && !strcmp(line,"a")) {
                            comm->inst.mode = MODE_ACC;
                            comm->inst.label = NULL;
                            comm->inst.mem_size = 1;
                        } else {
                            comm->inst.label = strdup(line);
                            comm->inst.mode = MODE_ABS;
                            comm->inst.mem_size = 3;
                        }
                        *temp = char_temp;
                    }
                }
            }
            /* print some info about instruction for debug */
            if (verbose_mode) {
                printf("in->mode: %hhu\n", comm->inst.mode);
                if (comm->inst.label) {
                    printf("in->label: '%s'\n", comm->inst.label);
                } else {
                    puts("in->label: null");
                }
                printf("in->mem-size: %hhu\n", comm->inst.mem_size);
            }
            /* Add instruction to list */
            dyn_list_add(&command_list, comm);

            /* Line is not an instruction */
        } else {
            char firstchar = *line;
            char *colon_pos = strchr(line,':');
            char char_following_colon;
            /*
                if there is no colon, cannot be a label
            */
            if (colon_pos != NULL) {
                char_following_colon = *(colon_pos + 1);
            } else {
                char_following_colon = 0xFF;
            }

            if (((firstchar == '@') || (firstchar >= 'a' && firstchar <= 'z')) && colon_pos != NULL && char_following_colon <= 0x20) {
                comm = malloc(sizeof(struct command));
                *colon_pos = '\0';
                if (firstchar != '@') {
                    comm->label = strdup(line);
                    lastgloballabel = comm->label;
                } else {
                    if (!lastgloballabel) {
                        printf("no global label before cheap local label '%s' declared.\n", line);
                        exit(1);
                    }
                    comm->label = malloc(2 + strlen(lastgloballabel) + strlen(line));
                    strcpy(comm->label, "@");
                    strcat(comm->label, lastgloballabel);
                    strcat(comm->label, line);
                }
                comm->label2 = NULL;
                *colon_pos = ':';
                comm->is_directive = 0;
                dyn_list_add(&command_list, comm);
            } else if (firstchar == '.') {
                comm = malloc(sizeof(struct command));
                comm->label = strdup(line);
                comm->label2 = NULL;
                comm->is_directive = 1;
                comm->directive_data = NULL;
                comm->directive_data_len = 0;
                dyn_list_add(&command_list, comm);
            } else {
                char *temp;
                char char_temp;
                char *equal_pos = strchr(line, '=');
                if (equal_pos == NULL) {
                    printf("illegal statement: '%s'\n", line_first);
                    exit(0);
                }
                temp = searchwhitespacerev(equal_pos - 1) + 1;
                char_temp = *temp;
                *temp = '\0';

                if (*line == '@') {
                    comm->label = malloc(2 + strlen(lastgloballabel) + strlen(line));
                    strcpy(comm->label, "@");
                    strcat(comm->label, lastgloballabel);
                    strcat(comm->label, line);
                } else {
                    comm->label = strdup(line);
                }

                *temp = char_temp;
                line = findwhitespace(equal_pos + 1);
                temp = findwhitespacerev(line) + 1;
                char_temp = *temp;
                *temp = '\0';
                comm->label2 = strdup(line);

                *temp = char_temp;
                comm->is_directive = 0;

                dyn_list_add(&command_list, comm);
            }
            if (verbose_mode) {
                if (comm->label2 != NULL) {
                    printf("label1: '%s' label2: '%s'\n", comm->label, comm->label2);
                } else {
                    printf("is_directive: %hhx label/directive: '%s'\n", comm->is_directive, comm->label);
                }
            }
        }


        /* add semicolon back to line */
        if (semicolon_temp != NULL) {
            *semicolon_temp = ';';
        }
        /* set end of line back to carriage return*/
        *temp = '\n';
        ++temp;
    }
    // Null terminating byte may have been removed, so re-add it
    *fileloc_term = '\0';
}

/* 
	Loop through command list,
	figure out values of labels and interpret assembler directives
*/
void second_pass() {
    unsigned short start_address = program_start_addr;
    unsigned short prg_ptr = start_address;
    unsigned short i;

    init_dyn_list(&sym_table);

    for (i = 0; i < command_list.length; ++i) {
        struct command *curr_command = command_list.list[i];
        if (curr_command->is_directive != 0 && curr_command->label != NULL) {
            char *space_pos;
            char *line;
            // Check which directive
            line = curr_command->label;
            if (verbose_mode) { printf("directive: '%s'\n", line); }
			++line;
			space_pos = strchr(line, 0x20);
			if (space_pos == NULL) { space_pos = strlen(line) + line; }
			*space_pos = '\0';
			if (!strcmp("db", line) || !strcmp("byte", line) || !strcmp("dw", line) || !strcmp("word", line)) {
				struct dyn_list bytes;
				unsigned char size = 0;
				unsigned char i;
				unsigned char *data; 
				unsigned char data_width = (!strcmp("db", line) || !strcmp("byte", line)) ? 1 : 2;
				
				space_pos = findwhitespace(space_pos + 1);
				init_dyn_list(&bytes);
				while (1) {
					char *comma_pos;
					char *end_pos;
					char *start_pos;
					unsigned short x;
					
					start_pos = findwhitespace(space_pos);
					comma_pos = strchr(start_pos, ',');
					if (comma_pos == NULL) {
						end_pos = findwhitespacerev(start_pos) + 1;
					} else {
						end_pos = searchwhitespacerev(comma_pos - 1) + 1;
					}
					*end_pos = '\0';
					
					//printf("'%s'\n", start_pos);
					
					if (is_num(start_pos)) {
						x = parse_num(start_pos);
					} else {
						x = get_symbol_value(start_pos);
					}
					if (x >= (data_width == 2 ? 65536 : 256)) {
						printf("'%u' out of range [0,255]\n", x);
						exit(0);
					}
					dyn_list_add(&bytes, (void *)x);
					++size;
					
					if (comma_pos == NULL) {
						break;
					}
					space_pos = comma_pos + 1;
					
				}
				data = malloc(size * data_width);
				for (i = 0; i < size; i++) {
					data[i*data_width] = (unsigned short)(bytes.list[i]) % 256;
					if (data_width == 2) {
						data[i*2+1] = ((unsigned short)(bytes.list[i])) / 256;
					}
				}
				curr_command->directive_data = data;
				curr_command->directive_data_len = size * data_width;
				prg_ptr += size;
			} else if (!strcmp("res", line)) {
                int size;
                int value = 0;
                int i;
                char *comma_pos;

                space_pos = findwhitespace(space_pos + 1);
                comma_pos = strchr(space_pos, ',');
                if (comma_pos != NULL) {
                    *comma_pos = '\0';
                    comma_pos = findwhitespace(comma_pos + 1);
                    *(findwhitespacerev(comma_pos) + 1) = '\0';
                }
                *(findwhitespacerev(space_pos) + 1) = '\0';

                if (comma_pos != NULL) {
                    if (is_num(comma_pos)) {
                        value = parse_num(comma_pos);
                    } else {
                        value = get_symbol_value(comma_pos);
                    }
                }

                if (is_num(space_pos)) {
                    size = parse_num(space_pos);
                } else {
                    size = get_symbol_value(space_pos);
                }

                printf("space_pos: '%s'\n", space_pos);
                printf("comma_pos: '%s'\n", comma_pos);

                curr_command->directive_data = malloc(size);
                for (i = 0; i < size; i++) {
                    ((char *) curr_command->directive_data)[i] = value;
                }
                curr_command->directive_data_len = size;
            } else if (!strcmp("asciiz",line)) {
				char *left_quote_pos, *right_quote_pos;
				char *data;
				left_quote_pos = strchr(space_pos+1, '"');
				right_quote_pos = strrchr(space_pos+1, '"');
				if (left_quote_pos == right_quote_pos || !left_quote_pos || !right_quote_pos) {
					printf("illegal format for string literal: '%s'\n", space_pos+1);
					exit(0);
				}
				*right_quote_pos = '\0';
				++left_quote_pos;

				data = strdup(left_quote_pos);
                #ifndef __CC65__
                strtoupper(data);
                #endif

                curr_command->directive_data = data;
                curr_command->directive_data_len = strlen(left_quote_pos) + 1;
                prg_ptr += curr_command->directive_data_len;
            } else if (!strcmp("org", line) || !strcmp("pc", line)) {
                curr_command->label = strdup(".org");
                curr_command->label2 = strdup(space_pos + 1);
                prg_ptr = parse_num(curr_command->label2);
            } else {
                printf("unknown assembler directive '%s' found\n", line - 1);
                exit(0);
            }
            /*
                label definition
                either:
                    FXN:

                    or

                    FXN_ADDR = $2000
            */
        } else if (curr_command->label != NULL) {
            if (curr_command->label2 == NULL) {
                struct string_short_pair *pair = malloc(sizeof(struct string_short_pair));
                unsigned short i;
                pair->key = curr_command->label;
                pair->val = prg_ptr;
                for (i = 0; i < sym_table.length; ++i) {
                    if (!strcmp(pair->key, ((struct string_short_pair *)sym_table.list[i])->key)) {
                        printf("error: redefinition of symbol '%s'\n", pair->key);
                        exit(1);
                    }
                }
                dyn_list_add(&sym_table, pair);
            } else {
                struct string_short_pair *pair = malloc(sizeof(struct string_short_pair));
                unsigned short i;
                pair->key = curr_command->label;
                for (i = 0; i < sym_table.length; ++i) {
                    if (!strcmp(pair->key, ((struct string_short_pair *)sym_table.list[i])->key)) {
                        printf("error: redefinition of symbol '%s'\n", pair->key);
                        exit(1);
                    }
                }
                if (is_num(curr_command->label2)) {
                    pair->val = parse_num(curr_command->label2);
                } else {
                    pair->val = get_symbol_value(curr_command->label2);
                }

                dyn_list_add(&sym_table, pair);
            }
            /*
                Instructions
                Add instruction mem length to prg ptr
            */
        } else {
            prg_ptr += curr_command->inst.mem_size;
        }

    }
    printf("\nsym_table:\n");
    for (i = 0; i < sym_table.length; ++i) {
        struct string_short_pair *curr_pair = sym_table.list[i];
        printf("key: '%s' val: $%04x\n", curr_pair->key, curr_pair->val);
    }
}

/* 
	Figure out values of each instruction
	Either parse number or lookup label in sym table 
	
	Also determine opcodes for each instruction
*/
void third_pass() {
    unsigned short i;
    unsigned short prg_ptr = program_start_addr;

    /*for (i = 0; i < command_list.length; ++i) {
        struct command *curr_command = command_list.list[i];
        printf("inst->type: %hu\n", curr_command->inst.type);
        printf("inst->mode: %hu\n", curr_command->inst.mode);
        printf("inst->mem_size: %hu\n", curr_command->inst.mem_size);
        printf("inst->label: \"%s\"\n", curr_command->label ? curr_command->inst.label : "null");
        printf("comm->label: \"%s\"\n", curr_command->label ? curr_command->label : "null");

        putchar('\n');
    }*/
    lastgloballabel = NULL;

    for (i = 0; i < command_list.length; ++i) {
        struct command *curr_command = command_list.list[i];
        /* Checking if command is an instruction */
        if (curr_command->label == NULL) {
            char *lbl;
            unsigned char mode;
            unsigned char size;
            unsigned char opcode = opcode_matrix[curr_command->inst.type][curr_command->inst.mode];
            if (opcode == 0xFF) {
                printf("illegal addressing mode: instruction '%s' with mode %hu\n", instruction_string_list[curr_command->inst.type], curr_command->inst.mode);
                printf("index: %u\n", i);
                exit(0);
            }
            curr_command->inst.opcode = opcode;

            //printf("instruction: %s\n", instruction_string_list[curr_command->inst.type]);
            //printf("opcode: $%hx\n", opcode);

            lbl = curr_command->inst.label;
            mode = curr_command->inst.mode;
            size = curr_command->inst.mem_size;
            if (mode >= MODE_IMM && mode <= MODE_IND_Y) {
                unsigned short temp_val;
                if (is_num(lbl)) {
                    temp_val = parse_num(lbl);
                } else {
                    temp_val = get_symbol_value(lbl);
                }
                if (size == 2 && temp_val >= 256) {
                    printf("Range error: '%s' - '%s' out of range [0,255]\n", instruction_string_list[curr_command->inst.type], lbl);
                }
                curr_command->inst.val = temp_val;
            } else if (mode == MODE_REL) {
                unsigned short lbl_val;
                signed short temp_val;
                if (is_num(lbl)) {
                    temp_val = parse_num(lbl);
                } else {
                    lbl_val = get_symbol_value(lbl);
                    //printf("prg-ptr: %hu, lbl-val: %hu\n", prg_ptr, lbl_val);
                    temp_val = ((long int)lbl_val) - ((long int)prg_ptr) - 2;
                    //printf("temp-val: %d\n", temp_val);
                }
                if (temp_val > 127 && temp_val < -128) {
                    printf("Range error: '%s' - '%s',%d out of range [-128,127]\n", instruction_string_list[curr_command->inst.type], lbl, temp_val);
                }
                if (temp_val >= 0) {
                    curr_command->inst.val = temp_val;
                } else {
                    curr_command->inst.val = 256 + temp_val;
                }
            }
            //printf("size: %hu\n", size);
            //printf("val: %hu\n\n", curr_command->inst.val);

            prg_ptr += size;
        } else if (curr_command->is_directive) {
            if (!memcmp(".org", curr_command->label, 4)) {
                prg_ptr = parse_num(curr_command->label2);
            }
            prg_ptr += curr_command->directive_data_len;
        } else {
            if (*curr_command->label != '\0' && *curr_command->label != '@') {
                lastgloballabel = curr_command->label;
            }
        }
    }
}

/* Write data to file */
void fourth_pass() {
    char *write_loc = fileloc;
    int fd = open_file_write();
    unsigned short i;

    if (program_start_addr == cx16_startaddr) {
        write(fd, cx16_prg_header, cx16_prg_header_len);
    }
    for (i = 0; i < command_list.length; ++i) {
        struct command *curr_command = command_list.list[i];
        if (curr_command->label == NULL) {
            //printf("opcode: %hu\n", curr_command->inst.opcode);
            write(fd, &(curr_command->inst.opcode), 1);
            write(fd, &(curr_command->inst.val), curr_command->inst.mem_size - 1);
        } else if (curr_command->is_directive && curr_command->directive_data_len != 0) {
            write(fd, curr_command->directive_data, curr_command->directive_data_len);
        }
        //puts("");
    }

    close(fd);
}

int open_file_write() {
    int fd;
    char *period;

    //printf("%hu\n", strlen(output_filename));
    if (strlen(output_filename) != 0) {
        strcpy(filename, output_filename);
    } else {
        period = strrchr(filename, '.');
        if (period != NULL) {
            *period = '\0';
        }
        strcat(filename, ".prg");
    }

    printf("output-filename: '%s'\n", filename);
    fd = creat(filename, 0777);

    if (errno) {
        printf("error: '%s'\n", strerror(errno));
    } else {
        puts("errno = 0");
    }
    return fd;
}

unsigned char is_num(char *string) {
    return (*string == '$' || *string == '%' || (*string >= 0x30 && *string <= 0x39));
}

unsigned short parse_num(char *string) {
    unsigned short num;

    if (*string == '$') {
        sscanf(string+1,"%hx",&num);
        return num;
    } else if (*string == '%') {
        unsigned short shift_amnt = 0;
        char *temp;
        num = 0;
        ++string;
        temp = string + strlen(string) - 1;
        while (temp >= string) {
            if (*temp == '1') {
                num += (1 << shift_amnt);
            } else if (*temp > '1' || *temp < '0') {
                printf("illegal number format: '%s'\n", string - 1);
                exit(0);
            }
            ++shift_amnt;
            --temp;
        }
        //printf("binary: '%s' num: %u\n", string, num);
        return num;
    } else if (*string >= 0x30 && *string <= 0x39) {
        sscanf(string,"%hu",&num);
        return num;
    }
}

unsigned char is_in_symtable(char *string) {
    unsigned short i;

    for (i = 0; i < sym_table.length; ++i) {
        struct string_short_pair *curr_pair = sym_table.list[i];
        if (!strcmp(curr_pair->key, string)) {
            return 1;
        }
    }
    return 0;
}

unsigned short get_symbol_value(char *string) {
    if (*string == '<') {
        return find_in_symtable(string+1) & 0xFF;
    } else if (*string == '>') {
        return find_in_symtable(string+1) >> 8;
    } else {
        return find_in_symtable(string);
    }
}

unsigned short find_in_symtable(char *string) {
    unsigned short i;

    for (i = 0; i < sym_table.length; ++i) {
        struct string_short_pair *curr_pair = sym_table.list[i];
        if (!strcmp(curr_pair->key, string)) {
            return curr_pair->val;
        } else if (*string == '@' && lastgloballabel != NULL) {
            if (!memcmp(curr_pair->key + 1, lastgloballabel, strlen(lastgloballabel)) && !strcmp(strchr(curr_pair->key + 1, '@'), string)) {
                return curr_pair->val;
            }
        }
    }
    printf("definition for symbol '%s' not found!\n", string);
    exit(0);
}