#pragma once
#include "config_app.h"
#include "codec.h"

class Cst816d;
class Speaker {
public:
  static Speaker *GetInstance();

  AudioCodec* GetAudioCodec();
  void PlayFiller();
  Cst816d* GetTouchpad() {
    return cst816d_;
  }

  static void touchpad_timer_callback(void* arg);
private:
  Speaker();
  ~Speaker();

  void InitializeCodecI2c();
  void InitializeCodecI2c_Touch();
  void InitializeCst816DTouchPad();

private:
  i2c_master_bus_handle_t codec_i2c_bus_;
  i2c_master_bus_handle_t i2c_bus_;

  Cst816d *cst816d_;
  esp_timer_handle_t touchpad_timer_;
};

