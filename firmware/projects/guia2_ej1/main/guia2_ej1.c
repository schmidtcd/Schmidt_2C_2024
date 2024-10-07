/*! @mainpage guia2_ej1
 *
 * \section genDesc General Description
 *
 * En este proyecto se miden distancias con un sensor de ultrasonido y se aplica el uso de Tareas.
 *
 * @section hardConn Hardware Connection
 *
 * |    Peripheral  |   ESP32   	|
 * |:--------------:|:--------------|
 * | 	ECHO	 	| 	GPIO_3		|
 * | 	TRIGGER	 	| 	GPIO_2		|
 * | 	  +5V 	 	| 	  +5V		|
 * | 	  GND	 	| 	  GND		|
 *
 *
 * @section changelog Changelog
 *
 * |   Date	    | Description                                    |
 * |:----------:|:-----------------------------------------------|
 * | 04/09/2024 | Document creation		                         |
 *
 * @author Cristian Schmidt (cristian.schmidt@ingenieria.uner.edu.ar)
 *
 */

/*==================[inclusions]=============================================*/
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "led.h"
#include "hc_sr04.h"
#include "switch.h"
#include "lcditse0803.h"

/*==================[macros and definitions]=================================*/
#define CONFIG_BLINK_PERIOD_ULTRASONIDO 1000
#define CONFIG_BLINK_PERIOD_LEDS 1000
#define CONFIG_BLINK_PERIOD_TECLAS 300
#define CONFIG_BLINK_PERIOD_LCD 1000

/*==================[internal data definition]===============================*/
bool encendido = false;
bool hold = false;
uint16_t distancia = 0;
TaskHandle_t ultrasonido_task_handle = NULL;
TaskHandle_t leds_task_handle = NULL;
TaskHandle_t teclas_task_handle = NULL;
TaskHandle_t lcd_task_handle = NULL;

/*==================[internal functions declaration]=========================*/
/**
 * @brief Tarea encargada de manejar la medición de distancia con el sensor de ultrasonido.
 *
 * @param pvParameter
 */
static void medirDistancia(void *pvParameter)
{
	while (true)
	{
		if (encendido)
		{
			distancia = HcSr04ReadDistanceInCentimeters();
		}
		vTaskDelay(CONFIG_BLINK_PERIOD_ULTRASONIDO / portTICK_PERIOD_MS);
	}
}

/**
 * @brief Tarea encargada de controlar el encendido de los leds.
 *
 * @param pvParameter
 */
static void controlLeds(void *pvParameter)
{
	while (true)
	{
		if (encendido)
		{
			if (distancia < 10)
			{
				LedOff(LED_1);
				LedOff(LED_2);
				LedOff(LED_3);
			}
			else if (distancia >= 10 && distancia < 20)
			{
				LedOff(LED_2);
				LedOff(LED_3);
				LedOn(LED_1);
			}
			else if (distancia >= 20 && distancia <= 30)
			{
				LedOff(LED_3);
				LedOn(LED_1);
				LedOn(LED_2);
			}
			else if (distancia > 30)
			{
				LedOn(LED_1);
				LedOn(LED_2);
				LedOn(LED_3);
			}
		}
		else
		{
			LedOff(LED_1);
			LedOff(LED_2);
			LedOff(LED_3);
		}
		vTaskDelay(CONFIG_BLINK_PERIOD_LEDS / portTICK_PERIOD_MS);
	}
}

/**
 * @brief Tarea encargada de manejar las teclas.
 *
 * @param pvParameter
 */
static void controlTeclas(void *pvParameter)
{
	uint8_t teclas;
	while (true)
	{
		teclas = SwitchesRead();
		switch (teclas)
		{
		case SWITCH_1:
			encendido = !encendido;
			if (!encendido)
			{
				hold = false;
			}
			break;
		case SWITCH_2:
			hold = !hold;
		default:
			break;
		}
		vTaskDelay(CONFIG_BLINK_PERIOD_TECLAS / portTICK_PERIOD_MS);
	}
}

/**
 * @brief Tarea encargada de controlar la visualización en el display lcd.
 *
 * @param pvParameter
 */
static void controlLCD(void *pvParameter)
{
	while (true)
	{
		if (encendido)
		{
			if (!hold)
			{
				LcdItsE0803Write(distancia);
			}
		}
		else
		{
			LcdItsE0803Off();
		}
		vTaskDelay(CONFIG_BLINK_PERIOD_LCD / portTICK_PERIOD_MS);
	}
}


/*==================[external functions definition]==========================*/
/**
 * @brief Aplicación que permite medir distancias y visualizarlas en un display lcd y controlar el encendido de los leds.
 *
 */
void app_main(void)
{
	LedsInit();
	SwitchesInit();
	LcdItsE0803Init();
	HcSr04Init(GPIO_3, GPIO_2);
	xTaskCreate(&medirDistancia, "ultrasonido", 512, NULL, 5, &ultrasonido_task_handle);
	xTaskCreate(&controlLeds, "leds", 512, NULL, 5, &leds_task_handle);
	xTaskCreate(&controlTeclas, "teclas", 512, NULL, 5, &teclas_task_handle);
	xTaskCreate(&controlLCD, "lcd", 512, NULL, 5, &lcd_task_handle);
}