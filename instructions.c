#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "instructions.h"
#include "dyn_list.h"

unsigned char cx16_prg_header[] = {0x01, 0x08, 0x0B, 0x08, 0x30, 0x03, 0x9E, 0x32, 0x30, 0x36, 0x31, 0x00, 0x00, 0x00};
unsigned char cx16_prg_header_len = sizeof(cx16_prg_header);
unsigned short cx16_startaddr = 2061;

char instruction_string_list[][4] = {
	"adc",
	"and",
	"asl",
	"bcc",
	"bcs",
	"beq",
	"bit",
	"bmi",
	"bne",
	"bpl",
	"brk",
	"bvc",
	"bvs",
	"clc",
	"cld",
	"cli",
	"clv",
	"cmp",
	"cpx",
	"cpy",
	"dec",
	"dex",
	"dey",
	"eor",
	"inc",
	"inx",
	"iny",
	"jmp",
	"jsr",
	"lda",
	"ldx",
	"ldy",
	"lsr",
	"nop",
	"ora",
	"pha",
	"php",
    "phx",
    "phy",
	"pla",
	"plp",
    "plx",
    "ply",
	"rol",
	"ror",
	"rti",
	"rts",
	"sbc",
	"sec",
	"sed",
	"sei",
	"sta",
    "stp",
    "stx",
	"sty",
    "stz",
	"tax",
	"tay",
	"tsx",
	"txa",
	"txs",
	"tya",
};
unsigned char instruction_string_list_len = sizeof(instruction_string_list) / sizeof(instruction_string_list[0]);


