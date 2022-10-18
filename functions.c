#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void strtolower(char *string) {
	while (*string) {
		if (*string >= 'A' && *string <= 'Z') {
			*string += ('a' - 'A');
		}
		++string;
	}
}

void strtoupper(char *string) {
	while (*string) {
		if (*string >= 'a' && *string <= 'z') {
			*string -= ('a' - 'A');
		}
		++string;
	}
}

void cbm_chrout(char c) {
	putchar(c);
}

char *findwhitespace(char *string) {
	while (*string) {
		if (*string >= 0x21) { break; }
		string++;
	}
	return string;
}

char *searchwhitespacerev(char *char_pointer) {
		while (1) {
			if (*char_pointer >= 0x21) { return char_pointer; }
			char_pointer--;
		}
}

char *findwhitespacerev(char *string) {
	return searchwhitespacerev(string + strlen(string));
}