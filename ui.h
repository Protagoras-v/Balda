#pragma once

typedef enum {
	SCREEN_MAIN,
	SCREEN_SETTINGS,
	SCREEN_GAME,
	SCREEN_LEADERBOARD,
} AppState;


StatusCode ui_init(SDL_Window** window, SDL_Renderer** renderer);

StatusCode ui_handleh_events(SDL_Window* window, SDL_Renderer* renderer, AppState* screen, bool* f);