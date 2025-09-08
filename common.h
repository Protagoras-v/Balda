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
#define LEADERBOARD_SIZE 50
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
#define SCREEN_HEIGHT 1200
#define SCREEN_WIDTH 900


#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))


typedef struct WordCell {
	unsigned char y : 4;
	unsigned char x : 4;
	char letter;
} WordCell;


bool is_it_ru_letter(const unsigned char c);

bool is_word_valid(const unsigned char* word);

unsigned char to_lower(unsigned char c);

void reverse_word(char* word);

void WordCell_to_char(WordCell source[], char dest[], int word_len);