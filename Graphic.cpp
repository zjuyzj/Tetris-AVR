// OLED size in pixel: 128 * 64 | Orientation: vertical
// Left, right and bottom borders: 1 pixel width | White

// Status bar size in pixel: 8 * 64

// Playground (Black):
// Height: 128 - 8 = 120 (Pixel)
// Width: 64 - 2 * 1 = 62 (Pixel)

// Tetris in playground:
// Single tetirs piece: 5 * 5 in pixel
// 62 = 5 * 10 + 11 (black gap between and around 10 pieces) + 1 (extra black gap)
// 120 = 20 * 5 + 20 (black gap between and on the top of 20 pieces)

#include <Arduino.h>
#include "Graphics.h"
#include "SSD1306.h"

// Clear screen and draw borders at the same time
void clear_screen(void) {
    unsigned char column, page;
    set_ptr_ssd1306(0, 127, 0, 7);
    for (column = 0; column < 128; column++) {
        for (page = 0; page < 8; page++) {
            // Draw the left border (continuous)
            if (page == 0) send_data_byte_ssd1306(0x01);
            // Draw the right border (continuous)
            else if (page == 7) send_data_byte_ssd1306(0x80);
            // Clear other part of the screen
            else send_data_byte_ssd1306(0x00); 
        }
    }
    // Draw bottom line: All 8 pages (64 pixels),
    // 1 column at the bottom
    set_ptr_ssd1306(0, 0, 0, 7);
    for (page = 0; page < 8; page++){
        send_data_byte_ssd1306(0xFF);
    }   
}

// (1)(3)(5)(7)
// (0)(2)(4)(6)
// Fixed 4 * 2 next piece indicator
// 8 mini block space, each mini block is 3 pixels * 3 pixels
// Gap is 1 pixel on each mini block's left-hand side and top
// Indicator size: 16 pixels * 8 pixels
const unsigned char piece_mini_block_map[8][8] PROGMEM = {
    {1, 0, 1, 0, 1, 0, 1, 0}, 
    {0, 0, 1, 1, 1, 0, 1, 0},
    {0, 0, 1, 1, 0, 1, 0, 1},
    {0, 0, 0, 0, 1, 1, 1, 1},
    {0, 0, 1, 0, 1, 1, 0, 1},
    {0, 0, 1, 0, 1, 1, 1, 0},
    {0, 0, 0, 1, 1, 1, 1, 0},
    {1, 0, 1, 0, 1, 0, 0, 0}
};

// Merge two mini block into one byte for a column of certain page
unsigned char next_piece_hint_pixel_buf[2][8];

void draw_next_piece_hint(piece_type next_piece_type) {
    unsigned char mini_block_id;
    unsigned char *buffer_ptr;
    const unsigned char *prg_ptr = nullptr;
    memset(next_piece_hint_pixel_buf, 0x00, 2 * 8);
    for(mini_block_id = 0; mini_block_id < 8; mini_block_id++){
        prg_ptr = &piece_mini_block_map[next_piece_type][mini_block_id];
        if(pgm_read_byte(prg_ptr) == 0) continue;
        for(unsigned char i = 0; i < 3; i++){
            buffer_ptr = &next_piece_hint_pixel_buf[mini_block_id >> 2][((mini_block_id & 0x01) << 2) + i];
            *buffer_ptr |= 0x07 << (((mini_block_id >> 1) & 0x01) << 2);
        }
    }
    unsigned char column, page;
    set_ptr_ssd1306(120, 127, 6, 7);
    for (column = 0; column < 8; column++) {
        for (page = 0; page < 2; page++) {
            send_data_byte_ssd1306(next_piece_hint_pixel_buf[page][column]);
        }
    }
}

