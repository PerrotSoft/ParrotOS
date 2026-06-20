#include "ParrotOS_API.h"
// #include "console.h"
// #include "console.c"
// #include "cmd.h"

// void StdioHandler(uint64_t Type, void* Context) {
//     EFI_SYSTEM_CONTEXT_X64* ctx = (EFI_SYSTEM_CONTEXT_X64*)Context;
//     uint32_t syscall_num = (uint32_t)ctx->rax;

//     switch (syscall_num) {
//         case 0x01:
//             PrintChar((CHAR16)ctx->rcx, (uint32_t)ctx->rdx, (uint32_t)ctx->r8);
//             break;
            
//         case 0x02:
//             PrintString((const CHAR16*)ctx->rcx, (uint32_t)ctx->rdx, (uint32_t)ctx->r8);
//             break;
//         case 0x03:
//             ConsoleClear();
//             break;

//         case 0x04:
//             ctx->rax = (uint64_t)GetCursorX(); 
//             ctx->rdx = (uint64_t)GetCursorY();
//             break;

//         case 0x05:
//             SetCursorPos((int)ctx->rcx, (int)ctx->rdx);
//             break;
//         case 0x10: 
//             ctx->rax = (uint64_t)ReadChar();
//             break;

//         case 0x11:
//             ctx->rax = (uint64_t)ReadLine();
//             break;
//     }
//     if (syscall_num < 0x10) {
//         RenderConsole();
//     }
// }

// void main(struct Process* pr) {
//     GfxSetFont((CHAR16*)L"SysFont");
//     SysRegisterHandler(0x80, (void*)StdioHandler);
//     ConsoleClear();
//     while (1) {
//         Cmd();
//         TaskYield();
//     }
// }
#include "MicroSVG.h"
void main(struct Process* pr){
    GfxSetFont((CHAR16*)L"SysFont");
    GfxClear(0x121314);
    GfxPrint( 10,10,23, 0x00FF7F,L"Welcome to Benchmark");
    GfxDrawLine(1, 35, 300, 35, 0x00FF00);
    GfxDrawLine(1, 36, 300, 36, 0x00FF00);
    GfxDrawLine(1, 37, 300, 37, 0x00FF00);
    GfxDrawLine(1, 38, 300, 38, 0x00FF00);
    GfxDrawLine(1, 39, 300, 39, 0x00FF00);
   
    while (1) {
        TaskYield();
    }
}