#include "include/pex.h"
#include "include/task.h"
#include "include/drivers/fat32.h"
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>

INT32 FindFreeTaskSlot(VOID) {
    for (INT32 i = 1; i < MAX_TASKS; i++) {
        if (!tasks[i].active) {
            return i;
        }
    }
    return -1;
}
void vy() {
    Print(L"Test OK");
}

struct y {
    INT32 id;
    VOID (*vy)(VOID); // Правильное объявление указателя на функцию
};

// Объявляем глобально, чтобы данные жили во время работы задачи
struct y uy;

EFI_STATUS LoadAndStartPex(CHAR16* Path) {
    EFI_STATUS Status;
    EC16 file;
    
    INT32 id = FindFreeTaskSlot();
    if (id == -1) return EFI_OUT_OF_RESOURCES;

    // Заполняем структуру
    uy.id = id;
    uy.vy = vy;

    Status = ReadFileByPath(Path, &file);
    if (EFI_ERROR(Status)) return Status;

    // Передаем указатель на структуру uy как аргумент (void*)
    Status = task_create_with_arg(id, (VOID (*)(VOID*))file.Message, &uy);
    
    return Status;
}