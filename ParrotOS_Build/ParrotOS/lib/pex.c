#include "include/pex.h"
#include "include/task.h"
#include "include/drivers/fat32.h"
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>

INT32 FindFreeTaskSlot() {
    for (INT32 i = 1; i < MAX_TASKS; i++) {
        if (tasks[i].active == FALSE) {
            return i;
        }
    }
    return -1;
}

EFI_STATUS LoadAndStartPex(CHAR16* Path) {
    EFI_STATUS Status;
    EC16 file;
    INT32 id = FindFreeTaskSlot();
    if (id == -1) {
        return EFI_OUT_OF_RESOURCES;
    }

    Status = ReadFileByPath(Path, &file);
    if (EFI_ERROR(Status)) {
        return Status;
    }

    Status = task_create(id, (VOID (*)(VOID))file.Message);
    
    if (EFI_ERROR(Status)) {
        gBS->FreePool(file.Message); 
        return Status;
    }
    tasks[id].storage = file.Message;

    return EFI_SUCCESS;
}