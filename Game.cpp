#include <Arduino.h>
#include "Graphics.h"
#include "Keypad.h"

#define DEFAULT_DROP_INTERVAL 500 // Unit: millisecond

static struct {
    bool is_started;
    game_mode mode;
    unsigned int score, award_factor;
    unsigned int line_eliminated;
    unsigned long start_time;
    unsigned long drop_interval;
    piece_type next_piece_type;
} game_status;

// There are 2 (NUM_OF_BLOCK_FOR_PIECE_ENVELOPE / 2) invisible blocks on the left
// and on the right, marked with BLOCK_INACTIVE (the same as blocks that already landed),
// to simplify the collision detection for the leftmost and rightmost pieces when doing rotation. 

// As the same way, there are 1 dummy block at the bottom as playground's bottom border, 
// and 4 dummy blocks at the top (NUM_OF_BLOCK_FOR_PIECE_ENVELOPE) to detect "block overflow" (game-over)
static block_status playground_block_buffer[1 + 20 + 4][2 + 10 + 2];

void _init_playground(void){
    block_status status;
    unsigned char x, y;
    for (y = 0; y < 24; y++){
        for (x = 0; x < 14; x++){
            if(y == 0) status = BLOCK_INACTIVE;
            else if((x < 2) && (y < 1 + 20))
                status = BLOCK_INACTIVE;
            else if((x >= 2 + 10) && (y < 1 + 20))
                status = BLOCK_INACTIVE;
            else status = NO_BLOCK;
            playground_block_buffer[y][x] = status;
        }
    }
}

void _draw_playground(void) {
    unsigned char x, y;
    block_status block_status_layer[10], block;
    for(y = 1; y < 21; y++){
        // Scan the layer to find if there's need to update the screen
        for(x = 2; x < 12; x++){
            block = playground_block_buffer[y][x];
            if((block == BLOCK_TO_CLEAN) ||
               (block == BLOCK_TO_DRAW)) break;
        }
        if(x == 13) continue; // This layer needn't to redraw
        for(x = 2; x < 12; x++){
            block_status_layer[x - 2] = playground_block_buffer[y][x];
            // Block that clean from screen is no needed to update next frame
            if(playground_block_buffer[y][x] == BLOCK_TO_CLEAN)
                playground_block_buffer[y][x] = NO_BLOCK;
        }
        draw_playground_layer(y - 1, block_status_layer);
    }
}

// NUM_OF_BLOCK_FOR_PIECE_ENVELOPE = MAX(NUM_OF_BLOCK_FOR_PIECES) ^ 2
// Thus, rotation operation can be done in this envelope
// Column-wise block map (sparce bitmap) for basic tetris piece in playground
// Corresponding piece: I, J, L, O, S, T, Z, Short I
// Each block is 5 * 5 size in pixel
const unsigned char piece_block_map[8][16] = {
    {0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0},
    {0, 1, 0, 0, 0, 1, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0},
    {0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 1, 1, 0, 1, 1, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 1, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 1, 1, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0},
    {0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0}
};

typedef struct {
    unsigned char block_map[4][4];
    char pos_x, pos_y; // pos_y may be minus
} piece_info;

piece_info piece_active, piece_backup;

void _write_piece_to_playground(piece_info *piece, block_status status){
    char piece_x, piece_y;
    unsigned char offset_x, offset_y;
    unsigned char playground_x, playground_y;
    piece_x = piece->pos_x;
    piece_y = piece->pos_y;
    for(offset_y = 0; offset_y < 4; offset_y++){
        for(offset_x = 0; offset_x < 4; offset_x++){
            playground_x = piece_x + offset_x;
            playground_y = piece_y + offset_y;
            if(piece->block_map[offset_x][offset_y])
                playground_block_buffer[playground_y][playground_x] = status;
        }
    }
}

void _update_piece_to_playground(bool to_inactive) {
    if(!to_inactive){
        _write_piece_to_playground(&piece_backup, BLOCK_TO_CLEAN);
        _write_piece_to_playground(&piece_active, BLOCK_TO_DRAW);
    }else{
        _write_piece_to_playground(&piece_active, BLOCK_INACTIVE);
    }
}

