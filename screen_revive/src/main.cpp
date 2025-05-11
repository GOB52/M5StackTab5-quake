#include <Arduino.h>
#include <M5GFX.h>

M5GFX display;

void setup()
{
    display.begin();
    display.clear(TFT_DARKGREEN);
}

void loop(){
    delay(1000);
}