// Row-wise pixel map (dense bitmap) for score number
// Each number size is 6 * 8 in pixel, two MSBs in the left is always 0
// The first row is always 0x00 to make an extra gap on the top
const unsigned char number_pixel_map[10][8] PROGMEM = {
    {0x00, 0x1c, 0x22, 0x26, 0x2a, 0x32, 0x22, 0x1c},
    {0x00, 0x1c, 0x08, 0x08, 0x08, 0x08, 0x0c, 0x08},
    {0x00, 0x3e, 0x02, 0x04, 0x18, 0x20, 0x22, 0x1c},
    {0x00, 0x1c, 0x22, 0x20, 0x18, 0x20, 0x22, 0x1c},
    {0x00, 0x10, 0x10, 0x3e, 0x12, 0x14, 0x18, 0x10},
    {0x00, 0x1c, 0x22, 0x20, 0x20, 0x1e, 0x02, 0x3e},
    {0x00, 0x1c, 0x22, 0x22, 0x1e, 0x02, 0x04, 0x18},
    {0x00, 0x04, 0x04, 0x04, 0x08, 0x10, 0x20, 0x3e},
    {0x00, 0x1c, 0x22, 0x22, 0x1c, 0x22, 0x22, 0x1c},
    {0x00, 0x0c, 0x10, 0x20, 0x3c, 0x22, 0x22, 0x1c}
};

// In one page, there's only one dight in the middle (6-bit width with two 1 pixel boader)
void draw_score(long score) {
    const unsigned char *prg_ptr = nullptr;
    unsigned char dight, digit_id, column, byte_data;
    for(digit_id = 0 ; digit_id < 6; digit_id++){
        dight = score % 10;
        score /= 10;
        for(column = 0; column < 8; column++){
            prg_ptr = &number_pixel_map[dight][column];
            byte_data = pgm_read_byte(prg_ptr) << 1;
            set_ptr_ssd1306(120 + column, 120 + column,
                            5 - digit_id, 5 - digit_id);
            send_data_byte_ssd1306(byte_data);
        }
    }
}

// Place at most ten 5 pixels * 5 pixels block in a line
// Use all 8 pages in a column (including border pixels)
unsigned char playground_column_pixel_buffer[8];

// Draw blocks in a certain layer
void draw_playground_layer(unsigned char layer, block_status *block_status_layer) {
    block_status block_type;
    unsigned char block_id, left_pix_id;
    unsigned int bit_pattern;
    memset(playground_column_pixel_buffer, 0, 8);
    playground_column_pixel_buffer[0] |= 0x01; // Re-draw (to keep) both border lines
    playground_column_pixel_buffer[7] |= 0x80;
    for(block_id = 0; block_id < 10; block_id++){
        block_type = block_status_layer[block_id];
        if(block_type == NO_BLOCK) continue;
        else if(block_type == BLOCK_TO_CLEAN) continue;
        // left boader line and the first gap take up 2 pixels
        left_pix_id = 2 + block_id * 6;
        bit_pattern = 0b111110 << (left_pix_id % 8);
        playground_column_pixel_buffer[left_pix_id / 8] |= bit_pattern & 0xFF;
        if((bit_pattern & ~0xFF) != 0)
            playground_column_pixel_buffer[left_pix_id / 8 + 1] |= (bit_pattern & ~0xFF) >> 8;
    }
    unsigned char column, page, bottom_column = layer * 6 + 1;
    set_ptr_ssd1306(bottom_column, bottom_column + 4, 0, 7);
    for (column = 0; column < 5; column++)
        for (page = 0; page <= 7; page++)
            send_data_byte_ssd1306(playground_column_pixel_buffer[page]);
}

