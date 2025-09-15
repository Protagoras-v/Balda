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


StatusCode ui_set_screen_context(SDL_Renderer* renderer, ScreenContext* context, 
	MainScreen* main_screen,
	SettingsScreen* screen_settings,
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

	//main menu
	main_screen->header = createTextureFromText(renderer, context->header_font, "Балда");

	strncpy_s(main_screen->btn_new_game.text, MAX_UI_BUFFER_SIZE, "Новая игра", MAX_UI_BUFFER_SIZE);
	main_screen->btn_new_game.rect.x = 312;
	main_screen->btn_new_game.rect.y = 150;
	main_screen->btn_new_game.rect.w = 275;
	main_screen->btn_new_game.rect.h = 75;
	main_screen->btn_new_game.is_active = 0;
	main_screen->btn_new_game.is_hoverable = 1;
	main_screen->btn_new_game.is_hovered = 0;
	main_screen->btn_new_game.is_clicked = 0;
	main_screen->btn_new_game.texture = createTextureFromText(renderer, context->btn_font, main_screen->btn_new_game.text);
	if (main_screen->btn_new_game.texture == NULL) {
		return UI_SDL_TEXTURE_ERROR;
	}
	
	strncpy_s(main_screen->btn_load_game.text, MAX_UI_BUFFER_SIZE,  "Загрузить", MAX_UI_BUFFER_SIZE);
	main_screen->btn_load_game.rect.x = 312;
	main_screen->btn_load_game.rect.y = 250;
	main_screen->btn_load_game.rect.w = 275;
	main_screen->btn_load_game.rect.h = 75;
	main_screen->btn_load_game.is_active = 0;
	main_screen->btn_load_game.is_hoverable = 1;
	main_screen->btn_load_game.is_hovered = 0;
	main_screen->btn_load_game.is_clicked = 0;
	main_screen->btn_load_game.texture = createTextureFromText(renderer, context->btn_font, main_screen->btn_load_game.text);
	if (main_screen->btn_load_game.texture == NULL) {
		return UI_SDL_TEXTURE_ERROR;
	}

	strncpy_s(main_screen->btn_to_settings.text, MAX_UI_BUFFER_SIZE, "Настройки", MAX_UI_BUFFER_SIZE);
	main_screen->btn_to_settings.rect.x = 312;
	main_screen->btn_to_settings.rect.y = 350;
	main_screen->btn_to_settings.rect.w = 275;
	main_screen->btn_to_settings.rect.h = 75;
	main_screen->btn_to_settings.is_active = 0;
	main_screen->btn_to_settings.is_hoverable = 1;
	main_screen->btn_to_settings.is_hovered = 0;
	main_screen->btn_to_settings.is_clicked = 0;
	main_screen->btn_to_settings.texture = createTextureFromText(renderer, context->btn_font, main_screen->btn_to_settings.text);
	if (main_screen->btn_to_settings.texture == NULL) {
		return UI_SDL_TEXTURE_ERROR;
	}

	strncpy_s(main_screen->btn_to_leaderboard.text, MAX_UI_BUFFER_SIZE, "Таблица лидеров", MAX_UI_BUFFER_SIZE);
	main_screen->btn_to_leaderboard.rect.x = 312;
	main_screen->btn_to_leaderboard.rect.y = 450;
	main_screen->btn_to_leaderboard.rect.w = 275;
	main_screen->btn_to_leaderboard.rect.h = 75;
	main_screen->btn_to_leaderboard.is_active = 0;
	main_screen->btn_to_leaderboard.is_hoverable = 1;
	main_screen->btn_to_leaderboard.is_hovered = 0;
	main_screen->btn_to_leaderboard.is_clicked = 0;
	main_screen->btn_to_leaderboard.texture = createTextureFromText(renderer, context->btn_font, main_screen->btn_to_leaderboard.text);
	if (main_screen->btn_to_leaderboard.texture == NULL) {
		return UI_SDL_TEXTURE_ERROR;
	}

	strncpy_s(main_screen->btn_exit.text, MAX_UI_BUFFER_SIZE, "Выход", MAX_UI_BUFFER_SIZE);
	main_screen->btn_exit.rect.x = 312;
	main_screen->btn_exit.rect.y = 550;
	main_screen->btn_exit.rect.w = 275;
	main_screen->btn_exit.rect.h = 75;
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

	screen_settings->header = createTextureFromText(renderer, context->header_font, "Настройки");

	strncpy_s(screen_settings->btn_to_main.text, MAX_UI_BUFFER_SIZE, "Назад", MAX_UI_BUFFER_SIZE);
	screen_settings->btn_to_main.rect.x = 10;
	screen_settings->btn_to_main.rect.y = 10;
	screen_settings->btn_to_main.rect.w = 150;
	screen_settings->btn_to_main.rect.h = 50;
	screen_settings->btn_to_main.is_active = diff == 0 ? 1 : 0;
	screen_settings->btn_to_main.is_hoverable = 1;
	screen_settings->btn_to_main.is_hovered = 0;
	screen_settings->btn_to_main.is_clicked = 0; //добавить выбор в зависимости от настроек
	screen_settings->btn_to_main.texture = createTextureFromText(renderer, context->btn_font, screen_settings->btn_to_main.text);
	if (screen_settings->btn_to_main.texture == NULL) {
		return UI_SDL_TEXTURE_ERROR;
	}

	strncpy_s(screen_settings->btn_easy.text, MAX_UI_BUFFER_SIZE, "Легко", MAX_UI_BUFFER_SIZE);
	screen_settings->btn_easy.rect.x = 100;
	screen_settings->btn_easy.rect.y = 150;
	screen_settings->btn_easy.rect.w = 200;
	screen_settings->btn_easy.rect.h = 75;
	screen_settings->btn_easy.is_active = diff == 1 ? 1 : 0;
	screen_settings->btn_easy.is_hoverable = 0;
	screen_settings->btn_easy.is_hovered = 0;
	screen_settings->btn_easy.is_clicked = 0; //добавить выбор в зависимости от настроек
	screen_settings->btn_easy.texture = createTextureFromText(renderer, context->btn_font, screen_settings->btn_easy.text);
	if (screen_settings->btn_easy.texture == NULL) {
		return UI_SDL_TEXTURE_ERROR;
	}

	strncpy_s(screen_settings->btn_mid.text, MAX_UI_BUFFER_SIZE, "Нормально", MAX_UI_BUFFER_SIZE);
	screen_settings->btn_mid.rect.x = 350;
	screen_settings->btn_mid.rect.y = 150;
	screen_settings->btn_mid.rect.w = 200;
	screen_settings->btn_mid.rect.h = 75;
	screen_settings->btn_mid.is_active = diff == 2 ? 1 : 0;
	screen_settings->btn_mid.is_hoverable = 0;
	screen_settings->btn_mid.is_hovered = 0;
	screen_settings->btn_mid.is_clicked = 0; //добавить выбор в зависимости от настроекb
	screen_settings->btn_mid.texture = createTextureFromText(renderer, context->btn_font, screen_settings->btn_mid.text);
	if (screen_settings->btn_mid.texture == NULL) {
		return UI_SDL_TEXTURE_ERROR;
	}

	strncpy_s(screen_settings->btn_hard.text, MAX_UI_BUFFER_SIZE, "Сложно", MAX_UI_BUFFER_SIZE);
	screen_settings->btn_hard.rect.x = 600;
	screen_settings->btn_hard.rect.y = 150;
	screen_settings->btn_hard.rect.w = 200;
	screen_settings->btn_hard.rect.h = 75;
	screen_settings->btn_hard.is_active = 0;
	screen_settings->btn_hard.is_hoverable = 0;
	screen_settings->btn_hard.is_hovered = 0;
	screen_settings->btn_hard.is_clicked = 0; //добавить выбор в зависимости от настроек
	screen_settings->btn_hard.texture = createTextureFromText(renderer, context->btn_font, screen_settings->btn_hard.text);
	if (screen_settings->btn_hard.texture == NULL) {
		return UI_SDL_TEXTURE_ERROR;
	}


	screen_settings->first_player = createTextureFromText(renderer, context->btn_font, "Первый ход");
	if (screen_settings->first_player == NULL) {
		return UI_SDL_TEXTURE_ERROR;
	}

	strncpy_s(screen_settings->btn_p1.text, MAX_UI_BUFFER_SIZE, "Игрок", MAX_UI_BUFFER_SIZE);
	screen_settings->btn_p1.rect.x = 350;
	screen_settings->btn_p1.rect.y = 250;
	screen_settings->btn_p1.rect.w = 200;
	screen_settings->btn_p1.rect.h = 75;
	screen_settings->btn_p1.is_active = frst_player == 1 ? 1 : 0;
	screen_settings->btn_p1.is_hoverable = 0;
	screen_settings->btn_p1.is_hovered = 0;
	screen_settings->btn_p1.is_clicked = 0; //добавить выбор в зависимости от настроекb
	screen_settings->btn_p1.texture = createTextureFromText(renderer, context->btn_font, screen_settings->btn_p1.text);
	if (screen_settings->btn_p1.texture == NULL) {
		return UI_SDL_TEXTURE_ERROR;
	}

	strncpy_s(screen_settings->btn_p2.text, MAX_UI_BUFFER_SIZE, "Компьютер", MAX_UI_BUFFER_SIZE);
	screen_settings->btn_p2.rect.x = 600;
	screen_settings->btn_p2.rect.y = 250;
	screen_settings->btn_p2.rect.w = 200;
	screen_settings->btn_p2.rect.h = 75;
	screen_settings->btn_p2.is_active = frst_player == 2 ? 1 : 0;
	screen_settings->btn_p2.is_hoverable = 0;
	screen_settings->btn_p2.is_hovered = 0;
	screen_settings->btn_p2.is_clicked = 0; //добавить выбор в зависимости от настроек
	screen_settings->btn_p2.texture = createTextureFromText(renderer, context->btn_font, screen_settings->btn_p2.text);
	if (screen_settings->btn_p2.texture == NULL) {
		return UI_SDL_TEXTURE_ERROR;
	}


	screen_settings->time_limit = createTextureFromText(renderer, context->btn_font, "Лимит хода компьютера (мс)");
	if (screen_settings->first_player == NULL) {
		return UI_SDL_TEXTURE_ERROR;
	}
	screen_settings->timelimit_field.rect.x = 575;
	screen_settings->timelimit_field.rect.y = 362;
	screen_settings->timelimit_field.rect.w = 225;
	screen_settings->timelimit_field.rect.h = 50;
	screen_settings->timelimit_field.is_active = 0;
	screen_settings->timelimit_field.is_hovered = 0; 
	screen_settings->timelimit_field.is_clicked = 0;
	screen_settings->timelimit_field.cursorPos = 0;
	screen_settings->timelimit_field.text[0] = '\0';


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
		if (field->cursorPos > 0) {
			//memmove because its safe for crossed parts of memmory
			memmove(&field->text[field->cursorPos - 1], &field->text[field->cursorPos], strlen(field->text) - field->cursorPos + 1);
			field->cursorPos--;
		}
		break;
	case SDLK_RIGHT:
		if (field->cursorPos < strlen(field->text))
			field->cursorPos++;
		break;
	case SDLK_LEFT:
		if (field->cursorPos > 0)
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


StatusCode ui_handle_events(SDL_Renderer* render, ScreenContext context, 
	MainScreen* main_screen, 
	SettingsScreen* sett_screen) {
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
				break;
			case SCREEN_GAME:
				break;
			}
		}
	}

	return SUCCESS;
}



