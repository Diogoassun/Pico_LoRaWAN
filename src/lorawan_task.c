/*
 * =====================================================================================
 * Filename:  lorawan_task.c
 * Description:  Task LoRaWAN com configurações para teste de consumo (SF12 + Max Power)
 * =====================================================================================
 */

#include "lorawan_task.h"
#include "pico/lorawan.h"
#include "LoRaMac.h"
#include "hardware/spi.h"
#include "hardware/gpio.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "pico/stdlib.h"
#include <stdio.h>
#include <string.h>

// ============================================================================
// 1. CONFIGURAÇÃO DE HARDWARE
// ============================================================================

static const struct lorawan_sx1276_settings sx1276_settings = {
    .spi = {
        .inst = spi0,
        .mosi = 19,
        .miso = 16,
        .sck  = 18,
        .nss  = 17,
    },
    .reset = 15,
    .dio0  = 20,
    .dio1  = 21,
};

// ============================================================================
// 2. CREDENCIAIS ABP
// ============================================================================

static const struct lorawan_abp_settings abp_settings = {
    .device_address      = "00012345",
    .network_session_key = "4C45CC20EF7B371C27C2CADBC9CFE486",
    .app_session_key     = "787093B500D63CCD81FE88D338BED172",
    .channel_mask        = "FF000000000000000000",   // AU915 FSB2
};

extern SemaphoreHandle_t xLoRaInitSemaphore;
static volatile bool is_joined = false;

// ============================================================================
// 4. TASK LORAWAN
// ============================================================================

void vLoRaWanTask(void *pvParameters)
{
    printf("[LoRaWAN] Inicializando para teste de consumo (SF12 + MAX POWER)...\n");

    lorawan_debug(true);

    if (lorawan_init_abp(&sx1276_settings, LORAMAC_REGION_AU915, &abp_settings) != 0) {
        printf("[LoRaWAN] ERRO FATAL: lorawan_init_abp falhou.\n");
        vTaskDelete(NULL);
        return;
    }

    lorawan_join();

    uint32_t timeout = 0;
    while (!lorawan_is_joined() && timeout < 100) {
        lorawan_process();
        vTaskDelay(pdMS_TO_TICKS(100));
        timeout++;
    }

    if (!lorawan_is_joined()) {
        printf("[LoRaWAN] ERRO: Join ABP nao confirmado.\n");
        vTaskDelete(NULL);
        return;
    }

    printf("[LoRaWAN] Dispositivo pronto.\n");
    is_joined = true;

    // ========================================================================
    // FORÇAR CONFIGURAÇÕES PARA TESTE DE FONTE DE BANCADA
    // ========================================================================
    MibRequestConfirm_t mibReq;

    // 1. FORÇAR POTÊNCIA MÁXIMA (Aumenta o valor da corrente no pico)
    mibReq.Type = MIB_CHANNELS_TX_POWER;
    mibReq.Param.ChannelsTxPower = 0; // 0 é a potência máxima na maioria das regiões
    LoRaMacMibSetRequestConfirm(&mibReq);
    printf("[TESTE] Potencia de transmissao configurada para o MAXIMO.\n");

    // 2. FORÇAR SF12 / DR_0 (Aumenta o tempo da transmissão para ~1 segundo)
    // Isso permite que o display da fonte de bancada "tenha tempo" de mostrar a corrente.
    mibReq.Type = MIB_CHANNELS_DATARATE;
    mibReq.Param.ChannelsDatarate = DR_0; // DR_0 em AU915 é SF12
    LoRaMacMibSetRequestConfirm(&mibReq);
    printf("[TESTE] DataRate configurado para DR_0 (SF12) - Transmissao Lenta.\n");
    // ========================================================================

    if (xLoRaInitSemaphore != NULL) {
        xSemaphoreGive(xLoRaInitSemaphore);
    }

    while (1) {
        lorawan_process();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void LoRaWan_Send(uint8_t *data, uint8_t size)
{
    if (data == NULL || size == 0 || !is_joined) return;

    int result = lorawan_send_unconfirmed(data, size, 1);

    if (result == 0) {
        printf("[LoRaWAN] Enviando pacote... Observe a fonte de bancada agora!\n");
    } else {
        printf("[LoRaWAN] Erro ao enfileirar: %d\n", result);
    }
}