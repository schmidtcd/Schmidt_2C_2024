/*! @mainpage guia2_ej4
 *
 * @section genDesc General Description
 *
 * En este proyecto se realiza un conversor DAC que permite convertir una señal digital en una señal analógica, además se realiza un conversor ADC para convertir la señal analógica generada en una señal digital y enviar los datos por el puerto serie.
 *
 * @section hardConn Hardware Connection
 *
 * |    Peripheral  |   ESP32   	|
 * |:--------------:|:--------------|
 * | 	Punto Medio Potenciómetro	 	| 	CH1		|
 * | 	Extremo 1 Potenciómetro	 	| 	CH0	|
 * | 	Extremo 2 Potenciómetro	 	| 	GND	|
 *
 * @section changelog Changelog
 *
 * |   Date	    | Description                                    |
 * |:----------:|:-----------------------------------------------|
 * | 09/10/2024 | Document creation		                         |
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
#include "timer_mcu.h"
#include "uart_mcu.h"
#include "analog_io_mcu.h"

/*==================[macros and definitions]=================================*/
#define CONFIG_BLINK_PERIOD_MUESTREO_US 1000 * 1000 * 0.002
#define CONFIG_BLINK_PERIOD_VISUALIZACION_US 1000 * 4
#define BUFFER_SIZE 231

/*==================[internal data definition]===============================*/
TaskHandle_t conversor_adc_task_handle = NULL;
TaskHandle_t conversor_dac_task_handle = NULL;
TaskHandle_t main_task_handle = NULL;

const char ecg[BUFFER_SIZE] = {
    76, 77, 78, 77, 79, 86, 81, 76, 84, 93, 85, 80,
    89, 95, 89, 85, 93, 98, 94, 88, 98, 105, 96, 91,
    99, 105, 101, 96, 102, 106, 101, 96, 100, 107, 101,
    94, 100, 104, 100, 91, 99, 103, 98, 91, 96, 105, 95,
    88, 95, 100, 94, 85, 93, 99, 92, 84, 91, 96, 87, 80,
    83, 92, 86, 78, 84, 89, 79, 73, 81, 83, 78, 70, 80, 82,
    79, 69, 80, 82, 81, 70, 75, 81, 77, 74, 79, 83, 82, 72,
    80, 87, 79, 76, 85, 95, 87, 81, 88, 93, 88, 84, 87, 94,
    86, 82, 85, 94, 85, 82, 85, 95, 86, 83, 92, 99, 91, 88,
    94, 98, 95, 90, 97, 105, 104, 94, 98, 114, 117, 124, 144,
    180, 210, 236, 253, 227, 171, 99, 49, 34, 29, 43, 69, 89,
    89, 90, 98, 107, 104, 98, 104, 110, 102, 98, 103, 111, 101,
    94, 103, 108, 102, 95, 97, 106, 100, 92, 101, 103, 100, 94, 98,
    103, 96, 90, 98, 103, 97, 90, 99, 104, 95, 90, 99, 104, 100, 93,
    100, 106, 101, 93, 101, 105, 103, 96, 105, 112, 105, 99, 103, 108,
    99, 96, 102, 106, 99, 90, 92, 100, 87, 80, 82, 88, 77, 69, 75, 79,
    74, 67, 71, 78, 72, 67, 73, 81, 77, 71, 75, 84, 79, 77, 77, 76, 76,
};

/*==================[internal functions declaration]=========================*/
/**
 * @brief Función del Timer A encargada de notificar a la tarea del conversor ADC.
 *
 * @param param
 */
void FuncTimerA(void *param)
{
	vTaskNotifyGiveFromISR(conversor_adc_task_handle, pdFALSE);
}

/**
 * @brief Función del Timer B encargada de notificar a la tarea del conversor DAC.
 *
 * @param param
 */
void FuncTimerB(void *param)
{
	vTaskNotifyGiveFromISR(conversor_dac_task_handle, pdFALSE);
}

/**
 * @brief Conversor ADC que convierte un dato analógico en digital y lo envía por el puerto serie.
 *
 * @param pvParameter
 */
static void conversorADC(void *pvParameter)
{
	uint16_t valor_adc = 0;
	while (true)
	{
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
		AnalogInputReadSingle(CH1, &valor_adc);
		UartSendString(UART_PC, (char *)UartItoa(valor_adc, 10));
		UartSendString(UART_PC, "\r\n");
	}
}

/**
 * @brief Conversor DAC que convierte un dato digital en analógico para permitir la conversión de señales.
 *
 * @param pvParameter
 */
static void conversorDAC(void *pvParameter)
{
	uint8_t indice = 0;
	while (true)
	{
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
		AnalogOutputWrite(ecg[indice]);
		indice = indice + 1;
		if (indice > (BUFFER_SIZE - 1))
		{
			indice = 0;
		}
	}
}

/*==================[external functions definition]==========================*/
/**
 * @brief Aplicación que permite convertir una señal de ecg digital en una señal analógica con el conversor DAC y luego con el conversor ADC se convierte dicha señal en digital y se envía por el puerto serie para su visualización con un software externo.
 *
 */
void app_main(void)
{
	timer_config_t timer_adc = {
		.timer = TIMER_A,
		.period = CONFIG_BLINK_PERIOD_MUESTREO_US,
		.func_p = FuncTimerA,
		.param_p = NULL};
	TimerInit(&timer_adc);

	timer_config_t timer_dac = {
		.timer = TIMER_B,
		.period = CONFIG_BLINK_PERIOD_VISUALIZACION_US,
		.func_p = FuncTimerB,
		.param_p = NULL};
	TimerInit(&timer_dac);

	serial_config_t my_uart = {
		.port = UART_PC,
		.baud_rate = 115200,
		.func_p = NULL,
		.param_p = NULL};
	UartInit(&my_uart);

	analog_input_config_t adc_conversor = {
		.input = CH1,
		.mode = ADC_SINGLE,
		.func_p = NULL,
		.param_p = NULL,
		.sample_frec = 0};
	AnalogInputInit(&adc_conversor);

	AnalogOutputInit();

	xTaskCreate(&conversorADC, "conversorADC", 512, NULL, 5, &conversor_adc_task_handle);
	xTaskCreate(&conversorDAC, "conversorDAC", 512, NULL, 5, &conversor_dac_task_handle);

	TimerStart(timer_adc.timer);
	TimerStart(timer_dac.timer);
}
/*==================[end of file]============================================*/