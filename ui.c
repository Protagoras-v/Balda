#include <SDL.h>
#include <SDL_ttf.h>

#include "common.h"
#include "status_codes.h"
#include "ui.h"

#define WHITE 255, 255, 255, 255
#define BLACK 0, 0, 0, 0
#define BLUE 0, 94, 102, 0
#define GRAY 184, 184, 184, 0
#define GRAY_HOVERED 222, 222, 222, 0


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
		printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
		return SDL_ERROR;
	}
	else {
		*window = SDL_CreateWindow("Balda", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
		if (*window == NULL) {
			printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
			return SDL_ERROR;
		}
		else {
			*renderer = SDL_CreateRenderer(*window, -1, SDL_RENDERER_ACCELERATED);
			if (*renderer == NULL) {
				printf("ERRROR RENDERER: %s\n", SDL_GetError());
				return SDL_ERROR;
			}
			else {
				SDL_SetRenderDrawColor(*renderer, 0xff, 0xff, 0xff, 0xff);
				//включаем альфа-канал
				//SDL_SetRenderDrawBlendMode(*renderer, SDL_BLENDMODE_BLEND);

				if (TTF_Init() == -1) {
					printf("SDL_TTF could not initialize! SDL_TTF Error: %s\n", TTF_GetError());
					return SDL_ERROR;
				}
			}

		}
	}

	return SUCCESS;
}


StatusCode ui_set_screen_context(SDL_Renderer* renderer, ScreenContext* context, MainScreen* main_screen) {
	context->btn_font = TTF_OpenFont(BTN_FONT_FILENAME, 22);
	if (context->btn_font == NULL) {
		fprintf(stderr, "Ошибка при открытии шрифта %s, TTF_Error: %s\n", BTN_FONT_FILENAME, TTF_GetError());
		return SDL_OPEN_FONT_ERROR;
	}

	context->header_font = TTF_OpenFont(HEADER_FONT_FILENAME, 34);
	if (context->btn_font == NULL) {
		fprintf(stderr, "Ошибка при открытии шрифта %s, TTF_Error: %s\n", HEADER_FONT_FILENAME, TTF_GetError());
		return SDL_OPEN_FONT_ERROR;
	}

	//main menu
	main_screen->header = createTextureFromText(renderer, context->header_font, "Балда");

	strncpy_s(main_screen->btn_main_new_game.text, MAX_UI_BUFFER_SIZE, "Новая игра", MAX_UI_BUFFER_SIZE);
	main_screen->btn_main_new_game.rect.x = 312;
	main_screen->btn_main_new_game.rect.y = 150;
	main_screen->btn_main_new_game.rect.w = 275;
	main_screen->btn_main_new_game.rect.h = 75;
	main_screen->btn_main_new_game.isActive = 0;
	main_screen->btn_main_new_game.isHoverable = 1;
	main_screen->btn_main_new_game.isHovered = 0;
	main_screen->btn_main_new_game.isClicked = 0;
	main_screen->btn_main_new_game.texture = createTextureFromText(renderer, context->btn_font, main_screen->btn_main_new_game.text);
	if (main_screen->btn_main_new_game.texture == NULL) {
		return SDL_TEXTURE_ERROR;
	}
	
	strncpy_s(main_screen->btn_main_load_game.text, MAX_UI_BUFFER_SIZE,  "Загрузить", MAX_UI_BUFFER_SIZE);
	main_screen->btn_main_load_game.rect.x = 312;
	main_screen->btn_main_load_game.rect.y = 250;
	main_screen->btn_main_load_game.rect.w = 275;
	main_screen->btn_main_load_game.rect.h = 75;
	main_screen->btn_main_load_game.isActive = 0;
	main_screen->btn_main_load_game.isHoverable = 1;
	main_screen->btn_main_load_game.isHovered = 0;
	main_screen->btn_main_load_game.isClicked = 0;
	main_screen->btn_main_load_game.texture = createTextureFromText(renderer, context->btn_font, main_screen->btn_main_load_game.text);
	if (main_screen->btn_main_load_game.texture == NULL) {
		return SDL_TEXTURE_ERROR;
	}

	strncpy_s(main_screen->btn_main_to_settings.text, MAX_UI_BUFFER_SIZE, "Настройки", MAX_UI_BUFFER_SIZE);
	main_screen->btn_main_to_settings.rect.x = 312;
	main_screen->btn_main_to_settings.rect.y = 350;
	main_screen->btn_main_to_settings.rect.w = 275;
	main_screen->btn_main_to_settings.rect.h = 75;
	main_screen->btn_main_to_settings.isActive = 0;
	main_screen->btn_main_to_settings.isHoverable = 1;
	main_screen->btn_main_to_settings.isHovered = 0;
	main_screen->btn_main_to_settings.isClicked = 0;
	main_screen->btn_main_to_settings.texture = createTextureFromText(renderer, context->btn_font, main_screen->btn_main_to_settings.text);
	if (main_screen->btn_main_to_settings.texture == NULL) {
		return SDL_TEXTURE_ERROR;
	}

	strncpy_s(main_screen->btn_main_to_leaderboard.text, MAX_UI_BUFFER_SIZE, "Таблица лидеров", MAX_UI_BUFFER_SIZE);
	main_screen->btn_main_to_leaderboard.rect.x = 312;
	main_screen->btn_main_to_leaderboard.rect.y = 450;
	main_screen->btn_main_to_leaderboard.rect.w = 275;
	main_screen->btn_main_to_leaderboard.rect.h = 75;
	main_screen->btn_main_to_leaderboard.isActive = 0;
	main_screen->btn_main_to_leaderboard.isHoverable = 1;
	main_screen->btn_main_to_leaderboard.isHovered = 0;
	main_screen->btn_main_to_leaderboard.isClicked = 0;
	main_screen->btn_main_to_leaderboard.texture = createTextureFromText(renderer, context->btn_font, main_screen->btn_main_to_leaderboard.text);
	if (main_screen->btn_main_to_leaderboard.texture == NULL) {
		return SDL_TEXTURE_ERROR;
	}

	strncpy_s(main_screen->btn_main_exit.text, MAX_UI_BUFFER_SIZE, "Выход", MAX_UI_BUFFER_SIZE);
	main_screen->btn_main_exit.rect.x = 312;
	main_screen->btn_main_exit.rect.y = 550;
	main_screen->btn_main_exit.rect.w = 275;
	main_screen->btn_main_exit.rect.h = 75;
	main_screen->btn_main_exit.isActive = 0;
	main_screen->btn_main_exit.isHoverable = 1;
	main_screen->btn_main_exit.isHovered = 0;
	main_screen->btn_main_exit.isClicked = 0;
	main_screen->btn_main_exit.texture = createTextureFromText(renderer, context->btn_font, main_screen->btn_main_exit.text);
	if (main_screen->btn_main_exit.texture == NULL) {
		return SDL_TEXTURE_ERROR;
	}

	return SUCCESS;
}


