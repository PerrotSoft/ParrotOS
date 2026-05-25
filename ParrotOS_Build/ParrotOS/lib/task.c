// task.c
#include "include/task.h"
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>

__attribute__((used, visibility("default"))) struct Vector task_list;
__attribute__((used, visibility("default"))) INT64  current_task = 0;
__attribute__((used, visibility("default"))) VOID   *uefi_stack_save = NULL;

VOID EFIAPI SwitchContext(VOID **OldStackPtr, VOID *NewStack);
EFI_EVENT TaskTimerEvent;

VOID init_scheduler(VOID) {
    VectorInit(&task_list, 1);
    current_task = 0;
}

VOID task_yield(VOID) {
    INT64 count = (INT64)VectorCount(&task_list);
    
    if (count <= 0) {
        return;
    }

    if (current_task >= count) {
        current_task = 0;
    }

    task_t *curr = (task_t *)task_list._at(&task_list, current_task);
    INT64 next_task = -1;
    
    for (INT64 i = 1; i <= count; i++) {
        INT64 idx = (current_task + i) % count;
        task_t *t = (task_t *)task_list._at(&task_list, idx);

        if (t && t->active) {
            next_task = idx;
            break;
        }
    }

    if (next_task != -1 && next_task != current_task) {
        INT64 old_task = current_task;
        current_task = next_task;
        
        task_t *t_old = (task_t *)task_list._at(&task_list, old_task);
        task_t *t_new = (task_t *)task_list._at(&task_list, next_task);

        SwitchContext(&t_old->sp, t_new->sp);
    } else if (next_task == -1 && curr && !curr->active) {
        SwitchContext(&curr->sp, uefi_stack_save);
    }
}

VOID task_exit(VOID) {
    if (VectorCount(&task_list) == 0) return;
    
    task_t *t = (task_t *)task_list._at(&task_list, current_task);
    if (t) {
        t->active = FALSE;
    }
    
    task_yield();
    while (1) { __asm__ volatile ("hlt"); }
}

VOID task_stop_and_run(INT64 id) {
    task_t *t = (task_t *)VectorGet(&task_list, id);
    if (!t) return;
    t->active = !t->active;
}

VOID task_exitx(INT64 id) {
    task_t *t = (task_t *)VectorGet(&task_list, id);
    if (!t) return;
    
    t->active = FALSE;

    if (task_list.items[current_task].id != id) {
        if (t->stack_limit) gBS->FreePool(t->stack_limit);
        if (t->storage) gBS->FreePool(t->storage);
        gBS->FreePool(t);
        VectorRemove(&task_list, id);
    }
    
    task_yield();
}

EFI_STATUS task_create(INT64 id, VOID (*entry)(VOID), UINT64 STACK_SIZE) {
    if (STACK_SIZE == 0 || STACK_SIZE > 0x1000000) STACK_SIZE = 0x10000;
    
    VOID *stack = NULL;
    EFI_STATUS Status = gBS->AllocatePool(EfiBootServicesData, STACK_SIZE, &stack);
    
    if (EFI_ERROR(Status)) return Status;

    SetMem(stack, STACK_SIZE, 0);

    task_t task;
    task.stack_limit = stack;
    task.active = TRUE;
    task.storage = NULL;
    UINTN *sp = (UINTN *)((UINT8 *)stack + STACK_SIZE);
    
    // Идеальное выравнивание под x64 ABI (16-byte align + Shadow Space)
    sp = (UINTN *)((UINTN)sp & ~0xF); 
    
#if defined(MDE_CPU_X64)
    sp -= 4; // 32 байта Shadow space (Обязательно для Microsoft x64 ABI)
    *(--sp) = (UINTN)task_exit; // Возврат, если задача выйдет из функции
    *(--sp) = (UINTN)entry;     // Инструкция для возврата из SwitchContext (запуск задачи)
    *(--sp) = 0x202;            // RFLAGS (Прерывания включены)
    *(--sp) = 0;                // RCX (Первый аргумент)
    *(--sp) = 0;                // RDX (Второй аргумент)
    *(--sp) = 0;                // R8
    *(--sp) = 0;                // R9
    *(--sp) = 0;                // RBP
    *(--sp) = 0;                // RBX
    *(--sp) = 0;                // RDI
    *(--sp) = 0;                // RSI
    *(--sp) = 0;                // R12
    *(--sp) = 0;                // R13
    *(--sp) = 0;                // R14
    *(--sp) = 0;                // R15
#elif defined(MDE_CPU_IA32)
    *(--sp) = 0;
    *(--sp) = (UINTN)task_exit;
    *(--sp) = (UINTN)entry;
    *(--sp) = 0x202;
    for(int i=0; i<8; i++) *(--sp) = 0; 
#endif

    task.sp = (VOID*)sp;
    task_t *task_ptr = NULL;
    EFI_STATUS s2 = gBS->AllocatePool(EfiLoaderData, sizeof(task_t), (VOID**)&task_ptr);
    if (EFI_ERROR(s2)) {
        gBS->FreePool(stack);
        return s2;
    }
    gBS->CopyMem(task_ptr, &task, sizeof(task_t));
    VectorPush(&task_list, id, task_ptr);
    return EFI_SUCCESS;
}

