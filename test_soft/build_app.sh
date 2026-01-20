#!/bin/bash

# 1. Компиляция (без стандартных библиотек и с поддержкой коротких wchar для UEFI)
gcc -ffreestanding -fshort-wchar -m64 -c hello.c -o hello.o

# 2. Линковка в чистый бинарник
ld -T app.ld hello.o -o hello.bin

# 3. Формирование PEX заголовка (структура из вашего pex.h)
# Signature: 'M', 'Z' (2 байта)
# Version: 1 (1 байт)
# EntryPoint: 0 (4 байта)
# ProgramSize: размер hello.bin (4 байта)
# StackSize: 4096 (4 байта)
# Остальное — нули

printf "MZ\x01\x00\x00\x00\x00" > p.pex
# Размер программы (Little Endian)
stat -c%s hello.bin | xargs printf "%08x" | sed 's/\(..\)\(..\)\(..\)\(..\)/\\x\4\\x\3\\x\2\\x\1/' | xargs printf >> p.pex
# Размер стека (4096 = 0x1000)
printf "\x00\x10\x00\x00" >> p.pex
# Заполнение нулями до 64 байт (имя программы и прочее)
printf "HelloApp" >> p.pex
truncate -s 64 p.pex

# 4. Склеивание
cat hello.bin >> p.pex

echo "Готово! Файл p.pex создан."