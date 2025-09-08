#include <SDL.h>
#include <SDL_ttf.h>

#include "common.h"
#include "status_codes.h"

StatusCode init(SDL_Window** window, SDL_Renderer** renderer) {
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