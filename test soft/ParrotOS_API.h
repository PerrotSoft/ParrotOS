#ifndef PARROT_API_H
#define PARROT_API_H

#include <stdint.h>

typedef uint16_t CHAR16;

static const CHAR16* _api_font = L"SysFont";

extern void main();
__attribute__((used, section(".text.boot")))
void _start() {
    main();
    asm volatile ("movq $0x03, %%rax; int $0x25" : : : "rax");
}

void ConPrintChar(CHAR16 c) { asm volatile ("movq $0x01, %%rax; movq %0, %%rcx; int $0x21" : : "r"((uint64_t)c) : "rax", "rcx"); }
void ConPrint(const CHAR16* msg) { asm volatile ("movq $0x02, %%rax; movq %0, %%rcx; int $0x21" : : "r"(msg) : "rax", "rcx"); }
void ConSetAttribute(uint64_t attr) { asm volatile ("movq $0x04, %%rax; movq %0, %%rcx; int $0x21" : : "r"(attr) : "rax", "rcx"); }
void ConClear() { asm volatile ("movq $0x05, %%rax; int $0x21" : : : "rax"); }
void ConSetCursor(uint64_t x, uint64_t y) { asm volatile ("movq $0x06, %%rax; movq %0, %%rcx; movq %1, %%rdx; int $0x21" : : "r"(x), "r"(y) : "rax", "rcx", "rdx"); }

uint64_t KbdGetKey() { uint64_t r; asm volatile ("movq $0x01, %%rax; int $0x22; movq %%rax, %0" : "=r"(r) : : "rax"); return r; }
uint64_t KbdHasKey() { uint64_t r; asm volatile ("movq $0x02, %%rax; int $0x22; movq %%rax, %0" : "=r"(r) : : "rax"); return r; }
void KbdReset() { asm volatile ("movq $0x03, %%rax; int $0x22" : : : "rax"); }

uint64_t FileRead(const CHAR16* path, void* buffer) { uint64_t r; asm volatile ("movq $0x01, %%rax; movq %1, %%rcx; movq %2, %%rdx; int $0x23; movq %%rax, %0" : "=r"(r) : "r"(path), "r"(buffer) : "rax", "rcx", "rdx"); return r; }
uint64_t DiskSet(CHAR16 letter) { uint64_t r; asm volatile ("movq $0x02, %%rax; movq %1, %%rcx; int $0x23; movq %%rax, %0" : "=r"(r) : "r"((uint64_t)letter) : "rax", "rcx"); return r; }
uint64_t FileWrite(const CHAR16* path, void* data, uint64_t size) { uint64_t r; asm volatile ("movq $0x03, %%rax; movq %1, %%rcx; movq %2, %%rdx; movq %3, %%r8; int $0x23; movq %%rax, %0" : "=r"(r) : "r"(path), "r"(data), "r"(size) : "rax", "rcx", "rdx", "r8"); return r; }
uint64_t FileCreate(const CHAR16* path) { uint64_t r; asm volatile ("movq $0x04, %%rax; movq %1, %%rcx; int $0x23; movq %%rax, %0" : "=r"(r) : "r"(path) : "rax", "rcx"); return r; }
uint64_t FileDelete(const CHAR16* path) { uint64_t r; asm volatile ("movq $0x05, %%rax; movq %1, %%rcx; int $0x23; movq %%rax, %0" : "=r"(r) : "r"(path) : "rax", "rcx"); return r; }
uint64_t FileGetSize(const CHAR16* path, uint64_t* size_ptr) { uint64_t r; asm volatile ("movq $0x06, %%rax; movq %1, %%rcx; movq %2, %%rdx; int $0x23; movq %%rax, %0" : "=r"(r) : "r"(path), "r"(size_ptr) : "rax", "rcx", "rdx"); return r; }
// Добавь это в ParrotOS_API.h
CHAR16* FsListDir() {
    uint64_t result;
    asm volatile (
        "movq $0x07, %%rax;" // Номер функции ListDir
        "int $0x23;"         // Прерывание Storage
        "movq %%rax, %0"     // Получаем указатель на строку
        : "=r"(result)
        :
        : "rax"
    );
    return (CHAR16*)result;
}

