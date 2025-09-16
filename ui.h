#pragma once

#include <SDL.h>
#include <SDL_ttf.h>
#include "common.h"
#include "game_logic.h"

typedef enum {
	SCREEN_MAIN,
	SCREEN_SETTINGS,
	SCREEN_GAME,
	SCREEN_LEADERBOARD,
} Screen;

typedef struct Button {
	char text[MAX_UI_BUFFER_SIZE];
	SDL_Rect rect;

	SDL_Texture* texture;

	unsigned int is_hoverable : 1;
	unsigned int is_hovered : 1;
	unsigned int is_clicked : 1;
	unsigned int is_active : 1;
} Button;

typedef struct InputField {
	SDL_Rect rect;

	unsigned int is_there_new_letter : 1;
	char new_letter;
	char text[MAX_UI_BUFFER_SIZE]; //cp1251

	int cursorPos;
	unsigned int is_active : 1;
	unsigned int is_hovered : 1;
	unsigned int is_clicked : 1;
} InputField;


typedef struct MainScreen {
	SDL_Texture* header;
	Button btn_new_game;
	Button btn_load_game;
	Button btn_to_settings;
	Button btn_to_leaderboard;
	Button btn_exit;
} MainScreen;

typedef struct SettingsScreen {
	SDL_Texture* header;

	Button btn_to_main;

	Button btn_easy;
	Button btn_mid;
	Button btn_hard;

	SDL_Texture* first_player;
	Button btn_p1;
	Button btn_p2;

	SDL_Texture* time_limit;
	InputField timelimit_field;
} SettingsScreen;

typedef struct LeaderboardScreen {
	SDL_Texture* header;
	SDL_Texture* users[LEADERBOARD_SIZE];
	SDL_Texture* scores[LEADERBOARD_SIZE];
	SDL_Texture* nums[LEADERBOARD_SIZE];

	int count_of_records;

	Button btn_back;
} LeaderboardScreen;


typedef struct UICell {
	SDL_Texture* texture;
	unsigned int x:8; //field cords (not a SDL!)
	unsigned int y:8; //field cords (not a SDL!)

	unsigned int is_cursored : 1;
	unsigned int is_selected : 1; //is it part of a new word
} UICell;

typedef struct GameScreen {
	SDL_Texture* player; //text
	SDL_Texture* player_score;
	SDL_Texture* computer; //text
	SDL_Texture* computer_score;

	Button btn_up;
	Button btn_down;
	Button btn_left;
	Button btn_right;

	UICell grid[FIELD_SIZE][FIELD_SIZE];
	int cursor_x;
	int cursor_y;

	//и области со словами, + нужно разобарться с алертсами
} GameScreen;

typedef struct ScreenContext {
	TTF_Font* btn_font;
	TTF_Font* header_font;
	TTF_Font* input_field_font;
	TTF_Font* text_font; 
	TTF_Font* cell_font; 
	Screen current_screen;
} ScreenContext;


StatusCode ui_init(SDL_Window** window, SDL_Renderer** renderer);

StatusCode ui_handle_events(
	SDL_Renderer* render,
	ScreenContext context,
	MainScreen* main_screen,
	SettingsScreen* sett_screen,
	LeaderboardScreen* lb_screen
);

StatusCode ui_update_logic(
	SDL_Renderer* renderer,
	ScreenContext* context,
	MainScreen* main_screen,
	SettingsScreen* sett_screen,
	LeaderboardScreen* lb_screen,
	GameScreen* g_screen,
	Game** game,
	Dictionary* dict,
	GameSettings* settings,
	Leaderboard* lb,
	bool* f
);

StatusCode ui_render(SDL_Renderer* renderer, ScreenContext context,
	MainScreen main_screen,
	SettingsScreen settings_screen,
	LeaderboardScreen lb_screen,
	GameScreen g_screen
);

StatusCode ui_set_screen_context(SDL_Renderer* renderer, ScreenContext* context,
	MainScreen* main_screen,
	SettingsScreen* screen_settings,
	LeaderboardScreen* lb_screen,
	GameScreen* g_screen,
	GameSettings* game_settings);