#include "include/pex.h"
#include "include/task.h"
#include "include/drivers/fat32.h"
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>

// Определение структуры должно быть идентичным в ядре и в API (PEX)
// Используем packed, чтобы компилятор не добавлял лишние отступы
#pragma pack(push, 1)
struct y {
    INT32 id;
    VOID (EFIAPI *vy)(VOID); // EFIAPI гарантирует правильное соглашение о вызовах
};
#pragma pack(pop)

/**
 * Находит свободный слот для задачи
 */
INT32 FindFreeTaskSlot(VOID) {
    for (INT32 i = 1; i < MAX_TASKS; i++) {
        if (!tasks[i].active) {
            return i;
        }
    }
    return -1;
}

/**
 * Тестовая функция-callback, которую вызовет PEX-модуль
 */
VOID EFIAPI vy(VOID) {
    Print(L"Callback от ядра: Test OK\n");
}

/**
 * Загружает PEX файл с диска и запускает его как новую задачу с аргументами
 */
EFI_STATUS LoadAndStartPex(CHAR16* Path) {
    EFI_STATUS Status;
    EC16 file;
    
    // 1. Ищем свободное место в таблице задач
    INT32 id = FindFreeTaskSlot();
    if (id == -1) {
        Print(L"Ошибка: Нет свободных слотов для задач.\n");
        return EFI_OUT_OF_RESOURCES;
    }

    // 2. Читаем файл с диска
    Status = ReadFileByPath(Path, &file);
    if (EFI_ERROR(Status)) {
        Print(L"Ошибка загрузки файла %s: %r\n", Path, Status);
        return Status;
    }

    // 3. Выделяем память под структуру аргументов.
    // Мы используем динамическую память (AllocatePool), чтобы у каждой задачи
    // были свои собственные данные, и они не затирались.
    struct y* arg_data = AllocatePool(sizeof(struct y));
    if (arg_data == NULL) {
        return EFI_OUT_OF_RESOURCES;
    }

    arg_data->id = id;
    arg_data->vy = vy;

    // 4. Создаем задачу
    // Передаем:
    // id - номер слота
    // file.Message - указатель на начало кода в памяти (точка входа)
    // arg_data - указатель на нашу структуру, который попадет в RCX и RDI
    Status = task_create_with_arg(id, (VOID (*)(VOID*))file.Message, arg_data);
    
    if (EFI_ERROR(Status)) {
        Print(L"Ошибка создания задачи: %r\n", Status);
        FreePool(arg_data);
        return Status;
    }

    Print(L"PEX %s успешно загружен. Task ID: %d\n", Path, id);
    
    return EFI_SUCCESS;
}