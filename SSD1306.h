#ifndef _SSD1306_H_
#define _SSD1306_H_

void init_ssd1306(void);

void set_ptr_ssd1306(unsigned char col_s, unsigned char col_e,
                     unsigned char page_s, unsigned char page_e);

void send_data_byte_ssd1306(unsigned char data);

#endif