// Check collision between active piece and the playground part
bool _check_collision(void) {
    char piece_x, piece_y;
    unsigned char offset_x, offset_y;
    unsigned char playground_x, playground_y;
    piece_x = piece_active.pos_x;
    piece_y = piece_active.pos_y;
    for(offset_y = 0; offset_y < 4; offset_y++){
        for(offset_x = 0; offset_x < 4; offset_x++){
            if(!piece_active.block_map[offset_x][offset_y]) continue;
            playground_x = piece_x + offset_x;
            playground_y = piece_y + offset_y;
            if(playground_block_buffer[playground_y][playground_x] == BLOCK_INACTIVE)
                return true;
        }
    }
    return false;
}

void _rotate_piece_active(void){
    unsigned char i, j, rotated_block_map[4][4] = {0};
    for (i = 0; i < 4; i++)
        for (j = 0; j < 4; j++)
            rotated_block_map[j][i] = piece_active.block_map[4 - i - 1][j];
    memcpy(piece_active.block_map, rotated_block_map, 16);
}

void _load_new_piece(piece_type piece_type_to_load, bool is_on_screen) {
    memcpy(piece_active.block_map, &piece_block_map[piece_type_to_load][0], 16);
    // Piece default generation position: (5, 20) 
    piece_active.pos_x = 5;
    piece_active.pos_y = 20;
    if (is_on_screen) {
        piece_backup = piece_active;
        _write_piece_to_playground(&piece_active, BLOCK_TO_DRAW);
    }
}

void _set_game_mode(game_mode mode){
    game_status.drop_interval = DEFAULT_DROP_INTERVAL - 150 * mode;
    game_status.award_factor = mode;
}

static const unsigned int score_lut[4] = {40, 100, 300, 1200};
void _update_score(unsigned char line_eliminated_once){
    game_status.line_eliminated += line_eliminated_once;
    if(line_eliminated_once > 4) line_eliminated_once = 4;
    game_status.score += score_lut[line_eliminated_once - 1] * (game_status.award_factor + 1);
    if (game_status.line_eliminated % 10 == 0) {
        game_status.award_factor++;
        if(game_status.drop_interval <= 50)
            game_status.drop_interval = 50;
        else game_status.drop_interval -= 10;
    }
}

void _process_inactive_line() {
    unsigned char first_layer_to_check, last_layer_to_check;
    unsigned char layer_idx, block_idx;
    unsigned char line_eliminated_once = 0;
    
    // Step 1: Clean all completed lines (may not continous) caused by this landed block
    // From the bottom-left corner of the landed piece,
    // i.e. the y_pos of piece_active to get the first layer to check,
    // and noted that we only check visiable layer
    if (piece_active.pos_y < 1) first_layer_to_check = 1;
    else first_layer_to_check = piece_active.pos_y;
    last_layer_to_check = piece_active.pos_y + 4;
    for(layer_idx = first_layer_to_check; layer_idx < last_layer_to_check; layer_idx++) {
        for(block_idx = 2; block_idx < 12; block_idx++)
            if(playground_block_buffer[layer_idx][block_idx] == NO_BLOCK) break;
        if (block_idx < 12) continue;
        // This line is fully filled and ready to be eliminated
        for (block_idx = 2; block_idx < 12; block_idx++)
            playground_block_buffer[layer_idx][block_idx] = BLOCK_TO_CLEAN;
        first_layer_to_check = layer_idx + 1;
        line_eliminated_once++;
        _draw_playground();
    }

    // Step 2: Update the score
    _update_score(line_eliminated_once);
    draw_score(game_status.score);

    // Step 3: Move all remaining inactive pieces down 
    // to filled the blank(s) caused by eliminated layer(s)
    // The whole playground may need to be moved downwards
    unsigned char layer_to_copy_idx;
    for(; line_eliminated_once > 0; line_eliminated_once--){
        // We will find all the eliminated layer(s) again, from bottom to top
        for (layer_idx = 1; layer_idx < 20; layer_idx++) {
            for(block_idx = 2; block_idx < 12; block_idx++)
                if (playground_block_buffer[layer_idx][block_idx] != NO_BLOCK) break;
            if(block_idx < 12) continue; // This layer is not an eliminated layer
            // Copy the content at the top of the eliminated layer to fill this empty layer
            for (layer_to_copy_idx = layer_idx + 1; layer_to_copy_idx < 20; layer_to_copy_idx++) {
                for (block_idx = 2; block_idx < 12; block_idx++) {
                    if (playground_block_buffer[layer_to_copy_idx][block_idx] == BLOCK_TO_DRAW ||
                        playground_block_buffer[layer_to_copy_idx][block_idx] == BLOCK_INACTIVE)
                        playground_block_buffer[layer_to_copy_idx - 1][block_idx] = BLOCK_TO_DRAW;
                    else playground_block_buffer[layer_to_copy_idx - 1][block_idx] = BLOCK_TO_CLEAN;
                }
            }
        }
        // Make sure all active blocks become to inactive blocks, since all 
        // these blocks in the scene will become "the landed part" finally
        for (char r = 1; r < 24; r++) {
            for (char c = 2; c < 12; c++) {
                if (playground_block_buffer[r][c] == BLOCK_TO_DRAW)
                    playground_block_buffer[r][c] = BLOCK_INACTIVE;
            }
        }
        _draw_playground();
    }
}

