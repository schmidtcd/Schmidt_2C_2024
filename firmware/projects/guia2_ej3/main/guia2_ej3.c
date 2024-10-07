/*! @mainpage guia2_ej3
 *
 * \section genDesc General Description
 *
 * En este proyecto se miden distancias con un sensor de ultrasonido, se envían los datos mediante Puerto Serie a la PC, y permite controlar la ESP-32 mediante dicho puerto.
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
#include "uart_mcu.h"
#include <string.h>

/*==================[macros and definitions]=================================*/
#define CONFIG_BLINK_PERIOD_GENERAL_US 1000 * 1000

/*==================[internal data definition]===============================*/
bool encendido = false;
bool hold = false;
uint16_t distancia = 0;
TaskHandle_t ultrasonido_task_handle = NULL;
TaskHandle_t leds_task_handle = NULL;
TaskHandle_t lcd_task_handle = NULL;
TaskHandle_t uart_task_handle = NULL;

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
	if (encendido)
	{
		hold = !hold;
	}
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
	vTaskNotifyGiveFromISR(uart_task_handle, pdFALSE);		  /* Envía una notificación a la tarea asociada al uart */
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
		if (encendido && !hold)
		{
			distancia = HcSr04ReadDistanceInCentimeters();
		}
		else if (!encendido)
		{
			distancia = 0;
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

/**
 * @brief Función encargada del control del programa mediante el Puerto Serie.
 *
 * @param param
 */
void uartControl(void *param)
{
	uint8_t tecla;
	UartReadByte(UART_PC, &tecla);
	if (tecla == 'O' || tecla == 'o')
	{
		encendido = !encendido;
		if (!encendido)
		{
			hold = false;
		}
	}
	else if (tecla == 'H' || tecla == 'h')
	{
		if (encendido)
		{
			hold = !hold;
		}
	}
}

/**
 * @brief Tarea asociada al control del Puerto Serie.
 *
 */
void uartTask()
{
	while (true)
	{

		char *extension = " cm\r\n";
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY); /* La tarea espera en este punto hasta recibir una notificación */
		if (encendido)
		{
			UartSendString(UART_PC, (char *)UartItoa(distancia, 10));
			UartSendString(UART_PC, extension);
		}
	}
}

/*==================[external functions definition]==========================*/
/**
 * @brief Aplicación que permite medir distancias y visualizarlas en un display lcd y controlar el encendido de los leds usando Tareas, Interrupciones y Timers. Además permite visualizar las mediciones en un terminal de PC y controlar la placa mediante el Puerto Serie.
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

	serial_config_t my_uart = {
		.port = UART_PC,
		.baud_rate = 9600,
		.func_p = uartControl,
		.param_p = NULL};

	UartInit(&my_uart);

	SwitchActivInt(SWITCH_1, teclaUno, NULL);
	SwitchActivInt(SWITCH_2, teclaDos, NULL);

	xTaskCreate(&medirDistancia, "ultrasonido", 512, NULL, 5, &ultrasonido_task_handle);
	xTaskCreate(&controlLeds, "leds", 512, NULL, 5, &leds_task_handle);
	xTaskCreate(&controlLCD, "lcd", 512, NULL, 5, &lcd_task_handle);
	xTaskCreate(&uartTask, "UART", 512, NULL, 5, &uart_task_handle);

	TimerStart(timer_general.timer);
}