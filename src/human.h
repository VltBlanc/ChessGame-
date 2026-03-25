#ifndef HUMAN_H
#define HUMAN_H

#include "chess.h"

void chess_init(void *memory, unsigned long size);
void chess_move(const chess_state_t *state, move_t *choices, int count, int *index);

#endif // HUMAN_H
