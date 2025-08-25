#pragma once
#include <process.h>
#include "game_logic.h"

typedef struct AIState AIState;

AIState* ai_state_init();

StatusCode ai_start_turn(Game* game, AIState* state, Dictionary* dict);

StatusCode ai_set_start(AIState* state, bool n);

StatusCode ai_give_additional_time(AIState* state, bool n, int additional_time);


bool ai_status(AIState* state);

bool ai_need_additional_time(AIState* state);

bool ai_word_founded(AIState* state);

int ai_get_percentage(AIState* state);

HANDLE ai_get_handle(AIState* state);

Move* ai_get_move(AIState* state);
