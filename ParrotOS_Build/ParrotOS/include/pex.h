#ifndef PEX_H
#define PEX_H

#include <Uefi.h>
#include "../include/Vector.h"

#define Min_Process 10

#pragma pack(push, 1)
typedef struct {
    CHAR8  Magic[3];          // "PEX"
    UINT8  ProtocolVersion;
    UINT32 EntryPoint;
    UINT64 MemorySizeMB;
    UINT64 ProgramSize;
    UINT8  MinRights;
    CHAR8  ExtraDeps[64];
} PEX_HEADER;
#pragma pack(pop)

struct Process { 
    INT64 ID;
    const CHAR16* Name;
    UINT8 Rights;
    VOID* ArgContext; 
    VOID* storage;
    BOOLEAN active;
    UINT64 sizeMem;
    INT64 ParentID;
};

void       ProcessManagerInit();
EFI_STATUS LoadAndStartPex(CHAR16* Path, struct Process init_data);
UINT8      Process_Exit(INT64 ID);
struct Process* GetTaskById(INT64 ID);
void       ProcessStop(INT64 ID);
INT64*     GetAllIDOfProcess(struct Process* p, UINT64* count);
#endif