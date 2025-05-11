// Porting for M5Stack Tab5 by GOB (X:@GOB_52_GOB)
#include <M5Unified.h>
#include <M5GFX.h>
#include "quakekeys.h"
#include "quakedef.h"
#include "esp_log.h"
#include <array>

static const char TAG[] = "TCH";

struct Button
{
    Button(const uint8_t qkey,const int16_t l, const int16_t t, const int16_t w, const int16_t h) :
            _key{qkey}, _left{l}, _top{t}, _right{_left + w}, _bottom{_top + h} {}

    void update(const m5gfx::touch_point_t* tps, const uint8_t size) {
        auto prev = _pressed;
        _pressed = _wasPressed = _wasReleased = false;

        for(uint_fast8_t i = 0; i < size; ++i) {
            if(tps[i].x >= _left && tps[i].x < _right &&
               tps[i].y >= _top && tps[i].y < _bottom) {
                _pressed = true;
                _wasPressed = !prev;
            }
        }
        _wasReleased = prev && !_pressed;
    }
    inline uint8_t key() const { return _key; }
    inline bool wasPressed() const { return _wasPressed; }
    inline bool wasReleasedd() const { return _wasReleased; }

    uint8_t _key{};
    int16_t _left{}, _top{}, _right{}, _bottom{};
    bool _pressed{}, _wasPressed{},_wasReleased{};
};

struct VirtualButton {
    VirtualButton(const int16_t x, const int16_t y) : _ox{x}, _oy{y} {}

    static std::array<Button,10> _table;
    
    bool update(int*key, int* down, m5gfx::touch_point_t* tps, const uint8_t size) {
        for(uint_fast8_t i=0;i<size;++i) {
            tps[i].x -= _ox;
            tps[i].y -= _oy;
        }

        for(auto&& b : _table)
        {
            b.update(tps, size);

            if(b.wasPressed()){
                *key = b.key();
                *down = 1;
                return true;
            }
            else if(b.wasReleasedd()){
                *key = b.key();
                *down = 0;
                return true;
            }
        }
        return false;
    }

    void render(M5GFX* out) {
        if(_dirty)
        {
            for(auto&& b : _table) {
                out->fillRect(b._left + _ox, b._top + _oy, b._right - b._left, b._bottom - b._top, TFT_DARKGRAY);
                out->drawRect(b._left + _ox, b._top + _oy, b._right - b._left, b._bottom - b._top, TFT_WHITE);
            }
            _dirty = false;
        }
    }

    int16_t _ox{},_oy{};
    bool _dirty{true};
};

static VirtualButton virtual_button(0,1280-400);

std::array<Button, 10> VirtualButton::_table= {
    Button(K_UPARROW, 80, 0, 240, 200),
    Button(K_DOWNARROW, 80, 200, 240, 200),
    Button('a', 0,0,80,80), // up angle
    Button('z', 0,320, 80, 80), // down angle
    Button(K_LEFTARROW, 0,80,80,240),
    Button(K_RIGHTARROW, 240+80,80,80,240),
    Button(K_CTRL, 720-100, 400-100, 100, 100), // atack
    Button(K_ENTER, 720-200, 400-100, 100, 100),
    Button(',', 720-200, 400-200, 100, 100), // left-shift
    Button('.', 720-100, 400-200, 100, 100), // right-shift
};

void draw_touchpad(M5GFX* out){
    virtual_button.render(out);

}

extern "C" int touch_input(int *down, int *key){

    m5gfx::touch_point_t tps[5]; 
    auto count = M5.Display.getTouchRaw(tps, 5);

    return virtual_button.update(key, down, tps, count);
}
