/**
 * =====================================================================================
 * Filename:  main.cpp
 * Version:  2.0 (Debug Enhanced)
 * Institution: UFC - Campus Quixadá
 * =====================================================================================
 */
#include <stdio.h>
#include "pico/stdlib.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "hardware/pwm.h"
#include "hardware/spi.h"
#include "hardware/gpio.h"
#include "pico/binary_info.h"
#include "hardware/watchdog.h"
#include "hardware/vreg.h"
#include <string.h>

// Includes do Projeto
#include "../includes/pluviometro.hpp"
#include "lorawan_task.h"
#include "spi_diagnostic.h"

// Definições de Hardware
#define LED_PIN         25
#define RADIO_NSS       17
#define RADIO_SCK       18
#define RADIO_MOSI      19
#define RADIO_MISO      16
#define RADIO_RST       15

// UART Debug
#define UART_PRINT_ID        uart0
#define UART_PRINT_TX        0
#define UART_PRINT_RX        1
#define UART_PRINT_BAUD_RATE 9600

// Globais
SemaphoreHandle_t xLoRaInitSemaphore = NULL;
extern uint slice_num; // Definido no pluviometro.cpp
extern "C" {
    uint8_t ucHeap[configTOTAL_HEAP_SIZE] __attribute__((aligned(8)));
}
// Protótipos
void vLoRaSenderTask(void *pvParameters);
void uart_config(void);
bool check_sx1276_spi(void);

// ============================================================================
// DIAGNÓSTICO SPI (Crucial para ver se o rádio está vivo)
// ============================================================================
bool check_sx1276_spi() {
    printf("[DIAG] Testando comunicacao SPI com SX1276...\n");
    
    // O registrador 0x42 (RegVersion) deve retornar sempre 0x12
    uint8_t reg = 0x42;
    uint8_t version = 0;
    
    gpio_put(RADIO_NSS, 0); // Seleciona o rádio
    sleep_ms(1);
    spi_write_blocking(spi0, &reg, 1);
    spi_read_blocking(spi0, 0x00, &version, 1);
    sleep_ms(1);
    gpio_put(RADIO_NSS, 1); // Deseleciona
    
    printf("[DIAG] Reg 0x42 (Version) lido: 0x%02X\n", version);
    
    if (version == 0x12) {
        printf("[DIAG] SUCESSO: SX1276 detectado!\n");
        return true;
    } else {
        printf("[DIAG] ERRO: SX1276 nao respondeu ou versao incorreta.\n");
        printf("       Verifique se o radio tem 3.3V e se MISO/MOSI estao corretos.\n");
        return false;
    }
}

// ============================================================================
// MAIN
// ============================================================================
int main() {
    // 1. Inicialização Básica e LED
    stdio_init_all();
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_put(LED_PIN, 1); // LED aceso indica "Energia OK"

    // 2. Aguarda USB Serial (com timeout para não travar em campo)
    uint32_t usb_timeout = 0;
    while (!stdio_usb_connected() && usb_timeout < 50) {
        sleep_ms(100);
        usb_timeout++;
    }

    printf("\n\n======================================\n");
    printf("    UFC QUIXADA - LORAWAN DEBUG       \n");
    printf("======================================\n");

    // 3. Verificação de causa do Reboot
    if (watchdog_caused_reboot()) {
        printf("[ALERTA] Reboot por WATCHDOG! Cheque a fonte de energia.\n");
    } else {
        printf("[INFO] Partida limpa (Power-on).\n");
    }

    // Nota sobre o vreg: 11 significa VREG_VOLTAGE_1_10 (1.1V), o que é NORMAL.
    printf("[INFO] VREG Core Voltage Code: %d\n", vreg_get_voltage());

    // 4. Configuração UART e Hardware SPI para diagnóstico
    uart_config();
    
    // Inicializa SPI0 para o diagnóstico (mesmos pinos da lorawan_task)
    spi_init(spi0, 1000000); // 1MHz para teste
    gpio_set_function(RADIO_SCK, GPIO_FUNC_SPI);
    gpio_set_function(RADIO_MOSI, GPIO_FUNC_SPI);
    gpio_set_function(RADIO_MISO, GPIO_FUNC_SPI);
    
    gpio_init(RADIO_NSS);
    gpio_set_dir(RADIO_NSS, GPIO_OUT);
    gpio_put(RADIO_NSS, 1); // NSS deve começar em HIGH

    // Realiza o teste de hardware antes de subir o RTOS
    if (!check_sx1276_spi()) {
        printf("[ERRO FATAL] Hardware LoRa inacessivel. Verifique conexoes.\n");
        // Pisca rápido para erro de hardware
        while(1) { gpio_put(LED_PIN, 1); sleep_ms(50); gpio_put(LED_PIN, 0); sleep_ms(50); }
    }

    // 5. Recursos do FreeRTOS
    xLoRaInitSemaphore = xSemaphoreCreateBinary();
    if (xLoRaInitSemaphore == NULL) {
        printf("[ERRO] Falha ao criar semaforo.\n");
        return -1;
    }

    // 6. Criação de Tasks
    printf("[RTOS] Criando Tasks...\n");
    
    BaseType_t r1 = xTaskCreate(vLoRaWanTask, "LoRaStack", 4096, NULL, 4, NULL);
    BaseType_t r2 = xTaskCreate(vLoRaSenderTask, "LoRaSender", 4096, NULL, 2, NULL);

    if (r1 != pdPASS || r2 != pdPASS) {
        printf("[ERRO FATAL] Memoria insuficiente (Heap).\n");
        while(1);
    }

    printf("[RTOS] Iniciando Scheduler...\n");
    vTaskStartScheduler();

    // Se chegar aqui, o heap acabou
    while (1) {
        printf("Falha no Scheduler!\n");
        sleep_ms(1000);
    }

    return 0;
}

// ============================================================================
// TASK: Sender
// ============================================================================
// ============================================================================
// TASK: Sender
// ============================================================================
void vLoRaSenderTask(void *pvParameters) {
    printf("[Sender] Aguardando Semáforo LoRa...\n");

    if (xSemaphoreTake(xLoRaInitSemaphore, portMAX_DELAY) == pdTRUE) {
        printf("[Sender] Autorizado! Iniciando loop.\n");
    }

    while (true) {
        char payload_str[100]; 

        float Int_calculada = 0.000121f; 

        snprintf(payload_str, sizeof(payload_str), "c|%f", Int_calculada);

        printf("[Sender] Preparando envio (String): %s\n", payload_str);

        uart_tx_wait_blocking(UART_PRINT_ID); 

        LoRaWan_Send((uint8_t*)payload_str, strlen(payload_str));

        // Feedback Visual
        for (int i = 0; i < 6; i++) {
            gpio_put(LED_PIN, !gpio_get(LED_PIN));
            vTaskDelay(pdMS_TO_TICKS(100));
        }
        gpio_put(LED_PIN, 0);

        vTaskDelay(pdMS_TO_TICKS(15000)); // Envia a cada 120s
    }
}

void uart_config(void) {
    uart_init(UART_PRINT_ID, UART_PRINT_BAUD_RATE);
    gpio_set_function(UART_PRINT_TX, GPIO_FUNC_UART);
    gpio_set_function(UART_PRINT_RX, GPIO_FUNC_UART);
}