unsigned char opcode_matrix[][12] = {
	/* MODE    IMP,  IMM,  ZP,   ZPX,  ABS,  ABX,  ABY,  IND,  INX,  INY   ACC,  REL */
	/* ADC */ {0xFF, 0x69, 0x65, 0x75, 0x6d, 0x7d, 0x79, 0x72, 0x61, 0x71, 0xFF, 0xFF},
	/* AND */ {0xFF, 0x29, 0x25, 0x35, 0x2d, 0x3d, 0x39, 0x32, 0x21, 0x31, 0xFF, 0xFF},
	/* ASL */ {0xFF, 0xFF, 0x06, 0x16, 0x0e, 0x1e, 0xFF, 0xFF, 0xFF, 0xFF, 0x0a, 0xFF},
	/* BCC */ {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x90},
	/* BCS */ {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xB0},
	/* BEQ */ {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF0},
	/* BIT */ {0xFF, 0xFF, 0x24, 0xFF, 0x2c, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
	/* BMI */ {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x30},
	/* BNE */ {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xD0},
	/* BPL */ {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x10},
	/* BRK */ {0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
	/* BVC */ {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x50},
	/* BVS */ {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x70},
	/* CLC */ {0x18, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
	/* CLD */ {0x38, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
	/* CLI */ {0x58, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
	/* CLV */ {0xb8, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
	/* CMP */ {0xFF, 0xc9, 0xc5, 0xd5, 0xcd, 0xdd, 0xd9, 0xd2, 0xc1, 0xd1, 0xFF, 0xFF},
	/* CPX */ {0xFF, 0xe0, 0xe4, 0xFF, 0xec, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
	/* CPY */ {0xFF, 0xc0, 0xc4, 0xFF, 0xcc, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
	/* DEC */ {0xFF, 0xFF, 0xc6, 0xd6, 0xce, 0xde, 0xFF, 0xFF, 0xFF, 0xFF, 0x3a, 0xFF},
	/* DEX */ {0xca, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
	/* DEY */ {0x88, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
    /* EOR */ {0xFF, 0x49, 0x45, 0x55, 0x4d, 0x5d, 0x59, 0x52, 0x41, 0x51, 0xFF, 0xFF},
	/* INC */ {0xFF, 0xFF, 0xe6, 0xf6, 0xee, 0xfe, 0xFF, 0xFF, 0xFF, 0xFF, 0x1a, 0xFF},
	/* INX */ {0xe8, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
	/* INY */ {0xc8, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
	/* JMP */ {0xFF, 0xFF, 0xFF, 0xFF, 0x4c, 0xFF, 0xFF, 0x6c, 0xFF, 0xFF, 0xFF, 0xFF},
	/* JSR */ {0xFF, 0xFF, 0xFF, 0xFF, 0x20, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
	/* LDA */ {0xFF, 0xa9, 0xa5, 0xb5, 0xad, 0xbd, 0xb9, 0xb2, 0xa1, 0xb1, 0xFF, 0xFF},
	/* LDX */ {0xFF, 0xa2, 0xa6, 0xb6, 0xae, 0xFF, 0xbe, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, // Note: zp,Y addressing not supported. you can use abs,Y though
	/* LDY */ {0xFF, 0xa0, 0xa4, 0xb4, 0xac, 0xbc, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
	/* LSR */ {0xFF, 0xFF, 0x46, 0x56, 0x4e, 0x5e, 0xFF, 0xFF, 0xFF, 0xFF, 0x4a, 0xFF},
	/* NOP */ {0xea, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
	/* ORA */ {0xFF, 0x09, 0x05, 0x15, 0x0d, 0x1d, 0x19, 0x12, 0x01, 0x11, 0xFF, 0xFF},
	/* PHA */ {0x48, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
	/* PHP */ {0x08, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
    /* PHX */ {0xDA, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
    /* PHY */ {0x5A, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
    /* PLA */ {0x68, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
	/* PLP */ {0x28, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
    /* PLX */ {0xFA, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
    /* PLY */ {0x7A, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
    /* ROL */ {0xFF, 0xFF, 0x26, 0x36, 0x2e, 0x3e, 0xFF, 0xFF, 0xFF, 0xFF, 0x2a, 0xFF},
	/* ROR */ {0xFF, 0xFF, 0x66, 0x76, 0x6e, 0x7e, 0xFF, 0xFF, 0xFF, 0xFF, 0x6a, 0xFF},
	/* RTI */ {0x40, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
	/* RTS */ {0x60, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
	/* SBC */ {0xFF, 0xe9, 0xe5, 0xf5, 0xed, 0xfd, 0xf9, 0xFF, 0xe1, 0xf1, 0xFF, 0xFF},
	/* SEC */ {0x38, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
	/* SED */ {0xf8, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
	/* SEI */ {0x78, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
	/* STA */ {0xFF, 0xFF, 0x85, 0x95, 0x8d, 0x9d, 0x99, 0x92, 0x81, 0x91, 0xFF, 0xFF},
    /* STP */ {0xDB, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
    /* STX */ {0xFF, 0xFF, 0x86, 0x96, 0x8e, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, // Note: zp,Y addressing not supported. oops
	/* STY */ {0xFF, 0xFF, 0x84, 0x94, 0x8c, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
    /* STZ */ {0xFF, 0xFF, 0x64, 0x74, 0x9c, 0x9e, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
	/* TAX */ {0xaa, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
	/* TAY */ {0xa8, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
	/* TSX */ {0xba, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
	/* TXA */ {0x8a, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
	/* TXS */ {0x9a, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
	/* TYA */ {0x98, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
};

/*
	gets the instruction index (not the opcode) based on the first 3 bytes of string.
*/
unsigned char get_instruction_from_string(char *string) {
	unsigned char get_min = 0;
	unsigned char get_max = instruction_string_list_len;
	
	while (get_max >= get_min) {
		unsigned char get_mid = (get_min + get_max) >> 1;
		int get_cmp = memcmp(string, instruction_string_list[get_mid], 3);
		//printf("string: '%s', get-mid: %u, instruction-list[x]: '%s'\n", string, get_mid, instruction_string_list[get_mid]);
		//printf("get-mid: %d\n", get_mid);
		//printf("get-cmp: %d\n", get_cmp);
		
		if (get_cmp == 0) {
			return get_mid;
		} else if (get_cmp > 0) {
			get_min = get_mid + 1;
		} else {
			if (get_mid == 0) {
				return 0xFF;
			}
			get_max = get_mid - 1;
		}
	}
	return 0xFF;
}