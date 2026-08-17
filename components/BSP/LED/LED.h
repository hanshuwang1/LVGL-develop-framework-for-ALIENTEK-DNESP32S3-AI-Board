#ifndef _COMPONENTS_BSP_LED_LED_H
#define _COMPONENTS_BSP_LED_LED_H

#include "driver/gpio.h"

#define LED_GPIO_PIN GPIO_NUM_1

enum GPIO_OUTPUT_STATE{
	PIN_RESET,
	PIN_SET
};

#define LED0(X) do{X ? \
					gpio_set_level(LED_GPIO_PIN, 1) : \
					gpio_set_level(LED_GPIO_PIN, 0); \
				} while(0)
#define LED_TOGGLE() do{gpio_set_level(LED_GPIO_PIN, !gpio_get_level(LED_GPIO_PIN));} while(0)

void led_init(void);

#endif // _COMPONENTS_BSP_LED_LED_H