EFI_STATUS task_create_with_arg(INT64 id, VOID (*entry)(VOID*), VOID* arg, UINT64 STACK_SIZE) {
    if (STACK_SIZE == 0 || STACK_SIZE > 0x1000000) STACK_SIZE = 0x10000;
    
    VOID *stack = NULL;
    EFI_STATUS Status = gBS->AllocatePool(EfiBootServicesData, STACK_SIZE, &stack);
    if (EFI_ERROR(Status)) return Status;

    SetMem(stack, STACK_SIZE, 0);
    task_t task;
    task.stack_limit = stack;
    task.active = TRUE;
    task.storage = NULL;

    UINTN *sp = (UINTN *)((UINT8 *)stack + STACK_SIZE);
    sp = (UINTN *)((UINTN)sp & ~0xF); 

#if defined(MDE_CPU_X64)
    sp -= 4; // 32 байта Shadow space
    *(--sp) = (UINTN)task_exit; 
    *(--sp) = (UINTN)entry; 
    *(--sp) = 0x202;      
    *(--sp) = (UINTN)arg;       // Передаем аргумент в RCX (как требует спецификация Microsoft ABI)
    *(--sp) = 0;                // RDX
    *(--sp) = 0;                // R8
    *(--sp) = 0;                // R9
    *(--sp) = 0;                // RBP
    *(--sp) = 0;                // RBX
    *(--sp) = 0;                // RDI
    *(--sp) = 0;                // RSI
    *(--sp) = 0;                // R12
    *(--sp) = 0;                // R13
    *(--sp) = 0;                // R14
    *(--sp) = 0;                // R15
#elif defined(MDE_CPU_IA32)
    *(--sp) = (UINTN)arg;       
    *(--sp) = (UINTN)task_exit;
    *(--sp) = (UINTN)entry;
    *(--sp) = 0x202;
    for(int i=0; i<8; i++) *(--sp) = 0; 
#endif

    task.sp = (VOID*)sp;
    task_t *task_ptr = NULL;
    EFI_STATUS s2 = gBS->AllocatePool(EfiLoaderData, sizeof(task_t), (VOID**)&task_ptr);
    if (EFI_ERROR(s2)) {
        gBS->FreePool(stack);
        return s2;
    }
    gBS->CopyMem(task_ptr, &task, sizeof(task_t));
    VectorPush(&task_list, id, task_ptr);
    return EFI_SUCCESS;
}

VOID task_start_first() {
    if (VectorCount(&task_list) == 0) return;
    
    current_task = 0;
    SwitchContext(&uefi_stack_save, ((task_t*)task_list._at(&task_list, 0))->sp);
}

#if defined(MDE_CPU_X64)
__attribute__((naked))
VOID EFIAPI SwitchContext(VOID **OldStackPtr, VOID *NewStack) {
    asm volatile (
        "pushfq \n"
        "pushq %%rcx \n"
        "pushq %%rdx \n"
        "pushq %%r8 \n"
        "pushq %%r9 \n"
        "pushq %%rbp \n"
        "pushq %%rbx \n"
        "pushq %%rdi \n"
        "pushq %%rsi \n"
        "pushq %%r12 \n"
        "pushq %%r13 \n"
        "pushq %%r14 \n"
        "pushq %%r15 \n"
        
        "movq %%rsp, (%%rcx) \n" // RCX все еще хранит 1-й аргумент (OldStackPtr)
        "movq %%rdx, %%rsp \n"   // RDX все еще хранит 2-й аргумент (NewStack)
        
        "popq %%r15 \n"
        "popq %%r14 \n"
        "popq %%r13 \n"
        "popq %%r12 \n"
        "popq %%rsi \n"
        "popq %%rdi \n"
        "popq %%rbx \n"
        "popq %%rbp \n"
        "popq %%r9 \n"
        "popq %%r8 \n"
        "popq %%rdx \n"
        "popq %%rcx \n"
        "popfq \n"
        "retq \n"
        : : : "memory"
    );
}
#elif defined(MDE_CPU_IA32)
__attribute__((naked))
VOID EFIAPI SwitchContext(VOID **OldStackPtr, VOID *NewStack) {
    asm volatile (
        "pushfd \n"
        "pushad \n"
        "movl 40(%%esp), %%eax \n"
        "movl %%esp, (%%eax) \n"
        "movl 44(%%esp), %%esp \n"
        "popad \n"
        "popfd \n"
        "ret \n"
        : : : "memory", "eax"
    );
}
#else
    #error "Architecture not supported (Only X64 and IA32)"
#endif