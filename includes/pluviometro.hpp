/*
 * =====================================================================================
 *
 *       Filename:  pluviometro.hpp
 *
 *    Description:  -
 *
 *        Version:  1.0
 *        Created:  16/01/2025 13:04:48
 *       Revision:  none
 *       Compiler:  -
 *
 *         Author:  Isaac Vinicius, isaacvinicius2121@alu.ufc.br
 *   Organization:  UFC-Quixadá
 *
 * =====================================================================================
*/

#ifndef PLUVIOMETRO_HPP
#define PLUVIOMETRO_HPP

#include "hardware/uart.h"
#include "hardware/gpio.h"



#define SENSOR_HALL_PIN 7       /* gpio sensor hall */
#define DEBOUNCE_DELAY 200      /* Debounce de 200 ms */
#define PRECIPITACAO  0.350404  /* Precipitação por tombo: 0.350404 mm */

void inicializa_sensor_pluviometro(uint8_t gpio);
#endif
/*****************************END OF FILE**************************************/