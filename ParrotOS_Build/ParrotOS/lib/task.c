#include "include/task.h"
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>

__attribute__((used, visibility("default"))) task_t tasks[MAX_TASKS];
__attribute__((used, visibility("default"))) INT32  current_task = 0;
__attribute__((used, visibility("default"))) VOID   *uefi_stack_save = NULL;

VOID EFIAPI SwitchContext(VOID **OldStackPtr, VOID *NewStack);

VOID init_scheduler(VOID) {
    SetMem(tasks, sizeof(tasks), 0);
    current_task = 0;
    tasks[0].active = TRUE; 
}

VOID task_exit(VOID) {
    tasks[current_task].active = FALSE;

    if (tasks[current_task].stack_limit != NULL) {
        gBS->FreePool(tasks[current_task].stack_limit);
        tasks[current_task].stack_limit = NULL;
    }

    if (tasks[current_task].storage != NULL) {
        gBS->FreePool(tasks[current_task].storage);
        tasks[current_task].storage = NULL;
    }
    task_yield();
}

EFI_STATUS task_create(INT32 id, VOID (*entry)(VOID)) {
    if (id >= MAX_TASKS) return EFI_OUT_OF_RESOURCES;

    VOID *stack = NULL;
    EFI_STATUS Status = gBS->AllocatePool(EfiBootServicesData, STACK_SIZE, &stack);
    
    if (EFI_ERROR(Status)) {
        return Status;
    }

    SetMem(stack, STACK_SIZE, 0);

    tasks[id].stack_limit = stack;
    tasks[id].active = TRUE;
    tasks[id].storage = NULL;
    UINTN *sp = (UINTN *)((UINT8 *)stack + STACK_SIZE);
    sp = (UINTN *)((UINTN)sp & ~0xF);
    
#if defined(MDE_CPU_X64)

    *(--sp) = (UINTN)task_exit;
    *(--sp) = (UINTN)entry; 
    *(--sp) = 0x202;            
    *(--sp) = 0;                
    *(--sp) = 0;                
    *(--sp) = 0;                
    *(--sp) = 0;                
    *(--sp) = 0;                
    *(--sp) = 0;               
    *(--sp) = 0;                
    *(--sp) = 0;

#elif defined(MDE_CPU_IA32)
    *(--sp) = (UINTN)task_exit;
    *(--sp) = (UINTN)entry;
    *(--sp) = 0x202;
    for(int i=0; i<8; i++) *(--sp) = 0; 
#endif

    tasks[id].sp = (VOID*)sp;
    
    return EFI_SUCCESS;
}

VOID task_yield(VOID) {
    INT32 next_task = current_task;
    INT32 prev_task = current_task;
    BOOLEAN found = FALSE;
    for (INT32 i = 0; i < MAX_TASKS; i++) {
        next_task++;
        if (next_task >= MAX_TASKS) next_task = 0;

        if (tasks[next_task].active) {
            found = TRUE;
            break;
        }
    }
    if (!found || next_task == prev_task) {
        return;
    }

    current_task = next_task;
    SwitchContext(&tasks[prev_task].sp, tasks[next_task].sp);
}
#if defined(MDE_CPU_X64)
__attribute__((naked))
VOID EFIAPI SwitchContext(VOID **OldStackPtr, VOID *NewStack) {
    asm volatile (
        "pushfq \n"
        "pushq %%rbp \n"
        "pushq %%rbx \n"
        "pushq %%rdi \n"
        "pushq %%rsi \n"
        "pushq %%r12 \n"
        "pushq %%r13 \n"
        "pushq %%r14 \n"
        "pushq %%r15 \n"
        "movq %%rsp, (%%rcx) \n"
        "movq %%rdx, %%rsp \n"
        "popq %%r15 \n"
        "popq %%r14 \n"
        "popq %%r13 \n"
        "popq %%r12 \n"
        "popq %%rsi \n"
        "popq %%rdi \n"
        "popq %%rbx \n"
        "popq %%rbp \n"
        "popfq \n"

        "retq \n"
        : : : "memory"
    );
}

__attribute__((naked)) VOID task_start_first(VOID) {
    asm volatile (
        "pushq %%rbp \n"
        "pushq %%rbx \n"
        "pushq %%rdi \n"
        "pushq %%rsi \n"
        "pushq %%r12 \n"
        "pushq %%r13 \n"
        "pushq %%r14 \n"
        "pushq %%r15 \n"
        
        "movq %%rsp, uefi_stack_save(%%rip) \n"
        "leaq tasks(%%rip), %%rax \n"
        "movq (%%rax), %%rsp \n"
        "popq %%r15 \n"
        "popq %%r14 \n"
        "popq %%r13 \n"
        "popq %%r12 \n"
        "popq %%rsi \n"
        "popq %%rdi \n"
        "popq %%rbx \n"
        "popq %%rbp \n"
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

__attribute__((naked)) VOID task_start_first(VOID) {
    asm volatile (
        "pushfd \n"
        "pushad \n"
        
        "movl %%esp, uefi_stack_save \n"
        "movl tasks, %%esp \n"
        "popad \n"
        "popfd \n"
        
        "ret \n"
        : : : "memory"
    );
}

#else
    #error "Architecture not supported (Only X64 and IA32)"
#endif