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
/**
 * @brief Estructura que permite configurar los GPIOs
 * 
 */
typedef struct
{
	gpio_t pin; /*!< GPIO pin number */
	io_t dir;	/*!< GPIO direction '0' IN;  '1' OUT*/
} gpioConf_t;

/*==================[internal functions declaration]=========================*/
/**
 * @brief Función que permite separar un numero en sus digitos
 * 
 * @param data Número a separar
 * @param digits Cantidad de digitos del número
 * @param bcd_number Arreglo que contiene cada uno de los digitos separados
 */
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
/**
 * @brief Función que permite cambiar el estado de un GPIO con el valor del digito
 * 
 * @param digito_bcd Digito de 8 bits que se desea cargar en los GPIOs
 * @param pines_gpio Arreglo de GPIOs a los cuales se les modifica el estado
 */
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
/**
 * @brief Función que permite cambiar el estados de los GPIOs de datos y control del lcd
 * 
 * @param dato Número de 32 bits a mostrar en el display
 * @param digitos Cantidad de digitos del número a mostrar
 * @param pines_gpio Arreglos de GPIOs de datos
 * @param pines_lcd Arreglo de GPIOs de lcd
 */
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
/**
 * @brief Prueba de visualización de un número de 3 digitos en el display lcd
 * 
 */
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
