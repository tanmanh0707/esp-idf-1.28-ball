#include "sdcard.h"

#include <utime.h>
#include <sdmmc_cmd.h>
#include <driver/sdmmc_host.h>
#include <driver/gpio.h>
#include <esp_vfs_fat.h>
#include "display.h"

#define TAG "SD"

static sdmmc_card_t *_sdcardHdl = nullptr;
static bool _sdcardMounted = false;

bool SDCARD_Init()
{
  static bool inititalized = false;
  bool res = false;

  if (inititalized == true) {
    log_w("SD Card is already initialized!");
    res = true;
    return res;
  }

  sdmmc_host_t host = SDMMC_HOST_DEFAULT();
  sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
  host.command_timeout_ms = 1000;
  host.max_freq_khz = SDMMC_FREQ_DEFAULT;

  slot_config.width = 1;
  slot_config.clk = CONFIG_SD_CLK_PIN;
  slot_config.cmd = CONFIG_SD_CMD_PIN;
  slot_config.d0 = CONFIG_SD_D0_PIN;

  host.flags = SDMMC_HOST_FLAG_1BIT;

  esp_vfs_fat_sdmmc_mount_config_t mount_config = {
    .format_if_mount_failed = false,
    .max_files = 100,
    .allocation_unit_size = 16 * 1024,
    .disk_status_check_enable = false,
    .use_one_fat = false
  };

  int attempts = 0;

  DISPLAY_DrawText("Mounting...");
  do {
    esp_err_t ret = esp_vfs_fat_sdmmc_mount(CONFIG_SDCARD_MOUNT_POINT, &host, &slot_config, &mount_config, &_sdcardHdl);
    if (ret == ESP_OK) {
      log_i("SD Card mounted!");
      _sdcardMounted = true;
      res = true;
    } else {
      attempts++;

      if (attempts <= CONFIG_SDCARD_MOUNT_RETRY) {
        DISPLAY_ClearScreen();
        DISPLAY_DrawText("Mount failed, retrying...");
        log_e("SD Card mount failed!, retrying (%d)", attempts);
        vTaskDelay(pdMS_TO_TICKS(1000));
      } else {
        DISPLAY_ClearScreen();
        DISPLAY_DrawText("Mount failed!");
        log_e("SD Card mount failed after %d retries!", CONFIG_SDCARD_MOUNT_RETRY);
      }
    }
  } while (res == false && attempts <= CONFIG_SDCARD_MOUNT_RETRY);

  inititalized = true;
  return res;
}

bool SDCARD_Mounted() {
  return _sdcardMounted;
}