bool _process_movement(bool is_movement_down){
    if (_check_collision()) {
        piece_active = piece_backup;
        // Piece touch-down with or without overflow
        if(is_movement_down){
            // In both cases, update the current piece to BLOCK_INACTIVE
            _update_piece_to_playground(true);
            // Find possible completed line(s), remove it(them),
            // and calculated the score and update difficulty
            _process_inactive_line();
            // Load a new tetris piece off-screen to detect overflow
            // Off-screen: not written to playground_block_buffer
            _load_new_piece(game_status.next_piece_type, false);
            // Piece overflow, then game is over
            if (_check_collision()){
                draw_game_over();
                return false;
            }
            // No overflow, then reload the new tetris on-screen
            _load_new_piece(game_status.next_piece_type, true);
            // Reset key status to avoid unexpected
            // holding-key speed-up for newly created piece
            reset_key_state(); 
            game_status.next_piece_type = (piece_type)random(0, 8);
            draw_next_piece_hint(game_status.next_piece_type);
        }
    }else{
        _update_piece_to_playground(false);
        _draw_playground();
    }
    return true;
}

void reset_game(void) {
    game_status.is_started = false;
    game_status.mode = EASY;
    game_status.next_piece_type = (piece_type)random(0, 8);
    game_status.score = 0;
    game_status.line_eliminated = 0;
    clear_screen();
    draw_score(game_status.score);
    draw_next_piece_hint(game_status.next_piece_type);
    draw_menu(game_status.mode);
}

bool step_game(void){
    key_type key = read_key();
    if(game_status.is_started){
        if(millis() - game_status.start_time > game_status.drop_interval){
            game_status.start_time = millis();
            piece_backup = piece_active;
            piece_active.pos_y -= 1;
            return _process_movement(true);
        }else if(key != NO_KEY){
            bool is_movement_down = false;
            piece_backup = piece_active;
            if(key == KEY_LEFT){
                piece_active.pos_x -= 1;
            }else if(key == KEY_RIGHT){
                piece_active.pos_x += 1;
            }else if(key == KEY_DOWN){
                piece_active.pos_y -= 1;
                is_movement_down = true;
            }else if(key == KEY_ROTATE){
                _rotate_piece_active();
            }
            return _process_movement(is_movement_down);
        }else return true;
    }else{
        if(key == NO_KEY) return true;
        if(key == KEY_DOWN){
            if(game_status.mode == HARD)
                game_status.mode = EASY;
            else game_status.mode = (game_mode)(game_status.mode + 1);
            draw_menu(game_status.mode);    
        }else if(key == KEY_ROTATE){
            _set_game_mode(game_status.mode);
            game_status.is_started = true;
            game_status.start_time = millis();
            clear_screen();
            draw_score(game_status.score);
            draw_next_piece_hint(game_status.next_piece_type);
            _init_playground();
            _load_new_piece((piece_type)random(0, 8), true);
            _draw_playground();
        }
        return true;
    }
}