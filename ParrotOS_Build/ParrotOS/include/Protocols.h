#ifndef PROTOCOLS_H
#define PROTOCOLS_H
#pragma once

#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include "../include/Vector.h"
#include "../include/task.h"
#include "../include/pex.h"

typedef struct {
    INT64 TaskID;    
    INT64 ProcessID;
} TASK_PROCESS_MAP;

extern Vector task_registry; 
extern Vector prs;   
extern INT64 current_task;   

static inline void INIT_PROTOCOLS() {
    if (task_registry._push == NULL) {
        VectorInit(&task_registry, 10);
    }
}

static inline struct Process* GetCurrentCallerProcess() {
    if (task_registry._push == NULL) INIT_PROTOCOLS();
    INT64 tid = current_task; 
    for (UINT64 i = 0; i < task_registry._cnt(&task_registry); i++) {
        TASK_PROCESS_MAP* map = (TASK_PROCESS_MAP*)task_registry._at(&task_registry, i);
        if (map != NULL && map->TaskID == tid) {
            return (struct Process*)prs.GetById(map->ProcessID);
        }
    }
    return NULL;
}
static inline BOOLEAN IFProcessHasRight(UINT8 right) {
    return (GetCurrentCallerProcess()->Rights <= right) != 0;
}
static inline void RegisterTaskToProcess(INT64 tid, INT64 pid) {
    if (task_registry._push == NULL) INIT_PROTOCOLS();
    TASK_PROCESS_MAP* map = AllocateZeroPool(sizeof(TASK_PROCESS_MAP));
    if (map == NULL) return;
    map->TaskID = tid;
    map->ProcessID = pid;
    task_registry._push(&task_registry, tid, map);
}

static inline void DeRegisterTaskToProcess(INT64 tid) {
    if (task_registry._push == NULL) INIT_PROTOCOLS();
    for (UINT64 i = 0; i < task_registry._cnt(&task_registry); i++) {
        TASK_PROCESS_MAP* map = (TASK_PROCESS_MAP*)task_registry._at(&task_registry, i);
        if (map != NULL && map->TaskID == tid) {
            FreePool(map);
            task_registry._rem(&task_registry, i);
            return;
        }
    }
}

#endif