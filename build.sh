#!/bin/bash

# Trava o script se der algum erro no meio do caminho
set -e

echo "=========================================="
echo " Iniciando Build Automático do LoRaWAN..."
echo "=========================================="

# Garante que o script vai rodar na pasta raiz onde o script está salvo, 
# evitando criar "build/build" não importa de onde você chame o comando.
cd "$(dirname "$0")"

# 1. Exporta a variável do FreeRTOS
export FREERTOS_KERNEL_PATH=$(pwd)/FreeRTOS-Kernel
echo "[1/4] FreeRTOS configurado em: $FREERTOS_KERNEL_PATH"

# 2. Limpa o ambiente antigo
echo "[2/4] Limpando compilações antigas..."
rm -rf build

# 3. Cria o novo ambiente
echo "[3/4] Preparando o CMake..."
mkdir build
cd build
cmake ..

# 4. Compila o código usando os 4 núcleos do processador
echo "[4/4] Compilando o firmware..."
make -j4

echo "=========================================="
echo " 🎉 SUCESSO! O arquivo main.uf2 está pronto na pasta build/"
echo "=========================================="