/*! @mainpage guia1_ej4_5_6
 *
 * \section genDesc General Description
 *
 * En este proyecto se crean funciones para el manejo del display LCD para graficar numeros de 3 o menos digitos.
 * @section changelog Changelog
 *
 * |   Date	    | Description                                    |
 * |:----------:|:-----------------------------------------------|
 * |14/08/2024 | Document creation		                         |
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
#include "gpio_mcu.h"

/*==================[macros and definitions]=================================*/

/*==================[internal data definition]===============================*/
typedef struct
{
	gpio_t pin; /*!< GPIO pin number */
	io_t dir;	/*!< GPIO direction '0' IN;  '1' OUT*/
} gpioConf_t;

/*==================[internal functions declaration]=========================*/
void convertToBcdArray(uint32_t data, uint8_t digits, uint8_t *bcd_number)
{
	uint8_t resto;
	uint32_t var_aux = data;

	for (uint8_t i = 0; i < digits; i++)
	{
		resto = var_aux % 10;
		var_aux = var_aux / 10;
		bcd_number[(digits - 1) - i] = resto;
	}
}

void cambiarEstadoGPIO(uint8_t digito_bcd, gpioConf_t *pines_gpio)
{
	uint8_t mascara = 1;
	for (uint8_t i = 0; i < 4; i++)
	{
		if (digito_bcd & mascara)
		{
			GPIOOn(pines_gpio[i].pin);
		}
		else
		{
			GPIOOff(pines_gpio[i].pin);
		}
		mascara = mascara << 1;
	}
}

void cambiarEstado32BitsGPIO(uint32_t dato, uint8_t digitos, gpioConf_t *pines_gpio, gpioConf_t *pines_lcd)
{
	uint8_t arreglo_digitos[digitos];
	convertToBcdArray(dato, digitos, arreglo_digitos);
	for (uint8_t i = 0; i < digitos; i++)
	{
		cambiarEstadoGPIO(arreglo_digitos[i], pines_gpio);
		GPIOOn(pines_lcd[i].pin);
		GPIOOff(pines_lcd[i].pin);
	}
	
}
/*==================[external functions definition]==========================*/
void app_main(void)
{
	gpioConf_t arreglo_gpios[4] = {
		{GPIO_20, GPIO_OUTPUT},
		{GPIO_21, GPIO_OUTPUT},
		{GPIO_22, GPIO_OUTPUT},
		{GPIO_23, GPIO_OUTPUT}};

	gpioConf_t arreglo_gpios_lcd[3] = {
		{GPIO_19, GPIO_OUTPUT},
		{GPIO_18, GPIO_OUTPUT},
		{GPIO_9, GPIO_OUTPUT}};

	for (uint8_t i = 0; i < 4; i++)
	{
		GPIOInit(arreglo_gpios[i].pin, arreglo_gpios[i].dir);
	}
	for (uint8_t i = 0; i < 3; i++)
	{
		GPIOInit(arreglo_gpios_lcd[i].pin, arreglo_gpios_lcd[i].dir);
	}
	
	uint32_t numero = 788;
	cambiarEstado32BitsGPIO(numero, 3, arreglo_gpios, arreglo_gpios_lcd);
}
