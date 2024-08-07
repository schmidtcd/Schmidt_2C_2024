/*! @mainpage guia1_ej3
 *
 * \section genDesc General Description
 *
 * En este proyecto se implementa una funcion que permite controlar los leds de acuerdo a las propiedades de una estructura que representa el comportamiento de los leds.
 * @section changelog Changelog
 *
 * |   Date	    | Description                                    |
 * |:----------:|:-----------------------------------------------|
 * | 07/08/2024 | Document creation		                         |
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
#include "switch.h"

/*==================[macros and definitions]=================================*/
#define CONFIG_BLINK_PERIOD 100
#define ON 0
#define OFF 1
#define TOGGLE 2

/*==================[internal data definition]===============================*/
typedef struct
{
	uint8_t mode;	  //  ON, OFF, TOGGLE
	uint8_t n_led;	  //  indica el número de led a controlar
	uint8_t n_ciclos; //  indica la cantidad de ciclos de ncendido/apagado
	uint16_t periodo; //  indica el tiempo de cada ciclo
} leds;

/*==================[internal functions declaration]=========================*/
void controlLeds(leds *my_leds)
{
	switch (my_leds->mode)
	{
	case ON:
		switch (my_leds->n_led)
		{
		case 1:
			LedOn(LED_1);
			break;
		case 2:
			LedOn(LED_2);
			break;
		case 3:
			LedOn(LED_3);
			break;
		default:
			break;
		}
		break;
	case OFF:
		switch (my_leds->n_led)
		{
		case 1:
			LedOff(LED_1);
			break;
		case 2:
			LedOff(LED_2);
			break;
		case 3:
			LedOff(LED_3);
			break;
		default:
			break;
		}
		break;
	case TOGGLE:
		for (uint8_t i = 0; i < my_leds->n_ciclos; i++)
		{
			switch (my_leds->n_led)
			{
			case 1:
				LedToggle(LED_1);
				break;
			case 2:
				LedToggle(LED_2);
				break;
			case 3:
				LedToggle(LED_3);
				break;
			default:
				break;
			}
			for (uint8_t j = 0; j < (my_leds->periodo / CONFIG_BLINK_PERIOD); j++)
			{
				vTaskDelay(CONFIG_BLINK_PERIOD / portTICK_PERIOD_MS);
			}
		}
		break;
	default:
		break;
	}
}
/*==================[external functions definition]==========================*/
void app_main(void)
{
	// Prueba para medir en osciloscopio (LED3 -> GPIO5)
	LedsInit();
	leds var_leds;
	var_leds.n_led = 3;
	var_leds.n_ciclos = 10;
	var_leds.periodo = 500;
	var_leds.mode = TOGGLE;
	controlLeds(&var_leds);

	// //Prueba LED ON
	// var_leds.mode = ON;
	// controlLeds(&var_leds);
	// vTaskDelay(2000/ portTICK_PERIOD_MS);

	// //Prueba LED OFF
	// var_leds.mode = OFF;
	// controlLeds(&var_leds);
	// vTaskDelay(2000/ portTICK_PERIOD_MS);

	// //Prueba LED TOGGLE
	// var_leds.n_led = 2;
	// var_leds.mode = TOGGLE;
	// controlLeds(&var_leds);
}
