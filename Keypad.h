#ifndef _KEYPAD_H_
#define _KEYPAD_H_

typedef enum {KEY_LEFT = 0, KEY_RIGHT, KEY_DOWN, KEY_ROTATE, NO_KEY} key_type;

key_type read_key(void);
void reset_key_state(void);

#endif