#ifndef _GRAPHICS_H_
#define _GRAPHICS_H_

typedef enum {TYPE_I = 0, TYPE_J, TYPE_L, TYPE_O, TYPE_S, TYPE_T, TYPE_Z, TYPE_SHORT_I} piece_type;
typedef enum {NO_BLOCK = 0, BLOCK_INACTIVE, BLOCK_TO_DRAW, BLOCK_TO_CLEAN} block_status;
typedef enum {EASY, NORMAL, HARD} game_mode;

void clear_screen(void);
void draw_score(long score);
void draw_next_piece_hint(piece_type next_piece_type);
void draw_playground_layer(unsigned char layer, block_status *block_status_layer);
void draw_menu(game_mode selection);
void draw_game_over(void);

#endif