const unsigned char letter_pixel_map[26 + 2][8] PROGMEM = {
    {0x20, 0x50, 0x88, 0x88, 0xF8, 0x88, 0x88, 0x00}, // 'A'
    {0xF0, 0x88, 0x88, 0xF0, 0x88, 0x88, 0xF0, 0x00}, // 'B'
    {0x70, 0x88, 0x80, 0x80, 0x80, 0x88, 0x70, 0x00}, // 'C'
    {0xF0, 0x88, 0x88, 0x88, 0x88, 0x88, 0xF0, 0x00}, // 'D'
    {0xF8, 0x80, 0x80, 0xF0, 0x80, 0x80, 0xF8, 0x00}, // 'E'
    {0xF8, 0x80, 0x80, 0xF0, 0x80, 0x80, 0x80, 0x00}, // 'F'
    {0x70, 0x88, 0x80, 0x80, 0xB8, 0x88, 0x78, 0x00}, // 'G'
    {0x88, 0x88, 0x88, 0xF8, 0x88, 0x88, 0x88, 0x00}, // 'H'
    {0x70, 0x20, 0x20, 0x20, 0x20, 0x20, 0x70, 0x00}, // 'I'
    {0x38, 0x10, 0x10, 0x10, 0x10, 0x90, 0x60, 0x00}, // 'J'
    {0x88, 0x90, 0xA0, 0xC0, 0xA0, 0x90, 0x88, 0x00}, // 'K'
    {0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0xF8, 0x00}, // 'L'
    {0x88, 0xD8, 0xA8, 0xA8, 0x88, 0x88, 0x88, 0x00}, // 'M'
    {0x88, 0x88, 0xC8, 0xA8, 0x98, 0x88, 0x88, 0x00}, // 'N'
    {0x70, 0x88, 0x88, 0x88, 0x88, 0x88, 0x70, 0x00}, // 'O'
    {0xF0, 0x88, 0x88, 0xF0, 0x80, 0x80, 0x80, 0x00}, // 'P'
    {0x70, 0x88, 0x88, 0x88, 0xA8, 0x90, 0x68, 0x00}, // 'Q'
    {0xF0, 0x88, 0x88, 0xF0, 0xA0, 0x90, 0x88, 0x00}, // 'R'
    {0x70, 0x88, 0x80, 0x70, 0x08, 0x88, 0x70, 0x00}, // 'S'
    {0xF8, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x00}, // 'T'
    {0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x70, 0x00}, // 'U'
    {0x88, 0x88, 0x88, 0x88, 0x88, 0x50, 0x20, 0x00}, // 'V'
    {0x88, 0x88, 0x88, 0xA8, 0xA8, 0xD8, 0x88, 0x00}, // 'W'
    {0x88, 0x88, 0x50, 0x20, 0x50, 0x88, 0x88, 0x00}, // 'X'
    {0x88, 0x88, 0x50, 0x20, 0x20, 0x20, 0x20, 0x00}, // 'Y'
    {0xF8, 0x08, 0x10, 0x20, 0x40, 0x80, 0xF8, 0x00}, // 'Z'
    {0x40, 0x20, 0x10, 0x08, 0x10, 0x20, 0x40, 0x00}, // '>'
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}  // ' '
};

char str_menu[5][8 - 2 + 1] = { "MINI", "Tetris", " EASY", " NORM", " HARD" };
char str_over[2][8 - 2 + 1] = { "Game", "Over" };

unsigned char _swap_byte_bit(unsigned char chr){
    unsigned char mask_left = 0x80, mask_right = 0x01;
    unsigned char result = 0x00;
    for(unsigned char i = 0; i < 8 / 2; i++){
        if(chr & mask_left) result |= mask_right;
        if(chr & mask_right) result |= mask_left;
        mask_left >>= 1;
        mask_right <<= 1;
    }
    return result;
}

void _draw_str_center(char *str, unsigned char start_column){
    unsigned char str_length, page_start, page_end;
    unsigned char chr, chr_bitmap;
    unsigned char column_cnt, page_cnt;
    str_length = strlen(str);
    page_start = (8 - str_length) / 2;
    page_end = page_start + str_length - 1;
    set_ptr_ssd1306(start_column, start_column + 8 - 1, page_start, page_end);
    for (column_cnt = 0; column_cnt < 8; column_cnt++){
        for (page_cnt = 0; page_cnt < str_length; page_cnt++){
            chr = str[page_cnt];
            if(chr >= 'A' && chr <= 'Z') chr -= 'A';
            else if(chr >= 'a' && chr <= 'z') chr -= 'a';
            else if(chr == '>') chr = 26;
            else chr = 27;
            chr_bitmap = pgm_read_byte(&letter_pixel_map[chr][7 - column_cnt]);
            chr_bitmap = _swap_byte_bit(chr_bitmap);
            send_data_byte_ssd1306(chr_bitmap);
        }
    }
}

// Draw a 4-line menu in playground's center
void draw_menu(game_mode selection){
    _draw_str_center(str_menu[0] , 80 + 15);
    _draw_str_center(str_menu[1] , 80);
    for(unsigned char i = 0 ; i < 3; i++){
        if(i == selection) str_menu[2 + i][0] = '>';
        else str_menu[2 + i][0] = ' ';
        _draw_str_center(str_menu[2 + i] , 25 + 15 * (2 - i));
    }
}

void draw_game_over(void){
    _draw_str_center(str_over[0] , 128 - 8 - 50);
    _draw_str_center(str_over[1] , 50);
}