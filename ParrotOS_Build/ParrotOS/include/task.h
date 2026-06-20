#ifndef _TASK_H_
#define _TASK_H_

#include <Uefi.h>
#include "Vector.h"
#pragma pack(push, 1)
typedef struct {
    VOID    *sp;            // Вершина стека (всегда смещение 0 для ASM)
    VOID    *stack_limit;   // Начало выделенной памяти (чтобы освобождать)
    VOID    *storage;       // Доп. данные
    BOOLEAN active;         // Флаг работы
    UINT8   padding[7];     // Выравнивание структуры
} task_t;
#pragma pack(pop)

extern struct Vector task_list;
extern INT64  current_task;
extern VOID   *uefi_stack_save;

VOID init_scheduler(VOID); 
EFI_STATUS task_create(INT64 id, VOID (*entry)(VOID), UINT64 STACK_SIZE);
EFI_STATUS task_create_with_arg(INT64 id, VOID (*entry)(VOID*), VOID* arg, UINT64 STACK_SIZE);
VOID task_yield(VOID);
VOID task_exit(VOID);
VOID task_exitx(INT64 id);
VOID task_stop_and_run(INT64 id);
VOID task_start_first(VOID);

#endif