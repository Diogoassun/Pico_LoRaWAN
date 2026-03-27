/**
 * =====================================================================================
 * spi_diagnostic.c - Teste de diagnóstico SPI/SX1276 ANTES da stack LoRaWAN
 * 
 * COMO USAR:
 *   1. No main.cpp, substitua temporariamente xTaskCreate(vLoRaWanTask...) por
 *      xTaskCreate(vSpiDiagnosticTask, "SPIDiag", 4096, NULL, 4, NULL);
 *   2. Observe o output serial.
 *   3. Após confirmar "RADIO OK", volte ao vLoRaWanTask normal.
 * =====================================================================================
 */

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/gpio.h"
#include "FreeRTOS.h"
#include "task.h"

// Pinos — devem bater exatamente com sx1276-board.c
#define DIAG_SCK        18
#define DIAG_MOSI       19
#define DIAG_MISO       16
#define DIAG_NSS        17
#define DIAG_RESET      15
#define DIAG_DIO0       2

// Registradores do SX1276
#define REG_VERSION     0x42    // Deve retornar 0x12
#define REG_OPMODE      0x01
#define REG_PACONFIG    0x09

// -----------------------------------------------------------------------
// Helpers de SPI raw (sem a stack, para diagnóstico puro)
// -----------------------------------------------------------------------

static void spi_select(void)   { gpio_put(DIAG_NSS, 0); }
static void spi_deselect(void) { gpio_put(DIAG_NSS, 1); }

static uint8_t spi_read_reg(uint8_t reg)
{
    uint8_t addr = reg & 0x7F; // bit 7 = 0 para leitura
    uint8_t val  = 0;
    spi_select();
    spi_write_blocking(spi0, &addr, 1);
    spi_read_blocking(spi0, 0x00, &val, 1);
    spi_deselect();
    return val;
}

static void spi_write_reg(uint8_t reg, uint8_t val)
{
    uint8_t buf[2] = { (uint8_t)(reg | 0x80), val }; // bit 7 = 1 para escrita
    spi_select();
    spi_write_blocking(spi0, buf, 2);
    spi_deselect();
}

// -----------------------------------------------------------------------
// Task de Diagnóstico
// -----------------------------------------------------------------------