//--------------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------MAIN MENU EVENTS---------------------------------------------------------
//--------------------------------------------------------------------------------------------------------------------------

void check_button_hovered(SDL_Event e, Button* btn) {
	if ((e.motion.x >= btn->rect.x && e.motion.x <= btn->rect.x + btn->rect.w)
		&& (e.motion.y >= btn->rect.y && e.motion.y <= btn->rect.y + btn->rect.h)
		&& btn->isHoverable == 1) {
		btn->isHovered = 1;
	}
	else {
		btn->isHovered = 0;
	}
}

void event_mainmenu_mousemotion(SDL_Event e, MainScreen* main_screen) {
	check_button_hovered(e, &main_screen->btn_main_new_game);
	check_button_hovered(e, &main_screen->btn_main_load_game);
	check_button_hovered(e, &main_screen->btn_main_to_settings);
	check_button_hovered(e, &main_screen->btn_main_to_leaderboard);
	check_button_hovered(e, &main_screen->btn_main_exit);
}

void event_mainmenu_mouseclick_down(SDL_Event e, ScreenContext* context, MainScreen* main_screen) {
	if (main_screen->btn_main_new_game.isHovered) {
		main_screen->btn_main_new_game.isHovered = 0;
		//start new game
	}
	else if (main_screen->btn_main_load_game.isHovered) {
		main_screen->btn_main_load_game.isHovered = 0;
		//choice the file
	}
	else if (main_screen->btn_main_to_settings.isHovered) {
		main_screen->btn_main_to_settings.isHovered = 0;
		//settings
	}
	else if (main_screen->btn_main_to_leaderboard.isHovered) {
		main_screen->btn_main_to_leaderboard.isHovered = 0;
		//open lb
	}
	else if (main_screen->btn_main_exit.isHovered) {
		main_screen->btn_main_exit.isHovered = 0;
		main_screen->btn_main_exit.isClicked = 1;
		
		//exit
	}
}




StatusCode ui_handle_events(SDL_Window* window, SDL_Renderer* render, ScreenContext* context, MainScreen* main_screen, bool* f) {
	SDL_Event e;
	while (SDL_PollEvent(&e)) {
		if (e.type == SDL_QUIT) {
			*f = false;
		}
		else {
			switch (context->current_screen) {
			case SCREEN_MAIN:
				if (e.type == SDL_MOUSEMOTION) {
					event_mainmenu_mousemotion(e, main_screen);
				}
				else if (e.type == SDL_MOUSEBUTTONDOWN) {
					printf("click\n");
					event_mainmenu_mouseclick_down(e, context, main_screen);
				}
				if (main_screen->btn_main_exit.isClicked) {
					*f = false;
				}
				break;
			case SCREEN_SETTINGS:
				break;
			case SCREEN_LEADERBOARD:
				break;
			case SCREEN_GAME:
				break;
			}
		}
	}
}

static void render_button(SDL_Renderer* renderer, Button btn) {
	if (btn.isHovered)
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


void static ui_render_main(SDL_Window* window, SDL_Renderer* renderer, ScreenContext context, MainScreen main_screen) {
	SDL_SetRenderDrawColor(renderer, WHITE);
	SDL_RenderClear(renderer);

	SDL_Rect text_rect;
	SDL_QueryTexture(main_screen.header, NULL, NULL, &text_rect.w, &text_rect.h);
	text_rect.x = 375;
	text_rect.y = 50;
	SDL_RenderCopy(renderer, main_screen.header, NULL, &text_rect);

	render_button(renderer, main_screen.btn_main_new_game);
	render_button(renderer, main_screen.btn_main_load_game);
	render_button(renderer, main_screen.btn_main_to_settings);
	render_button(renderer, main_screen.btn_main_to_leaderboard);
	render_button(renderer, main_screen.btn_main_exit);

	SDL_RenderPresent(renderer);
}


StatusCode ui_render(SDL_Window* window, SDL_Renderer* renderer, ScreenContext context, MainScreen main_screen) {
	switch (context.current_screen) {
	case SCREEN_MAIN:
		ui_render_main(window, renderer, context, main_screen);
	}
}