void GfxClear(uint32_t color) { asm volatile ("movq $0x01, %%rax; movq %0, %%rcx; int $0x24" : : "r"((uint64_t)color) : "rax", "rcx"); }
void GfxPutPixel(int32_t x, int32_t y, uint32_t color) { asm volatile ("movq $0x02, %%rax; movq %0, %%rcx; movq %1, %%rdx; movq %2, %%r8; int $0x24" : : "r"((uint64_t)x), "r"((uint64_t)y), "r"((uint64_t)color) : "rax", "rcx", "rdx", "r8"); }
void GfxDrawLine(int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint32_t color) { asm volatile ("movq $0x03, %%rax; movq %0, %%rcx; movq %1, %%rdx; movq %2, %%r8; movq %3, %%r9; movq %4, %%r10; int $0x24" : : "r"((uint64_t)x1), "r"((uint64_t)y1), "r"((uint64_t)x2), "r"((uint64_t)y2), "r"((uint64_t)color) : "rax", "rcx", "rdx", "r8", "r9", "r10"); }
void GfxDrawBitmap(uint32_t* data, int32_t x, int32_t y, int32_t w, int32_t h) { asm volatile ("movq $0x04, %%rax; movq %0, %%rcx; movq %1, %%rdx; movq %2, %%r8; movq %3, %%r9; movq %4, %%r10; int $0x24" : : "r"(data), "r"((uint64_t)x), "r"((uint64_t)y), "r"((uint64_t)w), "r"((uint64_t)h) : "rax", "rcx", "rdx", "r8", "r9", "r10"); }
uint64_t GfxLoadFont(const CHAR16* path, const CHAR16* name) { uint64_t r; asm volatile ("movq $0x05, %%rax; movq %1, %%rcx; movq %2, %%rdx; int $0x24; movq %%rax, %0" : "=r"(r) : "r"(path), "r"(name) : "rax", "rcx", "rdx"); return r; }
void GfxSetFont(const CHAR16* name) { _api_font = name; }
void GfxPrint(int32_t x, int32_t y, int32_t size, uint32_t color, const CHAR16* text) {
    asm volatile ("movq $0x08, %%rax; movq %0, %%rcx; movq %1, %%rdx; movq %2, %%r8; movq %3, %%r9; movq %4, %%r10; movq %5, %%r11; int $0x24" 
    : : "r"(_api_font), "r"((uint64_t)x), "r"((uint64_t)y), "r"((uint64_t)size), "r"((uint64_t)color), "r"(text) : "rax", "rcx", "rdx", "r8", "r9", "r10", "r11");
}
void GfxGetScreenSize(int32_t* w, int32_t* h) {
    uint64_t width, height;
    asm volatile ("movq $0x09, %%rax; int $0x24; movq %%rbx, %0; movq %%rcx, %1" 
                  : "=r"(width), "=r"(height) : : "rax", "rbx", "rcx");
    *w = (int32_t)width;
    *h = (int32_t)height;
}
uint32_t GfxGetPixel(int32_t x, int32_t y) {
    uint64_t color;
    asm volatile ("movq $0x0A, %%rax; movq %1, %%rcx; movq %2, %%rdx; int $0x24; movq %%rax, %0" 
                  : "=r"(color) : "r"((uint64_t)x), "r"((uint64_t)y) : "rax", "rcx", "rdx");
    return (uint32_t)color;
}
void SB(){
    asm volatile ("movq $0x0C, %%rax; int $0x24" : : : "rax"); 
}
void TaskCreate(int32_t priority, void (*entry)(void)) { asm volatile ("movq $0x01, %%rax; movq %0, %%rcx; movq %1, %%rdx; int $0x25" : : "r"((uint64_t)priority), "r"(entry) : "rax", "rcx", "rdx"); }
void TaskYieldf() { asm volatile ("movq $0x05, %%rax; int $0x25" : : : "rax"); }
void TaskYield() { asm volatile ("movq $0x02, %%rax; int $0x25" : : : "rax"); }
void TaskExit() { asm volatile ("movq $0x03, %%rax; int $0x25" : : : "rax"); }
uint64_t PexRun(const CHAR16* path) { uint64_t r; asm volatile ("movq $0x06, %%rax; movq %1, %%rcx; int $0x25; movq %%rax, %0" : "=r"(r) : "r"(path) : "rax", "rcx"); return r; }

