#pragma once

/* DISPLAY */
#define CONFIG_DISPLAY_WIDTH                    240
#define CONFIG_DISPLAY_HEIGHT                   240
#define CONFIG_DISPLAY_MIRROR_X                 true
#define CONFIG_DISPLAY_MIRROR_Y                 false
#define CONFIG_DISPLAY_SWAP_XY                  false

#define CONFIG_DISPLAY_OFFSET_X                 0
#define CONFIG_DISPLAY_OFFSET_Y                 0


#define CONFIG_DISPLAY_BACKLIGHT_PIN            GPIO_NUM_42
#define CONFIG_DISPLAY_BACKLIGHT_OUTPUT_INVERT  true

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