//-----------------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------UPDATE LOGIC--------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------------------------------------------

void ui_update_logic_main(ScreenContext* context, MainScreen* main_screen, bool* f) {
	if (main_screen->btn_to_settings.is_clicked) {
		main_screen->btn_to_settings.is_clicked = 0;
		context->current_screen = SCREEN_SETTINGS;
	}
	else if (main_screen->btn_exit.is_clicked) {
		main_screen->btn_exit.is_clicked = 0;
		*f = false;
	}
}

void ui_update_logic_settings(ScreenContext* context, SettingsScreen* sett_screen, GameSettings* settings) {
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

StatusCode ui_update_logic(ScreenContext* context, 
	MainScreen* main_screen, 
	SettingsScreen* sett_screen, 
	GameSettings* settings,
	bool* f) {
	switch (context->current_screen) {
	case SCREEN_MAIN:
		ui_update_logic_main(context, main_screen, f);
		break;
	case SCREEN_SETTINGS:
		ui_update_logic_settings(context, sett_screen, settings);
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

	SDL_Rect text_rect = btn.rect;
	SDL_QueryTexture(btn.texture, NULL, NULL, &text_rect.w, &text_rect.h);
	text_rect.y = btn.rect.y + (btn.rect.h - text_rect.h) / 2;
	text_rect.x = btn.rect.x + (btn.rect.w - text_rect.w) / 2;
	SDL_RenderCopy(renderer, btn.texture, NULL, &text_rect);
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
	text_rect.x = 375;
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
	text_rect.x = 340;
	text_rect.y = 50;
	SDL_RenderCopy(renderer, settings_screen.header, NULL, &text_rect);

	render_button(renderer, settings_screen.btn_to_main);

	render_button(renderer, settings_screen.btn_easy);
	render_button(renderer, settings_screen.btn_mid);
	render_button(renderer, settings_screen.btn_hard);

	SDL_QueryTexture(settings_screen.first_player, NULL, NULL, &text_rect.w, &text_rect.h);
	text_rect.x = 120;
	text_rect.y = 277;
	SDL_RenderCopy(renderer, settings_screen.first_player, NULL, &text_rect);
	render_button(renderer, settings_screen.btn_p1);
	render_button(renderer, settings_screen.btn_p2);

	SDL_QueryTexture(settings_screen.time_limit, NULL, NULL, &text_rect.w, &text_rect.h);
	text_rect.x = 120;
	text_rect.y = 377;
	SDL_RenderCopy(renderer, settings_screen.time_limit, NULL, &text_rect);
	render_input_field(renderer, settings_screen.timelimit_field, context);

	SDL_RenderPresent(renderer);
}


StatusCode ui_render(SDL_Renderer* renderer, ScreenContext context, 
	MainScreen main_screen,
	SettingsScreen settings_screen) {
	switch (context.current_screen) {
	case SCREEN_MAIN:
		ui_render_main(renderer, context, main_screen);
		break;
	case SCREEN_SETTINGS:
		ui_render_settings(renderer, context, settings_screen);
	}

	return SUCCESS;
}