/*
 * =====================================================================================
 *
 * Filename:  lorawan_task.h
 *
 * Description:  Interface para a Task LoRaWAN e funções de envio
 *
 * =====================================================================================
*/

#ifndef LORAWAN_TASK_H
#define LORAWAN_TASK_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* * Inicializa a Task do FreeRTOS responsável por gerenciar a Stack LoRaWAN.
 * Deve ser passada para xTaskCreate no main.cpp.
 */
void vLoRaWanTask(void *pvParameters);

/* * Envia um pacote de dados via LoRaWAN.
 * data: Ponteiro para o array de bytes (payload)
 * size: Tamanho do array
 */
void LoRaWan_Send(uint8_t *data, uint8_t size);

#ifdef __cplusplus
}
#endif

#endif // LORAWAN_TASK_H