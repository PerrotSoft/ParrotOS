import os
import subprocess
import sys

def build_pdl():
    print("=== Сборка динамической библиотеки PDL ===")
    
    # 1. Компиляция C-кода в объектный файл с RIP-относительной адресацией
    gcc_cmd = (
        "gcc -ffreestanding -fshort-wchar -fPIC -m64 -fno-exceptions "
        "-fno-asynchronous-unwind-tables -O1 -c main.c -o main.o"
    )
    print(f"[{gcc_cmd}]")
    if subprocess.run(gcc_cmd, shell=True).returncode != 0:
        print("Ошибка: сбой компиляции main.c!")
        sys.exit(1)

    # 2. Скрипт компоновщика для сборки плоского бинарного файла (Flat Binary)
    # SUBALIGN(1) на .pdl_header и .pdl_exports — критично важно.
    # Без этого ld может выровнять начало .pdl_exports (например, на 32 байта),
    # вставив нулевой паддинг между заголовком и таблицей экспорта. Тогда
    # export_table_offset, вычисленный в main.c как sizeof(PDL_Header),
    # перестаёт совпадать с реальным расположением данных в файле, и
    # PdlGetProcAddress не находит ни одной функции (все символы NULL).
    ld_script = """
    OUTPUT_FORMAT("binary")
    SECTIONS {
        . = 0x0;
        .pdl_header : SUBALIGN(1) { *(.pdl_header) }
        .pdl_exports : SUBALIGN(1) { *(.pdl_exports) }
        .text : { *(.text*) }
        .rodata : { *(.rodata*) }
        .data : { *(.data*) }
        .bss : { *(.bss*) *(COMMON) }
        /DISCARD/ : { *(.comment) *(.note*) *(.eh_frame*) }
    }
    """
    with open("pdl.ld", "w") as f:
        f.write(ld_script)

    # 3. Линковка
    ld_cmd = "ld -T pdl.ld main.o -o main.pdl"
    print(f"[{ld_cmd}]")
    if subprocess.run(ld_cmd, shell=True).returncode != 0:
        print("Ошибка: сбой линковки main.pdl!")
        sys.exit(1)

    os.remove("pdl.ld")
    os.remove("main.o")
    print("Успешно: файл main.pdl готов к копированию на диск A:\\!")

if __name__ == "__main__":
    build_pdl()