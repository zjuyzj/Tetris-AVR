#include <Wire.h>

#define I2C_SPEED 400000

// Address Byte: 0111 10(SA0)(R/W#)
// I2C Slave Address: 011110 (b7-b2), fixed for SSD1306
// b1 is SA0 set by D/C# pin (data/command selection pin under SPI)
// b0 is R/W# control bit, always 0 for write operation in this file
#define SSD1306_I2C_ADDR_BYTE 0x3C

// Control Byte: (Co)(D/C#)00 0000
// Co is continuation bit, 0 for data and 1 for continuous command
#define SSD1306_CTRL_BYTE_CMD 0x80
#define SSD1306_CTRL_BYTE_DATA 0x40

#define SSD1306_CMD_DISP_OFF 0xAE
#define SSD1306_CMD_DISP_ON 0xAF

#define SSD1306_CMD_DISP_NORM 0xA6
#define SSD1306_CMD_DISP_REV 0xA7

#define SSD1306_CMD_SET_COL_ADDR 0x21
#define SSD1306_CMD_SET_PAGE_ADDR 0x22

// Double bytes command
#define SSD1306_CMD_SET_CONTRAST 0x81
// and the second byte is 8-bit contrast value

// Double bytes command
#define SSD1306_CMD_SET_VRAM_ADDR_MODE 0x20
// The second byte
#define SSD1306_CMD_SET_VRAM_ADDR_MODE_H 0x00
#define SSD1306_CMD_SET_VRAM_ADDR_MODE_V 0x01
#define SSD1306_CMD_SET_VRAM_ADDR_MODE_PAGE 0x02

// Double bytes command
#define SSD1306_CMD_SET_CHARGE_PUMP 0x8D
// The second byte
#define SSD1306_CMD_SET_CHARGE_PUMP_ENABLE 0x14

void _init_i2c(void) {
    Wire.begin();
    Wire.setClock(I2C_SPEED);
}

void _send_cmd(unsigned char cmd) {
    Wire.beginTransmission(SSD1306_I2C_ADDR_BYTE);
    Wire.write(SSD1306_CTRL_BYTE_CMD);
    Wire.write(cmd);
    Wire.endTransmission();
}

void init_ssd1306(void) {
    _init_i2c();
    _send_cmd(SSD1306_CMD_DISP_OFF);
    _send_cmd(SSD1306_CMD_DISP_NORM);
    _send_cmd(SSD1306_CMD_SET_CHARGE_PUMP);
    _send_cmd(SSD1306_CMD_SET_CHARGE_PUMP_ENABLE);
    _send_cmd(SSD1306_CMD_DISP_ON);
}

// Note that screen orientation is vertical
void set_ptr_ssd1306(unsigned char col_s, unsigned char col_e, unsigned char page_s, unsigned char page_e) {
    _send_cmd(SSD1306_CMD_SET_VRAM_ADDR_MODE);
    _send_cmd(SSD1306_CMD_SET_VRAM_ADDR_MODE_V);
    _send_cmd(SSD1306_CMD_SET_COL_ADDR);
    _send_cmd(col_s);
    _send_cmd(col_e);
    _send_cmd(SSD1306_CMD_SET_PAGE_ADDR);
    _send_cmd(page_s);
    _send_cmd(page_e);
}

void send_data_byte_ssd1306(unsigned char data) {
    Wire.beginTransmission(SSD1306_I2C_ADDR_BYTE);
    Wire.write(SSD1306_CTRL_BYTE_DATA);
    Wire.write(data);
    Wire.endTransmission();
}
