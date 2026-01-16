#include "sdcard.h"

#include <utime.h>
#include <sdmmc_cmd.h>
#include <driver/sdmmc_host.h>
#include <driver/gpio.h>
#include <esp_vfs_fat.h>
#include <dirent.h>
#include <sys/stat.h>
#include "esp_jpeg_dec.h"
#include <esp_check.h>
#include "display.h"
#include "lockguard.h"
#include "utils.h"

#define TAG "SD"

static sdmmc_card_t *_sdcardHdl = nullptr;
static bool _sdcardMounted = false;
std::unique_ptr<Mutex> _sdMutex = nullptr;

bool SDCARD_Init()
{
  static bool inititalized = false;
  bool res = false;

  if (inititalized == true) {
    log_w("SD Card is already initialized!");
    res = true;
    return res;
  }

  if (_sdMutex == nullptr) {
    _sdMutex = std::make_unique<Mutex>();
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

std::string SDCARD_GetFileExtension(const std::string& filename) {
  size_t dot_pos = filename.rfind('.');
  if (dot_pos == std::string::npos) {
    return "";
  }
  std::string ext = filename.substr(dot_pos + 1);
  for (char& c : ext) {
    c = std::tolower(static_cast<unsigned char>(c));
  }
  return ext;
}

void SDCARD_FileList(const char *directory_path, std::vector<std::string>& file_list, const char* ext_lowercase) {
  LockGuard guard(_sdMutex);
  file_list.clear();

  std::string full_path = std::string(CONFIG_SDCARD_MOUNT_POINT) + std::string(directory_path);
  DIR* dir = opendir(full_path.c_str());
  if (!dir) {
    log_e("Failed to open directory: %s", full_path.c_str());
    return;
  }

  struct dirent* entry;
  while ((entry = readdir(dir)) != nullptr) {
    if (entry->d_name[0] == '.') continue;

    std::string file_path = full_path + "/" + entry->d_name;
    struct stat st;
    if (stat(file_path.c_str(), &st) == 0 && S_ISREG(st.st_mode)) {
      std::string filename(entry->d_name);

      if (!ext_lowercase || SDCARD_GetFileExtension(filename) == ext_lowercase) {
        file_list.push_back(filename);
      }
    }
  }
  closedir(dir);
}

void SDCARD_PrintFileList(const char* directory_path) {
  std::vector<std::string> file_list;
  SDCARD_FileList(directory_path, file_list, nullptr);

  log_i("Files in directory: %s", directory_path);
  for (const auto& file : file_list) {
    log_i("  - %s", file.c_str());
  }
}

esp_err_t SDCARD_DecodeJpeg(const uint8_t *jpeg_data, size_t jpeg_size, uint16_t **pixels, uint32_t *width, uint32_t *height)
{
  *pixels = NULL;
  *width = 0;
  *height = 0;

  // Create JPEG decoder config
  jpeg_dec_config_t config = DEFAULT_JPEG_DEC_CONFIG();
  config.output_type = JPEG_PIXEL_FORMAT_RGB565_BE;  // RGB565 big-endian for display
  config.rotate = JPEG_ROTATE_0D;

  jpeg_dec_handle_t decoder = NULL;
  jpeg_error_t ret = jpeg_dec_open(&config, &decoder);
  if (ret != JPEG_ERR_OK || decoder == NULL) {
    log_e("Failed to open JPEG decoder: %d", ret);
    return ESP_FAIL;
  }

  // Setup IO for parsing header
  jpeg_dec_io_t io = {
    .inbuf = (uint8_t *)jpeg_data,
    .inbuf_len = (int)jpeg_size,
    .inbuf_remain = 0,
    .outbuf = NULL,
    .out_size = 0,
  };

  // Get image info first by parsing header
  jpeg_dec_header_info_t header_info;
  ret = jpeg_dec_parse_header(decoder, &io, &header_info);
  if (ret != JPEG_ERR_OK) {
    log_e("Failed to parse JPEG header: %d", ret);
    jpeg_dec_close(decoder);
    return ESP_FAIL;
  }

  *width = header_info.width;
  *height = header_info.height;
  log_i("JPEG dimensions: %lux%lu", *width, *height);

  // Get required output buffer size
  int outbuf_len = 0;
  ret = jpeg_dec_get_outbuf_len(decoder, &outbuf_len);
  if (ret != JPEG_ERR_OK) {
    log_e("Failed to get output buffer size: %d", ret);
    jpeg_dec_close(decoder);
    return ESP_FAIL;
  }

  // Allocate 16-byte aligned output buffer
  *pixels = (uint16_t *)jpeg_calloc_align(outbuf_len, 16);
  if (*pixels == NULL) {
    log_e("Failed to allocate %d bytes for JPEG decode", outbuf_len);
    jpeg_dec_close(decoder);
    return ESP_ERR_NO_MEM;
  }

  // Setup IO for decoding
  io.inbuf = (uint8_t *)jpeg_data;
  io.inbuf_len = (int)jpeg_size;
  io.outbuf = (uint8_t *)(*pixels);
  io.out_size = outbuf_len;

  // Decode the image
  ret = jpeg_dec_process(decoder, &io);
  if (ret != JPEG_ERR_OK) {
    log_e("Failed to decode JPEG: %d", ret);
    jpeg_free_align(*pixels);
    *pixels = NULL;
    jpeg_dec_close(decoder);
    return ESP_FAIL;
  }

  jpeg_dec_close(decoder);
  log_i("JPEG decoded successfully: %lux%lu", *width, *height);
  return ESP_OK;
}

esp_err_t SDCARD_DecodeJpegAndDisplay(const uint8_t *jpeg_data, size_t jpeg_size)
{
  uint16_t *pixels = NULL;
  uint32_t width = 0;
  uint32_t height = 0;

  esp_err_t ret = SDCARD_DecodeJpeg(jpeg_data, jpeg_size, &pixels, &width, &height);
  if (ret != ESP_OK || pixels == NULL) {
    return ret;
  }

  // Center the image on display
  int16_t x = (CONFIG_DISPLAY_WIDTH - width) / 2;
  int16_t y = (CONFIG_DISPLAY_HEIGHT - height) / 2;
  if (x < 0) x = 0;
  if (y < 0) y = 0;

  uint16_t disp_w = (width > CONFIG_DISPLAY_WIDTH) ? CONFIG_DISPLAY_WIDTH : width;
  uint16_t disp_h = (height > CONFIG_DISPLAY_HEIGHT) ? CONFIG_DISPLAY_HEIGHT : height;

  DISPLAY_PushPixels(x, y, disp_w, disp_h, pixels);

  jpeg_free_align(pixels);
  return ESP_OK;
}

esp_err_t SDCARD_DisplayImage(const char *file_path)
{
  if (!_sdcardMounted) {
    log_e("SD card not mounted");
    return ESP_ERR_INVALID_STATE;
  }

  // Build full path
  std::string full_path = std::string(CONFIG_SDCARD_MOUNT_POINT) + file_path;

  // Get file size
  struct stat st;
  if (stat(full_path.c_str(), &st) != 0) {
    log_e("Failed to stat file: %s", full_path.c_str());
    return ESP_ERR_NOT_FOUND;
  }

  size_t file_size = st.st_size;
  log_i("Reading JPEG: %s (%zu bytes)", full_path.c_str(), file_size);

  // Allocate buffer in PSRAM
  uint8_t *jpeg_data = (uint8_t *)heap_caps_malloc(file_size, MALLOC_CAP_SPIRAM);
  if (jpeg_data == NULL) {
    log_e("Failed to allocate %zu bytes in PSRAM", file_size);
    return ESP_ERR_NO_MEM;
  }

  // Read file
  FILE *f = fopen(full_path.c_str(), "rb");
  if (f == NULL) {
    log_e("Failed to open file: %s", full_path.c_str());
    heap_caps_free(jpeg_data);
    return ESP_ERR_NOT_FOUND;
  }

  size_t bytes_read = fread(jpeg_data, 1, file_size, f);
  fclose(f);

  if (bytes_read != file_size) {
    log_e("Read %zu bytes, expected %zu", bytes_read, file_size);
    heap_caps_free(jpeg_data);
    return ESP_FAIL;
  }

  // Decode and display
  esp_err_t ret = SDCARD_DecodeJpegAndDisplay(jpeg_data, file_size);

  heap_caps_free(jpeg_data);
  return ret;
}