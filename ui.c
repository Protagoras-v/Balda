#define _CRT_SECURE_NO_WARNINGS
#include <SDL.h>
#include <SDL_ttf.h>

#include "common.h"
#include "status_codes.h"
#include "ui.h"
#include "game_logic.h"
#include "ai.h"

#define WHITE 255, 255, 255, 255
#define BLACK 0, 0, 0, 0
#define BLUE 0, 94, 102, 0
#define GRAY 184, 184, 184, 0
#define GRAY_HOVERED 222, 222, 222, 0
#define YELLOW 255, 255, 0, 0
#define PURPLE 225, 28, 255, 0
#define GREEN 0, 255, 0, 0



static void ui_clear_word_selection(GameScreen* g_screen) {
	for (int y = 0; y < FIELD_SIZE; y++) {
		for (int x = 0; x < FIELD_SIZE; x++) {
			g_screen->grid[y][x].is_selected = 0;
		}
	}
}

//figure out text`s len in pixels before pointer
static int get_cursor_padding(TTF_Font* font, const char* text, int len) {
	//we need to copy* len* letters into buffer
	char buffer[MAX_UI_UTF8_BUFFER_SIZE] = { 0 };
	strncpy_s(buffer, MAX_UI_UTF8_BUFFER_SIZE, text, len);

	int w = 0;
	TTF_SizeUTF8(font, buffer, &w, NULL);

	return w;
}

//update BACKSPACE, ARROWS and RETURN keys, change cursor position and remove letters from input field
static void field_cursor_move(SDL_Event e, InputField* field) {
	switch (e.key.keysym.sym) {
	case SDLK_BACKSPACE:
		if (field->cursorPos > 0 && field->is_active) {
			//memmove because its safe for crossed parts of memmory
			memmove(&field->text[field->cursorPos - 1], &field->text[field->cursorPos], strlen(field->text) - field->cursorPos + 1);
			field->cursorPos--;
		}
		break;
	case SDLK_RIGHT:
		if (field->cursorPos < strlen(field->text) && field->is_active)
			field->cursorPos++;
		break;
	case SDLK_LEFT:
		if (field->cursorPos > 0 && field->is_active)
			field->cursorPos--;
		break;
	case SDLK_RETURN:
		if (field->is_active) {
			field->is_clicked = 1;
		}
	}
}

//if word is longer than WORDS_AREA_WIDTH it will bu cuted
static void cut_long_words(ScreenContext* context, char** words, int words_count) {
	int max_word_len = (WORDS_AREA_WIDTH - WORDS_AREA_PADDING * 2) / context->text_font_width;
	int len;
	for (int i = 0; i < words_count; i++) {
		len = strlen(words[i]);
		if (len > max_word_len) {
			//replace 2 last letters by ..
			for (int j = 1; j < 3; j++) {
				words[i][max_word_len - j] = '.';
			}
		}
	}
}


//receives cp1251 and automatically convert it to utf8
static SDL_Texture* createTextureFromText(SDL_Renderer* renderer, TTF_Font* font, char* text) {
	char utf8[MAX_UI_UTF8_BUFFER_SIZE];
	string_cp1251_to_utf8(text, MAX_UI_BUFFER_SIZE, utf8, MAX_UI_UTF8_BUFFER_SIZE);
	SDL_Surface* temp_surface = TTF_RenderUTF8_Blended(font, utf8, (SDL_Color) { BLACK });
	if (temp_surface == NULL) {
		fprintf(stderr, "Ошибка при создании поверхности %s\n", TTF_GetError());
		return NULL;
	}
	SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, temp_surface);
	if (texture == NULL) {
		fprintf(stderr, "Ошибка при созднии текстуры, %s\n", TTF_GetError());
		return NULL;
	}
	SDL_FreeSurface(temp_surface);

	return texture;
}


static void update_percent(SDL_Renderer* renderer, GameScreen* g_screen, ScreenContext* context, int new_percent) {
	if (g_screen->percent_texture != NULL) SDL_DestroyTexture(g_screen->percent_texture);
	char text[5] = { 0 };
	_itoa(new_percent, text, 10);

	//add '%'
	if (new_percent < 10) {
		text[1] = '%';
	}
	else if (new_percent < 100) {
		text[2] = '%';
	}
	else {
		text[3] = '%';
	}

	g_screen->percent_texture = createTextureFromText(renderer, context->text_font, text);

	if (new_percent == 100) {
		g_screen->percent = 0; //turn have been ended so texture will be 100%, but value will be zero (to hide percent texture on next frame)
	}
	else {
		g_screen->percent = new_percent;
	}
}


