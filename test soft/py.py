import os
import subprocess
import struct
import platform

def build_pex(bin_path, pex_path, memory_mb=2, min_rights=0, deps=""):
    """Упаковывает сырой бинарник в новый формат PEX для ParrotOS."""
    if not os.path.exists(bin_path):
        print(f"Ошибка: Файл {bin_path} не найден!")
        return False

    with open(bin_path, "rb") as f:
        code = f.read()
    
    # Структура нового заголовка PEX (89 байт)
    magic = b"PEX"
    version = 1
    header_size = 89 
    entry_point = header_size # Код начинается сразу после заголовка
    program_size = len(code)
    
    # Строка зависимостей (строго 64 байта, заполнена нулями в конце)
    deps_bytes = deps.encode('ascii')[:63].ljust(64, b'\x00')
    
    # Упаковка данных (Little Endian)
    header = struct.pack("<3s B I Q Q B 64s", 
                         magic, version, entry_point, memory_mb, 
                         program_size, min_rights, deps_bytes)
    
    os.makedirs(os.path.dirname(pex_path), exist_ok=True)
    with open(pex_path, "wb") as f:
        f.write(header + code)
        
    print(f"[{pex_path}] Собран (Заголовок PEX). Размер: {len(header) + len(code)} байт.")
    return True

def build():
    print("=== Компиляция ParrotOS PEX ===")
    
    # 1. Компиляция C-кода
    res1 = subprocess.run("gcc -ffreestanding -fshort-wchar -m64 -c hello.c -o hello.o", shell=True)
    res2 = subprocess.run("ld -T app.ld hello.o -o hello.bin", shell=True)
    
    if res1.returncode != 0 or res2.returncode != 0:
        print("Ошибка компиляции! Проверь код hello.c")
        return

    # 2. Создание start.pex с новым заголовком
    if not build_pex("hello.bin", "usb_root/start.pex"):
        return

    # 3. Пересборка образа диска
    print("Создание файловой системы boot.img...")
    subprocess.run("dd if=/dev/zero of=boot.img bs=1M count=2 status=none", shell=True)
    subprocess.run("mkfs.vfat boot.img", shell=True, stdout=subprocess.DEVNULL)
    subprocess.run("mcopy -i boot.img -s usb_root/* ::/", shell=True)

    # 4. Настройка параметров QEMU
    system = platform.system()
    accel = ""
    if system == "Linux":
        accel = "-enable-kvm -cpu host"
    elif system == "Windows":
        accel = "-accel whpx -cpu host"
        
    ovmf_path = "/usr/share/ovmf/OVMF.fd" if system == "Linux" else "OVMF.fd"
    
    # ВАЖНО: Добавлен -display sdl,gl=off для починки ошибки qemu_mutex_lock в WSL
    # Замени qemu_cmd в своем скрипте на это:
    qemu_cmd = (
        f"qemu-system-x86_64 "
        f"-m 512M "
        f"-drive if=pflash,format=raw,readonly=on,file=/usr/share/ovmf/OVMF.fd " # Убедись, что этот файл существует!
        f"-drive format=raw,file=boot.img " # Явно указали format=raw
        f"{accel} "
        f"-vga std -display sdl,gl=off -monitor stdio"
    )
    print(f"QEMU команда: {qemu_cmd}")
    # 5. Безопасный запуск эмулятора
    print("Сборка завершена. Запуск QEMU...")
    try:
        subprocess.run(qemu_cmd, shell=True)
    except KeyboardInterrupt:
        # Прячем уродливый красный текст питона при закрытии через Ctrl+C
        print("\n[Эмулятор остановлен пользователем]")

if __name__ == "__main__":
    build()