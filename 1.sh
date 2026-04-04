#!/usr/bin/env bash
set -e

# Текущая рабочая папка, где находится скрипт
ROOT_DIR="$(pwd)"
EDK_DIR="$ROOT_DIR/edk2"

echo "== Создание минимальной UEFI SDK в текущей папке =="

# 1. Зависимости
echo "[1/7] Установка зависимостей"
sudo apt update
sudo apt install -y \
  build-essential \
  uuid-dev \
  iasl \
  nasm \
  python3 \
  git

# 2. Удаляем старые папки
echo "[2/7] Подготовка каталогов"
rm -rf "$EDK_DIR" "$MINI_DIR" "$HELLO_DIR"

# 3. Клонирование edk2
echo "[3/7] Клонирование edk2"
git clone https://github.com/tianocore/edk2.git "$EDK_DIR"
cd "$EDK_DIR"
git submodule update --init --recursive

# 4. Инициализация edk2
echo "[4/7] Инициализация edk2"
source edksetup.sh

# 5. Сборка BaseTools
echo "[5/7] Сборка BaseTools"
make -C BaseTools