#define _CRT_SECURE_NO_WARNINGS
#include <SDL.h>
#include <SDL_ttf.h>

#include "common.h"
#include "status_codes.h"
#include "ui.h"
#include "game_logic.h"

#define WHITE 255, 255, 255, 255
#define BLACK 0, 0, 0, 0
#define BLUE 0, 94, 102, 0
#define GRAY 184, 184, 184, 0
#define GRAY_HOVERED 222, 222, 222, 0
#define YELLOW 255, 255, 0, 0
#define PURPLE 225, 28, 255, 0
#define GREEN 0, 255, 0, 0


static int get_cursor_padding(TTF_Font* font, const char* text, int len) {
	//figure out text`s len before pointer. So we need to copy len letters into buffer
	char buffer[MAX_UI_UTF8_BUFFER_SIZE] = { 0 };
	strncpy_s(buffer, MAX_UI_UTF8_BUFFER_SIZE, text, len);

	int w = 0;
	TTF_SizeUTF8(font, buffer, &w, NULL);

	return w;
}


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
	sett_screen->btn_to_main.is_clicked = 0; //добавить выбор в зависимости от настроек
	sett_screen->btn_to_main.texture = createTextureFromText(renderer, context->btn_font, sett_screen->btn_to_main.text);
	if (sett_screen->btn_to_main.texture == NULL) {
		return UI_SDL_TEXTURE_ERROR;
	}

	strncpy_s(sett_screen->btn_easy.text, MAX_UI_BUFFER_SIZE, "Легко", MAX_UI_BUFFER_SIZE);
	sett_screen->btn_easy.rect = (SDL_Rect){250, 150, 200, 75 };
	sett_screen->btn_easy.is_active = diff == 1 ? 1 : 0;
	sett_screen->btn_easy.is_hoverable = 0;
	sett_screen->btn_easy.is_hovered = 0;
	sett_screen->btn_easy.is_clicked = 0; //добавить выбор в зависимости от настроек
	sett_screen->btn_easy.texture = createTextureFromText(renderer, context->btn_font, sett_screen->btn_easy.text);
	if (sett_screen->btn_easy.texture == NULL) {
		return UI_SDL_TEXTURE_ERROR;
	}

	strncpy_s(sett_screen->btn_mid.text, MAX_UI_BUFFER_SIZE, "Нормально", MAX_UI_BUFFER_SIZE);
	sett_screen->btn_mid.rect = (SDL_Rect){ 500, 150, 200, 75 };
	sett_screen->btn_mid.is_active = diff == 2 ? 1 : 0;
	sett_screen->btn_mid.is_hoverable = 0;
	sett_screen->btn_mid.is_hovered = 0;
	sett_screen->btn_mid.is_clicked = 0; //добавить выбор в зависимости от настроекb
	sett_screen->btn_mid.texture = createTextureFromText(renderer, context->btn_font, sett_screen->btn_mid.text);
	if (sett_screen->btn_mid.texture == NULL) {
		return UI_SDL_TEXTURE_ERROR;
	}

	strncpy_s(sett_screen->btn_hard.text, MAX_UI_BUFFER_SIZE, "Сложно", MAX_UI_BUFFER_SIZE);
	sett_screen->btn_hard.rect = (SDL_Rect){750, 150, 200, 75 };
	sett_screen->btn_hard.is_active = 0;
	sett_screen->btn_hard.is_hoverable = 0;
	sett_screen->btn_hard.is_hovered = 0;
	sett_screen->btn_hard.is_clicked = 0; //добавить выбор в зависимости от настроек
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
	sett_screen->btn_p1.is_clicked = 0; //добавить выбор в зависимости от настроекb
	sett_screen->btn_p1.texture = createTextureFromText(renderer, context->btn_font, sett_screen->btn_p1.text);
	if (sett_screen->btn_p1.texture == NULL) {
		return UI_SDL_TEXTURE_ERROR;
	}

	strncpy_s(sett_screen->btn_p2.text, MAX_UI_BUFFER_SIZE, "Компьютер", MAX_UI_BUFFER_SIZE);
	sett_screen->btn_p2.rect = (SDL_Rect){ 750, 250, 200, 75 };
	sett_screen->btn_p2.is_active = frst_player == 2 ? 1 : 0;
	sett_screen->btn_p2.is_hoverable = 0;
	sett_screen->btn_p2.is_hovered = 0;
	sett_screen->btn_p2.is_clicked = 0; //добавить выбор в зависимости от настроек
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
	lb_screen->btn_back.is_clicked = 0; //добавить выбор в зависимости от настроек
	lb_screen->btn_back.texture = createTextureFromText(renderer, context->btn_font, lb_screen->btn_back.text);
	if (lb_screen->btn_back.texture == NULL) {
		return UI_SDL_TEXTURE_ERROR;
	}


	//game_screen
	g_screen->cursor_x = 2;
	g_screen->cursor_y = 2;
	g_screen->is_stoped = 0;
	g_screen->letter = '\0';
	g_screen->text_input = 0;
	g_screen->input_on_off = 0;
	g_screen->is_cursor_active = 1;
	g_screen->is_letter_placed = 0;
	g_screen->new_letter = 0;

	g_screen->computer = createTextureFromText(renderer, context->text_font, "Компьютер");
	g_screen->player = createTextureFromText(renderer, context->text_font, "Игрок");

	strncpy_s(g_screen->btn_up.text, MAX_UI_BUFFER_SIZE, "Вверх", MAX_UI_BUFFER_SIZE);
	g_screen->btn_up.rect.x = SCREEN_WIDTH/2 - 25;
	g_screen->btn_up.rect.y = 560;
	g_screen->btn_up.rect.w = 50;
	g_screen->btn_up.rect.h = 50;
	g_screen->btn_up.is_active = 0;
	g_screen->btn_up.is_hoverable = 1;
	g_screen->btn_up.is_hovered = 0;
	g_screen->btn_up.is_clicked = 0; 
	g_screen->btn_up.texture = NULL;

	strncpy_s(g_screen->btn_down.text, MAX_UI_BUFFER_SIZE, "Вниз", MAX_UI_BUFFER_SIZE);
	g_screen->btn_down.rect.x = SCREEN_WIDTH / 2 - 25;
	g_screen->btn_down.rect.y = 680;
	g_screen->btn_down.rect.w = 50;
	g_screen->btn_down.rect.h = 50;
	g_screen->btn_down.is_active = 0;
	g_screen->btn_down.is_hoverable = 1;
	g_screen->btn_down.is_hovered = 0;
	g_screen->btn_down.is_clicked = 0; //добавить выбор в зависимости от настроек
	g_screen->btn_down.texture = NULL;

	strncpy_s(g_screen->btn_left.text, MAX_UI_BUFFER_SIZE, "Влево", MAX_UI_BUFFER_SIZE);
	g_screen->btn_left.rect.x = SCREEN_WIDTH / 2 - 60 - 25;
	g_screen->btn_left.rect.y = 620;
	g_screen->btn_left.rect.w = 50;
	g_screen->btn_left.rect.h = 50;
	g_screen->btn_left.is_active = 0;
	g_screen->btn_left.is_hoverable = 1;
	g_screen->btn_left.is_hovered = 0;
	g_screen->btn_left.is_clicked = 0; //добавить выбор в зависимости от настроек
	g_screen->btn_left.texture = NULL;

	strncpy_s(g_screen->btn_right.text, MAX_UI_BUFFER_SIZE, "Вправо", MAX_UI_BUFFER_SIZE);
	g_screen->btn_right.rect.x = SCREEN_WIDTH / 2 + 60 - 25;
	g_screen->btn_right.rect.y = 620;
	g_screen->btn_right.rect.w = 50;
	g_screen->btn_right.rect.h = 50;
	g_screen->btn_right.is_active = 0;
	g_screen->btn_right.is_hoverable = 1;
	g_screen->btn_right.is_hovered = 0;
	g_screen->btn_right.is_clicked = 0; //добавить выбор в зависимости от настроек
	g_screen->btn_right.texture = NULL;

	return SUCCESS;
}


