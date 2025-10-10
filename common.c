
#include "common.h"

bool is_eng_letter_or_digit(unsigned char c) {
    if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'))
        return true;
    if (c >= '0' && c <= '9')
        return true;

    return false;
}

bool is_it_ru_letter(const unsigned char c) {
	if ((c >= 192 && c <= 223) || //�-�
		(c >= 224 && c <= 255) || //�-�
		(c == 168) || //�
		(c == 184)) { //� 
		return true;
	}
	return false;
}

bool is_it_ru_utf8_letter(const unsigned char* c) {
    if (c[2] != '\0' || c[1] == '\0') return false;
    unsigned char c1 = (unsigned char)c[0];
    unsigned char c2 = (unsigned char)c[1];

    // �-�
    if (c1 == 0xD0 && c2 >= 0x90 && c2 <= 0xAF) return true;
    // �-�
    if (c1 == 0xD0 && c2 >= 0xB0 && c2 <= 0xBF) return true;
    // �-�
    if (c1 == 0xD1 && c2 >= 0x80 && c2 <= 0x8F) return true;
    // �
    if (c1 == 0xD0 && c2 == 0x81) return true;
    // �
    if (c1 == 0xD1 && c2 == 0x91) return true;

    return false;
}

bool is_word_valid(unsigned char* word) {
	char* s = word;
	while (*s != '\0' && *s != '\n') {
		if (!is_it_ru_letter(*s)) {
			return 0;
		}
		s++;
	}
	return 1;
}

//cp1251
unsigned char to_lower(unsigned char c) {
	if (c >= 192 && c <= 223) {  //�-�
		return c + 32;
	}
	if (c == 168) { //�
		return 184; //�
	}
	return c;
}

void reverse_word(char* word) {
	size_t len = strlen(word);
	
	for (int i = 0; i < len / 2; i++) {
		char temp = word[len - i - 1];
		word[len - 1 - i] = word[i];
		word[i] = temp;
	}
}

void WordCell_to_char(WordCell source[], char dest[], int word_len) {
	int i = 0;
	while (i < word_len) {
		dest[i] = source[i++].letter;
	}
	dest[i] = '\0';
}


int letter_utf8_to_cp1251(const unsigned char in[2], unsigned char* cp) {
    unsigned char c0 = in[0];
    unsigned char c1 = in[1];

    // ASCII
    if (c0 < 0x80) {
        *cp = c0;
        return 1;
    }

    // �
    if (c0 == 0xD0 && c1 == 0x81) { *cp = 0xA8; return 2; }
    // �
    if (c0 == 0xD1 && c1 == 0x91) { *cp = 0xB8; return 2; }

    // � (D0 90) � � (D0 AF)
    if (c0 == 0xD0 && c1 >= 0x90 && c1 <= 0xAF) {
        *cp = 0xC0 + (c1 - 0x90);
        return 2;
    }

    // � (D0 B0) � � (D0 BF)
    if (c0 == 0xD0 && c1 >= 0xB0 && c1 <= 0xBF) {
        *cp = 0xE0 + (c1 - 0xB0);
        return 2;
    }

    // � (D1 80) � � (D1 8F)
    if (c0 == 0xD1 && c1 >= 0x80 && c1 <= 0x8F) {
        *cp = 0xF0 + (c1 - 0x80);
        return 2;
    }

    fprintf(stderr, "����������� UTF-8 ������ ��� ����������� � cp1251\n");
    return 0;
}