void vSpiDiagnosticTask(void *pvParameters)
{
    printf("\n========================================\n");
    printf("  SX1276 SPI DIAGNOSTIC\n");
    printf("========================================\n");

    // --- PASSO 1: Garante NSS alto antes de tudo ---
    gpio_init(DIAG_NSS);
    gpio_set_dir(DIAG_NSS, GPIO_OUT);
    gpio_put(DIAG_NSS, 1);
    vTaskDelay(pdMS_TO_TICKS(5));

    // --- PASSO 2: Inicializa SPI ---
    printf("[1] Inicializando SPI0 a 1MHz...\n");
    spi_init(spi0, 1 * 1000 * 1000);
    gpio_set_function(DIAG_SCK,  GPIO_FUNC_SPI);
    gpio_set_function(DIAG_MOSI, GPIO_FUNC_SPI);
    gpio_set_function(DIAG_MISO, GPIO_FUNC_SPI);
    vTaskDelay(pdMS_TO_TICKS(5));

    // --- PASSO 3: Reset físico ---
    printf("[2] Executando reset do radio...\n");
    gpio_init(DIAG_RESET);
    gpio_set_dir(DIAG_RESET, GPIO_OUT);
    gpio_put(DIAG_RESET, 0);
    vTaskDelay(pdMS_TO_TICKS(10));
    gpio_put(DIAG_RESET, 1);
    vTaskDelay(pdMS_TO_TICKS(10));
    printf("    Reset concluido.\n");

    // --- PASSO 4: Lê versão (3 tentativas) ---
    printf("[3] Lendo registrador VERSION (0x42)...\n");
    uint8_t version = 0;
    for (int t = 0; t < 3; t++) {
        version = spi_read_reg(REG_VERSION);
        printf("    Tentativa %d: 0x%02X\n", t + 1, version);
        vTaskDelay(pdMS_TO_TICKS(50));
    }

    if (version == 0x12) {
        printf("    [OK] Radio detectado corretamente!\n");
    } else if (version == 0x00) {
        printf("    [ERRO] Retornou 0x00 — MISO preso em GND ou NSS nao funcionando.\n");
        printf("           Verifique: solda do MISO (GPIO16), solda do NSS (GPIO17).\n");
    } else if (version == 0xFF) {
        printf("    [ERRO] Retornou 0xFF — MISO preso em VCC ou SPI invertido.\n");
        printf("           Verifique: MOSI/MISO nao estao trocados? Nivel logico 3.3V?\n");
    } else {
        printf("    [AVISO] Valor inesperado 0x%02X — possivel modulo diferente ou ruido.\n", version);
    }

    // --- PASSO 5: Teste de escrita/leitura ---
    printf("\n[4] Teste de escrita/leitura (RegOpMode)...\n");
    // Coloca em sleep mode (0x00) para escrita segura
    spi_write_reg(REG_OPMODE, 0x00);
    vTaskDelay(pdMS_TO_TICKS(10));
    uint8_t opmode = spi_read_reg(REG_OPMODE);
    printf("    Escreveu 0x00, leu 0x%02X — %s\n",
           opmode, opmode == 0x00 ? "[OK]" : "[FALHA] SPI so leitura? Verifique MOSI.");

    // Testa modo LoRa (0x80)
    spi_write_reg(REG_OPMODE, 0x80);
    vTaskDelay(pdMS_TO_TICKS(10));
    opmode = spi_read_reg(REG_OPMODE);
    printf("    Escreveu 0x80, leu 0x%02X — %s\n",
           opmode, opmode == 0x80 ? "[OK] Modo LoRa ativado." : "[FALHA]");

    // --- PASSO 6: Diagnóstico do DIO0 ---
    printf("\n[5] Estado do DIO0 (GPIO%d) em repouso: %d (esperado: 0)\n",
           DIAG_DIO0, gpio_get(DIAG_DIO0));

    // --- PASSO 7: Teste de velocidade SPI ---
    printf("\n[6] Repetindo leitura a 8MHz...\n");
    spi_set_baudrate(spi0, 8 * 1000 * 1000);
    vTaskDelay(pdMS_TO_TICKS(5));
    version = spi_read_reg(REG_VERSION);
    printf("    VERSION a 8MHz: 0x%02X — %s\n",
           version, version == 0x12 ? "[OK]" : "[FALHA] Use 1MHz.");

    // --- RESUMO FINAL ---
    printf("\n========================================\n");
    if (version == 0x12) {
        printf("  RESULTADO: RADIO OK — pode usar a stack.\n");
        printf("  Substitua vSpiDiagnosticTask por vLoRaWanTask no main.cpp.\n");
    } else {
        printf("  RESULTADO: FALHA DE COMUNICACAO SPI.\n");
        printf("  Checklist:\n");
        printf("  [ ] Fios SCK/MOSI/MISO/NSS/RESET conectados corretamente?\n");
        printf("  [ ] Modulo alimentado em 3.3V (nao 5V)?\n");
        printf("  [ ] Solda/jumper firme no modulo?\n");
        printf("  [ ] Pino MISO tem pull-up externo necessario?\n");
    }
    printf("========================================\n\n");

    // Mantém a task viva piscando LED para indicar fim do diagnóstico
    gpio_init(25);
    gpio_set_dir(25, GPIO_OUT);
    while (1) {
        gpio_put(25, 1); vTaskDelay(pdMS_TO_TICKS(500));
        gpio_put(25, 0); vTaskDelay(pdMS_TO_TICKS(500));
    }
}
