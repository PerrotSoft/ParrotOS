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
CHAR16* f = L"hi";
CHAR16* t(){
    return L"bsadhjdfsj";
}
struct g {
    INT32 id;
    CHAR16* (*HI)(VOID);
    CHAR16* f;
};
EFI_STATUS LoadAndStartPex(CHAR16* Path) {
    EFI_STATUS Status;
    EC16 file;
    struct g* f1 = NULL;

    // Выделяем память через Boot Services
    Status = gBS->AllocatePool(EfiLoaderData, sizeof(struct g), (VOID**)&f1);
    if (EFI_ERROR(Status)) return EFI_OUT_OF_RESOURCES;

    INT32 id = FindFreeTaskSlot();
    if (id == -1) {
        gBS->FreePool(f1);
        return EFI_OUT_OF_RESOURCES;
    }

    f1->HI = t;
    f1->f = f;
    f1->id = id;

    Status = ReadFileByPath(Path, &file);
    if (EFI_ERROR(Status)) {
        gBS->FreePool(f1);
        return Status;
    }

    // Передаем указатель на выделенную структуру
    Status = task_create_with_arg(id, (VOID (*)(VOID*))file.Message, f1);
    
    if (EFI_ERROR(Status)) {
        gBS->FreePool(f1);
        gBS->FreePool(file.Message); 
        return Status;
    }
    
    tasks[id].storage = file.Message;
    // ВАЖНО: f1 не удаляем здесь, он должен жить, пока работает программа!
    
    return EFI_SUCCESS;
}