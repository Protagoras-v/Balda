#include <SDL.h>
#include <SDL_ttf.h>

#include "common.h"
#include "status_codes.h"
#include "ui.h"


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
				SDL_SetRenderDrawBlendMode(*renderer, SDL_BLENDMODE_BLEND);

				if (TTF_Init() == -1) {
					printf("SDL_TTF could not initialize! SDL_TTF Error: %s\n", TTF_GetError());
					return SDL_ERROR;
				}
			}

		}
	}

	return SUCCESS;
}


StatusCode ui_set_screen_context(ScreenContext* context) {
	//main menu
	context->btn_main_new_game.rect.x = 375;
	context->btn_main_new_game.rect.y = 150;
	context->btn_main_new_game.rect.w = 150;
	context->btn_main_new_game.rect.h = 100;

	context->btn_main_load_game.rect.x = 375;
	context->btn_main_load_game.rect.y = 175;
	context->btn_main_load_game.rect.w = 150;
	context->btn_main_load_game.rect.h = 100;

	context->btn_main_to_settings.rect.x = 375;
	context->btn_main_to_settings.rect.y = 200;
	context->btn_main_to_settings.rect.w = 150;
	context->btn_main_to_settings.rect.h = 100;

	context->btn_main_to_leaderboard.rect.x = 375;
	context->btn_main_to_leaderboard.rect.y = 225;
	context->btn_main_to_leaderboard.rect.w = 150;
	context->btn_main_to_leaderboard.rect.h = 100;
}


//--------------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------MAIN MENU EVENTS---------------------------------------------------------
//--------------------------------------------------------------------------------------------------------------------------

void event_mainmenu_mousemotion(SDL_Event e, ScreenContext* context) {
	int i = 1;
}

void event_mainmenu_mouseclick_down(SDL_Event e, Screen* screen, ScreenContext* context) {
	int i = 1;
}




StatusCode ui_handle_events(SDL_Window* window, SDL_Renderer* render, Screen* screen, ScreenContext* context, bool* f) {
	SDL_Event e; 
	while (SDL_PollEvent(&e)) {
		if (e.type == SDL_QUIT) {
			*f = false;
		}
		else {
			switch (*screen) {
			case SCREEN_MAIN:
				if (e.type == SDL_MOUSEMOTION) {
					event_mainmenu_mousemotion(e, context);
				}
				else if (e.type == SDL_MOUSEBUTTONDOWN) {
					event_mainmenu_mouseclick_down(e, screen, context);
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