int letter_cp1251_to_utf8(unsigned char cp, unsigned char out[2]) {
    if (cp == 0) { 
        out[0] = 0x0;
        return 0; 
    }
    // ASCII == UTF8
    if (cp < 0x80) {
        out[0] = cp;
        return 1;
    }

    // �
    if (cp == 0xA8) { out[0] = 0xD0; out[1] = 0x81; return 2; }
    // �
    if (cp == 0xB8) { out[0] = 0xD1; out[1] = 0x91; return 2; }

    //� (0xC0) � � (0xDF)
    if (cp == (unsigned char)0xC0) { out[0] = 0xD0; out[1] = 0x90; return 2; }
    if (cp == (unsigned char)0xC1) { out[0] = 0xD0; out[1] = 0x91; return 2; }
    if (cp == (unsigned char)0xC2) { out[0] = 0xD0; out[1] = 0x92; return 2; }
    if (cp == (unsigned char)0xC3) { out[0] = 0xD0; out[1] = 0x93; return 2; }
    if (cp == (unsigned char)0xC4) { out[0] = 0xD0; out[1] = 0x94; return 2; }
    if (cp == (unsigned char)0xC5) { out[0] = 0xD0; out[1] = 0x95; return 2; }
    if (cp == (unsigned char)0xC6) { out[0] = 0xD0; out[1] = 0x96; return 2; }
    if (cp == (unsigned char)0xC7) { out[0] = 0xD0; out[1] = 0x97; return 2; }
    if (cp == (unsigned char)0xC8) { out[0] = 0xD0; out[1] = 0x98; return 2; }
    if (cp == (unsigned char)0xC9) { out[0] = 0xD0; out[1] = 0x99; return 2; }
    if (cp == (unsigned char)0xCA) { out[0] = 0xD0; out[1] = 0x9A; return 2; }
    if (cp == (unsigned char)0xCB) { out[0] = 0xD0; out[1] = 0x9B; return 2; }
    if (cp == (unsigned char)0xCC) { out[0] = 0xD0; out[1] = 0x9C; return 2; }
    if (cp == (unsigned char)0xCD) { out[0] = 0xD0; out[1] = 0x9D; return 2; }
    if (cp == (unsigned char)0xCE) { out[0] = 0xD0; out[1] = 0x9E; return 2; }
    if (cp == (unsigned char)0xCF) { out[0] = 0xD0; out[1] = 0x9F; return 2; }
    if (cp == (unsigned char)0xD0) { out[0] = 0xD0; out[1] = 0xA0; return 2; }
    if (cp == (unsigned char)0xD1) { out[0] = 0xD0; out[1] = 0xA1; return 2; }
    if (cp == (unsigned char)0xD2) { out[0] = 0xD0; out[1] = 0xA2; return 2; }
    if (cp == (unsigned char)0xD3) { out[0] = 0xD0; out[1] = 0xA3; return 2; }
    if (cp == (unsigned char)0xD4) { out[0] = 0xD0; out[1] = 0xA4; return 2; }
    if (cp == (unsigned char)0xD5) { out[0] = 0xD0; out[1] = 0xA5; return 2; }
    if (cp == (unsigned char)0xD6) { out[0] = 0xD0; out[1] = 0xA6; return 2; }
    if (cp == (unsigned char)0xD7) { out[0] = 0xD0; out[1] = 0xA7; return 2; }
    if (cp == (unsigned char)0xD8) { out[0] = 0xD0; out[1] = 0xA8; return 2; }
    if (cp == (unsigned char)0xD9) { out[0] = 0xD0; out[1] = 0xA9; return 2; }
    if (cp == (unsigned char)0xDA) { out[0] = 0xD0; out[1] = 0xAA; return 2; }
    if (cp == (unsigned char)0xDB) { out[0] = 0xD0; out[1] = 0xAB; return 2; }
    if (cp == (unsigned char)0xDC) { out[0] = 0xD0; out[1] = 0xAC; return 2; }
    if (cp == (unsigned char)0xDD) { out[0] = 0xD0; out[1] = 0xAD; return 2; }
    if (cp == (unsigned char)0xDE) { out[0] = 0xD0; out[1] = 0xAE; return 2; }
    if (cp == (unsigned char)0xDF) { out[0] = 0xD0; out[1] = 0xAF; return 2; }
    // � (0xE0) � � (0xFF)
    if (cp == (unsigned char)0xE0) { out[0] = 0xD0; out[1] = 0xB0; return 2; }
    if (cp == (unsigned char)0xE1) { out[0] = 0xD0; out[1] = 0xB1; return 2; }
    if (cp == (unsigned char)0xE2) { out[0] = 0xD0; out[1] = 0xB2; return 2; }
    if (cp == (unsigned char)0xE3) { out[0] = 0xD0; out[1] = 0xB3; return 2; }
    if (cp == (unsigned char)0xE4) { out[0] = 0xD0; out[1] = 0xB4; return 2; }
    if (cp == (unsigned char)0xE5) { out[0] = 0xD0; out[1] = 0xB5; return 2; }
    if (cp == (unsigned char)0xE6) { out[0] = 0xD0; out[1] = 0xB6; return 2; }
    if (cp == (unsigned char)0xE7) { out[0] = 0xD0; out[1] = 0xB7; return 2; }
    if (cp == (unsigned char)0xE8) { out[0] = 0xD0; out[1] = 0xB8; return 2; }
    if (cp == (unsigned char)0xE9) { out[0] = 0xD0; out[1] = 0xB9; return 2; }
    if (cp == (unsigned char)0xEA) { out[0] = 0xD0; out[1] = 0xBA; return 2; }
    if (cp == (unsigned char)0xEB) { out[0] = 0xD0; out[1] = 0xBB; return 2; }
    if (cp == (unsigned char)0xEC) { out[0] = 0xD0; out[1] = 0xBC; return 2; }
    if (cp == (unsigned char)0xED) { out[0] = 0xD0; out[1] = 0xBD; return 2; }
    if (cp == (unsigned char)0xEE) { out[0] = 0xD0; out[1] = 0xBE; return 2; }
    if (cp == (unsigned char)0xEF) { out[0] = 0xD0; out[1] = 0xBF; return 2; }
    if (cp == (unsigned char)0xF0) { out[0] = 0xD1; out[1] = 0x80; return 2; }
    if (cp == (unsigned char)0xF1) { out[0] = 0xD1; out[1] = 0x81; return 2; }
    if (cp == (unsigned char)0xF2) { out[0] = 0xD1; out[1] = 0x82; return 2; }
    if (cp == (unsigned char)0xF3) { out[0] = 0xD1; out[1] = 0x83; return 2; }
    if (cp == (unsigned char)0xF4) { out[0] = 0xD1; out[1] = 0x84; return 2; }
    if (cp == (unsigned char)0xF5) { out[0] = 0xD1; out[1] = 0x85; return 2; }
    if (cp == (unsigned char)0xF6) { out[0] = 0xD1; out[1] = 0x86; return 2; }
    if (cp == (unsigned char)0xF7) { out[0] = 0xD1; out[1] = 0x87; return 2; }
    if (cp == (unsigned char)0xF8) { out[0] = 0xD1; out[1] = 0x88; return 2; }
    if (cp == (unsigned char)0xF9) { out[0] = 0xD1; out[1] = 0x89; return 2; }
    if (cp == (unsigned char)0xFA) { out[0] = 0xD1; out[1] = 0x8A; return 2; }
    if (cp == (unsigned char)0xFB) { out[0] = 0xD1; out[1] = 0x8B; return 2; }
    if (cp == (unsigned char)0xFC) { out[0] = 0xD1; out[1] = 0x8C; return 2; }
    if (cp == (unsigned char)0xFD) { out[0] = 0xD1; out[1] = 0x8D; return 2; }
    if (cp == (unsigned char)0xFE) { out[0] = 0xD1; out[1] = 0x8E; return 2; }
    if (cp == (unsigned char)0xFF) { out[0] = 0xD1; out[1] = 0x8F; return 2; }

    fprintf(stderr, "��� ����������� cp1251->utf8 ��� �������� ����������� ������\n");
    return 0;
}

void string_cp1251_to_utf8(unsigned char cp1251[], size_t cp_len, unsigned char utf8[], size_t utf8_len) {
    int j = 0;
    for (int i = 0; i < cp_len; i++) {
        int symb_size = letter_cp1251_to_utf8(cp1251[i], &utf8[j]);
        if (symb_size == 0) break;
        else j += symb_size;
    }
    utf8[j] = '\0';
}