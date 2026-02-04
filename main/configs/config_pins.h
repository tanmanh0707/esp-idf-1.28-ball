#pragma once

/* DISPLAY */
#define CONFIG_DISPLAY_BACKLIGHT_PIN            GPIO_NUM_42

#define CONFIG_DISPLAY_SPI_SCLK_PIN             GPIO_NUM_4
#define CONFIG_DISPLAY_SPI_MOSI_PIN             GPIO_NUM_2
#define CONFIG_DISPLAY_SPI_CS_PIN               GPIO_NUM_5
#define CONFIG_DISPLAY_SPI_DC_PIN               GPIO_NUM_47
#define CONFIG_DISPLAY_SPI_RESET_PIN            GPIO_NUM_38

/* SD Card */
#define CONFIG_SD_CLK_PIN                       GPIO_NUM_17
#define CONFIG_SD_CMD_PIN                       GPIO_NUM_18
#define CONFIG_SD_D0_PIN                        GPIO_NUM_21
#define CONFIG_SD_D3_PIN                        GPIO_NUM_13
#define CONFIG_SD_CS_PIN                        CONFIG_SD_D3_PIN

/* Touch */
#define TP_PIN_NUM_TP_SDA                       GPIO_NUM_11
#define TP_PIN_NUM_TP_SCL                       GPIO_NUM_7
#define TP_PIN_NUM_TP_RST                       GPIO_NUM_6
#define TP_PIN_NUM_TP_INT                       GPIO_NUM_12

/* Codec */
#define AUDIO_I2S_GPIO_MCLK                     GPIO_NUM_16 //MCLK
#define AUDIO_I2S_GPIO_WS                       GPIO_NUM_45  //LRCK
#define AUDIO_I2S_GPIO_BCLK                     GPIO_NUM_9 //SCLK
#define AUDIO_I2S_GPIO_DIN                      GPIO_NUM_10 //DOUT
#define AUDIO_I2S_GPIO_DOUT                     GPIO_NUM_8  //DIN

#define AUDIO_CODEC_PA_PIN                      GPIO_NUM_46
#define AUDIO_CODEC_I2C_SDA_PIN                 GPIO_NUM_15
#define AUDIO_CODEC_I2C_SCL_PIN                 GPIO_NUM_14
