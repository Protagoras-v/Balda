#pragma once

#include <SDL.h>
#include <SDL_ttf.h>
#include "common.h"

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

	unsigned int isHoverable : 1;
	unsigned int isHovered : 1;
	unsigned int isClicked : 1;
	unsigned int isActive : 1;
} Button;

typedef struct MainScreen {
	SDL_Texture* header;
	Button btn_main_new_game;
	Button btn_main_load_game;
	Button btn_main_to_settings;
	Button btn_main_to_leaderboard;
	Button btn_main_exit;
} MainScreen;

typedef struct ScreenContext {
	TTF_Font* btn_font;
	TTF_Font* header_font;
	Screen current_screen;
} ScreenContext;


StatusCode ui_init(SDL_Window** window, SDL_Renderer** renderer);

StatusCode ui_handle_events(SDL_Window* window, SDL_Renderer* render, ScreenContext* context, MainScreen* main_screen, bool* f);

StatusCode ui_render(SDL_Window* window, SDL_Renderer* renderer, ScreenContext context, MainScreen main_screen);

StatusCode ui_set_screen_context(SDL_Renderer* renderer, ScreenContext* context, MainScreen* main_screen);