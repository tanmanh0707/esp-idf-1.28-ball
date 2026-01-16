#include "application.h"
#include "log_app.h"
#include "display.h"
#include "gifplayer.h"
#include "sdcard.h"
#include "db.h"

#define TAG "APP"
#define SLIDESHOW_FOLDER "/images"
#define SLIDESHOW_DELAY_MS 3000

Application::Application()
{
}

Application::~Application() {
}

void Application::Start() {
  DB_Init();
  DISPLAY_Init();
  GIF_Init();
  SDCARD_Init();
  DISPLAY_ClearScreen();
  log_i("Application Started!");

  // Get list of JPEG images from SD card
  std::vector<std::string> jpeg_files;
  SDCARD_FileList(SLIDESHOW_FOLDER, jpeg_files, "jpg");

  // Also add .jpeg files
  std::vector<std::string> jpeg_ext_files;
  SDCARD_FileList(SLIDESHOW_FOLDER, jpeg_ext_files, "jpeg");
  jpeg_files.insert(jpeg_files.end(), jpeg_ext_files.begin(), jpeg_ext_files.end());

  log_i("Found %d JPEG images in %s", jpeg_files.size(), SLIDESHOW_FOLDER);
  for (const auto& file : jpeg_files) {
    log_i("  - %s", file.c_str());
  }

  size_t current_index = 0;

  while (1)
  {
    if (jpeg_files.empty()) {
      log_w("No JPEG images found in %s", SLIDESHOW_FOLDER);
      vTaskDelay(pdMS_TO_TICKS(SLIDESHOW_DELAY_MS));
      continue;
    }

    // Build file path: /images/filename.jpg
    std::string file_path = std::string(SLIDESHOW_FOLDER) + "/" + jpeg_files[current_index];
    log_i("Displaying: %s (%d/%d)", file_path.c_str(), current_index + 1, jpeg_files.size());

    esp_err_t ret = SDCARD_DisplayImage(file_path.c_str());
    if (ret != ESP_OK) {
      log_e("Failed to display image: %s", file_path.c_str());
    }

    // Move to next image
    current_index = (current_index + 1) % jpeg_files.size();

    vTaskDelay(pdMS_TO_TICKS(SLIDESHOW_DELAY_MS));
  }
}
