#!/bin/bash

# Цвета для красоты и читаемости
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color

PROJECT_ROOT="/mnt/c/Users/perro/Desktop/Uefi_ParrotOS"
EDK2_PATH="$PROJECT_ROOT/edk2"

echo -e "${YELLOW}=== Parrot OS Build Environment Manager ===${NC}"
echo "Выберите действия (введите y/n):"

# --- ОПРОС ПОЛЬЗОВАТЕЛЯ ---

read -p "Переустановить EDK2 с нуля? (Удалит папку edk2 и скачает заново) [y/N]: " REINSTALL_EDK
read -p "Пересобрать BaseTools? (Нужно, если не видит GCC5) [y/N]: " BUILD_TOOLS
read -p "Сбросить конфигурацию (target.txt, tools_def.txt)? [y/N]: " RESET_CONF
read -p "Очистить папку Build и временные файлы? [y/N]: " CLEAN_BUILD
read -p "Запустить компиляцию проекта после настройки? [y/N]: " RUN_BUILD

echo -e "\n${GREEN}[*] Начинаем работу...${NC}"

# 1. ПОЛНАЯ ПЕРЕУСТАНОВКА EDK2
if [[ "$REINSTALL_EDK" == "y" || "$REINSTALL_EDK" == "Y" ]]; then
    echo -e "${RED}[!] Удаление старого EDK2...${NC}"
    cd "$PROJECT_ROOT"
    rm -rf edk2
    echo "[*] Клонирование репозитория..."
    git clone --recursive https://github.com/tianocore/edk2.git
    cd edk2
    # Сразу ставим зависимости системы
    sudo apt-get update && sudo apt-get install -y build-essential uuid-dev iasl git gcc python3 nasm
fi

cd "$EDK2_PATH" || { echo -e "${RED}Ошибка: папка edk2 не найдена!${NC}"; exit 1; }

# 2. ПЕРЕСБОРКА BASETOOLS
if [[ "$BUILD_TOOLS" == "y" || "$BUILD_TOOLS" == "Y" ]]; then
    echo -e "${YELLOW}[*] Компиляция BaseTools...${NC}"
    make -C BaseTools
fi

# 3. СБРОС КОНФИГУРАЦИИ
if [[ "$RESET_CONF" == "y" || "$RESET_CONF" == "Y" ]]; then
    echo -e "${YELLOW}[*] Обновление конфигурационных файлов...${NC}"
    rm -rf Conf/*.txt
    source edksetup.sh
    
    # Настройка под твой проект (GCC5 и X64)
    sed -i 's/^TOOL_CHAIN_TAG.*/TOOL_CHAIN_TAG        = GCC5/' Conf/target.txt
    sed -i 's/^TARGET_ARCH.*/TARGET_ARCH           = X64/' Conf/target.txt
    sed -i 's|^ACTIVE_PLATFORM.*|ACTIVE_PLATFORM       = ParrotOS_Build/Minimal.dsc|' Conf/target.txt
    
    # Исправление префикса для GCC5
    if ! grep -q "GCC5_X64_PREFIX" Conf/tools_def.txt; then
        echo "DEFINE GCC5_X64_PREFIX = /usr/bin/" >> Conf/tools_def.txt
    fi
fi

# 4. ОЧИСТКА ВРЕМЕННЫХ ФАЙЛОВ
if [[ "$CLEAN_BUILD" == "y" || "$CLEAN_BUILD" == "Y" ]]; then
    echo -e "${YELLOW}[*] Очистка папок сборки...${NC}"
    rm -rf "$PROJECT_ROOT/Build"
    rm -rf "$EDK2_PATH/Build"
fi

# 5. ЗАПУСК СБОРКИ
if [[ "$RUN_BUILD" == "y" || "$RUN_BUILD" == "Y" ]]; then
    echo -e "${GREEN}[*] Запуск компиляции Parrot OS...${NC}"
    source edksetup.sh
    bash "$PROJECT_ROOT/ParrotOS_Build/build_and_run.sh"
fi

echo -e "\n${GREEN}=== РАБОТА ЗАВЕРШЕНА ===${NC}"