//--------------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------EVENTS--------------------------------------------------------------
//--------------------------------------------------------------------------------------------------------------------------

void check_button_hovered(SDL_Event e, Button* btn) {
	if ((e.motion.x >= btn->rect.x && e.motion.x <= btn->rect.x + btn->rect.w)
		&& (e.motion.y >= btn->rect.y && e.motion.y <= btn->rect.y + btn->rect.h)) {
		btn->is_hovered = 1;
	}
	else {
		btn->is_hovered = 0;
	}
}

void check_field_hovered(SDL_Event e, InputField* field) {
	if ((e.motion.x >= field->rect.x && e.motion.x <= field->rect.x + field->rect.w)
		&& (e.motion.y >= field->rect.y && e.motion.y <= field->rect.y + field->rect.h)) {
		field->is_hovered = 1;
	}
	else {
		field->is_hovered = 0;
	}
}

void event_mainmenu_mousemotion(SDL_Event e, MainScreen* main_screen) {
	check_button_hovered(e, &main_screen->btn_new_game);
	check_button_hovered(e, &main_screen->btn_load_game);
	check_button_hovered(e, &main_screen->btn_to_settings);
	check_button_hovered(e, &main_screen->btn_to_leaderboard);
	check_button_hovered(e, &main_screen->btn_exit);
}

