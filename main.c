#include <stdio.h>
#include <stdlib.h>
#include <cbm.h>
#include <string.h>
#include "main.h"
#include "functions.h"
#include "instructions.h"
#include "dyn_list.h"

#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <peekpoke.h>

char *fileloc = (char *)0x7000;
char *fileloc_term;

struct dyn_list command_list;

char scratch[2 + 32];
char *filename = scratch + 2;
unsigned char filename_size = 32;
char output_filename[32];
unsigned char verbose_mode = 1;

void main() {
	__asm__ ("lda #142");
	__asm__ ("jsr $FFD2");
		
	if (PEEKW(0x90A0) == 0x5656) {
		strcpy(filename, *((char **)PEEKW(0x90A0)));
		verbose_mode = 0;
	} else {
		printf("enter filename: ");
		fgets(filename, filename_size, stdin);
		*strchr(filename, 0xd) = '\0';
		strtoupper(filename);

		loadfile(filename, fileloc);
		fileloc_term = strlen(fileloc) + fileloc;
		if (fileloc_term == fileloc) {
			puts("no such file exists");
			return;
		}
		printf("file length: %u\n", fileloc_term - fileloc);
	}
	if (PEEKW(0x90A0) == 0x5656) {
		*output_filename = '\0';
	} else { 
		printf("output filename? (blank=use file as basis):");
		fgets(output_filename, sizeof(output_filename), stdin);
		*strchr(output_filename, 0xd) = '\0';
		strtoupper(output_filename);
		
	}
	if (verbose_mode) {
		char *temp = malloc(16);
		printf("be verbose mode? (y/n):");
		fgets(temp, 16, stdin);
		verbose_mode = (*temp == 'y');
		free(temp);
	}	
	
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
}

/* 
	Loads file into location.
*/
void loadfile(char *filename, char *location) {
	char *temp = location;

	cbm_k_setnam(filename);
	cbm_k_setlfs(10, 8, 2);
	
	cbm_k_open();
	if (cbm_k_readst() != 0) {
		*((char *)location) = '\0';
		return;
	}
	
	cbm_k_chkin(10);	
	while (cbm_k_readst() == 0) {
		*temp = cbm_k_basin();
		++temp;
	}
	//printf("%hu\n", cbm_k_readst());
	--temp;
	*temp = '\0';
	cbm_k_close(10);
	cbm_k_clrch();
}

unsigned char is_letter(char c) {
	return ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'));
}

void print_chars(char *string) {
	while (*string) {
		printf("%hhx ", *string);
		++string;
	}
	__asm__ ("lda #$d");
	__asm__ ("jsr $FFD2");
}

void first_pass() {
	char *temp = fileloc;
	
	init_dyn_list(&command_list);
	/* makes life easier */
	strtoupper(temp);
	
	while (*temp) {
		/* setup line */
		char *line = temp;
		char *line_first = temp;
		unsigned char ins_num;
		struct command *comm = NULL;
		char *semicolon_temp;
		unsigned char string_index;
		
		/* find next \r and replace with null terminator */
		temp = strchr(line,0xd);
		if (temp == NULL) {
			temp = strchr(line, 0xa);
		}
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
		if (strlen(line) == 0 || strchr(line,0xd)) {
			if (semicolon_temp != NULL) {
				*semicolon_temp = ';';
			}
			*temp = 0xd;
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
				comm->inst.mem_size = 1;
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
						inst_xy_pos = strchr(line, 0x58);
						if (inst_xy_pos != NULL) {
							comm->inst.mode = MODE_ABS_X;
						} else {
							/* Else check for Y */
							inst_xy_pos = strchr(line, 0x59);
							if (inst_xy_pos == NULL) {
								printf("illegal addressing mode: '%s'\n", line_first);
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
			if (colon_pos) {
				char_following_colon = *(colon_pos + 1);
			} else {
				char_following_colon = 0xFF;
			}
			
			if ((firstchar >= 0x40 && firstchar < 0x41 + 26) && char_following_colon <= 0x20) {		
				comm = malloc(sizeof(struct command));
				*colon_pos = '\0';
				comm->label = strdup(line);			
				comm->label2 = NULL;
				*colon_pos = ':';
				comm->is_directive = 0; 
				dyn_list_add(&command_list, comm);
			} else if (*line == '.') {
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
				
				comm->label = strdup(line);
				
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
		*temp = '\r';
		++temp;
	}
	// Null terminating byte may have been removed, so re-add it
	*fileloc_term = '\0';
}

struct dyn_list sym_table;

/* 
	Loop through command list,
	figure out values of labels and interpret assembler directives
*/
void second_pass() {
	unsigned short start_address = cx16_startaddr;
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
			//printf("directive: '%s'\n", line);
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
						data[i*2+1] = (unsigned char)(bytes.list[i]) / 256;
					}
				}
				curr_command->directive_data = data;
				curr_command->directive_data_len = size * data_width;
				prg_ptr += size;
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
				
				curr_command->directive_data = data;
				curr_command->directive_data_len = strlen(left_quote_pos) + 1;
				prg_ptr += curr_command->directive_data_len;
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
				pair->key = curr_command->label;
				pair->val = prg_ptr;
				dyn_list_add(&sym_table, pair);
			} else {
				struct string_short_pair *pair = malloc(sizeof(struct string_short_pair));
				pair->key = curr_command->label;
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
		printf("key: '%s' val: %u\n", curr_pair->key, curr_pair->val);
	}
}

/* 
	Figure out values of each instruction
	Either parse number or lookup label in sym table 
	
	Also determine opcodes for each instruction
*/
void third_pass() {
	unsigned short i;
	unsigned short prg_ptr = cx16_startaddr;
	
	/*for (i = 0; i < command_list.length; ++i) {
		struct command *curr_command = command_list.list[i];
		printf("inst->type: %hu\n", curr_command->inst.type);
		printf("inst->mode: %hu\n", curr_command->inst.mode);
		printf("inst->mem_size: %hu\n", curr_command->inst.mem_size);
		printf("inst->label: \"%s\"\n", curr_command->label ? curr_command->inst.label : "null");
		printf("comm->label: \"%s\"\n", curr_command->label ? curr_command->label : "null");
		
		putchar('\n');
	}*/
	
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
			prg_ptr += curr_command->directive_data_len;
		}
	}
}

/* Write data to file */
void fourth_pass() {
	char *write_loc = fileloc;
	int fd = open_file_write();
	unsigned short i;
	
	write(fd, cx16_prg_header, cx16_prg_header_len);
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
	strtoupper(filename);
	
	*scratch = 's';
	*(scratch+1) = ':';
	cbm_k_setnam(scratch);
	cbm_k_setlfs(1, 8, 15);
	cbm_k_open();
	cbm_k_clrch();
	cbm_k_close(1);
	
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
		sscanf(string+1,"%x",&num);
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
		}
	}
	printf("definition for symbol '%s' not found!\n", string);
	exit(0);
}