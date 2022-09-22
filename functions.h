void __fastcall__ cbm_chrout(unsigned char c);

void __fastcall__ strtoupper(char *string);
void __fastcall__ strtolower(char *string);

/* Finds the first character > 0x20 (skips past whitespace chars) */
char *findwhitespace(char *string);
/* Returns first non whitespace character in a string */
char *findwhitespacerev(char *string);
/* Goes backward, finding first character not whitespace */
char *searchwhitespacerev(char *char_pointer);