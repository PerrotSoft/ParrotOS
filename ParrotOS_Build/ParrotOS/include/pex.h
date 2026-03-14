#ifndef PEX_H
#define PEX_H

#include <Uefi.h>
#include "../include/Vector.h"

#define Min_Process 10

typedef enum {
    RIGHT_SYS, 
    RIGHT_ROOT,
    RIGHT_ADMIN,
    RIGHT_USER
} PROCESS_RIGHTS;

struct Process {
    INT32 ID;
    const CHAR16* Name;
    PROCESS_RIGHTS rights;
    
    void* ArgContext; 
    void* storage;
    BOOLEAN active;
    UINTN stack_base; 
    INT32 ParentID;

    UINT8 (EFIAPI *Exit)();
    UINT8 (EFIAPI *GetProtocol)(UINT32 protocol_id, VOID** out_protocol);
};

void       ProcessManagerInit();
EFI_STATUS LoadAndStartPex(CHAR16* Path, struct Process init_data);
UINT8      Process_Exit(INT32 ID);
struct Process* GetTaskById(INT32 ID);
void       TaskStop(INT32 ID);

#endif