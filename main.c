#include <time.h>
#include <SDL.h>

#include "dict.h"
#include "game_logic.h"
#include "status_codes.h"
#include "common.h"
#include "ai.h"
#include "ui.h"


int main() {
	SetConsoleCP(1251);
	SetConsoleOutputCP(1251);
	setlocale(LC_ALL, "Russian");


	srand(time(NULL));

	StatusCode code;

	Game* game = NULL;

	GameSettings* settings = game_init_settings();
	if (settings == NULL) {
		return 1;
	}
	Dictionary* dict = dict_init(false);
	if (dict == NULL) {
		return 1;
	}
	Leaderboard* lb = game_leaderboard_init();
	if (lb == NULL) {
		return 1;
	}
	AIState* state = ai_state_init();
	if (state == NULL) {
		return 1;
	}

	SDL_Window* window = NULL;
	SDL_Renderer* renderer = NULL;
	SDL_Texture* Texture = NULL;

	ScreenContext context;
	context.current_screen = SCREEN_MAIN;

	if (ui_init(&window, &renderer) != SUCCESS) {
		SDL_Log("Unable to initialize program!\n");
		exit(1);
	}


	MainScreen main_screen;
	SettingsScreen sett_screen;

	if (ui_set_screen_context(renderer, &context, &main_screen, &sett_screen, settings) != SUCCESS) {
		return 1;
	}

	bool f = true;

	while (f) {
		if (ui_handle_events(renderer, context, &main_screen, &sett_screen) == UI_QUIT) break;
		ui_update_logic(&context, &main_screen, &sett_screen, settings, &f);
		ui_render(renderer, context, main_screen, sett_screen);
	}

	return 0;
}