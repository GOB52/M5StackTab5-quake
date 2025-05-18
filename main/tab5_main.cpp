// Porting for M5Stack Tab5 by GOB (X:@GOB_52_GOB)
#include <M5Unified.h>
#include <assert.h>
#include "quakegeneric.h"
#include "eth_connect.h"
#include "input.h"
#include "display.h"
#include "driver/sdmmc_host.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"

static void mount_sdcard(void) {
    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();

    host.slot = SDMMC_HOST_SLOT_1;
    slot_config.width = 4;

    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024
    };

    sdmmc_card_t *card;
    esp_err_t ret = esp_vfs_fat_sdmmc_mount("/sdcard", &host, &slot_config, &mount_config, &card);
    if (ret != ESP_OK) {
        ESP_LOGE("SD", "Failed to mount filesystem. Error: %s", esp_err_to_name(ret));
    } else {
        ESP_LOGI("SD", "SD card mounted successfully");
    }
}



extern "C" void QG_Init(void) {
}

extern "C" void QG_Quit(void) {
	display_quit();
	while(1) vTaskDelay(100);
}

static void quake_task(void *param) {
	char *argv[]={
		"quake", 
		"-basedir", "/sdcard/", // For SD
                "-nosound", // Sound not support yet
		NULL
	};
	//initialize Quake
	QG_Create(4, argv);

	int64_t oldtime_us = esp_timer_get_time();
	while (1) {
		// Run the frame at the correct duration.
		int64_t newtime_us = esp_timer_get_time();
		QG_Tick((double)(newtime_us - oldtime_us)/1000000.0);
		oldtime_us = newtime_us;
	}
}

extern "C" void app_main() {
    M5.begin();

    mount_sdcard();
    display_init();
    ethernet_connect();
    input_init();

    int stack_depth=200*1024;
    StaticTask_t *taskbuf=(StaticTask_t*)calloc(1, sizeof(StaticTask_t));
    uint8_t *stackbuf=(uint8_t*)calloc(stack_depth, 1);
    xTaskCreateStaticPinnedToCore(quake_task, "quake", stack_depth, NULL, 2, (StackType_t*)stackbuf, taskbuf, 0);
}