//update words areas textures and score if they were changed (every turn after word placement by default)
static void update_words_areas(SDL_Renderer* renderer, Game* game, GameScreen* g_screen, ScreenContext* context) {
	char** user_words;
	int uw_count;
	char** computer_words;
	int cw_count;

	StatusCode code = game_get_player_words(game, 1, &user_words, &uw_count);
	if (code != SUCCESS) {
		fprintf(stderr, "update_words_areas() error code - %\n", code);
	}
	code = game_get_player_words(game, 2, &computer_words, &cw_count);
	if (code != SUCCESS) {
		fprintf(stderr, "update_words_areas() error code - %\n", code);
	}

	cut_long_words(context, user_words, uw_count);
	cut_long_words(context, computer_words, cw_count);
	g_screen->user_words.words_count = uw_count;
	g_screen->computer_words.words_count = cw_count;

	SDL_Surface* uw_surface = SDL_CreateRGBSurface(0,
		WORDS_AREA_WIDTH,
		(context->text_font_height + WORDS_AREA_TEXT_INRERVAL) * MAX_WORDS_COUNT / 2, 
		32, 
		0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
	//copy string by string to main surface
	int y_offset = 0;
	for (int i = 0; i < uw_count; i++) {
		char utf8_s[MAX_UI_UTF8_BUFFER_SIZE] = { 0 };
		string_cp1251_to_utf8(user_words[i], strlen(user_words[i]), utf8_s, MAX_UI_UTF8_BUFFER_SIZE);
		SDL_Surface* text_surf = TTF_RenderUTF8_Blended(context->text_font, utf8_s, (SDL_Color) {BLACK});
		SDL_Rect dest = { 0, y_offset, text_surf->w, text_surf->h };
		SDL_BlitSurface(text_surf, NULL, uw_surface, &dest);
		y_offset += context->text_font_height + WORDS_AREA_TEXT_INRERVAL;
		SDL_FreeSurface(text_surf);
	}
	if (g_screen->user_words.texture != NULL) SDL_DestroyTexture(g_screen->user_words.texture);
	g_screen->user_words.texture = SDL_CreateTextureFromSurface(renderer, uw_surface);
	SDL_FreeSurface(uw_surface);

	//computer`s words
	SDL_Surface* cw_surface = SDL_CreateRGBSurface(0,
		WORDS_AREA_WIDTH,
		(context->text_font_height + WORDS_AREA_TEXT_INRERVAL) * MAX_WORDS_COUNT / 2,
		32, 
		0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
	//copy string by string to main surface
	y_offset = 0;
	for (int i = 0; i < cw_count; i++) {
		char utf8_s[MAX_UI_UTF8_BUFFER_SIZE] = { 0 };
		string_cp1251_to_utf8(computer_words[i], strlen(computer_words[i]), utf8_s, MAX_UI_UTF8_BUFFER_SIZE);
		SDL_Surface* text_surf = TTF_RenderUTF8_Blended(context->text_font, utf8_s, (SDL_Color) { BLACK });
		SDL_Rect dest = { 0, y_offset, text_surf->w, text_surf->h };
		SDL_BlitSurface(text_surf, NULL, cw_surface, &dest);
		y_offset += text_surf->h + WORDS_AREA_TEXT_INRERVAL;
		SDL_FreeSurface(text_surf);
	}
	if (g_screen->computer_words.texture != NULL) SDL_DestroyTexture(g_screen->computer_words.texture);
	g_screen->computer_words.texture = SDL_CreateTextureFromSurface(renderer, cw_surface);
	SDL_FreeSurface(cw_surface);

	//update score
	int u_score, c_score;

	code = game_get_score(game, 1, &u_score);
	if (code != SUCCESS) fprintf(stderr, "error code - %d, update_words_areas()\n", code);
	code = game_get_score(game, 2, &c_score);
	if (code != SUCCESS) fprintf(stderr, "error code - %d, update_words_areas()\n", code);

	printf("%d %d\n", u_score, c_score);
	char u_score_s[5];
	char c_score_s[5];
	_itoa(u_score, u_score_s, 10);
	_itoa(c_score, c_score_s, 10);
	g_screen->user_score = createTextureFromText(renderer, context->text_font, u_score_s);
	g_screen->comp_score = createTextureFromText(renderer, context->text_font, c_score_s);
}


// set (or reset) g_screen, make a graphic representation of game when it was started
static void load_game_screen(SDL_Renderer* renderer, ScreenContext* context, GameScreen* g_screen, Game* game, GameSettings* settings) {
	g_screen->current_player = game_get_settings_first_player(settings);
	g_screen->percent = 0;
	g_screen->percent_texture = NULL;
	g_screen->is_stoped = 0;
	g_screen->letter = '\0';
	g_screen->text_input = 0;
	g_screen->input_switch = 0;
	g_screen->is_cursor_active = 1;
	g_screen->is_letter_placed = 0;
	g_screen->new_letter = 0;
	g_screen->is_space_pressed = 0;
	g_screen->starting_selection = 0;
	g_screen->confirm_selection = 0;
	g_screen->input_switch = 0;
	g_screen->delete_letter = 0;
	g_screen->new_selected_cell_y = -1;
	g_screen->new_selected_cell_x = -1;
	g_screen->message_invalid_word = 0;
	g_screen->message_ask_for_username = 0;
	g_screen->message_end_game = 0;
	g_screen->message_quit_confirm = 0;
	g_screen->message_save = 0;

	int h, w;
	int start_x = 0;
	int start_y = 0;
	int cell_size = 0;
	game_get_field_size(game, &h, &w);
	//for different field sizes - different coordinates of cell[0][0]
	if (w == 5 && h == 5) {
		start_x = 350;
		start_y = 75;
		cell_size = 100;
	}
	else {
		fprintf(stderr, "ERROR: THERE ARE NO start_x AND start_y VALUES FOR %d x %d field!!!\n", h, w);
	}
	for (int y = 0; y < h; y++) {
		for (int x = 0; x < w; x++) {
			g_screen->grid[y][x].rect = (SDL_Rect){ start_x + cell_size * x, start_y + cell_size * y, cell_size, cell_size };
			g_screen->grid[y][x].texture = NULL;
			char letter[2] = { 0 };
			game_get_cell_letter(game, y, x, &letter[0]);
			if (letter[0] == '\0') {
				g_screen->grid[y][x].texture = NULL;
			}
			else {
				if (g_screen->grid[y][x].texture != NULL) SDL_DestroyTexture(g_screen->grid[y][x].texture);
				g_screen->grid[y][x].texture = createTextureFromText(renderer, context->cell_font, &letter);
			}
		}
	}
	//cursor
	g_screen->cursor_y = h / 2; // for 5x5 height = width = 5 (not 4)
	g_screen->cursor_x = w / 2;
	g_screen->grid[g_screen->cursor_y][g_screen->cursor_x].is_cursored = 1;

	int p1 = 0;
	int p2 = 0;
	char p1_score[8], p2_score[8];
	game_get_score(game, 1, p1_score);
	game_get_score(game, 1, p2_score);
	_itoa(p1, p1_score, 10);
	_itoa(p2, p2_score, 10);

	g_screen->computer_score_texture = createTextureFromText(renderer, context->text_font, p2_score);
	g_screen->player_score_texture = createTextureFromText(renderer, context->text_font, p1_score);

	update_words_areas(renderer, game, g_screen, context);
}



//update field, score and etc. after ai turn
static void update_game_screen(SDL_Renderer* renderer, ScreenContext* context, GameScreen* g_screen, Game* game) {
	int w, h;
	game_get_field_size(game, &h, &w);
	for (int y = 0; y < h; y++) {
		for (int x = 0; x < w; x++) {
			char letter[2] = { 0 };
			game_get_cell_letter(game, y, x, &letter[0]);
			if (letter[0] == '\0') {
				g_screen->grid[y][x].texture = NULL;
			}
			else {
				if (g_screen->grid[y][x].texture != NULL) SDL_DestroyTexture(g_screen->grid[y][x].texture);
				g_screen->grid[y][x].texture = createTextureFromText(renderer, context->cell_font, &letter);
			}
		}
	}
	//cursor
	g_screen->grid[g_screen->cursor_y][g_screen->cursor_x].is_cursored = 1;

	int p1 = 0;
	int p2 = 0;
	char p1_score[8], p2_score[8];
	game_get_score(game, 1, p1_score);
	game_get_score(game, 1, p2_score);
	_itoa(p1, p1_score, 10);
	_itoa(p2, p2_score, 10);

	g_screen->computer_score_texture = createTextureFromText(renderer, context->text_font, p2_score);
	g_screen->player_score_texture = createTextureFromText(renderer, context->text_font, p1_score);

	update_words_areas(renderer, game, g_screen, context);
}


static void destroy_game_screen(GameScreen* g_screen, Game* game) {
	int w, h;
	game_get_field_size(game, &h, &w);
	for (int y = 0; y < h; y++) {
		for (int x = 0; x < w; x++) {
			if (g_screen->grid[y][x].texture != NULL) SDL_DestroyTexture(g_screen->grid[y][x].texture);
			g_screen->grid[y][x].texture = NULL;
			g_screen->grid[y][x].is_cursored = 0;
			g_screen->grid[y][x].is_selected = 0;
			g_screen->grid[y][x].rect = (SDL_Rect){ 0, 0, 0, 0 };
		}
	}
	/*SDL_DestroyTexture(g_screen->ask_for_username_message_texture);
	SDL_DestroyTexture(g_screen->computer_score_texture);
	SDL_DestroyTexture(g_screen->computer_texture);
	SDL_DestroyTexture(g_screen->end_game_message_texture);
	SDL_DestroyTexture(g_screen->invalid_word_message_texture);
	SDL_DestroyTexture(g_screen->need_additional_time_texture1);
	SDL_DestroyTexture(g_screen->need_additional_time_texture2);
	SDL_DestroyTexture(g_screen->percent_texture);
	SDL_DestroyTexture(g_screen->player_score_texture);
	SDL_DestroyTexture(g_screen->player_texture);
	SDL_DestroyTexture(g_screen->quit_confirm_message_texture);
	SDL_DestroyTexture(g_screen->save_message_texture);
	g_screen->ask_for_username_message_texture = NULL;
	g_screen->computer_score_texture = NULL;
	g_screen->computer_texture = NULL;
	g_screen->end_game_message_texture = NULL;
	g_screen->invalid_word_message_texture = NULL;
	g_screen->need_additional_time_texture1 = NULL;
	g_screen->need_additional_time_texture2 = NULL;
	g_screen->percent_texture = NULL;
	g_screen->player_score_texture = NULL;
	g_screen->player_texture = NULL;
	g_screen->quit_confirm_message_texture = NULL;
	g_screen->save_message_texture = NULL;

	SDL_DestroyTexture(g_screen->btn_down.texture);
	SDL_DestroyTexture(g_screen->btn_up.texture);
	SDL_DestroyTexture(g_screen->btn_left.texture);
	SDL_DestroyTexture(g_screen->btn_right.texture);
	SDL_DestroyTexture(g_screen->btn_exit.texture);
	SDL_DestroyTexture(g_screen->btn_save.texture);
	SDL_DestroyTexture(g_screen->btn_message_no.texture);
	SDL_DestroyTexture(g_screen->btn_message_yes.texture);
	g_screen->btn_down.texture = NULL;
	g_screen->btn_up.texture = NULL;
	g_screen->btn_left.texture = NULL;
	g_screen->btn_right.texture = NULL;
	g_screen->btn_exit.texture = NULL;
	g_screen->btn_save.texture = NULL;
	g_screen->btn_message_no.texture = NULL;
	g_screen->btn_message_yes.texture = NULL;*/

	SDL_DestroyTexture(g_screen->user_words.texture);
	g_screen->user_words.texture = NULL;
	g_screen->user_words.is_hovered = 0;
	g_screen->user_words.scroll = 0;
	g_screen->user_words.words_count = 0;

	SDL_DestroyTexture(g_screen->computer_words.texture);
	g_screen->computer_words.texture = NULL;
	g_screen->computer_words.is_hovered = 0;
	g_screen->computer_words.scroll = 0;
	g_screen->computer_words.words_count = 0;
}


static void load_leaderboard(SDL_Renderer* renderer, ScreenContext context, LeaderboardScreen* screen, Leaderboard* lb) {
	char usernames[LEADERBOARD_SIZE][LEADERBOARD_MAX_NAME_LEN];
	int scores[LEADERBOARD_SIZE];
	int size;
	StatusCode code = game_get_leaderboard(lb, usernames, scores, &size);
	if (code != SUCCESS) {
		fprintf(stderr, "Ошибка в update_leaderboard(), код: %d\n", code);
		return;
	}
	screen->count_of_records = size;
	for (int i = 0; i < size; i++) {
		char snum[10] = { 0 };
		_itoa(i + 1, snum, 10);

		char score[10] = { 0 };
		_itoa(scores[i], score, 10);

		screen->nums[i] = createTextureFromText(renderer, context.text_font, snum);
		screen->users[i] = createTextureFromText(renderer, context.text_font, usernames[i]);
		screen->scores[i] = createTextureFromText(renderer, context.text_font, score);
	}
}




StatusCode ui_init(SDL_Window** window, SDL_Renderer** renderer) {
	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		printf("SDL could not initialize! UI_SDL_ERROR: %s\n", SDL_GetError());
		return UI_SDL_ERROR;
	}
	else {
		*window = SDL_CreateWindow("Balda", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
		if (*window == NULL) {
			printf("Window could not be created! UI_SDL_ERROR: %s\n", SDL_GetError());
			return UI_SDL_ERROR;
		}
		else {
			*renderer = SDL_CreateRenderer(*window, -1, SDL_RENDERER_ACCELERATED);
			if (*renderer == NULL) {
				printf("ERRROR RENDERER: %s\n", SDL_GetError());
				return UI_SDL_ERROR;
			}
			else {
				SDL_SetRenderDrawColor(*renderer, 0xff, 0xff, 0xff, 0xff);
				//включаем альфа-канал
				//SDL_SetRenderDrawBlendMode(*renderer, SDL_BLENDMODE_BLEND);

				if (TTF_Init() == -1) {
					printf("SDL_TTF could not initialize! SDL_TTF Error: %s\n", TTF_GetError());
					return UI_SDL_ERROR;
				}

			}
		}
	}
	return SUCCESS;
}


StatusCode ui_set_screen_context(
	SDL_Renderer* renderer, 
	ScreenContext * context,
	MainScreen* main_screen,
	SettingsScreen* sett_screen,
	LeaderboardScreen* lb_screen,
	GameScreen* g_screen,
	GameSettings* game_settings) {
	context->btn_font = TTF_OpenFont(BTN_FONT_FILENAME, BTN_FONT_SIZE);
	if (context->btn_font == NULL) {
		fprintf(stderr, "Ошибка при открытии шрифта %s, TTF_Error: %s\n", BTN_FONT_FILENAME, TTF_GetError());
		return UI_SDL_OPEN_FONT_ERROR;
	}context->alert_btn_font = TTF_OpenFont(ALERT_BTN_FONT_FILENAME, ALERT_BTN_FONT_SIZE);
	if (context->alert_btn_font == NULL) {
		fprintf(stderr, "Ошибка при открытии шрифта %s, TTF_Error: %s\n", BTN_FONT_FILENAME, TTF_GetError());
		return UI_SDL_OPEN_FONT_ERROR;
	}
	context->header_font = TTF_OpenFont(HEADER_FONT_FILENAME, HEADER_FONT_SIZE);
	if (context->header_font == NULL) {
		fprintf(stderr, "Ошибка при открытии шрифта %s, TTF_Error: %s\n", HEADER_FONT_FILENAME, TTF_GetError());
		return UI_SDL_OPEN_FONT_ERROR;
	}
	context->input_field_font = TTF_OpenFont(INPUT_FONT_FILENAME, INPUT_FONT_SIZE);
	if (context->input_field_font == NULL) {
		fprintf(stderr, "Ошибка при открытии шрифта %s, TTF_Error: %s\n", INPUT_FONT_FILENAME, TTF_GetError());
		return UI_SDL_OPEN_FONT_ERROR;
	}
	context->text_font = TTF_OpenFont(TEXT_FONT_FILENAME, TEXT_FONT_SIZE);
	if (context->text_font == NULL) {
		fprintf(stderr, "Ошибка при открытии шрифта %s, TTF_Error: %s\n", TEXT_FONT_FILENAME, TTF_GetError());
		return UI_SDL_OPEN_FONT_ERROR;
	}
	context->cell_font = TTF_OpenFont(CELL_FONT_FILENAME, CELL_FONT_SIZE);
	if (context->cell_font == NULL) {
		fprintf(stderr, "Ошибка при открытии шрифта %s, TTF_Error: %s\n", CELL_FONT_FILENAME, TTF_GetError());
		return UI_SDL_OPEN_FONT_ERROR;
	}

	//main menu
	main_screen->header = createTextureFromText(renderer, context->header_font, "Балда");

	main_screen->message_filename = 0;
	main_screen->message_invalid_input = 0;
	main_screen->close_message = 0;

	main_screen->texture_message_filename = createTextureFromText(renderer, context->alert_btn_font, "Введите имя файла сохранения (он должен быть в save)");
	main_screen->texture_message_invalid_input = createTextureFromText(renderer, context->alert_btn_font, "Ошибка при открытии файла! (Нажмите ENTER, чтобы закрыть)");

	main_screen->rect_message = (SDL_Rect){ ALERT_X, ALERT_Y, ALERT_WIDTH, ALERT_HEIGHT };

	strncpy_s(main_screen->btn_new_game.text, MAX_UI_BUFFER_SIZE, "Новая игра", MAX_UI_BUFFER_SIZE);
	main_screen->btn_new_game.rect = (SDL_Rect) {SCREEN_WIDTH / 2 - 137, 150, 275, 75};
	main_screen->btn_new_game.is_active = 0;
	main_screen->btn_new_game.is_hoverable = 1;
	main_screen->btn_new_game.is_hovered = 0;
	main_screen->btn_new_game.is_clicked = 0;
	main_screen->btn_new_game.texture = createTextureFromText(renderer, context->btn_font, main_screen->btn_new_game.text);
	if (main_screen->btn_new_game.texture == NULL) {
		return UI_SDL_TEXTURE_ERROR;
	}
	
	strncpy_s(main_screen->btn_load_game.text, MAX_UI_BUFFER_SIZE,  "Загрузить", MAX_UI_BUFFER_SIZE);
	main_screen->btn_load_game.rect = (SDL_Rect){ SCREEN_WIDTH / 2 - 137, 250, 275, 75 };
	main_screen->btn_load_game.is_active = 0;
	main_screen->btn_load_game.is_hoverable = 1;
	main_screen->btn_load_game.is_hovered = 0;
	main_screen->btn_load_game.is_clicked = 0;
	main_screen->btn_load_game.texture = createTextureFromText(renderer, context->btn_font, main_screen->btn_load_game.text);
	if (main_screen->btn_load_game.texture == NULL) {
		return UI_SDL_TEXTURE_ERROR;
	}

	strncpy_s(main_screen->btn_to_settings.text, MAX_UI_BUFFER_SIZE, "Настройки", MAX_UI_BUFFER_SIZE);
	main_screen->btn_to_settings.rect = (SDL_Rect){ SCREEN_WIDTH / 2 - 137, 350, 275, 75 };
	main_screen->btn_to_settings.is_active = 0;
	main_screen->btn_to_settings.is_hoverable = 1;
	main_screen->btn_to_settings.is_hovered = 0;
	main_screen->btn_to_settings.is_clicked = 0;
	main_screen->btn_to_settings.texture = createTextureFromText(renderer, context->btn_font, main_screen->btn_to_settings.text);
	if (main_screen->btn_to_settings.texture == NULL) {
		return UI_SDL_TEXTURE_ERROR;
	}

	strncpy_s(main_screen->btn_to_leaderboard.text, MAX_UI_BUFFER_SIZE, "Таблица лидеров", MAX_UI_BUFFER_SIZE);
	main_screen->btn_to_leaderboard.rect = (SDL_Rect){ SCREEN_WIDTH / 2 - 137, 450, 275, 75 };
	main_screen->btn_to_leaderboard.is_active = 0;
	main_screen->btn_to_leaderboard.is_hoverable = 1;
	main_screen->btn_to_leaderboard.is_hovered = 0;
	main_screen->btn_to_leaderboard.is_clicked = 0;
	main_screen->btn_to_leaderboard.texture = createTextureFromText(renderer, context->btn_font, main_screen->btn_to_leaderboard.text);
	if (main_screen->btn_to_leaderboard.texture == NULL) {
		return UI_SDL_TEXTURE_ERROR;
	}

	strncpy_s(main_screen->btn_exit.text, MAX_UI_BUFFER_SIZE, "Выход", MAX_UI_BUFFER_SIZE);
	main_screen->btn_exit.rect = (SDL_Rect){ SCREEN_WIDTH / 2 - 137, 550, 275, 75 };
	main_screen->btn_exit.is_active = 0;
	main_screen->btn_exit.is_hoverable = 1;
	main_screen->btn_exit.is_hovered = 0;
	main_screen->btn_exit.is_clicked = 0;
	main_screen->btn_exit.texture = createTextureFromText(renderer, context->btn_font, main_screen->btn_exit.text);
	if (main_screen->btn_exit.texture == NULL) {
		return UI_SDL_TEXTURE_ERROR;
	}

	main_screen->filename_field.rect = (SDL_Rect){ ALERT_X + ALERT_WIDTH / 2 - (ALERT_WIDTH - 100) / 2, ALERT_Y + ALERT_HEIGHT / 2 - (50 / 2), (ALERT_WIDTH - 100), 50 };
	main_screen->filename_field.is_active = 0;
	main_screen->filename_field.is_hovered = 0;
	main_screen->filename_field.is_clicked = 0;
	main_screen->filename_field.text[0] = '\0';
	main_screen->filename_field.cursorPos = strlen(main_screen->filename_field.text);

	//settings
	unsigned int diff = game_get_settings_difficulty(game_settings);
	unsigned int timelimit = game_get_settings_timelimit(game_settings);
	unsigned int frst_player = game_get_settings_first_player(game_settings);

	sett_screen->header = createTextureFromText(renderer, context->header_font, "Настройки");

	strncpy_s(sett_screen->btn_to_main.text, MAX_UI_BUFFER_SIZE, "Назад", MAX_UI_BUFFER_SIZE);
	sett_screen->btn_to_main.rect.x = 10;
	sett_screen->btn_to_main.rect.y = 10;
	sett_screen->btn_to_main.rect.w = 150;
	sett_screen->btn_to_main.rect.h = 50;
	sett_screen->btn_to_main.is_active = 0;
	sett_screen->btn_to_main.is_hoverable = 1;
	sett_screen->btn_to_main.is_hovered = 0;
	sett_screen->btn_to_main.is_clicked = 0; 
	sett_screen->btn_to_main.texture = createTextureFromText(renderer, context->btn_font, sett_screen->btn_to_main.text);
	if (sett_screen->btn_to_main.texture == NULL) {
		return UI_SDL_TEXTURE_ERROR;
	}

	strncpy_s(sett_screen->btn_easy.text, MAX_UI_BUFFER_SIZE, "Легко", MAX_UI_BUFFER_SIZE);
	sett_screen->btn_easy.rect = (SDL_Rect){250, 150, 200, 75 };
	sett_screen->btn_easy.is_active = diff == 0 ? 1 : 0;
	sett_screen->btn_easy.is_hoverable = 0;
	sett_screen->btn_easy.is_hovered = 0;
	sett_screen->btn_easy.is_clicked = 0;
	sett_screen->btn_easy.texture = createTextureFromText(renderer, context->btn_font, sett_screen->btn_easy.text);
	if (sett_screen->btn_easy.texture == NULL) {
		return UI_SDL_TEXTURE_ERROR;
	}

	strncpy_s(sett_screen->btn_mid.text, MAX_UI_BUFFER_SIZE, "Нормально", MAX_UI_BUFFER_SIZE);
	sett_screen->btn_mid.rect = (SDL_Rect){ 500, 150, 200, 75 };
	sett_screen->btn_mid.is_active = diff == 1 ? 1 : 0;
	sett_screen->btn_mid.is_hoverable = 0;
	sett_screen->btn_mid.is_hovered = 0;
	sett_screen->btn_mid.is_clicked = 0; 
	sett_screen->btn_mid.texture = createTextureFromText(renderer, context->btn_font, sett_screen->btn_mid.text);
	if (sett_screen->btn_mid.texture == NULL) {
		return UI_SDL_TEXTURE_ERROR;
	}

	strncpy_s(sett_screen->btn_hard.text, MAX_UI_BUFFER_SIZE, "Сложно", MAX_UI_BUFFER_SIZE);
	sett_screen->btn_hard.rect = (SDL_Rect){750, 150, 200, 75 };
	sett_screen->btn_hard.is_active = diff == 2 ? 1 : 0;
	sett_screen->btn_hard.is_hoverable = 0;
	sett_screen->btn_hard.is_hovered = 0;
	sett_screen->btn_hard.is_clicked = 0; 
	sett_screen->btn_hard.texture = createTextureFromText(renderer, context->btn_font, sett_screen->btn_hard.text);
	if (sett_screen->btn_hard.texture == NULL) {
		return UI_SDL_TEXTURE_ERROR;
	}


	sett_screen->first_player = createTextureFromText(renderer, context->btn_font, "Первый ход");
	if (sett_screen->first_player == NULL) {
		return UI_SDL_TEXTURE_ERROR;
	}

	strncpy_s(sett_screen->btn_p1.text, MAX_UI_BUFFER_SIZE, "Игрок", MAX_UI_BUFFER_SIZE);
	sett_screen->btn_p1.rect = (SDL_Rect){ 500, 250, 200, 75 };
	sett_screen->btn_p1.is_active = frst_player == 1 ? 1 : 0;
	sett_screen->btn_p1.is_hoverable = 0;
	sett_screen->btn_p1.is_hovered = 0;
	sett_screen->btn_p1.is_clicked = 0;
	sett_screen->btn_p1.texture = createTextureFromText(renderer, context->btn_font, sett_screen->btn_p1.text);
	if (sett_screen->btn_p1.texture == NULL) {
		return UI_SDL_TEXTURE_ERROR;
	}

	strncpy_s(sett_screen->btn_p2.text, MAX_UI_BUFFER_SIZE, "Компьютер", MAX_UI_BUFFER_SIZE);
	sett_screen->btn_p2.rect = (SDL_Rect){ 750, 250, 200, 75 };
	sett_screen->btn_p2.is_active = frst_player == 2 ? 1 : 0;
	sett_screen->btn_p2.is_hoverable = 0;
	sett_screen->btn_p2.is_hovered = 0;
	sett_screen->btn_p2.is_clicked = 0;
	sett_screen->btn_p2.texture = createTextureFromText(renderer, context->btn_font, sett_screen->btn_p2.text);
	if (sett_screen->btn_p2.texture == NULL) {
		return UI_SDL_TEXTURE_ERROR;
	}


	sett_screen->time_limit = createTextureFromText(renderer, context->btn_font, "Лимит хода компьютера (мс)");
	if (sett_screen->first_player == NULL) {
		return UI_SDL_TEXTURE_ERROR;
	}
	sett_screen->timelimit_field.rect = (SDL_Rect){ 725, 362, 225, 55 };
	sett_screen->timelimit_field.is_active = 0;
	sett_screen->timelimit_field.is_hovered = 0; 
	sett_screen->timelimit_field.is_clicked = 0;
	_itoa(timelimit, sett_screen->timelimit_field.text, 10);
	sett_screen->timelimit_field.cursorPos = strlen(sett_screen->timelimit_field.text);

	//leaderboard
	lb_screen->count_of_records = 0;
	lb_screen->header = createTextureFromText(renderer, context->btn_font, "Таблица лидеров");
	
	strncpy_s(lb_screen->btn_back.text, MAX_UI_BUFFER_SIZE, "Назад", MAX_UI_BUFFER_SIZE);
	lb_screen->btn_back.rect.x = 10;
	lb_screen->btn_back.rect.y = 10;
	lb_screen->btn_back.rect.w = 159;
	lb_screen->btn_back.rect.h = 50;
	lb_screen->btn_back.is_active = 0;
	lb_screen->btn_back.is_hoverable = 1;
	lb_screen->btn_back.is_hovered = 0;
	lb_screen->btn_back.is_clicked = 0; 
	lb_screen->btn_back.texture = createTextureFromText(renderer, context->btn_font, lb_screen->btn_back.text);
	if (lb_screen->btn_back.texture == NULL) {
		return UI_SDL_TEXTURE_ERROR;
	}


	//game_screen

	strncpy_s(g_screen->btn_exit.text, MAX_UI_BUFFER_SIZE, "Завершить", MAX_UI_BUFFER_SIZE);
	g_screen->btn_exit.rect.x = 10;
	g_screen->btn_exit.rect.y = 10;
	g_screen->btn_exit.rect.w = 200;
	g_screen->btn_exit.rect.h = 50;
	g_screen->btn_exit.is_active = 0;
	g_screen->btn_exit.is_hoverable = 1;
	g_screen->btn_exit.is_hovered = 0;
	g_screen->btn_exit.is_clicked = 0;
	g_screen->btn_exit.texture = createTextureFromText(renderer, context->btn_font, g_screen->btn_exit.text);
	if (g_screen->btn_exit.texture == NULL) {
		return UI_SDL_TEXTURE_ERROR;
	}

	strncpy_s(g_screen->btn_save.text, MAX_UI_BUFFER_SIZE, "Сохранить", MAX_UI_BUFFER_SIZE);
	g_screen->btn_save.rect.y = 10;
	g_screen->btn_save.rect.w = 200;
	g_screen->btn_save.rect.h = 50;
	g_screen->btn_save.rect.x = SCREEN_WIDTH - g_screen->btn_save.rect.w - 10;
	g_screen->btn_save.is_active = 0;
	g_screen->btn_save.is_hoverable = 1;
	g_screen->btn_save.is_hovered = 0;
	g_screen->btn_save.is_clicked = 0;
	g_screen->btn_save.texture = createTextureFromText(renderer, context->btn_font, g_screen->btn_save.text);
	if (g_screen->btn_save.texture == NULL) {
		return UI_SDL_TEXTURE_ERROR;
	}

	g_screen->computer_texture = createTextureFromText(renderer, context->text_font, "Компьютер");
	g_screen->player_texture = createTextureFromText(renderer, context->text_font, "Игрок");

	strncpy_s(g_screen->btn_up.text, MAX_UI_BUFFER_SIZE, "Вверх", MAX_UI_BUFFER_SIZE);
	g_screen->btn_up.rect.x = SCREEN_WIDTH/2 - 25;
	g_screen->btn_up.rect.y = 660;
	g_screen->btn_up.rect.w = 50;
	g_screen->btn_up.rect.h = 50;
	g_screen->btn_up.is_active = 0;
	g_screen->btn_up.is_hoverable = 1;
	g_screen->btn_up.is_hovered = 0;
	g_screen->btn_up.is_clicked = 0; 
	g_screen->btn_up.texture = NULL;

	strncpy_s(g_screen->btn_down.text, MAX_UI_BUFFER_SIZE, "Вниз", MAX_UI_BUFFER_SIZE);
	g_screen->btn_down.rect.x = SCREEN_WIDTH / 2 - 25;
	g_screen->btn_down.rect.y = 780;
	g_screen->btn_down.rect.w = 50;
	g_screen->btn_down.rect.h = 50;
	g_screen->btn_down.is_active = 0;
	g_screen->btn_down.is_hoverable = 1;
	g_screen->btn_down.is_hovered = 0;
	g_screen->btn_down.is_clicked = 0; 
	g_screen->btn_down.texture = NULL;

	strncpy_s(g_screen->btn_left.text, MAX_UI_BUFFER_SIZE, "Влево", MAX_UI_BUFFER_SIZE);
	g_screen->btn_left.rect.x = SCREEN_WIDTH / 2 - 60 - 25;
	g_screen->btn_left.rect.y = 720;
	g_screen->btn_left.rect.w = 50;
	g_screen->btn_left.rect.h = 50;
	g_screen->btn_left.is_active = 0;
	g_screen->btn_left.is_hoverable = 1;
	g_screen->btn_left.is_hovered = 0;
	g_screen->btn_left.is_clicked = 0; 
	g_screen->btn_left.texture = NULL;

	strncpy_s(g_screen->btn_right.text, MAX_UI_BUFFER_SIZE, "Вправо", MAX_UI_BUFFER_SIZE);
	g_screen->btn_right.rect.x = SCREEN_WIDTH / 2 + 60 - 25;
	g_screen->btn_right.rect.y = 720;
	g_screen->btn_right.rect.w = 50;
	g_screen->btn_right.rect.h = 50;
	g_screen->btn_right.is_active = 0;
	g_screen->btn_right.is_hoverable = 1;
	g_screen->btn_right.is_hovered = 0;
	g_screen->btn_right.is_clicked = 0;
	g_screen->btn_right.texture = NULL;

	//alerts
	g_screen->need_additional_time_texture1 = createTextureFromText(renderer, context->alert_btn_font, "Компьютер не успел найти слово за заданный интервал");
	g_screen->need_additional_time_texture2 = createTextureFromText(renderer, context->alert_btn_font, "хотите дать ему дополнительное время?");
	g_screen->message_additional_time = 0;

	g_screen->invalid_word_message_texture = createTextureFromText(renderer, context->alert_btn_font, "Такого слова нет в словаре,\n хотите добавить его?");
	g_screen->message_invalid_word = 0;
	g_screen->rect_message = (SDL_Rect){ ALERT_X, ALERT_Y, ALERT_WIDTH, ALERT_HEIGHT };

	strncpy_s(g_screen->btn_message_yes.text, MAX_UI_BUFFER_SIZE, "Да", MAX_UI_BUFFER_SIZE);
	g_screen->btn_message_yes.rect = (SDL_Rect){ ALERT_X + 100 - 40, ALERT_Y + ALERT_HEIGHT - 50 - 25, 80, 50 };
	g_screen->btn_message_yes.is_active = 0;
	g_screen->btn_message_yes.is_hoverable = 1;
	g_screen->btn_message_yes.is_hovered = 0;
	g_screen->btn_message_yes.is_clicked = 0; 
	g_screen->btn_message_yes.texture = NULL;
	g_screen->btn_message_yes.texture = createTextureFromText(renderer, context->alert_btn_font, g_screen->btn_message_yes.text);
	if (g_screen->btn_message_yes.texture == NULL) {
		return UI_SDL_TEXTURE_ERROR;
	}

	strncpy_s(g_screen->btn_message_no.text, MAX_UI_BUFFER_SIZE, "Нет", MAX_UI_BUFFER_SIZE);
	g_screen->btn_message_no.rect = (SDL_Rect){ ALERT_X + ALERT_WIDTH - 100 - 40, ALERT_Y + ALERT_HEIGHT - 50 - 25, 80, 50 };
	g_screen->btn_message_no.is_active = 0;
	g_screen->btn_message_no.is_hoverable = 1;
	g_screen->btn_message_no.is_hovered = 0;
	g_screen->btn_message_no.is_clicked = 0; 
	g_screen->btn_message_no.texture = NULL;
	g_screen->btn_message_no.texture = createTextureFromText(renderer, context->alert_btn_font, g_screen->btn_message_no.text);
	if (g_screen->btn_message_no.texture == NULL) {
		return UI_SDL_TEXTURE_ERROR;
	}

	g_screen->ask_for_username_message_texture = createTextureFromText(renderer, context->alert_btn_font, "Введите имя");
	g_screen->input_field.rect = (SDL_Rect){ ALERT_X + ALERT_WIDTH / 2 - (ALERT_WIDTH - 100) / 2, ALERT_Y + ALERT_HEIGHT / 2 - (50 / 2), (ALERT_WIDTH - 100), 50};
	g_screen->input_field.is_active = 0;
	g_screen->input_field.is_hovered = 0;
	g_screen->input_field.is_clicked = 0;
	g_screen->input_field.text[0] = '\0';
	g_screen->input_field.cursorPos = 0;

	g_screen->end_game_message_texture = createTextureFromText(renderer, context->alert_btn_font, "Игра окончена! Нажмите Enter, чтобы выйти");

	g_screen->quit_confirm_message_texture = createTextureFromText(renderer, context->alert_btn_font, "Вы уверены, что хотите закончить игру?");
	g_screen->save_message_texture = createTextureFromText(renderer, context->alert_btn_font, "Введите имя файла сохранения");

	
	//words
	g_screen->user_words.rect = (SDL_Rect){ 50, 125, WORDS_AREA_WIDTH, WORDS_AREA_HEIGHT };
	g_screen->user_words.texture = NULL;
	g_screen->user_words.scroll = 0;	
	g_screen->user_words.words_count = 0;	
	g_screen->user_words.is_hovered = 0;	

	g_screen->computer_words.rect = (SDL_Rect){SCREEN_WIDTH - WORDS_AREA_WIDTH - 50, 125, WORDS_AREA_WIDTH, WORDS_AREA_HEIGHT };
	g_screen->computer_words.texture = NULL;
	g_screen->computer_words.scroll = 0;
	g_screen->computer_words.words_count = 0;
	g_screen->computer_words.is_hovered = 0;

	g_screen->comp_scroll = 0;
	g_screen->user_scroll = 0;

	TTF_SizeText(context->text_font, "рум", NULL, &context->text_font_height); //for the correct value there should be both upper case and letter with a squiggle at the bottom (p, q, A, J...)
	TTF_SizeText(context->text_font, "а", &context->text_font_width, NULL); //for the correct value there should be both upper case and letter with a squiggle at the bottom (p, q, A, J...)
	g_screen->page_limit = (WORDS_AREA_HEIGHT - WORDS_AREA_PADDING * 2 + WORDS_AREA_TEXT_INRERVAL) / (context->text_font_height + WORDS_AREA_TEXT_INRERVAL) + 1;

	return SUCCESS;
}


//--------------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------EVENTS--------------------------------------------------------------
//--------------------------------------------------------------------------------------------------------------------------

static void check_button_hovered(SDL_Event e, Button* btn) {
	if ((e.motion.x >= btn->rect.x && e.motion.x <= btn->rect.x + btn->rect.w)
		&& (e.motion.y >= btn->rect.y && e.motion.y <= btn->rect.y + btn->rect.h)) {
		btn->is_hovered = 1;
	}
	else {
		btn->is_hovered = 0;
	}
}

static void check_field_hovered(SDL_Event e, InputField* field) {
	if ((e.motion.x >= field->rect.x && e.motion.x <= field->rect.x + field->rect.w)
		&& (e.motion.y >= field->rect.y && e.motion.y <= field->rect.y + field->rect.h)) {
		field->is_hovered = 1;
	}
	else {
		field->is_hovered = 0;
	}
}

static bool check_cell_selected(SDL_Event e, UICell* cell) {
	if ((e.motion.x >= cell->rect.x && e.motion.x <= cell->rect.x + cell->rect.w)
		&& (e.motion.y >= cell->rect.y && e.motion.y <= cell->rect.y + cell->rect.h)
		&& cell->texture != NULL) {
		cell->is_selected = 1;
		return true;
	}
	return false;
}

static void check_word_area_hovered(SDL_Event e, WordsArea* area) {
	if ((e.motion.x >= area->rect.x && e.motion.x <= area->rect.x + area->rect.w)
		&& (e.motion.y >= area->rect.y && e.motion.y <= area->rect.y + area->rect.h)
		&& area->texture != NULL) {
		area->is_hovered = 1;
	}
	else {
		area->is_hovered = 0;
	}
}

static event_mainmenu_mousemotion(SDL_Event e, MainScreen* main_screen) {
	if (main_screen->message_filename) {
		check_field_hovered(e, &main_screen->filename_field);
	}
	//burrons can be hovered only if no one message is active
	if (!main_screen->message_filename && !main_screen->message_invalid_input) {
		check_button_hovered(e, &main_screen->btn_new_game);
		check_button_hovered(e, &main_screen->btn_load_game);
		check_button_hovered(e, &main_screen->btn_to_settings);
		check_button_hovered(e, &main_screen->btn_to_leaderboard);
		check_button_hovered(e, &main_screen->btn_exit);
	}
}

static void event_mainmenu_mouseclick_down(SDL_Event e, MainScreen* main_screen) {
	if (main_screen->btn_new_game.is_hovered) {
		main_screen->btn_new_game.is_hovered = 0;
		main_screen->btn_new_game.is_clicked = 1;
		//start new game
	}
	else if (main_screen->btn_load_game.is_hovered) {
		main_screen->btn_load_game.is_hovered = 0;
		main_screen->btn_load_game.is_clicked = 1;
		//choice the file
	}
	else if (main_screen->btn_to_settings.is_hovered) {
		main_screen->btn_to_settings.is_hovered = 0;
		main_screen->btn_to_settings.is_clicked = 1;
		//settings
	}
	else if (main_screen->btn_to_leaderboard.is_hovered) {
		main_screen->btn_to_leaderboard.is_hovered = 0;
		main_screen->btn_to_leaderboard.is_clicked = 1;
		//open lb
	}
	else if (main_screen->btn_exit.is_hovered) {
		main_screen->btn_exit.is_hovered = 0;
		main_screen->btn_exit.is_clicked = 1;
		//exit
	}
	//input field 
	else if (main_screen->filename_field.is_hovered) {
		main_screen->filename_field.is_clicked = 1;
		//???
		main_screen->filename_field.is_hovered = 0;
		//???
	}
}

static void event_mainmenu_keydown(SDL_Event e, MainScreen* main_screen) {
	if (main_screen->message_filename) field_cursor_move(e, &main_screen->filename_field);
	if (main_screen->message_invalid_input && e.key.keysym.sym == SDLK_RETURN) {
		main_screen->close_message = 1;
	}
}

static void event_mainmenu_text_input(SDL_Event e, MainScreen* main_screen) {
	if (main_screen->message_filename) {
		char cp1251 = '\0';
		letter_utf8_to_cp1251(e.text.text, &cp1251);
		if (is_eng_letter_or_digit(cp1251)) {
			main_screen->filename_field.new_letter = cp1251;
			main_screen->filename_field.is_there_new_letter = 1;
		}
	}
}



static void event_settings_mousemotion(SDL_Event e, SettingsScreen* sett_screen) {
	if (!sett_screen->timelimit_field.is_active) {
		check_button_hovered(e, &sett_screen->btn_to_main);
		check_button_hovered(e, &sett_screen->btn_easy);
		check_button_hovered(e, &sett_screen->btn_mid);
		check_button_hovered(e, &sett_screen->btn_hard);
		check_button_hovered(e, &sett_screen->btn_p1);
		check_button_hovered(e, &sett_screen->btn_p2);
	}
	check_field_hovered(e, &sett_screen->timelimit_field);
}

static void event_settings_mouseclick(SDL_Event e, SettingsScreen* sett_screen) {
	if (sett_screen->btn_to_main.is_hovered) {
		sett_screen->btn_to_main.is_clicked = 1;
		sett_screen->btn_to_main.is_hovered = 0;
	}
	else if (sett_screen->btn_easy.is_hovered) {
		if (!sett_screen->btn_easy.is_active) {
			sett_screen->btn_easy.is_clicked = 1;
			sett_screen->btn_easy.is_active = 1;
			sett_screen->btn_mid.is_active = 0;
			sett_screen->btn_hard.is_active = 0;
		}
	}
	else if (sett_screen->btn_mid.is_hovered) {
		if (!sett_screen->btn_mid.is_active) {
			sett_screen->btn_mid.is_clicked = 1;
			sett_screen->btn_easy.is_active = 0;
			sett_screen->btn_mid.is_active = 1;
			sett_screen->btn_hard.is_active = 0;
		}
	}
	else if (sett_screen->btn_hard.is_hovered) {
		if (!sett_screen->btn_hard.is_active) {
			sett_screen->btn_hard.is_clicked = 1;
			sett_screen->btn_easy.is_active = 0;
			sett_screen->btn_mid.is_active = 0;
			sett_screen->btn_hard.is_active = 1;
		}
	}

	else if (sett_screen->btn_p1.is_hovered) {
		if (!sett_screen->btn_p1.is_active) {
			sett_screen->btn_p1.is_clicked = 1;
			sett_screen->btn_p1.is_active = 1;
			sett_screen->btn_p2.is_active = 0;
		}
	}
	else if (sett_screen->btn_p2.is_hovered) {
		if (!sett_screen->btn_p2.is_active) {
			sett_screen->btn_p2.is_clicked = 1;
			sett_screen->btn_p1.is_active = 0;
			sett_screen->btn_p2.is_active = 1;
		}
	}
	else if (sett_screen->timelimit_field.is_hovered) {
		sett_screen->timelimit_field.is_clicked = 1;
		//is_active will be changed in ui_update_logic()
	}
}

static void event_settings_keydown(SDL_Event e, SettingsScreen* sett_screen) {
	field_cursor_move(e, &sett_screen->timelimit_field);
}

static void event_settings_text_input(SDL_Event e, SettingsScreen* sett_screen) {
	char* c = e.text.text;
	if (c[0] >= '0' && c[0] <= '9' && c[1] == '\0') {
		sett_screen->timelimit_field.new_letter = c[0];
		sett_screen->timelimit_field.is_there_new_letter = 1;
	}
}


static void event_lb_mousemotion(SDL_Event e, LeaderboardScreen* lb_screen) {
	check_button_hovered(e, &lb_screen->btn_back);
}

static void event_lb_mouseclick(SDL_Event e, LeaderboardScreen* lb_screen) {
	if (lb_screen->btn_back.is_hovered) {
		lb_screen->btn_back.is_hovered = 0;
		lb_screen->btn_back.is_clicked = 1;
	}
}

//
//All logic links with the differentiation of parts of turn (letter placing, selection and confirmation) are managed in event part. 
//For example, the event "space is pressed" will be registered only if is_letter_place == 1
//
static void event_game_keydown(SDL_Event e, GameScreen* g_screen) {
	if ((g_screen->message_ask_for_username || g_screen->message_save) && g_screen->input_field.is_active) {
		field_cursor_move(e, &g_screen->input_field);
	}
	switch (e.key.keysym.sym) {
	case SDLK_RETURN:
		if (g_screen->message_end_game) {
			g_screen->is_stoped = 1;
		}
		else if (g_screen->current_player == 2 || g_screen->message_save) {
			return;
		}
		else if (!g_screen->is_letter_placed) {
			printf("ENTER PRESSED\n");
			g_screen->input_switch = 1;
		}
		else {
			g_screen->confirm_selection = 1;
		}
		break;
	case SDLK_SPACE:
		//if its computeer`s turn we don`t do anuthing
		if (g_screen->current_player == 2) {
			return;
		}
		if (g_screen->is_letter_placed && g_screen->is_space_pressed == 0) {
			g_screen->is_space_pressed = 1;
			g_screen->starting_selection = 1;
		}
		break;
	case SDLK_ESCAPE:
		//ТУТ НУЖНО СДЕЛАТЬ АЛЕРТ О ВЫХОДЕ, ТАКЖЕ НУЖНО ЗАМОРОЗИТЬ ИИ КАКИМ-ТО ОБРАЗОМ
		if (g_screen->current_player == 2) {
			return;
		}
		else if (g_screen->is_letter_placed) {
			g_screen->delete_letter = 1;
		}
		else if (g_screen->text_input) {
			g_screen->letter = '\0';
			g_screen->input_switch = 1;
		}
		//and also there should be end game (do you want to end game? do you want to save it?)
		break;
	case SDLK_LEFT:
		if (!g_screen->is_letter_placed) {
			g_screen->btn_left.is_clicked = 1;
		}
		break;
	case SDLK_RIGHT:
		if (!g_screen->is_letter_placed) {
			g_screen->btn_right.is_clicked = 1;
		}
		break;
	case SDLK_UP:
		if (!g_screen->is_letter_placed) {
			g_screen->btn_up.is_clicked = 1;
		}
		break;
	case SDLK_DOWN:
		if (!g_screen->is_letter_placed) {
			g_screen->btn_down.is_clicked = 1;
		}
		break;
	}
}


static void event_game_keyup(SDL_Event e, GameScreen* g_screen) {
	//if its computeer`s turn we don`t do anuthing
	if (g_screen->current_player == 2) {
		return;
	}
	if (e.key.keysym.sym == SDLK_SPACE) {
		if (g_screen->is_letter_placed) {
			g_screen->is_space_pressed = 0;
		}
	}
}


static void event_game_text_input(SDL_Event e, GameScreen* g_screen) {
	//receive username
	if (g_screen->message_ask_for_username || g_screen->message_save){
		char cp1251 = '\0';
		letter_utf8_to_cp1251(e.text.text, &cp1251);
		if (is_eng_letter_or_digit(cp1251)) {
			g_screen->input_field.new_letter = cp1251;
			g_screen->input_field.is_there_new_letter = 1;
		}
	}
	//letter selection
	else if (is_it_ru_utf8_letter(e.text.text) && g_screen->text_input) {
		unsigned char cp1251_lett = '\0';
		letter_utf8_to_cp1251(e.text.text, &cp1251_lett);
		g_screen->letter = to_lower(cp1251_lett);
		g_screen->new_letter = 1;
	}
}


static void event_game_mousemotion(SDL_Event e, GameScreen* g_screen) {
	//words areas scrolling check first
	check_word_area_hovered(e, &g_screen->user_words);
	check_word_area_hovered(e, &g_screen->computer_words);

	if ((g_screen->message_save || g_screen->message_ask_for_username) && !g_screen->input_field.is_active) {
		check_field_hovered(e, &g_screen->input_field);
	}
	else if (g_screen->message_additional_time || g_screen->message_quit_confirm) {
		check_button_hovered(e, &g_screen->btn_message_yes);
		check_button_hovered(e, &g_screen->btn_message_no);
	}
	else if (g_screen->message_invalid_word) {
		check_button_hovered(e, &g_screen->btn_message_yes);
		check_button_hovered(e, &g_screen->btn_message_no);
	}
	else if (g_screen->is_cursor_active) {
		check_button_hovered(e, &g_screen->btn_up);
		check_button_hovered(e, &g_screen->btn_down);
		check_button_hovered(e, &g_screen->btn_left);
		check_button_hovered(e, &g_screen->btn_right);
	}
	//it can be 1 only if g_screen->is_letter_placed == 1
	else if (g_screen->is_space_pressed) {
		for (int y = 0; y < FIELD_SIZE; y++) {
			for (int x = 0; x < FIELD_SIZE; x++) {
				if (check_cell_selected(e, &g_screen->grid[y][x])) {
					g_screen->new_selected_cell_x = x;
					g_screen->new_selected_cell_y = y;
					return;
				}
			}
		}
	}
	//if no one pop-up message is active we can handle exit and save buttons hover
	if (!g_screen->message_end_game && !g_screen->message_quit_confirm && !g_screen->message_save) {
		check_button_hovered(e, &g_screen->btn_exit);
		check_button_hovered(e, &g_screen->btn_save);
	}
}

static void event_game_mouseclick(SDL_Event e, GameScreen* g_screen) {
	if ((g_screen->message_save || g_screen->message_ask_for_username) && g_screen->input_field.is_hovered) {
		g_screen->input_field.is_hovered = 0;
		g_screen->input_field.is_clicked = 1;
	}

	//message button
	else if (g_screen->btn_message_yes.is_hovered) {
		g_screen->btn_message_yes.is_hovered = 0;
		g_screen->btn_message_yes.is_clicked = 1;
	}
	else if (g_screen->btn_message_no.is_hovered) {
		g_screen->btn_message_no.is_hovered = 0;
		g_screen->btn_message_no.is_clicked = 1;
	}

	else if (g_screen->btn_exit.is_hovered) {
		g_screen->btn_exit.is_hovered = 0;
		g_screen->btn_exit.is_clicked = 1;
	}
	else if (g_screen->btn_save.is_hovered) {
		g_screen->btn_save.is_hovered = 0;
		g_screen->btn_save.is_clicked = 1;
	}

	//if its computer`s turn we don`t handle any buttons and etc. except messages
	if (g_screen->current_player == 2) {
		return;
	}

	//control keys
	else if (g_screen->btn_up.is_hovered) {
		g_screen->btn_up.is_clicked = 1;
	}
	else if (g_screen->btn_down.is_hovered) {
		g_screen->btn_down.is_clicked = 1;
	}
	else if (g_screen->btn_left.is_hovered) {
		g_screen->btn_left.is_clicked = 1;
	}
	else if (g_screen->btn_right.is_hovered) {
		g_screen->btn_right.is_clicked = 1;
	}
}

static void event_game_mousewheel(SDL_Event e, GameScreen* g_screen) {
	//In SDL y = 0 is a top of the page, so you'd expec tha scrolling wheel handling events to be the same... No, actually WHEEL UP - add_scroll = +1, DOWN - -1, i think that's illogical, so I decide to invert it
	if (g_screen->user_words.is_hovered) {
		g_screen->user_scroll = -e.wheel.y;
		printf("%d", -e.wheel.y);
	}
	else if (g_screen->computer_words.is_hovered) {
		g_screen->comp_scroll = -e.wheel.y;
		printf("%d", -e.wheel.y);
	}
}


StatusCode ui_handle_events(SDL_Renderer* render, ScreenContext* context, 
	MainScreen* main_screen, 
	SettingsScreen* sett_screen,
	LeaderboardScreen* lb_screen,
	GameScreen* g_screen) {
	SDL_Event e;
	while (SDL_PollEvent(&e)) {
		if (e.type == SDL_QUIT) {
			return UI_QUIT;
		}
		else {
			switch (context->current_screen) {
			case SCREEN_MAIN:
				if (e.type == SDL_MOUSEMOTION) {
					event_mainmenu_mousemotion(e, main_screen);
				}
				else if (e.type == SDL_MOUSEBUTTONDOWN) {
					printf("click\n");
					event_mainmenu_mouseclick_down(e, main_screen);
				}
				else if (e.type == SDL_KEYDOWN) {
					event_mainmenu_keydown(e, main_screen);
				}
				else if (e.type == SDL_TEXTINPUT) {
					event_mainmenu_text_input(e, main_screen);
				}
				break;
			case SCREEN_SETTINGS:
				if (e.type == SDL_MOUSEMOTION) {
					event_settings_mousemotion(e, sett_screen);
				}
				else if (e.type == SDL_MOUSEBUTTONDOWN) {
					printf("click\n");
					event_settings_mouseclick(e, sett_screen);
				}
				else if (e.type == SDL_KEYDOWN) {
					event_settings_keydown(e, sett_screen);
				}
				else if (e.type == SDL_TEXTINPUT) {
					event_settings_text_input(e, sett_screen);
				}

				break;
			case SCREEN_LEADERBOARD:
				if (e.type == SDL_MOUSEMOTION) {
					event_lb_mousemotion(e, lb_screen);
				}
				else if (e.type == SDL_MOUSEBUTTONDOWN) {
					printf("click\n");
					event_lb_mouseclick(e, lb_screen);
				}
				break;
			case SCREEN_GAME:
				if (e.type == SDL_KEYDOWN) {
					event_game_keydown(e, g_screen);
				}
				else if (e.type == SDL_KEYUP) {
					event_game_keyup(e, g_screen);
				}
				else if (e.type == SDL_MOUSEMOTION) {
					event_game_mousemotion(e, g_screen);
				}
				else if (e.type == SDL_MOUSEBUTTONDOWN) {
					event_game_mouseclick(e, g_screen);
				}
				else if (e.type == SDL_MOUSEWHEEL) {
					event_game_mousewheel(e, g_screen);
				}
				else if (e.type == SDL_TEXTINPUT) {
					event_game_text_input(e, g_screen);
				}
				break;
			}
		}
	}

	return SUCCESS;
}



//-----------------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------UPDATE LOGIC--------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------------------------------------------

static void check_ai_state_updates(SDL_Renderer* renderer, ScreenContext* context, Game* game, Leaderboard* lb, AIState* state, GameScreen* g_screen) {
	StatusCode code;
	if (ai_gave_up(state)) {
		fprintf(stderr, "gave up\n");
		g_screen->message_ask_for_username = 1;

	}
	else if (ai_need_additional_time(state) && !g_screen->message_additional_time) {
		fprintf(stderr, "NEED TIME\n");
		int reply = 0;
		g_screen->message_additional_time = 1;
	}
	else if (ai_word_found(state)) {
		fprintf(stderr, "WORD\n");
		char ai_word[MAX_WORD_LEN + 1];

		EnterCriticalSection(ai_get_cs(state));
		code = game_apply_generated_move(game, *ai_get_move(state));
		game_get_word_from_move(*ai_get_move(state), ai_word);
		LeaveCriticalSection(ai_get_cs(state));

		if (code == SUCCESS) {
			ai_set_stop(state);
			update_game_screen(renderer, context, g_screen, game);
			update_words_areas(renderer, game, g_screen, context);
			g_screen->current_player = 1;
			ui_clear_word_selection(g_screen);
			g_screen->letter = '\0';
			g_screen->is_letter_placed = 0;
			g_screen->is_cursor_active = 1;
		}
		else {
			fprintf(stderr, "Ошибка при получении слова от компьютера\n");
		}
	}
}

static void ui_update_logic_main(
	SDL_Renderer* renderer,
	ScreenContext* context, 
	MainScreen* main_screen,
	LeaderboardScreen* lb_screen, 
	GameScreen* g_screen,
	Game** game,
	Dictionary* dict, 
	GameSettings* settings,
	Leaderboard* lb, 
	bool* f) 
{
	StatusCode code;

	if (main_screen->btn_new_game.is_clicked) {
		main_screen->btn_new_game.is_clicked = 0;
		*game = game_create(settings, dict);
		if (*game == NULL) {
			fprintf(stderr, "Ошибка при создании игры!\n");
		}
		load_game_screen(renderer, context, g_screen, *game, settings);
		context->current_screen = SCREEN_GAME;
	}
	else if (main_screen->btn_load_game.is_clicked) {
		main_screen->message_filename = 1;
		main_screen->btn_load_game.is_clicked = 0;
	}
	else if (main_screen->btn_to_settings.is_clicked) {
		main_screen->btn_to_settings.is_clicked = 0;
		context->current_screen = SCREEN_SETTINGS;
	}
	else if (main_screen->btn_to_leaderboard.is_clicked) {
		main_screen->btn_to_leaderboard.is_clicked = 0;
		load_leaderboard(renderer, *context, lb_screen, lb);
		context->current_screen = SCREEN_LEADERBOARD;
	}
	else if (main_screen->btn_exit.is_clicked) {
		main_screen->btn_exit.is_clicked = 0;
		*f = false;
	}

	//input field
	else if (main_screen->message_filename) {
		if (main_screen->filename_field.is_clicked) {
			//activate field
			if (!main_screen->filename_field.is_active) {
				main_screen->filename_field.is_active = 1;
				SDL_StartTextInput();
			}
			//disable the field if it`s already active, check the value, load game and change screen context
			else {
				if (main_screen->filename_field.text[0] != '\0') {
					*game = game_load(dict, main_screen->filename_field.text);
					if (*game != NULL) {
						load_game_screen(renderer, context, g_screen, *game, settings);
						context->current_screen = SCREEN_GAME;
					}
					else {
						//alert about invalid filename
						main_screen->message_invalid_input = 1;
					}
				}
				main_screen->filename_field.text[0] = '\0';
				main_screen->filename_field.is_active = 0;
				main_screen->filename_field.cursorPos = 0;
				main_screen->message_filename = 0;
				SDL_StopTextInput();
			}
			main_screen->filename_field.is_clicked = 0;
		}
		else if (main_screen->filename_field.is_there_new_letter) {
			InputField* field = &main_screen->filename_field;
			if (strlen(main_screen->filename_field.text) < INPUT_FIELD_LIMIT) {
				char buffer[MAX_UI_BUFFER_SIZE] = { 0 };
				memcpy(buffer, field->text, field->cursorPos);
				memcpy(buffer + field->cursorPos, &field->new_letter, 1);
				memcpy(buffer + field->cursorPos + 1, field->text + field->cursorPos, strlen(field->text + field->cursorPos) + 1);

				memcpy(field->text, buffer, MAX_UI_BUFFER_SIZE);
				field->cursorPos++;
			}
			main_screen->filename_field.is_there_new_letter = 0;
		}
	}
	//close message about invalid filename
	else if (main_screen->message_invalid_input && main_screen->close_message) {
		main_screen->message_invalid_input = 0;
		main_screen->close_message = 0;
	}
}


static void ui_update_logic_settings(ScreenContext* context, SettingsScreen* sett_screen, GameSettings* settings) {
	StatusCode code;
	if (sett_screen->btn_to_main.is_clicked) {
		sett_screen->btn_to_main.is_clicked = 0;
		context->current_screen = SCREEN_MAIN;
	}
	else if (sett_screen->btn_easy.is_clicked) {
		sett_screen->btn_easy.is_clicked = 0;
		code = game_set_difficulty(settings, 0);
		if (code != SUCCESS) fprintf(stderr, "err code - %d\n", code);
	}
	else if (sett_screen->btn_mid.is_clicked) {
		sett_screen->btn_mid.is_clicked = 0;
		printf("TES\n");
		code = game_set_difficulty(settings, 1);
		if (code != SUCCESS) fprintf(stderr, "err code - %d\n", code);
	}
	else if (sett_screen->btn_hard.is_clicked) {
		sett_screen->btn_hard.is_clicked = 0;
		code = game_set_difficulty(settings, 2);
		if (code != SUCCESS) fprintf(stderr, "err code - %d\n", code);
	}

	else if (sett_screen->timelimit_field.is_clicked) {
		if (!sett_screen->timelimit_field.is_active) {
			sett_screen->timelimit_field.is_active = 1;
			SDL_StartTextInput();
		}
		else {
			sett_screen->timelimit_field.is_active = 0;
			//apply new value or restore previous if new is invalid
			int new_timelimit = atoi(sett_screen->timelimit_field.text);
			if (new_timelimit < 10) {
				sett_screen->timelimit_field.text[0] = '1';
				sett_screen->timelimit_field.text[1] = '0';
				sett_screen->timelimit_field.cursorPos = 2;
				new_timelimit = 10;
			}
			if (new_timelimit > 60000) {
				sett_screen->timelimit_field.text[0] = '6';
				sett_screen->timelimit_field.text[1] = '0';
				sett_screen->timelimit_field.text[2] = '0';
				sett_screen->timelimit_field.text[3] = '0';
				sett_screen->timelimit_field.text[4] = '0';
				sett_screen->timelimit_field.cursorPos = 5;
				new_timelimit = 60;
			}
			game_set_timelimit(settings, new_timelimit);
			SDL_StopTextInput();
		}
		sett_screen->timelimit_field.is_clicked = 0;
	}
	else if (sett_screen->timelimit_field.is_there_new_letter) {
		InputField* field = &sett_screen->timelimit_field;
		if (strlen(sett_screen->timelimit_field.text) < 5) {
			char buffer[MAX_UI_BUFFER_SIZE] = { 0 };
			memcpy(buffer, field->text, field->cursorPos);
			memcpy(buffer + field->cursorPos, &field->new_letter, 1);
			memcpy(buffer + field->cursorPos + 1, field->text + field->cursorPos, strlen(field->text + field->cursorPos) + 1);

			memcpy(field->text, buffer, MAX_UI_BUFFER_SIZE);
			field->cursorPos++;
		}
		sett_screen->timelimit_field.is_there_new_letter = 0;
	}
}

static void ui_update_logic_leaderboard(ScreenContext* context, LeaderboardScreen* lb_screen) {
	if (lb_screen->btn_back.is_clicked) {
		lb_screen->btn_back.is_clicked = 0;
		context->current_screen = SCREEN_MAIN;
	}
}

//if it`s compute`s turn, update percentage and check do it complete computations
static void update_computer_turn(SDL_Renderer* renderer, 
	Game* game, 
	AIState* state, 
	Leaderboard* lb, 
	ScreenContext* context, 
	GameScreen* g_screen) 
{
	check_ai_state_updates(renderer, context, game, lb, state, g_screen);
	if (g_screen->message_additional_time) {
		if (g_screen->btn_message_yes.is_clicked) {
			//give it a very big amount of time
			ai_give_additional_time(state, true, 100000); //ms
			SetEvent(ai_get_handle(state));

			g_screen->message_additional_time = 0;
		}
		else if (g_screen->btn_message_no.is_clicked) {
			ai_give_additional_time(state, false, 0);
			ai_set_stop(state);
			SetEvent(ai_get_handle(state));

			g_screen->message_ask_for_username = 1;

			g_screen->message_additional_time = 0;
		}
	}
	
	unsigned int new_percent = (unsigned int)ai_get_percentage(state);
	if (new_percent != g_screen->percent) {
		update_percent(renderer, g_screen, context, new_percent);
		//fprintf(stderr, "percent - %d\n", new_percent);
	}
}


static void ui_update_logic_game(SDL_Renderer* renderer, 
	ScreenContext* context, 
	GameScreen* g_screen, 
	Game** game, 
	Dictionary* dict, 
	Leaderboard* lb, 
	AIState* state) 
{
	StatusCode code;

	//END OF THE GAME
	if (g_screen->message_end_game && g_screen->is_stoped) {
		g_screen->message_end_game = 0;
		g_screen->is_stoped = 0;
		destroy_game_screen(g_screen, *game);
		game_destroy(game);
		context->current_screen = SCREEN_MAIN;
	}

	//EXIT BUTTON 
	else if (g_screen->btn_exit.is_clicked) {
		g_screen->message_quit_confirm = 1;
		g_screen->btn_exit.is_clicked = 0;
	}
	//SAVE BUTTON
	else if (g_screen->btn_save.is_clicked) {
		g_screen->message_save = 1;
		g_screen->btn_save.is_clicked = 0;
	}

	//SCROLLING IN WORDS AREAS
	//user
	else if (g_screen->user_scroll != 0 && g_screen->user_words.words_count > g_screen->page_limit) {
		int add_scroll = g_screen->user_scroll;
		//down
		if (g_screen->user_words.scroll + g_screen->page_limit < g_screen->user_words.words_count && add_scroll > 0) {
			g_screen->user_words.scroll += add_scroll;
			//scroll value can be more than 1 (when user scrolls very quickly) and if new scroll value + page_limit (number of last word on page) bigger than words_count, we just set max possible value 
			if (g_screen->user_words.scroll + g_screen->page_limit > g_screen->user_words.words_count) {
				g_screen->user_words.scroll = g_screen->user_words.words_count - g_screen->page_limit;
			}
		}
		//up
		if (g_screen->user_words.scroll > 0  && add_scroll < 0) {
			g_screen->user_words.scroll += add_scroll;
			if (g_screen->user_words.scroll < 0) g_screen->user_words.scroll = 0;
		}
		g_screen->user_scroll = 0;
	}
	//comp
	else if (g_screen->comp_scroll != 0 && g_screen->computer_words.words_count > g_screen->page_limit) {
		int add_scroll = g_screen->comp_scroll;
		//down
		if (g_screen->computer_words.scroll + g_screen->page_limit < g_screen->computer_words.words_count && add_scroll > 0) {
			g_screen->computer_words.scroll += add_scroll;
			//scroll value can be more than 1 (when user scrolls very quickly) and if new scroll value + page_limit (number of last word on page) bigger than words_count, we just set max possible value 
			if (g_screen->computer_words.scroll + g_screen->page_limit > g_screen->computer_words.words_count) {
				g_screen->computer_words.scroll = g_screen->computer_words.words_count - g_screen->page_limit;
			}
		}
		//up
		if (g_screen->computer_words.scroll > 0 && add_scroll < 0) {
			g_screen->computer_words.scroll += add_scroll;
			if (g_screen->computer_words.scroll < 0) g_screen->computer_words.scroll = 0;
		}
		g_screen->comp_scroll = 0;
	}

	//MESSAGE QUIT CONFIRM
	else if (g_screen->message_quit_confirm) {
		if (g_screen->btn_message_yes.is_clicked) {
			g_screen->message_quit_confirm = 0;
			
			//check is user`s score higher than computer`s, if its true - ask his name, if its not - just show end game_message
			int id = 2;
			code = game_get_winner(*game, &id);
			if (code != SUCCESS) fprintf(stderr, "Error code in MESSAGE QUIT CONFIRM - %d\n", code);
			if (id == 1) {
				g_screen->message_ask_for_username = 1;
			}
			else {
				g_screen->message_end_game = 1;
			}
			g_screen->btn_message_yes.is_clicked = 0;
		}
		else if (g_screen->btn_message_no.is_clicked) {
			g_screen->message_quit_confirm = 0;
			g_screen->btn_message_no.is_clicked = 0;
		}
	}

	//MESSAGE SAVE
	else if (g_screen->message_save) {
		if (g_screen->input_field.is_clicked) {
			if (!g_screen->input_field.is_active) {
				g_screen->input_field.is_active = 1;
				SDL_StartTextInput();
			}
			else {
				if (g_screen->input_field.text[0] != '\0') {
					code = game_save(*game, g_screen->input_field.text);
					if (code != SUCCESS) fprintf(stderr, "Error! Code returned by game_save() - %d\n", code);
				}
				else {
					printf("Field is empty!\n");
				}
				g_screen->message_save = 0;
				g_screen->input_field.is_active = 0;
				SDL_StopTextInput();
			}
			g_screen->input_field.is_clicked = 0;
		}
		else if (g_screen->input_field.is_there_new_letter) {
			InputField* field = &g_screen->input_field;
			if (strlen(g_screen->input_field.text) < MAX_PATH_LEN) {
				char buffer[MAX_UI_BUFFER_SIZE] = { 0 };
				memcpy(buffer, field->text, field->cursorPos);
				memcpy(buffer + field->cursorPos, &field->new_letter, 1);
				memcpy(buffer + field->cursorPos + 1, field->text + field->cursorPos, strlen(field->text + field->cursorPos) + 1);

				memcpy(field->text, buffer, MAX_UI_BUFFER_SIZE);
				field->cursorPos++;
			}
			g_screen->input_field.is_there_new_letter = 0;
		}
	}

	//MESSAGE ASK FOR USERNAME
	if (g_screen->message_ask_for_username) {
		//username window
		if (g_screen->input_field.is_clicked) {
			printf("CLICKED!!!!\n");
			if (!g_screen->input_field.is_active) {
				g_screen->input_field.is_active = 1;
				SDL_StartTextInput();
				g_screen->input_field.is_clicked = 0;
			}
			else {
				//add user into leaderboard
				if (g_screen->input_field.text[0] != '\0') {
					code = game_add_into_leaderboard(lb, *game, g_screen->input_field.text);
					if (code != SUCCESS) {
						fprintf(stderr, "Failed to add user into lb, code - %d\n", code);
					}
				}
				g_screen->message_ask_for_username = 0;
				g_screen->message_end_game = 1;
				SDL_StopTextInput();
			}
		}
		else if (g_screen->input_field.is_there_new_letter) {
			InputField* field = &g_screen->input_field;
			if (strlen(g_screen->input_field.text) < INPUT_FIELD_LIMIT) {
				char buffer[MAX_UI_BUFFER_SIZE] = { 0 };
				memcpy(buffer, field->text, field->cursorPos);
				memcpy(buffer + field->cursorPos, &field->new_letter, 1);
				memcpy(buffer + field->cursorPos + 1, field->text + field->cursorPos, strlen(field->text + field->cursorPos) + 1);

				memcpy(field->text, buffer, MAX_UI_BUFFER_SIZE);
				field->cursorPos++;
			}
			g_screen->input_field.is_there_new_letter = 0;
		}
	}

	//COMPUTER TURN
	else if (g_screen->current_player == 2) {
		update_computer_turn(renderer, *game, state, lb, context, g_screen);
	}

	//All this conditions will not met if it`s a compute`s turn (current_player == 2), because no one button will be marked as clicked and no one key will be marked as pressed
	//MESSAGE BUTTONS
	else if (g_screen->message_invalid_word) {
		if (g_screen->btn_message_yes.is_clicked) {
			char word[MAX_WORD_LEN];
			game_get_word(*game, word);
			dict_add_word(dict, word);
			code = game_confirm_move(*game, dict);
			g_screen->btn_message_yes.is_clicked = 0;
			g_screen->message_invalid_word = 0;

			if (code == SUCCESS) {
				//start ai turn
				g_screen->current_player = 2;
				ai_start_turn(*game, state, dict);
			}
			
		}
		//if user dont want to add incorrect word into dictionary - clear move
		else if (g_screen->btn_message_no.is_clicked) {
			game_clear_move(*game);
			ui_clear_word_selection(g_screen);
			SDL_DestroyTexture(g_screen->grid[g_screen->cursor_y][g_screen->cursor_x].texture);
			g_screen->grid[g_screen->cursor_y][g_screen->cursor_x].texture = NULL;
			g_screen->grid[g_screen->cursor_y][g_screen->cursor_x].is_cursored = 1;
			g_screen->is_cursor_active = 1;
			g_screen->is_letter_placed = 0;
			g_screen->btn_message_yes.is_clicked = 0;
			g_screen->message_invalid_word = 0;
			printf("слово отклонено\n");
		}
	}

	//enter pressed (and if letter is not already placed)
	else if (g_screen->input_switch) {
		if (!g_screen->text_input) {
			if (is_cell_empty(game_get_field(*game), g_screen->cursor_y, g_screen->cursor_x) && is_letter_near(game_get_field(*game), g_screen->cursor_y, g_screen->cursor_x)) {
				printf("start input\n");
				g_screen->is_cursor_active = 0;

				g_screen->btn_up.is_hovered = 0;
				g_screen->btn_down.is_hovered = 0;
				g_screen->btn_left.is_hovered = 0;
				g_screen->btn_right.is_hovered = 0;

				g_screen->text_input = 1;
				SDL_StartTextInput();
			}
			else {
				printf("сюда нельзя поставить букву!\n");
			}
		}
		else {
			if (g_screen->letter == '\0') {
				printf("end input\n");
				g_screen->text_input = 0;

				//in case user pressed ESCAPE while selecting the letter
				if (g_screen->grid[g_screen->cursor_y][g_screen->cursor_x].texture != NULL) {
					SDL_DestroyTexture(g_screen->grid[g_screen->cursor_y][g_screen->cursor_x].texture);
					g_screen->grid[g_screen->cursor_y][g_screen->cursor_x].texture = NULL;
				}
				g_screen->is_cursor_active = 1;
				SDL_StopTextInput();
			}
			else {
				code = game_try_place_letter(*game, g_screen->cursor_y, g_screen->cursor_x, g_screen->letter);
				if (code == SUCCESS) {
					g_screen->is_letter_placed = 1;
					//hide cursor highlighting
					g_screen->grid[g_screen->cursor_y][g_screen->cursor_x].is_cursored = 0;
					g_screen->is_cursor_active = 0;
					g_screen->text_input = 0;
					SDL_StopTextInput();
				}
				else {
					g_screen->letter == '\0';
					g_screen->is_cursor_active = 1;
					//delete letter
					char t[2] = { g_screen->letter , '\0' };
					SDL_DestroyTexture(g_screen->grid[g_screen->cursor_y][g_screen->cursor_x].texture);
					g_screen->grid[g_screen->cursor_y][g_screen->cursor_x].texture = createTextureFromText(renderer, context->cell_font, t);

					SDL_StopTextInput();
				}
			}
		}
		g_screen->input_switch = 0;
	}
	//update letter
	else if (g_screen->new_letter) {
		//add it only into UI grid
		char t[2] = { g_screen->letter , '\0' };
		SDL_DestroyTexture(g_screen->grid[g_screen->cursor_y][g_screen->cursor_x].texture);
		g_screen->grid[g_screen->cursor_y][g_screen->cursor_x].texture = createTextureFromText(renderer, context->cell_font, t);
		g_screen->new_letter = 0;
	}
	// delete letter + clear word selection
	else if (g_screen->delete_letter) {
		game_clear_move(*game);
		ui_clear_word_selection(g_screen);
		SDL_DestroyTexture(g_screen->grid[g_screen->cursor_y][g_screen->cursor_x].texture);
		g_screen->grid[g_screen->cursor_y][g_screen->cursor_x].texture = NULL;
		g_screen->grid[g_screen->cursor_y][g_screen->cursor_x].is_cursored = 1;
		g_screen->is_cursor_active = 1;
		g_screen->is_letter_placed = 0;

		g_screen->delete_letter = 0;
	}

	// space was pressed, clear selection
	else if (g_screen->starting_selection) {
		game_cancel_word_selection(*game);
		ui_clear_word_selection(g_screen);
		g_screen->starting_selection = 0;
	}
	//if there is new selected cell, add it into Move 
	else if (g_screen->new_selected_cell_x != -1) {
  		code = game_add_cell_into_word(*game, g_screen->new_selected_cell_y, g_screen->new_selected_cell_x);
		//if this cell isn`t connected with previous and if it`s already selected
		if (code == FIELD_CELL_NOT_CONNECTED) {
			g_screen->grid[g_screen->new_selected_cell_y][g_screen->new_selected_cell_x].is_selected = 0;
		}

		g_screen->new_selected_cell_x = -1;
		g_screen->new_selected_cell_y = -1;
	}

	//confirm (ENTER)
	else if (g_screen->confirm_selection) {
		code = game_confirm_move(*game, dict);

		switch (code) {
		case GAME_INVALID_WORD:
			int q = 0;
			//алерт о том, что такого слова нет, предлагаем алертом добавить в словарь
			g_screen->message_invalid_word = 1;
			break;
		case GAME_WORD_USED:
			int qw = 0;
			//алерт о том, что слово использовано
			break;
		case GAME_WORD_DOESNT_CONTAIN_LETTER:
			int qe = 0;
			//алерт о том, что нет буквы в ее составе
			break;
		case SUCCESS:
			//start ai turn
			update_words_areas(renderer, *game, g_screen, context);
			g_screen->current_player = 2;
			ai_start_turn(*game, state, dict);
			break;
		
		}
		g_screen->confirm_selection = 0;
	}

	//cursor
	else if (g_screen->btn_up.is_clicked && g_screen->cursor_y > 0) {
		g_screen->btn_up.is_clicked = 0;
		g_screen->grid[g_screen->cursor_y][g_screen->cursor_x].is_cursored = 0;
		g_screen->cursor_y--;
		g_screen->grid[g_screen->cursor_y][g_screen->cursor_x].is_cursored = 1;
	}
	else if (g_screen->btn_down.is_clicked && g_screen->cursor_y < FIELD_SIZE - 1) {
		g_screen->btn_down.is_clicked = 0;
		g_screen->grid[g_screen->cursor_y][g_screen->cursor_x].is_cursored = 0;
		g_screen->cursor_y++;
		g_screen->grid[g_screen->cursor_y][g_screen->cursor_x].is_cursored = 1;
	}
	else if (g_screen->btn_left.is_clicked && g_screen->cursor_x > 0) {
		g_screen->btn_left.is_clicked = 0;
		g_screen->grid[g_screen->cursor_y][g_screen->cursor_x].is_cursored = 0;
		g_screen->cursor_x--;
		g_screen->grid[g_screen->cursor_y][g_screen->cursor_x].is_cursored = 1;
	}
	else if (g_screen->btn_right.is_clicked && g_screen->cursor_x < FIELD_SIZE - 1) {
		g_screen->btn_right.is_clicked = 0;
		g_screen->grid[g_screen->cursor_y][g_screen->cursor_x].is_cursored = 0;
		g_screen->cursor_x++;
		g_screen->grid[g_screen->cursor_y][g_screen->cursor_x].is_cursored = 1;
	}
}

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
	bool* f) 
{
	switch (context->current_screen) {
	case SCREEN_MAIN:
		ui_update_logic_main(renderer, context, main_screen, lb_screen, g_screen, game, dict, settings, lb, f);
		break;
	case SCREEN_SETTINGS:
		ui_update_logic_settings(context, sett_screen, settings);
		break;
	case SCREEN_LEADERBOARD:
		ui_update_logic_leaderboard(context, lb_screen);
		break;
	case SCREEN_GAME:
		ui_update_logic_game(renderer, context, g_screen, game, dict, lb, state);
		break;
	}
}



//-----------------------------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------RENDER-----------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------------------------------------------

static void render_button(SDL_Renderer* renderer, Button* btn) {
	if ((btn->is_hovered && btn->is_hoverable)|| btn->is_active)
		SDL_SetRenderDrawColor(renderer, GRAY_HOVERED);
	else
		SDL_SetRenderDrawColor(renderer, GRAY);

	SDL_RenderFillRect(renderer, &btn->rect);
	SDL_SetRenderDrawColor(renderer, BLUE);
	SDL_RenderDrawRect(renderer, &btn->rect);

	if (btn->texture != NULL) {
		SDL_Rect text_rect = btn->rect;
		SDL_QueryTexture(btn->texture, NULL, NULL, &text_rect.w, &text_rect.h);
		text_rect.y = btn->rect.y + (btn->rect.h - text_rect.h) / 2;
		text_rect.x = btn->rect.x + (btn->rect.w - text_rect.w) / 2;
		SDL_RenderCopy(renderer, btn->texture, NULL, &text_rect);
	}
}

void render_input_field(SDL_Renderer* renderer, InputField* field, ScreenContext* context) {
	SDL_SetRenderDrawColor(renderer, GRAY_HOVERED);
	SDL_RenderFillRect(renderer, &field->rect);

	SDL_SetRenderDrawColor(renderer, BLACK);
	SDL_RenderDrawRect(renderer, &field->rect);

	if (strlen(field->text) > 0) {
		SDL_Texture* textTexture = createTextureFromText(renderer, context->input_field_font, field->text);
		int textureW, textureH;
		SDL_QueryTexture(textTexture, NULL, NULL, &textureW, &textureH);

		SDL_Rect text_rect = {
			field->rect.x + 5,
			field->rect.y + (field->rect.h - textureH) / 2,
			textureW,
			textureH,
		};
		SDL_RenderCopy(renderer, textTexture, NULL, &text_rect);

		SDL_DestroyTexture(textTexture);
	}

	//cursor
	if (field->is_active) {
		SDL_Rect cursor = {
			field->rect.x + 5 + get_cursor_padding(context->input_field_font, field->text, field->cursorPos),
			field->rect.y + 5,
			2,
			field->rect.h - 10
		};
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
		SDL_RenderFillRect(renderer, &cursor);
	}
}


void static ui_render_main(SDL_Renderer* renderer, ScreenContext* context, MainScreen* main_screen) {
	SDL_SetRenderDrawColor(renderer, WHITE);
	SDL_RenderClear(renderer);

	SDL_Rect text_rect;
	SDL_QueryTexture(main_screen->header, NULL, NULL, &text_rect.w, &text_rect.h);
	text_rect.x = SCREEN_WIDTH / 2 - text_rect.w / 2;
	text_rect.y = 50;
	SDL_RenderCopy(renderer, main_screen->header, NULL, &text_rect);

	render_button(renderer, &main_screen->btn_new_game);
	render_button(renderer, &main_screen->btn_load_game);
	render_button(renderer, &main_screen->btn_to_settings);
	render_button(renderer, &main_screen->btn_to_leaderboard);
	render_button(renderer, &main_screen->btn_exit);

	//messages
	if (main_screen->message_filename) {
		SDL_SetRenderDrawColor(renderer, WHITE);
		SDL_RenderFillRect(renderer, &main_screen->rect_message);
		SDL_SetRenderDrawColor(renderer, BLACK);
		SDL_RenderDrawRect(renderer, &main_screen->rect_message);

		SDL_Rect text_rect;
		SDL_QueryTexture(main_screen->texture_message_filename, NULL, NULL, &text_rect.w, &text_rect.h);
		text_rect.x = ALERT_X + ALERT_WIDTH / 2 - text_rect.w / 2;
		text_rect.y = ALERT_Y + 50;
		SDL_RenderCopy(renderer, main_screen->texture_message_filename, NULL, &text_rect);

		render_input_field(renderer, &main_screen->filename_field, context);
	}
	else if (main_screen->message_invalid_input) {
		SDL_SetRenderDrawColor(renderer, WHITE);
		SDL_RenderFillRect(renderer, &main_screen->rect_message);
		SDL_SetRenderDrawColor(renderer, BLACK);
		SDL_RenderDrawRect(renderer, &main_screen->rect_message);

		SDL_Rect text_rect;
		SDL_QueryTexture(main_screen->texture_message_invalid_input, NULL, NULL, &text_rect.w, &text_rect.h);
		text_rect.x = ALERT_X + ALERT_WIDTH / 2 - text_rect.w / 2;
		text_rect.y = ALERT_Y + 50;
		SDL_RenderCopy(renderer, main_screen->texture_message_invalid_input, NULL, &text_rect);
	}

	SDL_RenderPresent(renderer);
}


void ui_render_settings(SDL_Renderer* renderer, ScreenContext* context, SettingsScreen* settings_screen) {
	SDL_SetRenderDrawColor(renderer, WHITE);
	SDL_RenderClear(renderer);

	SDL_Rect text_rect;
	SDL_QueryTexture(settings_screen->header, NULL, NULL, &text_rect.w, &text_rect.h);
	text_rect.x = SCREEN_WIDTH / 2 - text_rect.w / 2;
	text_rect.y = 50;
	SDL_RenderCopy(renderer, settings_screen->header, NULL, &text_rect);

	render_button(renderer, &settings_screen->btn_to_main);

	render_button(renderer, &settings_screen->btn_easy);
	render_button(renderer, &settings_screen->btn_mid);
	render_button(renderer, &settings_screen->btn_hard);

	SDL_QueryTexture(settings_screen->first_player, NULL, NULL, &text_rect.w, &text_rect.h);
	text_rect.x = 270;
	text_rect.y = 277;
	SDL_RenderCopy(renderer, settings_screen->first_player, NULL, &text_rect);
	render_button(renderer, &settings_screen->btn_p1);
	render_button(renderer, &settings_screen->btn_p2);

	SDL_QueryTexture(settings_screen->time_limit, NULL, NULL, &text_rect.w, &text_rect.h);
	text_rect.x = 270;
	text_rect.y = 377;
	SDL_RenderCopy(renderer, settings_screen->time_limit, NULL, &text_rect);
	render_input_field(renderer, &settings_screen->timelimit_field, context);

	SDL_RenderPresent(renderer);
}

void ui_render_leaderboard(SDL_Renderer* renderer, LeaderboardScreen* lb_screen) {
	SDL_SetRenderDrawColor(renderer, WHITE);
	SDL_RenderClear(renderer);

	SDL_Rect text_rect;
	SDL_QueryTexture(lb_screen->header, NULL, NULL, &text_rect.w, &text_rect.h);
	text_rect.x = SCREEN_WIDTH / 2 - text_rect.w / 2;
	text_rect.y = 20;
	SDL_RenderCopy(renderer, lb_screen->header, NULL, &text_rect);

	render_button(renderer, &lb_screen->btn_back);

	//render rows and columns
	
	for (int i = 0; i < lb_screen->count_of_records; i++) {
		//num
		SDL_Rect rect = { 10, 80 + (i * 40), 40, 40};
		SDL_SetRenderDrawColor(renderer, BLACK);
		SDL_RenderDrawRect(renderer, &rect);

		SDL_Rect t_rect = rect;
		SDL_QueryTexture(lb_screen->nums[i], NULL, NULL, &t_rect.w, &t_rect.h);
		t_rect.y = rect.y + (rect.h - t_rect.h) / 2;
		t_rect.x = rect.x + (rect.w - t_rect.w) / 2;
		SDL_RenderCopy(renderer, lb_screen->nums[i], NULL, &t_rect);

		rect = (SDL_Rect) { 50, 80 + (i * 40), SCREEN_WIDTH / 2 - 40, 40 };
		SDL_SetRenderDrawColor(renderer, BLACK);
		SDL_RenderDrawRect(renderer, &rect);
		t_rect = rect;
		SDL_QueryTexture(lb_screen->users[i], NULL, NULL, &t_rect.w, &t_rect.h);
		t_rect.y = rect.y + (rect.h - t_rect.h) / 2;
		t_rect.x = rect.x + (rect.w - t_rect.w) / 2;
		SDL_RenderCopy(renderer, lb_screen->users[i], NULL, &t_rect);

		rect = (SDL_Rect){ 50 + (SCREEN_WIDTH / 2 - 40), 80 + (i * 40), SCREEN_WIDTH / 2 - 40, 40};
		SDL_SetRenderDrawColor(renderer, BLACK);
		SDL_RenderDrawRect(renderer, &rect);
		t_rect = rect;
		SDL_QueryTexture(lb_screen->scores[i], NULL, NULL, &t_rect.w, &t_rect.h);
		t_rect.y = rect.y + (rect.h - t_rect.h) / 2;
		t_rect.x = rect.x + (rect.w - t_rect.w) / 2;
		SDL_RenderCopy(renderer, lb_screen->scores[i], NULL, &t_rect);
	}

	SDL_RenderPresent(renderer);
}

//this function is not suitable for different field sizes, static array grid[][] should be replaced by dinamically allocated one,
// also its need to receive field_size param
static void render_field(SDL_Renderer* renderer, UICell grid[][FIELD_SIZE], bool text_input_on) {
	for (int j = 0; j < FIELD_SIZE; j++) {
		for (int i = 0; i < FIELD_SIZE; i++) {
			SDL_Rect cell = grid[j][i].rect;
			if (grid[j][i].is_selected) {
				SDL_SetRenderDrawColor(renderer, GRAY_HOVERED);
				SDL_RenderFillRect(renderer, &cell);
				SDL_SetRenderDrawColor(renderer, BLACK);
				SDL_RenderDrawRect(renderer, &cell);
			}
			else if (grid[j][i].is_cursored && text_input_on) {
				SDL_SetRenderDrawColor(renderer, GREEN);
				SDL_RenderDrawRect(renderer, &cell);
				SDL_Rect border = (SDL_Rect){ cell.x + 1, cell.y + 1, cell.w - 2, cell.h - 2 };
				SDL_SetRenderDrawColor(renderer, GREEN);
				SDL_RenderDrawRect(renderer, &border);
			}
			else if (grid[j][i].is_cursored) {
				SDL_SetRenderDrawColor(renderer, YELLOW);
				SDL_RenderDrawRect(renderer, &cell);
				SDL_Rect border = (SDL_Rect){cell.x + 1, cell.y + 1, cell.w - 2, cell.h - 2};
				SDL_SetRenderDrawColor(renderer, YELLOW);
				SDL_RenderDrawRect(renderer, &border);
			}
			else {
				SDL_SetRenderDrawColor(renderer, BLACK);
				SDL_RenderDrawRect(renderer, &cell);
			}		
			//render texture if this cell isn`t empty
			if (grid[j][i].texture != NULL) {
				SDL_Rect cell_letter;
				SDL_QueryTexture(grid[j][i].texture, NULL, NULL, &cell_letter.w, &cell_letter.h);
				cell_letter.x = cell.x + cell.w / 2 - cell_letter.w / 2;
				cell_letter.y = cell.y + cell.h / 2 - cell_letter.h / 2;
				SDL_RenderCopy(renderer, grid[j][i].texture, NULL,&cell_letter);
			}
		}
	}
}

static void render_words_area(SDL_Renderer* renderer, ScreenContext* context, GameScreen* g_screen) {
	//user
	SDL_RenderDrawRect(renderer, &g_screen->user_words.rect);

	SDL_Rect dst_rect = g_screen->user_words.rect;
	dst_rect.y += WORDS_AREA_PADDING;
	dst_rect.h -= WORDS_AREA_PADDING * 2;
	SDL_Rect src_rect = { 0, 0, 0, 0 };
	src_rect.y = g_screen->user_words.scroll * (context->text_font_height + WORDS_AREA_TEXT_INRERVAL) - WORDS_AREA_TEXT_INRERVAL / 2;
	src_rect.w = WORDS_AREA_WIDTH;
	src_rect.h = (context->text_font_height + WORDS_AREA_TEXT_INRERVAL) * g_screen->page_limit;
	SDL_RenderCopy(renderer, g_screen->user_words.texture, &src_rect, &dst_rect);

	//computer
	SDL_RenderDrawRect(renderer, &g_screen->computer_words.rect);
	dst_rect = g_screen->computer_words.rect;
	dst_rect.y += WORDS_AREA_PADDING;
	dst_rect.h -= WORDS_AREA_PADDING * 2;
	src_rect = (SDL_Rect){ 0, 0, 0, 0 };
	src_rect.y = g_screen->computer_words.scroll * (context->text_font_height + WORDS_AREA_TEXT_INRERVAL) - WORDS_AREA_TEXT_INRERVAL / 2;
	src_rect.w = WORDS_AREA_WIDTH;
	src_rect.h = (context->text_font_height + WORDS_AREA_TEXT_INRERVAL) * g_screen->page_limit;
	SDL_RenderCopy(renderer, g_screen->computer_words.texture, &src_rect, &dst_rect);
}

//render only if percent value is not zero
static void render_percent(SDL_Renderer* renderer, GameScreen* g_screen) {
	if (g_screen->percent != 0) {
		SDL_Rect rect = { 50, 20, 0, 0 };
		SDL_QueryTexture(g_screen->percent_texture, NULL, NULL, &rect.w, &rect.h);
		rect.x = SCREEN_WIDTH / 2 - rect.w / 2;
		SDL_RenderCopy(renderer, g_screen->percent_texture, NULL, &rect);
	}
}

static void ui_render_game(SDL_Renderer* renderer, ScreenContext* context, GameScreen* g_screen) {
	SDL_SetRenderDrawColor(renderer, WHITE);
	SDL_RenderClear(renderer);

	render_button(renderer, &g_screen->btn_save);
	render_button(renderer, &g_screen->btn_exit);

	render_button(renderer, &g_screen->btn_up);
	render_button(renderer, &g_screen->btn_down);
	render_button(renderer, &g_screen->btn_left);
	render_button(renderer, &g_screen->btn_right);

	render_words_area(renderer, context, g_screen);

	render_percent(renderer, g_screen);

	render_field(renderer, g_screen->grid, g_screen->text_input);

	//"Player" and "Computer" text
	SDL_Rect rect = { 80, 95, 0, 0 };
	SDL_QueryTexture(g_screen->player_texture, NULL, NULL, &rect.w, &rect.h);
	SDL_RenderCopy(renderer, g_screen->player_texture, NULL, &rect);

	rect = (SDL_Rect) { 0, 95, 0, 0 };
	SDL_QueryTexture(g_screen->computer_texture, NULL, NULL, &rect.w, &rect.h);
	rect.x = SCREEN_WIDTH - rect.w - 45;
	SDL_RenderCopy(renderer, g_screen->computer_texture, NULL, &rect);

	//Score
	rect = (SDL_Rect){ 110, g_screen->user_words.rect.y + WORDS_AREA_HEIGHT + 20, 0, 0 };
	SDL_QueryTexture(g_screen->user_score, NULL, NULL, &rect.w, &rect.h);
	SDL_RenderCopy(renderer, g_screen->user_score, NULL, &rect);
	
	rect = (SDL_Rect){ SCREEN_WIDTH - 130, g_screen->computer_words.rect.y + WORDS_AREA_HEIGHT + 20, 0, 0 };
	SDL_QueryTexture(g_screen->comp_score, NULL, NULL, &rect.w, &rect.h);
	SDL_RenderCopy(renderer, g_screen->comp_score, NULL, &rect);


	// pop-up messages 
	// each messages have an implicit priority in rendering and events handling, for example, if message_save is active and ai timeout occurs, 
	// timeout message will not be shown and time out events will not be handled until the save_message with higher "priority" dissapears
	if (g_screen->message_quit_confirm) {
		SDL_SetRenderDrawColor(renderer, WHITE);
		SDL_RenderFillRect(renderer, &g_screen->rect_message);
		SDL_SetRenderDrawColor(renderer, BLACK);
		SDL_RenderDrawRect(renderer, &g_screen->rect_message);

		SDL_Rect text_rect;
		SDL_QueryTexture(g_screen->quit_confirm_message_texture, NULL, NULL, &text_rect.w, &text_rect.h);
		text_rect.x = ALERT_X + ALERT_WIDTH / 2 - text_rect.w / 2;
		text_rect.y = ALERT_Y + 50;
		SDL_RenderCopy(renderer, g_screen->quit_confirm_message_texture, NULL, &text_rect);

		render_button(renderer, &g_screen->btn_message_yes);
		render_button(renderer, &g_screen->btn_message_no);
	}
	else if (g_screen->message_save) {
		SDL_SetRenderDrawColor(renderer, WHITE);
		SDL_RenderFillRect(renderer, &g_screen->rect_message);
		SDL_SetRenderDrawColor(renderer, BLACK);
		SDL_RenderDrawRect(renderer, &g_screen->rect_message);

		SDL_Rect text_rect;
		SDL_QueryTexture(g_screen->save_message_texture, NULL, NULL, &text_rect.w, &text_rect.h);
		text_rect.x = ALERT_X + ALERT_WIDTH / 2 - text_rect.w / 2;
		text_rect.y = ALERT_Y + 50;
		SDL_RenderCopy(renderer, g_screen->save_message_texture, NULL, &text_rect);

		render_input_field(renderer, &g_screen->input_field, context);

	}
	else if (g_screen->message_additional_time) {
		SDL_SetRenderDrawColor(renderer, WHITE);
		SDL_RenderFillRect(renderer, &g_screen->rect_message);
		SDL_SetRenderDrawColor(renderer, BLACK);
		SDL_RenderDrawRect(renderer, &g_screen->rect_message);

		SDL_Rect text_rect1;
		SDL_QueryTexture(g_screen->need_additional_time_texture1, NULL, NULL, &text_rect1.w, &text_rect1.h);
		text_rect1.x = ALERT_X + ALERT_WIDTH / 2 - text_rect1.w / 2;
		text_rect1.y = ALERT_Y + ALERT_HEIGHT / 2 - 100 - text_rect1.h / 2;
		SDL_RenderCopy(renderer, g_screen->need_additional_time_texture1, NULL, &text_rect1);

		SDL_Rect text_rect2;
		SDL_QueryTexture(g_screen->need_additional_time_texture2, NULL, NULL, &text_rect2.w, &text_rect2.h);
		text_rect2.x = ALERT_X + ALERT_WIDTH / 2 - text_rect2.w / 2;
		text_rect2.y = text_rect1.y + (int)(text_rect1.h * 1.5) - text_rect2.h / 2;
		SDL_RenderCopy(renderer, g_screen->need_additional_time_texture2, NULL, &text_rect2);

		render_button(renderer, &g_screen->btn_message_yes);
		render_button(renderer, &g_screen->btn_message_no);
	}
	else if (g_screen->message_end_game) {
		SDL_SetRenderDrawColor(renderer, WHITE);
		SDL_RenderFillRect(renderer, &g_screen->rect_message);
		SDL_SetRenderDrawColor(renderer, BLACK);
		SDL_RenderDrawRect(renderer, &g_screen->rect_message);

		SDL_Rect text_rect;
		SDL_QueryTexture(g_screen->end_game_message_texture, NULL, NULL, &text_rect.w, &text_rect.h);
		text_rect.x = ALERT_X + ALERT_WIDTH / 2 - text_rect.w / 2;
		text_rect.y = ALERT_Y + ALERT_HEIGHT / 2 - text_rect.h / 2;
		SDL_RenderCopy(renderer, g_screen->end_game_message_texture, NULL, &text_rect);
	}
	else if (g_screen->message_ask_for_username) {
		SDL_SetRenderDrawColor(renderer, WHITE);
		SDL_RenderFillRect(renderer, &g_screen->rect_message);
		SDL_SetRenderDrawColor(renderer, BLACK);
		SDL_RenderDrawRect(renderer, &g_screen->rect_message);

		SDL_Rect text_rect;
		SDL_QueryTexture(g_screen->ask_for_username_message_texture, NULL, NULL, &text_rect.w, &text_rect.h);
		text_rect.x = ALERT_X + ALERT_WIDTH / 2 - text_rect.w / 2;
		text_rect.y = ALERT_Y + 50;
		SDL_RenderCopy(renderer, g_screen->ask_for_username_message_texture, NULL, &text_rect);

		render_input_field(renderer, &g_screen->input_field, context);
	}
	else if (g_screen->message_invalid_word) {
		SDL_SetRenderDrawColor(renderer, WHITE);
		SDL_RenderFillRect(renderer, &g_screen->rect_message);
		SDL_SetRenderDrawColor(renderer, BLACK);
		SDL_RenderDrawRect(renderer, &g_screen->rect_message);

		SDL_Rect text_rect;
		SDL_QueryTexture(g_screen->invalid_word_message_texture, NULL, NULL, &text_rect.w, &text_rect.h);
		text_rect.x = ALERT_X + ALERT_WIDTH / 2 - text_rect.w / 2;
		text_rect.y = ALERT_Y + 50;
		SDL_RenderCopy(renderer, g_screen->invalid_word_message_texture, NULL, &text_rect);

		render_button(renderer, &g_screen->btn_message_yes);
		render_button(renderer, &g_screen->btn_message_no);
	}

	SDL_RenderPresent(renderer);
}


StatusCode ui_render(SDL_Renderer* renderer, ScreenContext* context, 
	MainScreen* main_screen,
	SettingsScreen* settings_screen,
	LeaderboardScreen* lb_screen,
	GameScreen* g_screen
) {
	switch (context->current_screen) {
	case SCREEN_MAIN:
		ui_render_main(renderer, context, main_screen);
		break;
	case SCREEN_SETTINGS:
		ui_render_settings(renderer, context, settings_screen);
		break;
	case SCREEN_LEADERBOARD:
		ui_render_leaderboard(renderer, lb_screen);
		break;
	case SCREEN_GAME:
		ui_render_game(renderer, context, g_screen);
	}

	return SUCCESS;
}