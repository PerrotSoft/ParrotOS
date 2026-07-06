#ifndef PARROT_PDL_H
#define PARROT_PDL_H

#include <Uefi.h>

#define PDL_MAGIC 0x4C44505F // '_PDL' в Little Endian

// Идентификаторы поддерживаемых архитектур
#define PDL_ARCH_IA32   0x014C // x86 (32-бит)
#define PDL_ARCH_X86_64 0x8664 // x86_64 (64-бит)
#define PDL_ARCH_ARM64  0xAA64 // ARM64

// Автоматическое определение текущей архитектуры компилятора ядра
#if defined(MDE_CPU_X64) || defined(__x86_64__)
    #define CURRENT_PDL_ARCH PDL_ARCH_X86_64
#elif defined(MDE_CPU_IA32) || defined(__i386__)
    #define CURRENT_PDL_ARCH PDL_ARCH_IA32
#elif defined(MDE_CPU_AARCH64) || defined(__aarch64__)
    #define CURRENT_PDL_ARCH PDL_ARCH_ARM64
#else
    #define CURRENT_PDL_ARCH 0x0000
#endif

#pragma pack(push, 1)

// Обновленный заголовок динамической библиотеки (16 байт)
typedef struct {
    UINT32 Magic;              // '_PDL' (4 байта)
    UINT16 Version;            // Версия формата библиотеки, напр. 1 (2 байта)
    UINT16 Architecture;       // Архитектура: 0x8664 для x86_64 (2 байта)
    UINT32 ExportCount;        // Количество экспортируемых функций (4 байта)
    UINT32 ExportTableOffset;  // Смещение таблицы экспорта от начала файла (4 байта)
} PDL_HEADER;

// Запись о функции в таблице экспорта
typedef struct {
    CHAR8  Name[32];           // ASCII имя функции
    UINT64 Offset;             // Смещение кода функции от начала файла
} PDL_EXPORT_ENTRY;

#pragma pack(pop)

typedef struct {
    VOID* ImageBase;
    UINTN  ImageSize;
    UINT32 ExportCount;
    PDL_EXPORT_ENTRY* Exports;
} PDL_LIBRARY;

EFI_STATUS PdlLoad(CHAR16* Path, PDL_LIBRARY** OutLib);
VOID       PdlUnload(PDL_LIBRARY* Lib);
VOID* PdlGetProcAddress(PDL_LIBRARY* Lib, const CHAR8* ProcName);

#endif // PARROT_PDL_H