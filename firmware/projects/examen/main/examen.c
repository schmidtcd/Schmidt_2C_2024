/*! @mainpage Examen
 *
 * \section genDesc General Description
 *
 * En este proyecto se presenta una aplicacion para la detección de eventos peligrosos para ciclistas, para lo cual se emplean diferentes sensores y actuadores. Se cuenta con un sensor de ultrasonido para la deteccion de obstaculos, un acelerometro para la deteccion de caidas y un buzzer que actua de alarma sonora en caso de peligro. Además se envian notificaciones a una aplicacion de Smartphone.
 *
 * @section hardConn Hardware Connection
 *
 * |    Sensor HC-SR04  |   ESP32   	|
 * |:--------------:|:--------------|
 * | 	ECHO	 	| 	GPIO_3		|
 * | 	TRIGGER	 	| 	GPIO_2		|
 * | 	  +5V 	 	| 	  +5V		|
 * | 	  GND	 	| 	  GND		|
 * | 	BUZZER	 	| 	GPIO_20		|
 * | 	CH1 acelerometro	 	| 	CH1		|
 * | 	CH2 acelerometro	 	| 	CH2		|
 * | 	CH3 acelerometro	 	| 	CH3		|
 *
 * @section changelog Changelog
 *
 * |   Date	    | Description                                    |
 * |:----------:|:-----------------------------------------------|
 * | 04/11/2024 | Document creation		                         |
 * | 04/11/2024 | Document completion	                         |
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
#include "timer_mcu.h"
#include "uart_mcu.h"
#include "analog_io_mcu.h"

/*==================[macros and definitions]=================================*/
#define CONFIG_BLINK_PERIOD_ULTRASONIDO_US 1000 * 1000 * 0.5 //En microsegundos
#define CONFIG_BLINK_PERIOD_PRECAUCION_MS 1000 * 0.5 //En milisegundos
#define CONFIG_BLINK_PERIOD_PELIGRO_MS 1000 * 0.25 //En milisegundos
#define CONFIG_BLINK_PERIOD_MUESTREO_US 1000 * 1000 * 0.01 //En milisegundos

/*==================[internal data definition]===============================*/
/**
 * @brief variable que almacena la distancia medida por el sensor de ultrasonido
 * 
 */
uint16_t distancia = 0;
/**
 * @brief bandera que permite controlar si el buzzer esta encendido o apagado
 * 
 */
bool encendido = false;
/**
 * @brief Handle de la tarea encargada del medir la distancia con el sensor de ultrasonido
 * 
 */
TaskHandle_t ultrasonido_task_handle = NULL;
/**
 * @brief Handle de la tarea encargada de los leds
 * 
 */
TaskHandle_t leds_task_handle = NULL;
/**
 * @brief Handle de la tarea encargada de medir las aceleraciones del acelerometro
 * 
 */
TaskHandle_t acelerometro_task_handle = NULL;
/**
 * @brief Handle de la tarea encargada de controlar el buzzer
 * 
 */
TaskHandle_t control_buzzer_task_handle = NULL;

/*==================[internal functions declaration]=========================*/
/**
 * @brief Función del Timer A encargada de notificar a las tareas del sensor de ultrasonido y los leds
 * 
 * @param param 
 */
void FuncTimerA(void *param)
{
	vTaskNotifyGiveFromISR(ultrasonido_task_handle, pdFALSE); /* Envía una notificación a la tarea asociada al sensor ulrasonido */
	vTaskNotifyGiveFromISR(leds_task_handle, pdFALSE);		  /* Envía una notificación a la tarea asociada a los leds */
}

/**
 * @brief  Función del Timer A encargada de notificar a la tarea de lectura analogica del acelerometro
 * 
 * @param param 
 */
void FuncTimerB(void *param)
{
	vTaskNotifyGiveFromISR(acelerometro_task_handle, pdFALSE);
}

/**
 * @brief Tarea encargada de medir la distancia del sensor de ultrasonido e informar peligro o precaucion. 
 * 
 * @param pvParameter 
 */
static void medirDistancia(void *pvParameter)
{
	while (true)
	{
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY); /* La tarea espera en este punto hasta recibir una notificación */
		distancia = HcSr04ReadDistanceInCentimeters();
		// Señal de peligro y precaución
		if (distancia <= 300)
		{
			UartSendString(UART_CONNECTOR, "Peligro, vehículo cerca\r\n");
		}
		else if (distancia > 300 && distancia < 500)
		{
			UartSendString(UART_CONNECTOR, "Precaución, vehículo cerca\r\n");
		}
	}
}

/**
 * @brief Tarea encargada del control de encendido de los leds de acuerdo a la distancia del peligro
 * 
 * @param pvParameter 
 */
