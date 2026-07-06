#include "../include/pdl.h"
#include <Library/UefiBootServicesTableLib.h>
#include <Library/BaseMemoryLib.h>
#include "../include/drivers/DriverManager.h"

EFI_STATUS PdlLoad(CHAR16* Path, PDL_LIBRARY** OutLib) {
    if (!Path || !OutLib) return EFI_INVALID_PARAMETER;

    EC16 file;
    EFI_STATUS Status = ReadFileByPath(Path, &file);
    if (EFI_ERROR(Status) || !file.Message || file.FileSize < sizeof(PDL_HEADER)) {
        return EFI_NOT_FOUND;
    }

    // Быстрая проверка заголовка ЕЩЁ ДО выделения исполняемой памяти!
    PDL_HEADER* rawHdr = (PDL_HEADER*)file.Message;
    
    // 1. Проверяем сигнатуру '_PDL'
    if (rawHdr->Magic != PDL_MAGIC) {
        gBS->FreePool(file.Message);
        return EFI_UNSUPPORTED; 
    }

    // 2. КРИТИЧЕСКАЯ ПРОВЕРКА: Совпадает ли архитектура библиотеки с процессором?
    if (rawHdr->Architecture != CURRENT_PDL_ARCH) {
        // Если попытались загрузить 32-битную DLL в 64-битную ОС (или наоборот)
        gBS->FreePool(file.Message);
        return EFI_INCOMPATIBLE_VERSION; 
    }

    // Выделяем структуру управления
    PDL_LIBRARY* lib = NULL;
    Status = gBS->AllocatePool(EfiLoaderData, sizeof(PDL_LIBRARY), (VOID**)&lib);
    if (EFI_ERROR(Status)) {
        gBS->FreePool(file.Message);
        return Status;
    }

    // Выделяем память с правами исполнения кода (EfiLoaderCode)
    VOID* codeBuffer = NULL;
    Status = gBS->AllocatePool(EfiLoaderCode, file.FileSize, &codeBuffer);
    if (EFI_ERROR(Status)) {
        gBS->FreePool(file.Message);
        gBS->FreePool(lib);
        return Status;
    }

    gBS->CopyMem(codeBuffer, file.Message, file.FileSize);
    gBS->FreePool(file.Message);

    PDL_HEADER* hdr = (PDL_HEADER*)codeBuffer;

    lib->ImageBase   = codeBuffer;
    lib->ImageSize   = file.FileSize;
    lib->ExportCount = hdr->ExportCount;
    lib->Exports     = (PDL_EXPORT_ENTRY*)((UINT8*)codeBuffer + hdr->ExportTableOffset);

    *OutLib = lib;
    return EFI_SUCCESS;
}
static BOOLEAN AsciiStrEquals(const CHAR8* s1, const CHAR8* s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return (*s1 == *s2);
}
VOID PdlUnload(PDL_LIBRARY* Lib) {
    if (!Lib) return;
    if (Lib->ImageBase) {
        gBS->FreePool(Lib->ImageBase);
    }
    gBS->FreePool(Lib);
}

// 3. Получение абсолютного адреса функции по её имени
VOID* PdlGetProcAddress(PDL_LIBRARY* Lib, const CHAR8* ProcName) {
    if (!Lib || !ProcName || !Lib->ImageBase) return NULL;

    for (UINT32 i = 0; i < Lib->ExportCount; i++) {
        if (AsciiStrEquals(Lib->Exports[i].Name, ProcName)) {
            // Абсолютный адрес = База памяти + Смещение из таблицы
            return (VOID*)((UINT8*)Lib->ImageBase + Lib->Exports[i].Offset);
        }
    }
    return NULL;
}