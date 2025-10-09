#pragma once

#include <SDL.h>
#include <SDL_ttf.h>
#include "common.h"
#include "game_logic.h"
#include "ai.h"

typedef enum {
	SCREEN_MAIN,
	SCREEN_SETTINGS,
	SCREEN_GAME,
	SCREEN_LEADERBOARD,
} Screen;

typedef struct {
	SDL_Texture* texture; //one big texture
	SDL_Rect rect;
	unsigned int scroll : 6; // max scrolling value = words_count - page_limit (if this value is negative scrolling is unavailable too)
	unsigned int words_count : 6; //for 7x7 it is 42 turns / 2 players = 21 words max
	unsigned int is_hovered : 1;
} WordsArea;

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
	SDL_Texture* texture_message_filename;
	SDL_Texture* texture_message_invalid_input;

	Button btn_new_game;
	Button btn_load_game;
	Button btn_to_settings;
	Button btn_to_leaderboard;
	Button btn_exit;
	InputField filename_field;

	unsigned int message_filename : 1;
	unsigned int message_invalid_input : 1;
	unsigned int close_message : 1; //signal for closing message_invalid_input when user press ENTER

	SDL_Rect rect_message;
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

	Button btn_5x5;
	Button btn_7x7;
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
	SDL_Rect rect;

	unsigned int is_cursored : 1;
	unsigned int is_selected : 1; //is it part of a new word
} UICell;

typedef struct GameScreen {
	Button btn_exit;
	Button btn_save;

	SDL_Texture* percent_texture;
	unsigned int percent : 7;

	unsigned int current_player : 2; //0 isnt used, because 0 is an empty cell
	SDL_Texture* player_texture; //text
	SDL_Texture* player_score_texture;
	SDL_Texture* computer_texture; //text
	SDL_Texture* computer_score_texture;

	Button btn_up;
	Button btn_down;
	Button btn_left;
	Button btn_right;

	UICell** grid;
	unsigned int field_height : 8;
	unsigned int field_width : 8;

	unsigned int cursor_x : 8;
	unsigned int cursor_y : 8;

	unsigned int is_stoped : 1;
	unsigned int is_cursor_active : 1;
	
	char letter;
	unsigned int is_letter_placed : 1; // if letter is placed screen buttons have to be unresponsible, so we can check in event handlers which phase of the turn it is (letter placing or letter selection)
	unsigned int text_input : 1; //when user press RETURN and must select a letter
	unsigned int is_space_pressed : 1;

	//like a virtual keys
	unsigned int starting_selection : 1;
	unsigned int confirm_selection : 1;
	unsigned int input_switch : 1;
	unsigned int new_letter : 1;
	unsigned int delete_letter : 1; //and delete all selected cells too, i.e., return to "select a letter" step

	int new_selected_cell_y : 6;
	int new_selected_cell_x : 6;

	//alerts
	SDL_Rect rect_message;

	Button btn_message_yes;
	Button btn_message_no;

	SDL_Texture* invalid_word_message_texture;

	SDL_Texture* need_additional_time_texture1;
	SDL_Texture* need_additional_time_texture2;

	SDL_Texture* ask_for_username_message_texture;
	InputField input_field;

	SDL_Texture* end_game_message_texture;

	SDL_Texture* quit_confirm_message_texture;
	SDL_Texture* save_message_texture;

	unsigned int message_additional_time : 1;
	unsigned int message_invalid_word : 1;
	unsigned int message_ask_for_username : 1;
	unsigned int message_end_game : 1;
	unsigned int message_quit_confirm : 1; //are you sure?
	unsigned int message_save : 1;

	//user`s and computer`s words
	unsigned int page_limit : 6; // max number of words displayed in the words area without scrolling
	WordsArea user_words;
	WordsArea computer_words;
	int user_scroll : 6; //if not zero - it will be handled in ui_update_logic()
	int comp_scroll : 6; //if not zero - it will be handled in ui_update_logic()

	SDL_Texture* user_score;
	SDL_Texture* comp_score;

} GameScreen;

typedef struct ScreenContext {
	TTF_Font* btn_font;
	TTF_Font* alert_btn_font;
	TTF_Font* header_font;
	TTF_Font* input_field_font;
	TTF_Font* text_font; 
	TTF_Font* cell_font; 
	Screen current_screen;

	int text_font_height; //used for words area
	int text_font_width; //used for words area
} ScreenContext;


StatusCode ui_init(SDL_Window** window, SDL_Renderer** renderer);

StatusCode ui_handle_events(
	SDL_Renderer* render,
	ScreenContext* context,
	MainScreen* main_screen,
	SettingsScreen* sett_screen,
	LeaderboardScreen* lb_screen,
	GameScreen* g_screen
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
	AIState* state,
	Leaderboard* lb,
	bool* f
);


StatusCode ui_render(SDL_Renderer* renderer, ScreenContext* context,
	MainScreen* main_screen,
	SettingsScreen* settings_screen,
	LeaderboardScreen* lb_screen,
	GameScreen* g_screen
);

StatusCode ui_set_screen_context(SDL_Renderer* renderer, ScreenContext* context,
	MainScreen* main_screen,
	SettingsScreen* screen_settings,
	LeaderboardScreen* lb_screen,
	GameScreen* g_screen,
	GameSettings* game_settings);