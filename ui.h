#pragma once

#include <SDL.h>
#include "common.h"

typedef enum {
	SCREEN_MAIN,
	SCREEN_SETTINGS,
	SCREEN_GAME,
	SCREEN_LEADERBOARD,
} Screen;

typedef struct Button {
	char text[MAX_WORD_LEN];
	SDL_Rect rect;
	SDL_Texture* textTexture;

	unsigned int isHoverable : 1;
	unsigned int isHovered : 1;
	unsigned int isClicked : 1;
	unsigned int isActive : 1;
} Button;

typedef struct ScreenContext {
	//main menu
	Button btn_main_new_game;
	Button btn_main_load_game;
	Button btn_main_to_settings;
	Button btn_main_to_leaderboard;
	Button btn_main_exit;

} ScreenContext;


StatusCode ui_init(SDL_Window** window, SDL_Renderer** renderer);

StatusCode ui_handle_events(SDL_Window* window, SDL_Renderer* renderer, Screen* screen, ScreenContext* context, bool* f);

StatusCode ui_set_screen_context(ScreenContext* context);