void SysRegisterHandler(uint8_t vector, void* handler) { asm volatile ("movq $0x01, %%rax; movq %0, %%rbx; movq %1, %%rcx; int $0x26" : : "r"((uint64_t)vector), "r"(handler) : "rax", "rbx", "rcx"); }
void SysReboot() { asm volatile ("movq $0x02, %%rax; int $0x26" : : : "rax"); }
void SysShutdown() { asm volatile ("movq $0x03, %%rax; int $0x26" : : : "rax"); }
// AH=07: Смена текущей директории
uint64_t FsChangeDir(const CHAR16* path) { 
    uint64_t r; 
    asm volatile ("movq $0x07, %%rax; movq %1, %%rcx; int $0x23; movq %%rax, %0" : "=r"(r) : "r"(path) : "rax", "rcx"); 
    return r; 
}

// AH=09: Получить список всех дисков (строка через точку с запятой)
CHAR16* FsListDisks() { 
    uint64_t res; 
    asm volatile ("movq $0x09, %%rax; int $0x23; movq %%rax, %0" : "=r"(res) : : "rax"); 
    return (CHAR16*)res; 
}
// AH=06: Рисование одного символа
void GfxDrawChar(int32_t x, int32_t y, int32_t size, uint32_t color, CHAR16 c) { 
    asm volatile ("movq $0x06, %%rax; movq %0, %%rcx; movq %1, %%rdx; movq %2, %%r8; movq %3, %%r9; movq %4, %%r10; movq %5, %%r11; int $0x24" 
    : : "r"(_api_font), "r"((uint64_t)x), "r"((uint64_t)y), "r"((uint64_t)size), "r"((uint64_t)color), "r"((uint64_t)c) : "rax", "rcx", "rdx", "r8", "r9", "r10", "r11"); 
}

// AH=0B: Получить путь к иконке по умолчанию
CHAR16* GfxGetDefaultIcon() { 
    uint64_t r; 
    asm volatile ("movq $0x0B, %%rax; int $0x24; movq %%rax, %0" : "=r"(r) : : "rax"); 
    return (CHAR16*)r; 
}
// AH=04: Получить ID текущей задачи
uint64_t TaskGetCurrent() { 
    uint64_t r; 
    asm volatile ("movq $0x04, %%rax; int $0x25; movq %%rax, %0" : "=r"(r) : : "rax"); 
    return r; 
}
// AH=01: Инициализация сетевого интерфейса
uint64_t NetInit(const CHAR16* ip, const CHAR16* mask) { 
    uint64_t r; 
    asm volatile ("movq $0x01, %%rax; movq %1, %%rcx; movq %2, %%rdx; int $0x27; movq %%rax, %0" : "=r"(r) : "r"(ip), "r"(mask) : "rax", "rcx", "rdx"); 
    return r; 
}

// AH=06: DNS запрос (преобразование домена в IP)
uint64_t NetDnsLookup(const CHAR16* host, CHAR16* out_ip) { 
    uint64_t r; 
    asm volatile ("movq $0x06, %%rax; movq %1, %%rcx; movq %2, %%rdx; int $0x27; movq %%rax, %0" : "=r"(r) : "r"(host), "r"(out_ip) : "rax", "rcx", "rdx"); 
    return r; 
}
// AH=01: Издать звук заданной частоты и длительности
void AudioBeep(uint32_t freq, uint32_t ms) { 
    asm volatile ("movq $0x01, %%rax; movq %0, %%rcx; movq %1, %%rdx; int $0x28" : : "r"((uint64_t)freq), "r"((uint64_t)ms) : "rax", "rcx", "rdx"); 
}
// AH=01: Инициализация мыши
uint64_t MouseInit() { 
    uint64_t r; 
    asm volatile ("movq $0x01, %%rax; int $0x29; movq %%rax, %0" : "=r"(r) : : "rax"); 
    return r; 
}

// AH=02: Получить координаты и состояние кнопок
uint64_t MouseGetState(int32_t* x, int32_t* y, uint8_t* b1, uint8_t* b2) { 
    uint64_t r; 
    asm volatile ("movq $0x02, %%rax; movq %1, %%rcx; movq %2, %%rdx; movq %3, %%r8; movq %4, %%r9; int $0x29; movq %%rax, %0" 
    : "=r"(r) : "r"(x), "r"(y), "r"(b1), "r"(b2) : "rax", "rcx", "rdx", "r8", "r9"); 
    return r; 
}

#endif