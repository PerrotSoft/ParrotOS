#include "../include/pex.h"
#include "../include/task.h"
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include "../include/drivers/DriverManager.h"

Vector prs;

void ProcessManagerInit() {
    VectorInit(&prs, Min_Process);
}

struct Process* GetTaskById(INT32 ID) {
    return (struct Process*)prs.GetById(&prs, ID);
}

UINT8 EFIAPI Kernel_GetProtocol(UINT32 protocol_id, VOID** out_protocol) {
    INT32 curr_id = current_task; 
    struct Process* pr = GetTaskById(curr_id);
    
    if (!pr || !out_protocol) return 0;

    if (protocol_id == 100 && pr->rights > RIGHT_ADMIN) {
        return 0;
    }
    *out_protocol = NULL; 
    return 1;
}

UINT8 EFIAPI PExit() {
    INT32 id = current_task;
    return Process_Exit(id);
}

void TaskStop(INT32 ID) {
    task_stop_and_run(ID);

    struct Process* p = (struct Process*)prs.GetById(&prs, ID);
    if (p != NULL) {
        p->active = !p->active;
    }
}

UINT8 Process_Exit(INT32 ID) {
    struct Process* pr = GetTaskById(ID);
    if (!pr) return 0;
    task_exitx(ID);

    if (pr->storage) {
        gBS->FreePool(pr->storage);
    }
    prs.Remove(&prs, ID);
    gBS->FreePool(pr);
    
    return 1;
}
INT32 FindFreeTaskSlot(VOID) {
    for (INT32 i = 1; i < MAX_TASKS; i++) {
        if (!tasks[i].active) {
            return i;
        }
    }
    return -1; 
}
EFI_STATUS LoadAndStartPex(CHAR16* Path, struct Process init_data) {
    EFI_STATUS Status;
    void* buffer = NULL;
    EC16 e;
    struct Process* pr = NULL;

    Status = gBS->AllocatePool(EfiLoaderData, sizeof(struct Process), (VOID**)&pr);
    if (EFI_ERROR(Status)) return Status;
    *pr = init_data;

    INT32 id = FindFreeTaskSlot();
    if (id == -1) {
        gBS->FreePool(pr);
        return EFI_OUT_OF_RESOURCES;
    }
    Status = ReadFileByPath(Path, &e);
    buffer = e.Message;
    if (EFI_ERROR(Status)) {
        gBS->FreePool(pr);
        return Status;
    }

    pr->ID = id;
    pr->storage = buffer;
    pr->Exit = PExit;
    pr->GetProtocol = Kernel_GetProtocol;
    prs.Push(&prs, id, pr);

    Status = task_create_with_arg(id, (VOID (*)(VOID*))buffer, pr);
    
    if (EFI_ERROR(Status)) {
        Process_Exit(id);
        return Status;
    }

    return EFI_SUCCESS;
}