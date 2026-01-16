#pragma once
#include "config_app.h"
#include <esp_err.h>

bool SDCARD_Init();
bool SDCARD_Mounted();
void SDCARD_FileList(const char *directory_path, std::vector<std::string>& file_list, const char* ext_lowercase = nullptr);
void SDCARD_PrintFileList(const char* directory_path);
std::string SDCARD_GetFileExtension(const std::string& filename);

// JPEG decoding functions
esp_err_t SDCARD_DecodeJpeg(const uint8_t *jpeg_data, size_t jpeg_size, uint16_t **pixels, uint32_t *width, uint32_t *height);
esp_err_t SDCARD_DecodeJpegAndDisplay(const uint8_t *jpeg_data, size_t jpeg_size);
esp_err_t SDCARD_DisplayImage(const char *file_path);