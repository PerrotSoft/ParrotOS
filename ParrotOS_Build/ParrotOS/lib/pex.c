#include "../include/pex.h"
#include "../include/task.h"
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseMemoryLib.h>
#include "../include/drivers/DriverManager.h"
#include "../include/Vector.h"
#include "../include/Protocols.h"

Vector prs;
Vector task_registry;

void ProcessManagerInit() {
    VectorInit(&prs, Min_Process);
    INIT_PROTOCOLS();
}

struct Process* GetTaskById(INT64 ID) {
    return (struct Process*)prs.GetById(ID);
}
INT64* GetAllIDOfProcess(struct Process* p, UINT64* count) {
    Vector* v = &task_registry;
    INT64* ids = (INT64*)AllocateZeroPool(v->size * sizeof(INT64));
    UINT64 idx = 0;
    for (UINT64 i = 0; i < v->size; i++) {
        if (v->items[i].data != NULL && ((INT64)v->items[i].data) == p->ID) {
            ids[idx++] = v->items[i].id;
        }
    }
    *count = idx;
    return ids;
}
void ProcessStop(INT64 ID) {
    task_stop_and_run(ID);
    struct Process* p = (struct Process*)prs.GetById(ID);
    if (p != NULL) {
        p->active = !p->active;
    }
}

UINT8 Process_Exit(INT64 ID) {
    struct Process* pr = GetTaskById(ID);
    if (!pr) return 0;
    task_exitx(ID);

    if (pr->storage) {
        gBS->FreePool(pr->storage);
    }
    DeRegisterTaskToProcess(ID);
    prs.Remove(ID);
    gBS->FreePool(pr);
    
    return 1;
}

INT64 FindFreeTaskSlot(VOID) {
    static INT64 next_id = 1;
    while (VectorGet(&task_list, next_id) != NULL) {
        next_id++;
    }
    return next_id++;
}

EFI_STATUS LoadAndStartPex(CHAR16* Path, struct Process init_data) {
    EFI_STATUS Status;
    EC16 e;
    struct Process* pr = NULL;

    if (prs._push == NULL) ProcessManagerInit();

    Status = ReadFileByPath(Path, &e);
    if (EFI_ERROR(Status) || e.Message == NULL) return Status;

    UINTN TotalSize = e.FileSize + (2 * 1024 * 1024); 
    VOID* SafeBuffer = NULL;
    Status = gBS->AllocatePool(EfiLoaderData, TotalSize, &SafeBuffer);
    
    if (EFI_ERROR(Status)) {
        gBS->FreePool(e.Message);
        return Status;
    }

    gBS->SetMem(SafeBuffer, TotalSize, 0);
    gBS->CopyMem(SafeBuffer, e.Message, e.FileSize);
    gBS->FreePool(e.Message); // Удаляем старый маленький буфер
    e.Message = SafeBuffer;   // Теперь работаем с безопасным буфером

    PEX_HEADER* header = (PEX_HEADER*)e.Message;

    if (header->Magic[0] != 'P' || header->Magic[1] != 'E' || header->Magic[2] != 'X') {
        gBS->FreePool(e.Message);
        return EFI_UNSUPPORTED; 
    }

    Status = gBS->AllocatePool(EfiLoaderData, sizeof(struct Process), (VOID**)&pr);
    if (EFI_ERROR(Status)) {
        gBS->FreePool(e.Message);
        return Status;
    }
    
    gBS->CopyMem(pr, &init_data, sizeof(struct Process));

    INT32 id = FindFreeTaskSlot();
    if (id == -1) {
        gBS->FreePool(e.Message);
        gBS->FreePool(pr);
        return EFI_OUT_OF_RESOURCES;
    }

    pr->ID = id;
    pr->storage = e.Message;
    pr->active = TRUE;

    struct Process* caller = GetCurrentCallerProcess();
    if (caller != NULL) {
        pr->ParentID = caller->ID;
    } else {
        pr->ParentID = 0;
    }

    if (pr->sizeMem == 0) {
        pr->sizeMem = (header->MemorySizeMB > 0) ? (header->MemorySizeMB * 1024 * 1024) : (2 * 1024 * 1024); 
    }

    RegisterTaskToProcess(id, pr->ID);
    prs.Push(id, pr);

    VOID (*entry_point)(VOID*) = (VOID (*)(VOID*))((UINTN)e.Message + header->EntryPoint);

    Status = task_create_with_arg(id, entry_point, pr, (UINT64)pr->sizeMem);
    if (EFI_ERROR(Status)) {
        Process_Exit(id);
        return Status;
    }

    return EFI_SUCCESS;
}