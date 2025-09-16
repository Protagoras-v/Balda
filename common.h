#pragma once
#define _CRT_SECURE_NO_WARNINGS

#include <stdbool.h>
#include <stdio.h>
#include <windows.h>
#include <limits.h>
#include <process.h>
#include <string.h>
#include <stdlib.h>
#include <locale.h>


#define MAX_WORD_LEN 26
#define FIELD_SIZE 5
#define LEADERBOARD_SIZE 15
#define LEADERBOARD_MAX_NAME_LEN 30
#define MIN_WORD_LEN 3

//game_logic
#define STARTING_PLAYER_WORDS_CAPACITY 50
#define DEFAULT_DIFFICULTY 2
#define DEFAULT_MAX_TIME 60000 //ms
#define DEFAULT_FIRST_PLAYER 1

//dict
#define FILE_NAME "dictionary.txt"
#define FILE_STARTING_WORDS "starting_words.txt"

//ai
#define MINIMAX_DEPTH 2

//ui
#define MAX_UI_BUFFER_SIZE 50 //bufer size for cp1251 text like a btn.text
#define MAX_UI_UTF8_BUFFER_SIZE 2 * MAX_UI_BUFFER_SIZE //every cp1251 ru letter = 2 bytes in utf8

#define SCREEN_HEIGHT 750   
#define SCREEN_WIDTH 1200 // было 900

#define BTN_FONT_FILENAME "fonts/seenonim.ttf"
#define BTN_FONT_SIZE 22
#define HEADER_FONT_FILENAME "fonts/seenonim.ttf"
#define HEADER_FONT_SIZE 34
#define INPUT_FONT_FILENAME "fonts/seenonim.ttf"
#define INPUT_FONT_SIZE 22
#define TEXT_FONT_FILENAME "fonts/seenonim.ttf"
#define TEXT_FONT_SIZE 22
#define CELL_FONT_FILENAME "fonts/seenonim.ttf"
#define CELL_FONT_SIZE 22


#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))


typedef struct WordCell {
	unsigned char y : 4;
	unsigned char x : 4;
	char letter;
} WordCell;


bool is_it_ru_letter(const unsigned char c);
bool is_it_ru_utf8_letter(const unsigned char* c);

bool is_word_valid(const unsigned char* word);

unsigned char to_lower(unsigned char c);

void reverse_word(char* word);

void WordCell_to_char(WordCell source[], char dest[], int word_len);

void string_cp1251_to_utf8(unsigned char cp1251[], size_t cp_len, unsigned char utf8[], size_t utf8_len);