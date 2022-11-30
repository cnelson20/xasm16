struct instruction {
	unsigned char type;
	unsigned char mode;
	unsigned char mem_size;
	char *label;
	
	unsigned char opcode;
	unsigned short val;
};

struct command {
	struct instruction inst;
	char *label;
	char *label2;

    unsigned short line_num;

	unsigned char is_directive;
	unsigned char *directive_data;
	unsigned char directive_data_len;
};

extern unsigned char cx16_prg_header[];
extern unsigned char cx16_prg_header_len;
extern unsigned short cx16_startaddr;

extern unsigned char opcode_matrix[][12];

extern char instruction_string_list[][4];
extern unsigned char instruction_string_list_len;
unsigned char get_instruction_from_string(char *string);

#define IN_ADC 0
#define IN_AND 1
#define IN_ASL 2
#define IN_BCC 3
#define IN_BCS 4
#define IN_BEQ 5
#define IN_BIT 6
#define IN_BMI 7
#define IN_BNE 8
#define IN_BPL 9
#define IN_BRK 10
#define IN_BVC 11
#define IN_BVS 12
#define IN_CLC 13
#define IN_CLD 14
#define IN_CLI 15
#define IN_CLV 16
#define IN_CMP 17
#define IN_CPX 18
#define IN_CPY 19
#define IN_DEC 20
#define IN_DEX 21
#define IN_DEY 22
#define IN_EOR 23
#define IN_INC 24
#define IN_INX 25
#define IN_INY 26
#define IN_JMP 27
#define IN_JSR 28
#define IN_LDA 29
#define IN_LDX 30
#define IN_LDY 31
#define IN_LSR 32
#define IN_NOP 33
#define IN_ORA 34
#define IN_PHA 35
#define IN_PHP 36
#define IN_PLA 37
#define IN_PLP 38
#define IN_ROL 39
#define IN_ROR 40
#define IN_RTI 41
#define IN_RTS 42
#define IN_SBC 43
#define IN_SEC 44
#define IN_SED 45
#define IN_SEI 46
#define IN_STA 47
#define IN_STX 48
#define IN_STY 49
#define IN_TAX 50
#define IN_TAY 51
#define IN_TSX 52
#define IN_TXA 53
#define IN_TXS 54
#define IN_TYA 55

#define MODE_IMP 0
#define MODE_IMM 1
#define MODE_ZP 2
#define MODE_ZP_X 3
#define MODE_ABS 4
#define MODE_ABS_X 5
#define MODE_ABS_Y 6
#define MODE_IND 7
#define MODE_IND_X 8
#define MODE_IND_Y 9

#define MODE_REL 11
#define MODE_ACC 10
