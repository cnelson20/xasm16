void cbm_chrout(unsigned char c);

void strtoupper(char *string);
void strtolower(char *string);

/* Finds the first character > 0x20 (skips past whitespace chars) */
char *findwhitespace(char *string);
/* Returns first non whitespace character in a string */
char *findwhitespacerev(char *string);
/* Goes backward, finding first character not whitespace */
char *searchwhitespacerev(char *char_pointer);