void event_mainmenu_mouseclick_down(SDL_Event e, MainScreen* main_screen) {
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
}


void event_settings_mousemotion(SDL_Event e, SettingsScreen* sett_screen) {
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

void event_settings_mouseclick(SDL_Event e, SettingsScreen* sett_screen) {
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

void event_settings_keydown(SDL_Event e, SettingsScreen* sett_screen) {
	InputField* field = &sett_screen->timelimit_field;
	switch (e.key.keysym.sym) {
	case SDLK_BACKSPACE:
		if (field->cursorPos > 0 && sett_screen->timelimit_field.is_active) {
			//memmove because its safe for crossed parts of memmory
			memmove(&field->text[field->cursorPos - 1], &field->text[field->cursorPos], strlen(field->text) - field->cursorPos + 1);
			field->cursorPos--;
		}
		break;
	case SDLK_RIGHT:
		if (field->cursorPos < strlen(field->text) && sett_screen->timelimit_field.is_active)
			field->cursorPos++;
		break;
	case SDLK_LEFT:
		if (field->cursorPos > 0 && sett_screen->timelimit_field.is_active)
			field->cursorPos--;
		break;
	case SDLK_RETURN:
		if (sett_screen->timelimit_field.is_active) {
			sett_screen->timelimit_field.is_clicked = 1;
		}
	}
}

void event_settings_text_input(SDL_Event e, SettingsScreen* sett_screen) {
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


static void event_game_keydown(SDL_Event e, GameScreen* g_screen) {
	printf("ok\n");
	switch (e.key.keysym.sym) {
	case SDLK_RETURN:
		if (!g_screen->is_letter_placed) {
			printf("ENTER PRESSED\n");
			g_screen->input_on_off = 1;
		}
	}
}

static void event_game_text_input(SDL_Event e, GameScreen* g_screen) {
	if (is_it_ru_utf8_letter(e.text.text)) {
		unsigned char cp1251_lett = '\0';
		letter_utf8_to_cp1251(e.text.text, &cp1251_lett);
		g_screen->letter = cp1251_lett;
		g_screen->new_letter = 1;
	}
}

static void event_game_mousemotion(SDL_Event e, GameScreen* g_screen) {
	if (g_screen->is_cursor_active) {
		check_button_hovered(e, &g_screen->btn_up);
		check_button_hovered(e, &g_screen->btn_down);
		check_button_hovered(e, &g_screen->btn_left);
		check_button_hovered(e, &g_screen->btn_right);
	}
}

static void event_game_mouseclick(SDL_Event e, GameScreen* g_screen) {
	if (g_screen->btn_up.is_hovered) {
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


StatusCode ui_handle_events(SDL_Renderer* render, ScreenContext context, 
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
			switch (context.current_screen) {
			case SCREEN_MAIN:
				if (e.type == SDL_MOUSEMOTION) {
					event_mainmenu_mousemotion(e, main_screen);
				}
				else if (e.type == SDL_MOUSEBUTTONDOWN) {
					printf("click\n");
					event_mainmenu_mouseclick_down(e, main_screen);
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
				else if (e.type == SDL_MOUSEMOTION) {
					event_game_mousemotion(e, g_screen);
				}
				else if (e.type == SDL_MOUSEBUTTONDOWN) {
					event_game_mouseclick(e, g_screen);
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

//make a graphic representation of game (make field textures, update "cursored" and selected cells and etc.)
static void load_game_screen(SDL_Renderer* renderer, ScreenContext context, GameScreen* g_screen, Game* game) {
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
				g_screen->grid[y][x].texture = createTextureFromText(renderer, context.cell_font, &letter);
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

	g_screen->computer_score = createTextureFromText(renderer, context.text_font, p2_score);
	g_screen->player_score = createTextureFromText(renderer, context.text_font, p1_score);
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
	if (main_screen->btn_new_game.is_clicked) {
		main_screen->btn_new_game.is_clicked = 0;
		*game = game_create(settings, dict);
		if (*game == NULL) {
			fprintf(stderr, "Ошибка при создании игры!\n");
		}
		load_game_screen(renderer, *context, g_screen, *game);
		context->current_screen = SCREEN_GAME;
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
}

static void ui_update_logic_settings(ScreenContext* context, SettingsScreen* sett_screen, GameSettings* settings) {
	if (sett_screen->btn_to_main.is_clicked) {
		sett_screen->btn_to_main.is_clicked = 0;
		context->current_screen = SCREEN_MAIN;
	}
	else if (sett_screen->btn_easy.is_clicked) {
		sett_screen->btn_easy.is_clicked = 0;
		game_set_difficulty(settings, 0);
	}
	else if (sett_screen->btn_mid.is_clicked) {
		sett_screen->btn_mid.is_clicked = 0;
		game_set_difficulty(settings, 1);
	}
	else if (sett_screen->btn_hard.is_clicked) {
		sett_screen->btn_hard.is_clicked = 0;
		game_set_difficulty(settings, 2);
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


static void ui_update_logic_game(SDL_Renderer* renderer, ScreenContext* context, GameScreen* g_screen, Game** game, Dictionary* dict) {
	//first part, before letter was placed
	if (!g_screen->is_letter_placed) {
		if (g_screen->input_on_off) {
			if (!g_screen->text_input) {
				if (is_cell_empty(game_get_field(*game), g_screen->cursor_y, g_screen->cursor_x)) {
					printf("start input\n");
					g_screen->is_cursor_active = 0;
					g_screen->text_input = 1;
					SDL_StartTextInput();
				}
				else {
					printf("клетка занята!\n");
				}
			}
			else {
				if (g_screen->letter == '\0') {
					printf("end input\n");
					g_screen->text_input = 0;
					g_screen->is_cursor_active = 1;
					SDL_StopTextInput();
				}
				else {
					//call game_try_place_letter() and so on
				}
				//check is there letter, if it exists, start word selection
			}
			g_screen->input_on_off = 0;
		}
		//update letter
		if (g_screen->new_letter) {
			//add it only into UI grid
			g_screen->grid[g_screen->cursor_y][g_screen->cursor_x].texture = createTextureFromText(renderer, context->cell_font, g_screen->letter);
			g_screen->new_letter = 0;
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
		ui_update_logic_game(renderer, context, g_screen, game, dict);
		break;
	}
}



//-----------------------------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------RENDER-----------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------------------------------------------

static void render_button(SDL_Renderer* renderer, Button btn) {
	if ((btn.is_hovered && btn.is_hoverable)|| btn.is_active)
		SDL_SetRenderDrawColor(renderer, GRAY_HOVERED);
	else
		SDL_SetRenderDrawColor(renderer, GRAY);

	SDL_RenderFillRect(renderer, &btn.rect);
	SDL_SetRenderDrawColor(renderer, BLUE);
	SDL_RenderDrawRect(renderer, &btn.rect);

	if (btn.texture != NULL) {
		SDL_Rect text_rect = btn.rect;
		SDL_QueryTexture(btn.texture, NULL, NULL, &text_rect.w, &text_rect.h);
		text_rect.y = btn.rect.y + (btn.rect.h - text_rect.h) / 2;
		text_rect.x = btn.rect.x + (btn.rect.w - text_rect.w) / 2;
		SDL_RenderCopy(renderer, btn.texture, NULL, &text_rect);
	}
}

void render_input_field(SDL_Renderer* renderer, InputField field, ScreenContext context) {
	SDL_SetRenderDrawColor(renderer, GRAY_HOVERED);
	SDL_RenderFillRect(renderer, &field.rect);

	SDL_SetRenderDrawColor(renderer, BLACK);
	SDL_RenderDrawRect(renderer, &field.rect);

	if (strlen(field.text) > 0) {
		SDL_Texture* textTexture = createTextureFromText(renderer, context.input_field_font, field.text);
		int textureW, textureH;
		SDL_QueryTexture(textTexture, NULL, NULL, &textureW, &textureH);

		SDL_Rect text_rect = {
			field.rect.x + 5,
			field.rect.y + (field.rect.h - textureH) / 2,
			textureW,
			textureH,
		};
		SDL_RenderCopy(renderer, textTexture, NULL, &text_rect);

		SDL_DestroyTexture(textTexture);
	}

	//cursor
	if (field.is_active) {
		SDL_Rect cursor = {
			field.rect.x + 5 + get_cursor_padding(context.input_field_font, field.text, field.cursorPos),
			field.rect.y + 5,
			2,
			field.rect.h - 10
		};
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
		SDL_RenderFillRect(renderer, &cursor);
	}
}


void static ui_render_main(SDL_Renderer* renderer, ScreenContext context, MainScreen main_screen) {
	SDL_SetRenderDrawColor(renderer, WHITE);
	SDL_RenderClear(renderer);

	SDL_Rect text_rect;
	SDL_QueryTexture(main_screen.header, NULL, NULL, &text_rect.w, &text_rect.h);
	text_rect.x = SCREEN_WIDTH / 2 - text_rect.w / 2;
	text_rect.y = 50;
	SDL_RenderCopy(renderer, main_screen.header, NULL, &text_rect);

	render_button(renderer, main_screen.btn_new_game);
	render_button(renderer, main_screen.btn_load_game);
	render_button(renderer, main_screen.btn_to_settings);
	render_button(renderer, main_screen.btn_to_leaderboard);
	render_button(renderer, main_screen.btn_exit);

	SDL_RenderPresent(renderer);
}


void ui_render_settings(SDL_Renderer* renderer, ScreenContext context, SettingsScreen settings_screen) {
	SDL_SetRenderDrawColor(renderer, WHITE);
	SDL_RenderClear(renderer);

	SDL_Rect text_rect;
	SDL_QueryTexture(settings_screen.header, NULL, NULL, &text_rect.w, &text_rect.h);
	text_rect.x = SCREEN_WIDTH / 2 - text_rect.w / 2;
	text_rect.y = 50;
	SDL_RenderCopy(renderer, settings_screen.header, NULL, &text_rect);

	render_button(renderer, settings_screen.btn_to_main);

	render_button(renderer, settings_screen.btn_easy);
	render_button(renderer, settings_screen.btn_mid);
	render_button(renderer, settings_screen.btn_hard);

	SDL_QueryTexture(settings_screen.first_player, NULL, NULL, &text_rect.w, &text_rect.h);
	text_rect.x = 270;
	text_rect.y = 277;
	SDL_RenderCopy(renderer, settings_screen.first_player, NULL, &text_rect);
	render_button(renderer, settings_screen.btn_p1);
	render_button(renderer, settings_screen.btn_p2);

	SDL_QueryTexture(settings_screen.time_limit, NULL, NULL, &text_rect.w, &text_rect.h);
	text_rect.x = 270;
	text_rect.y = 377;
	SDL_RenderCopy(renderer, settings_screen.time_limit, NULL, &text_rect);
	render_input_field(renderer, settings_screen.timelimit_field, context);

	SDL_RenderPresent(renderer);
}

void ui_render_leaderboard(SDL_Renderer* renderer, ScreenContext context, LeaderboardScreen lb_screen) {
	SDL_SetRenderDrawColor(renderer, WHITE);
	SDL_RenderClear(renderer);

	SDL_Rect text_rect;
	SDL_QueryTexture(lb_screen.header, NULL, NULL, &text_rect.w, &text_rect.h);
	text_rect.x = SCREEN_WIDTH / 2 - text_rect.w / 2;
	text_rect.y = 20;
	SDL_RenderCopy(renderer, lb_screen.header, NULL, &text_rect);

	render_button(renderer, lb_screen.btn_back);

	//render rows and columns
	
	for (int i = 0; i < lb_screen.count_of_records; i++) {
		//num
		SDL_Rect rect = { 10, 80 + (i * 40), 40, 40};
		SDL_SetRenderDrawColor(renderer, BLACK);
		SDL_RenderDrawRect(renderer, &rect);

		SDL_Rect t_rect = rect;
		SDL_QueryTexture(lb_screen.nums[i], NULL, NULL, &t_rect.w, &t_rect.h);
		t_rect.y = rect.y + (rect.h - t_rect.h) / 2;
		t_rect.x = rect.x + (rect.w - t_rect.w) / 2;
		SDL_RenderCopy(renderer, lb_screen.nums[i], NULL, &t_rect);

		rect = (SDL_Rect) { 50, 80 + (i * 40), SCREEN_WIDTH / 2 - 40, 40 };
		SDL_SetRenderDrawColor(renderer, BLACK);
		SDL_RenderDrawRect(renderer, &rect);
		t_rect = rect;
		SDL_QueryTexture(lb_screen.users[i], NULL, NULL, &t_rect.w, &t_rect.h);
		t_rect.y = rect.y + (rect.h - t_rect.h) / 2;
		t_rect.x = rect.x + (rect.w - t_rect.w) / 2;
		SDL_RenderCopy(renderer, lb_screen.users[i], NULL, &t_rect);

		rect = (SDL_Rect){ 50 + (SCREEN_WIDTH / 2 - 40), 80 + (i * 40), SCREEN_WIDTH / 2 - 40, 40};
		SDL_SetRenderDrawColor(renderer, BLACK);
		SDL_RenderDrawRect(renderer, &rect);
		t_rect = rect;
		SDL_QueryTexture(lb_screen.scores[i], NULL, NULL, &t_rect.w, &t_rect.h);
		t_rect.y = rect.y + (rect.h - t_rect.h) / 2;
		t_rect.x = rect.x + (rect.w - t_rect.w) / 2;
		SDL_RenderCopy(renderer, lb_screen.scores[i], NULL, &t_rect);
	}

	SDL_RenderPresent(renderer);
}

//this function is not suitable for different field sizes, static array grid[][] should be replaced by dinamically allocated one,
// also its need to receive field_size param
static void render_field(SDL_Renderer* renderer, ScreenContext context, UICell grid[][FIELD_SIZE]) {
	const int start_x = 350;
	const int start_y = 20;
	const int cell_size = 100;
	for (int j = 0; j < FIELD_SIZE; j++) {
		for (int i = 0; i < FIELD_SIZE; i++) {
			SDL_Rect cell = (SDL_Rect){ start_x + cell_size * i, start_y + cell_size * j, cell_size, cell_size };
			if (grid[j][i].is_selected) {
				SDL_SetRenderDrawColor(renderer, GRAY_HOVERED);
				SDL_RenderFillRect(renderer, &cell);
				SDL_SetRenderDrawColor(renderer, BLACK);
				SDL_RenderDrawRect(renderer, &cell);
			}
			if (grid[j][i].is_cursored) {
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
				cell_letter.x = cell.x + cell_size / 2 - cell_letter.w / 2;
				cell_letter.y = cell.y + cell_size / 2 - cell_letter.h / 2;
				SDL_RenderCopy(renderer, grid[j][i].texture, NULL,&cell_letter);
			}
		}
	}
}

static void ui_render_game(SDL_Renderer* renderer, ScreenContext context, GameScreen g_screen) {
	SDL_SetRenderDrawColor(renderer, WHITE);
	SDL_RenderClear(renderer);

	render_button(renderer, g_screen.btn_up);
	render_button(renderer, g_screen.btn_down);
	render_button(renderer, g_screen.btn_left);
	render_button(renderer, g_screen.btn_right);

	render_field(renderer, context, g_screen.grid);
	SDL_RenderPresent(renderer);
}


StatusCode ui_render(SDL_Renderer* renderer, ScreenContext context, 
	MainScreen main_screen,
	SettingsScreen settings_screen,
	LeaderboardScreen lb_screen,
	GameScreen g_screen
) {
	switch (context.current_screen) {
	case SCREEN_MAIN:
		ui_render_main(renderer, context, main_screen);
		break;
	case SCREEN_SETTINGS:
		ui_render_settings(renderer, context, settings_screen);
		break;
	case SCREEN_LEADERBOARD:
		ui_render_leaderboard(renderer, context, lb_screen);
		break;
	case SCREEN_GAME:
		ui_render_game(renderer, context, g_screen);
	}

	return SUCCESS;
}