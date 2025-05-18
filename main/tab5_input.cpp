// Porting for M5Stack Tab5 by GOB (X:@GOB_52_GOB)
#include <M5Unified.h>
#include <M5GFX.h>
#include "quakekeys.h"
#include "quakedef.h"
#include "esp_log.h"
#include <array>

static const char TAG[] = "TAB5I";

extern const uint8_t btn0_bmp[];
extern const uint8_t btn1_bmp[];
static const uint8_t* btn_bmp[] = {
    btn0_bmp, btn1_bmp
};

struct Button
{
    Button(const uint8_t qkey,const int16_t l, const int16_t t, const int16_t w, const int16_t h) :
            _key{qkey}, _left{l}, _top{t}, _right{_left + w}, _bottom{_top + h} {}

    bool update(const m5gfx::touch_point_t* tps, const uint8_t size) {
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
        _needRender |= _wasPressed || _wasReleased;
        return _needRender;
    }
    inline uint8_t key() const { return _key; }
    inline bool isPressed() const { return _pressed; }
    inline bool wasPressed() const { return _wasPressed; }
    inline bool wasReleasedd() const { return _wasReleased; }

    uint8_t _key{};
    int16_t _left{}, _top{}, _right{}, _bottom{};
    bool _pressed{}, _wasPressed{},_wasReleased{}, _needRender{true};
};

struct VirtualButton {
    VirtualButton(const int16_t x, const int16_t y, Button* b, const uint32_t num)
            : _ox{x}, _oy{y}, _buttons{b}, _numButtons{num}{}

    bool init()
    {
        uint8_t idx{};
        for(auto&& s : _btn){
            s.setPsram(true);
            if(!s.createFromBmp(btn_bmp[idx++])){
                return false;
            }
        }
        return true;
    }
    
    bool update(int*key, int* down, m5gfx::touch_point_t* tps, const uint8_t size) {
        for(uint_fast8_t i=0;i<size;++i) {
            tps[i].x -= _ox;
            tps[i].y -= _oy;
        }

        for(uint32_t i=0;i < _numButtons; ++i){
            Button& b = _buttons[i];
            b.update(tps, size);
            // TODO: 最初の wasP/R しか返していないのでよろしくはないが
            // 呼び出し頻度が高ければ大きな問題にはならない
            // とはいえ対応はしておきたいところ
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
        for(uint32_t i=0;i < _numButtons; ++i) {
            render_button(out, _buttons[i]);
        }
    }

    void render_button(M5GFX* out, Button& b) {
        if(!b._needRender) return;

        int32_t x,y,w,h;
        out->getClipRect(&x, &y, &w, &h);

        LGFX_Sprite* s = &_btn[b.isPressed()];
        
        out->setClipRect(_ox + b._left, _oy + b._top, b._right - b._left, b._bottom - b._top);
        s->pushSprite(out, _ox, _oy, 0 /* palette index zero is transpalent */);

        out->setClipRect(x, y, w, h);
        b._needRender = false;
    }

    int16_t _ox{},_oy{};
    Button* _buttons{};
    uint32_t _numButtons{};

    LGFX_Sprite _btn[2];
};

static Button buttons[] = {
    Button(K_UPARROW,    113, 29,     125,  166), // forward
    Button(K_DOWNARROW,  113, 29+166, 125,  166), // backpedal
    Button(K_LEFTARROW,   15,128,       98, 139), // turn left
    Button(K_RIGHTARROW, 238,128,       98, 139), // turn right
    Button('a',           6,  15,      106, 111), // look up
    Button('z',           6, 269,      106, 111), // lokk down
    Button(K_CTRL,      589, 270,      125, 100), // atack 
    Button(K_ENTER,     462, 270,      125, 100), // jump / swim up
    Button(',',         462, 191,      125,  74), // step left
    Button('.',         589, 191,      125,  74), // step right
    Button(K_END,       352,  14,       93,  51), // center view
    Button(K_ESCAPE,    621,  14,       93,  51), // ESC
    Button('d',         239,  15,      106, 111), // swim up
    Button('c',         239, 269,      106, 111), // swim down
#if 0
    // change weapon directly
    Button('1',         421,  75,       58, 100),
    Button('2',         479,  75,       58, 100),
    Button('3',         538,  75,       58, 100),
    Button('4',         597,  75,       58, 100),
    Button('5',         656,  75,       58, 100),
#else
    //    Button('1',         421,  75,       58, 100),
    Button('/',         656,  75,       58, 100), // change weapon
#endif    


};

static VirtualButton virtual_button(0,1280-400, buttons, sizeof(buttons)/sizeof(buttons[0]));

void draw_touchpad(M5GFX* out){
    virtual_button.render(out);
}

extern "C" bool tab5_input_init() {
    auto ret = virtual_button.init();
    if(!ret){
        ESP_LOGE(TAG, "Failed to init tab5_input_init()");
    }
    return ret;
}


extern "C" int touch_input(int *down, int *key){

    m5gfx::touch_point_t tps[5]; 
    auto count = M5.Display.getTouchRaw(tps, 5);

    return virtual_button.update(key, down, tps, count);
}
