#define PRINT_DEBUG(s) printf ("%s: %s\n", __func__, s);

extern char *fileloc;
extern char *fileloc_term;

extern struct dyn_list instruction_list;

void main();
void loadfile(char *filename);

void first_pass();
void second_pass();
void third_pass();
void fourth_pass();

int open_file_write();

struct string_short_pair {
	char *key;
	unsigned short val;
};

unsigned char is_letter(char c);

unsigned char is_num(char *string);
unsigned short parse_num(char *string);

unsigned char is_in_symtable(char *string);
unsigned short get_symbol_value(char *string);
unsigned short find_in_symtable(char *string);