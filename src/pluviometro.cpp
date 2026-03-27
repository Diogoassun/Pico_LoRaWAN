/*
 * =====================================================================================
 *
 *       Filename:  pluviometro.cpp
 *
 *    Description:  -
 *
 *        Version:  1.0
 *        Created:  16/01/2025 13:07:35
 *       Revision:  none
 *       Compiler:  -
 *
 *         Author:  Isaac Vinicius, isaacvinicius2121@alu.ufc.br
 *   Organization:  UFC-Quixadá
 *
 * =====================================================================================
*/

#include "../includes/pluviometro.hpp"
#include "hardware/pwm.h"


uint slice_num;

void inicializa_sensor_pluviometro(uint8_t gpio) {
  /* Inicializando GPIO do sensor Hall como entrada com pull-up interno */
  gpio_init(gpio);
  gpio_pull_up(gpio);
  gpio_set_dir(gpio, GPIO_IN);

  /* Verificando se a GPIO está no canal B do PWM (necessário para contagem correta) */
  if (pwm_gpio_to_channel(gpio) != PWM_CHAN_B){
    uart_puts(uart0, "ERROR - GPIO Must be PWM Channel B\n\r");
  }

  /* Obtendo o número do slice PWM correspondente à GPIO */
  slice_num = pwm_gpio_to_slice_num(gpio);

  /* Configurando PWM para contar pulsos na borda de descida do sinal */
  pwm_config cfg = pwm_get_default_config();
  pwm_config_set_clkdiv_mode(&cfg, PWM_DIV_B_FALLING);  // Contando apenas bordas de descida
  pwm_config_set_clkdiv(&cfg, 1);                       // Sem divisão adicional de clock

  /* Inicializando PWM com a configuração definida, sem iniciar ainda */
  pwm_init(slice_num, &cfg, false);

  /* Definindo a função da GPIO como saída de PWM */
  gpio_set_function(gpio, GPIO_FUNC_PWM);

  /* Ativando o PWM para iniciar a contagem de pulsos */
  pwm_set_enabled(slice_num, true);
}

/*****************************END OF FILE**************************************/