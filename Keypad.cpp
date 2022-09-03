#include "Keypad.h"
#include <Arduino.h>

#define PIN_ANALOG_KEYS A0

#define DEBOUNCE_THRESHOLD 10
#define MAX_WAIT_INTERVAL 300
#define DEC_WAIT_INTERVAL_STEP 70

// Need Fine-Tuning, debuged the program with a multimeter
const int key_voltage_range[4][2] = { {0, 5}, {500, 510}, {324, 330}, {735, 745} };

unsigned long key_timing[4];

unsigned int debounce_counter;

// The longer hold the same key, the shorter update the position tetris piece,
// to make "hold-speedup" effect (but no shorter than a certain interval)

// Wait a holding key in a wait interval, and update piece position after timeout 
bool is_wait_key;
unsigned int key_wait_interval;

void reset_key_state(void){
    debounce_counter = 0;
    is_wait_key = false;
    key_wait_interval = MAX_WAIT_INTERVAL;
}

key_type read_key(void){
    int key_voltage = analogRead(PIN_ANALOG_KEYS);
    unsigned char key_pressed;
    for(key_pressed = KEY_LEFT; key_pressed <= KEY_ROTATE; key_pressed++){
        if(key_voltage < key_voltage_range[key_pressed][0]) continue;
        if(key_voltage > key_voltage_range[key_pressed][1]) continue;
        break;
    }
    if(key_pressed == NO_KEY){ // No key is pressed
        reset_key_state();
        return NO_KEY;
    }
    debounce_counter++;
    if(debounce_counter < DEBOUNCE_THRESHOLD)
        return NO_KEY;
    // Process the key immediately once pressed
    if(!is_wait_key){
        key_timing[key_pressed] = millis();
        is_wait_key = true;
        return (key_type)key_pressed;
    }
    if(millis() > key_timing[key_pressed] + key_wait_interval) {
        is_wait_key = false;
        if(key_wait_interval >= DEC_WAIT_INTERVAL_STEP)
            key_wait_interval -= DEC_WAIT_INTERVAL_STEP;
        return (key_type)key_pressed;
    }
    return NO_KEY;
}