static void controlLeds(void *pvParameter)
{
	while (true)
	{
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY); /* La tarea espera en este punto hasta recibir una notificación */
		if (distancia <= 300)
		{
			LedOn(LED_1);
			LedOn(LED_2);
			LedOn(LED_3);
		}
		else if (distancia > 300 && distancia < 500)
		{
			LedOn(LED_1);
			LedOn(LED_2);
			LedOff(LED_3);
		}
		else if (distancia >= 500)
		{
			LedOn(LED_1);
			LedOff(LED_2);
			LedOff(LED_3);
		}
	}
}

/**
 * @brief Tarea encargada de la lectura de los valores de aceleracion en 3 ejes del acelerometro
 * 
 * @param pvParameter 
 */
static void lecturaAcelerometro(void *pvParameter)
{
	uint16_t acelerometro_1 = 0;
	uint16_t acelerometro_2 = 0;
	uint16_t acelerometro_3 = 0;
	float aceleracion_x;
	float aceleracion_y;
	float aceleracion_z;
	float suma; 
	while (true)
	{
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
		AnalogInputReadSingle(CH1, &acelerometro_1);
		AnalogInputReadSingle(CH2, &acelerometro_2);
		AnalogInputReadSingle(CH3, &acelerometro_3);
		aceleracion_x = ((float)acelerometro_1 - 1650) / 0.3;
		aceleracion_y = ((float)acelerometro_2 - 1650) / 0.3;
		aceleracion_z = ((float)acelerometro_3 - 1650) / 0.3;
		suma = (aceleracion_x + aceleracion_y + aceleracion_z) / 1000;
		if (suma > 4)
		{
			UartSendString(UART_CONNECTOR, "Caída detectada\r\n");
		}
	}
}

/**
 * @brief Tarea encargada del control del buzzer para indicar la alerta de peligro o precaución
 * 
 * @param pvParameter 
 */
static void controlBuzzer(void *pvParameter)
{
	while (true)
	{
		if (distancia <= 500 && !encendido)
		{
			GPIOOn(GPIO_20);
			encendido = true;
		}
		else
		{
			GPIOOff(GPIO_20);
			encendido = false;
		}
		if (distancia <= 300)
		{
			vTaskDelay(CONFIG_BLINK_PERIOD_PELIGRO_MS / portTICK_PERIOD_MS);
		}
		else
		{
			vTaskDelay(CONFIG_BLINK_PERIOD_PRECAUCION_MS / portTICK_PERIOD_MS);
		}
	}
}

/**
 * @brief Aplicacion principal donde se inicializan los gpios, perifericos, timers y uart, ademas de la creaciones de las tareas.
 * 
 */
void app_main(void)
{
	LedsInit();
	HcSr04Init(GPIO_3, GPIO_2);
	GPIOInit(GPIO_20, GPIO_OUTPUT);
	timer_config_t timer_distancia = {
		.timer = TIMER_A,
		.period = CONFIG_BLINK_PERIOD_ULTRASONIDO_US,
		.func_p = FuncTimerA,
		.param_p = NULL};
	TimerInit(&timer_distancia);

	timer_config_t timer_acelerometro = {
		.timer = TIMER_B,
		.period = CONFIG_BLINK_PERIOD_MUESTREO_US,
		.func_p = FuncTimerB,
		.param_p = NULL};
	TimerInit(&timer_acelerometro);

	analog_input_config_t acelerometro_1 = {
		.input = CH1,
		.mode = ADC_SINGLE,
		.func_p = NULL,
		.param_p = NULL,
		.sample_frec = 0};
	AnalogInputInit(&acelerometro_1);

	analog_input_config_t acelerometro_2 = {
		.input = CH2,
		.mode = ADC_SINGLE,
		.func_p = NULL,
		.param_p = NULL,
		.sample_frec = 0};
	AnalogInputInit(&acelerometro_2);

	analog_input_config_t acelerometro_3 = {
		.input = CH3,
		.mode = ADC_SINGLE,
		.func_p = NULL,
		.param_p = NULL,
		.sample_frec = 0};
	AnalogInputInit(&acelerometro_3);

	serial_config_t my_uart = {
		.port = UART_CONNECTOR,
		.baud_rate = 115200,
		.func_p = NULL,
		.param_p = NULL};
	UartInit(&my_uart);

	xTaskCreate(&medirDistancia, "ultrasonido", 512, NULL, 5, &ultrasonido_task_handle);
	xTaskCreate(&controlLeds, "leds", 512, NULL, 5, &leds_task_handle);
	xTaskCreate(&lecturaAcelerometro, "lectura acelerometro", 512, NULL, 5, &acelerometro_task_handle);
	xTaskCreate(&controlBuzzer,"control buzzer", 512, NULL, 5, &control_buzzer_task_handle);
	TimerStart(timer_distancia.timer);
	TimerStart(timer_acelerometro.timer);
}