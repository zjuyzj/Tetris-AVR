#include <Arduino.h>
#include "SSD1306.h"
#include "Game.h"

#define PIN_SEED_NOISE 7

void setup() {
    init_ssd1306();
    randomSeed(analogRead(PIN_SEED_NOISE));
}

void loop() {
    reset_game();
    while(step_game());
    while(1);
}