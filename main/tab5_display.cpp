// Porting for M5Stack Tab5 by GOB (X:@GOB_52_GOB)
#include <M5Unified.h>
#include <esp_timer.h>
#include <esp_log.h>
#include "quakedef.h"

static uint16_t pal[256];
static uint8_t *cur_pixels;
static uint16_t *lcdbuf[2]={};
static int cur_buf=1;
static int draw_task_quit=0;

static TaskHandle_t draw_task_handle;
static SemaphoreHandle_t drawing_mux;

static int fps_ticks=0;
static int64_t start_time_fps_meas;

static auto& display = M5.Display;
static const char TAG[] = "TAB5D";

extern void draw_touchpad(M5GFX* out);

extern "C" void QG_DrawFrame(void *pixels) {
    xSemaphoreTake(drawing_mux, portMAX_DELAY);
    cur_pixels=(uint8_t*)pixels;
    xSemaphoreGive(drawing_mux);
    xTaskNotifyGive(draw_task_handle);
    fps_ticks++;
    if (fps_ticks>100) {
        int64_t newtime_us=esp_timer_get_time();
        int64_t fpstime=(newtime_us-start_time_fps_meas)/fps_ticks;
        fps_ticks=0;
        start_time_fps_meas = newtime_us;
        printf("Fps: %02f\n", 1000000.0/fpstime);
    }
}

static void draw_task(void *param) {
    uint16_t *rgbfb=(uint16_t*)heap_caps_calloc(QUAKEGENERIC_RES_X*QUAKEGENERIC_RES_Y, sizeof(uint16_t), MALLOC_CAP_DMA|MALLOC_CAP_SPIRAM);
    //uint16_t *rgbfb=(uint16_t*)heap_caps_calloc(QUAKEGENERIC_RES_X*QUAKEGENERIC_RES_Y, sizeof(uint16_t), MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA);
    assert(rgbfb);

    LGFX_Sprite sprite(&display);
    sprite.setBuffer(rgbfb, QUAKEGENERIC_RES_X, QUAKEGENERIC_RES_Y, 16);
    sprite.setPivot(QUAKEGENERIC_RES_X * 0.5f + 0.5f, QUAKEGENERIC_RES_Y * 0.5f + 0.5f);

    float scale = (float)display.width() / (float)QUAKEGENERIC_RES_X;
    int32_t draw_w = QUAKEGENERIC_RES_X * scale;
    int32_t draw_h = QUAKEGENERIC_RES_Y * scale;
    int32_t draw_y = (display.height() - draw_h) / 2;

    ESP_LOGI(TAG, "Screen;%d,%d %f", draw_w, draw_h, scale);
    
    while(!draw_task_quit) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
		
        xSemaphoreTake(drawing_mux, portMAX_DELAY);
        int64_t start_us = esp_timer_get_time();
        // convert pixels
        uint8_t *p=(uint8_t*)cur_pixels;
        uint16_t *lcdp=rgbfb;

        constexpr uint32_t len = QUAKEGENERIC_RES_X * QUAKEGENERIC_RES_Y;
        uint32_t count = len;
        while(count--) { *lcdp++=pal[*p++]; }
        xSemaphoreGive(drawing_mux);

        //do a draw to trigger fb flip
        sprite.pushRotateZoom(draw_w/2, 1280- draw_y/2 - 420, 0.0f, scale, scale);

        // virutal key pad
        draw_touchpad(&display);
        
        cur_buf=cur_buf?0:1;
        int64_t end_us = esp_timer_get_time();

        //Shows the maximum FPS possible given the lcd frame drawing code
        //printf("LCD Fps: %02f\n", 1000000.0/(end_us-start_us));
    }
}

extern "C" void QG_SetPalette(unsigned char palette[768]) {
    unsigned char *p=palette;
    for (int i=0; i<256; i++) {
        //convert to rgb565
        int b=(*p++)>>3;
        int g=(*p++)>>2;
        int r=(*p++)>>3;
        uint16_t v = r+(g<<5)+(b<<11);
        v = (v >> 8) | (v << 8);
        pal[i] = v;
    }
}

#if 0
#define CHAR_W 8
#define CHAR_H 16

static void draw_char(int x, int y, int ch, int fore, int back) {
    //make sure not to draw out of bounds
    if (x<0 || y<0) return;
    if (x+CHAR_W >= BSP_LCD_H_RES) return;
    if (y+CHAR_H >= BSP_LCD_V_RES) return;
    //write character
    uint16_t *fb=lcdbuf[cur_buf];
    for (int py=0; py<CHAR_H; py++) {
        for (int px=0; px<CHAR_W; px++) {
            int pix=fontdata_8x16[ch*CHAR_H+py]&(1<<px);
            int col=pix?fore:back;
            fb[BSP_LCD_H_RES*(y+py)+(x+(7-px))]=pal[col];
        }
    }
}
#endif

static void draw_end_screen(char *vgamem) {
}

extern "C" void display_quit() {
    draw_task_quit=1;
    xSemaphoreGive(drawing_mux);
    vTaskDelay(pdMS_TO_TICKS(30)+1);

    display.endWrite();
#if 0
    //we can now use the framebuffer to draw the end screen
    unsigned char *d;
    if (registered.value)
        d = COM_LoadHunkFile ("end2.bin"); 
    else
        d = COM_LoadHunkFile ("end1.bin"); 
    draw_end_screen((char*)d);
#endif
}

extern "C" void display_init() {
    display.startWrite();
    display.fillScreen(0);
    display.fillRect(0, display.height() - 400,display.width(), 400, TFT_DARKGRAY);
    
    drawing_mux=xSemaphoreCreateMutex();
    xTaskCreatePinnedToCore(draw_task, "draw", 4096*2, NULL, 3, &draw_task_handle, 1);
}

