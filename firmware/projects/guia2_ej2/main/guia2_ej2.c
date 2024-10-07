/*! @mainpage guia2_ej2
 *
 * \section genDesc General Description
 *
 * En este proyecto se miden distancias con un sensor de ultrasonido y se aplica el uso de Tareas, Interrupciones para el control de las teclas y Timers para el control de los tiempos.
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
 * | 25/09/2024 | Document creation		                         |
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
#include "timer_mcu.h"

/*==================[macros and definitions]=================================*/
#define CONFIG_BLINK_PERIOD_GENERAL_US 1000 * 1000

/*==================[internal data definition]===============================*/
bool encendido = false;
bool hold = false;
uint16_t distancia = 0;
TaskHandle_t ultrasonido_task_handle = NULL;
TaskHandle_t leds_task_handle = NULL;
TaskHandle_t lcd_task_handle = NULL;

/*==================[internal functions declaration]=========================*/
/**
 * @brief Función de interrupción para el SWITCH 1
 *
 */
void teclaUno()
{
	encendido = !encendido;
	if (!encendido)
	{
		hold = false;
	}
}

/**
 * @brief Función de interrupción para el SWITCH 2
 *
 */
void teclaDos()
{
	hold = !hold;
}

/**
 * @brief Función del Timer A encargada de notificar a las tareas.
 *
 * @param param
 */
void FuncTimerA(void *param)
{
	vTaskNotifyGiveFromISR(leds_task_handle, pdFALSE);		  /* Envía una notificación a la tarea asociada a los leds */
	vTaskNotifyGiveFromISR(lcd_task_handle, pdFALSE);		  /* Envía una notificación a la tarea asociada al LCD */
	vTaskNotifyGiveFromISR(ultrasonido_task_handle, pdFALSE); /* Envía una notificación a la tarea asociada al sensor ulrasonido */
}

/**
 * @brief Tarea encargada de manejar la medición de distancia con el sensor de ultrasonido.
 *
 * @param pvParameter
 */
static void medirDistancia(void *pvParameter)
{
	while (true)
	{
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY); /* La tarea espera en este punto hasta recibir una notificación */
		if (encendido)
		{
			distancia = HcSr04ReadDistanceInCentimeters();
		}
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
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY); /* La tarea espera en este punto hasta recibir una notificación */
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
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY); /* La tarea espera en este punto hasta recibir una notificación */
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
	}
}


/*==================[external functions definition]==========================*/
/**
 * @brief Aplicación que permite medir distancias y visualizarlas en un display lcd y controlar el encendido de los leds usando Tareas, Interrupciones y Timers.
 *
 */
void app_main(void)
{
	LedsInit();
	SwitchesInit();
	LcdItsE0803Init();
	HcSr04Init(GPIO_3, GPIO_2);
	timer_config_t timer_general = {
		.timer = TIMER_A,
		.period = CONFIG_BLINK_PERIOD_GENERAL_US,
		.func_p = FuncTimerA,
		.param_p = NULL};

	TimerInit(&timer_general);

	SwitchActivInt(SWITCH_1, teclaUno, NULL);
	SwitchActivInt(SWITCH_2, teclaDos, NULL);
	xTaskCreate(&medirDistancia, "ultrasonido", 512, NULL, 5, &ultrasonido_task_handle);
	xTaskCreate(&controlLeds, "leds", 512, NULL, 5, &leds_task_handle);
	xTaskCreate(&controlLCD, "lcd", 512, NULL, 5, &lcd_task_handle);

	TimerStart(timer_general.timer);
}