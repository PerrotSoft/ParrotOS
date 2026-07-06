#ifndef PARROT_API_H
#define PARROT_API_H
#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

typedef uint16_t CHAR16;
typedef uint8_t BOOL;
#define TRUE 1
#define FALSE 0

static const CHAR16* _api_font = L"SysFont";

typedef struct {
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
    uint64_t rbp, rsi, rdi, rdx, rcx, rbx, rax;
    uint64_t int_no, err_code;
    uint64_t rip, cs, rflags, rsp, ss;
} EFI_SYSTEM_CONTEXT_X64;

struct Process { 
    int64_t ID;
    const CHAR16* Name;
    uint8_t Rights;
    void* ArgContext; 
    void* storage;
    uint8_t active;
    uint64_t sizeMem;
    int64_t ParentID;
};

void main(struct Process* pr); 

__attribute__((used, section(".text.boot")))
void _start(struct Process* pr) { 
    main(pr); 
    asm volatile ("movq $0x04, %%rax; int $0x25" : : : "rax"); 
}

typedef struct {
    uint64_t Status;
    CHAR16* Message;
    uint64_t FileSize;
} EC16;

// Сканкоды клавиатуры
#define CHAR_BACKSPACE        0x0008
#define CHAR_TAB              0x0009
#define CHAR_LINEFEED         0x000A
#define CHAR_CARRIAGE_RETURN  0x000D
#define SCAN_NULL       0x0000
#define SCAN_UP         0x01FF0000
#define SCAN_DOWN       0x02FF0000
#define SCAN_RIGHT      0x03FF0000
#define SCAN_LEFT       0x04FF0000
#define SCAN_ESC        0x17FF0000

// ==========================================
// INT 0x20: SYSTEM TIME & INFO
// ==========================================
uint64_t SysGetTickCount() { uint64_t r; asm volatile ("movq $0x01, %%rax; int $0x20; movq %%rax, %0" : "=r"(r) : : "rax"); return r; }
void SysStall(uint64_t usec) { asm volatile ("movq $0x02, %%rax; movq %0, %%rcx; int $0x20" : : "r"(usec) : "rax", "rcx"); }
uint64_t SysGetBuild() { uint64_t r; asm volatile ("movq $0x03, %%rax; int $0x20; movq %%rax, %0" : "=r"(r) : : "rax"); return r; }
uint64_t SysGetVersion() { uint64_t r; asm volatile ("movq $0x04, %%rax; int $0x20; movq %%rax, %0" : "=r"(r) : : "rax"); return r; }
// ==========================================
// INT 0x24: GRAPHICS & BMP PARSING
// ==========================================
uint64_t SysParseBmp(const void* data, uint64_t size, uint32_t* out_w, uint32_t* out_h, uint32_t** out_buf) {
    uint64_t r;
    asm volatile (
        "movq $0x05, %%rax;"
        "movq %1, %%rcx;"
        "movq %2, %%rdx;"
        "movq %3, %%r8;"
        "movq %4, %%r9;"
        "movq %5, %%r10;"
        "int $0x24;"
        "movq %%rax, %0"
        : "=r"(r)
        : "r"(data), "r"(size), "r"(out_w), "r"(out_h), "r"(out_buf)
        : "rax", "rcx", "rdx", "r8", "r9", "r10"
    );
    return r;
}
// ==========================================
// INT 0x21: CONSOLE IO
// ==========================================
void ConPrintChar(CHAR16 c) { asm volatile ("movq $0x01, %%rax; movq %0, %%rcx; int $0x21" : : "r"((uint64_t)c) : "rax", "rcx"); }
void ConPrint(const CHAR16* msg) { asm volatile ("movq $0x02, %%rax; movq %0, %%rcx; int $0x21" : : "r"(msg) : "rax", "rcx"); }
void ConSetAttribute(uint64_t attr) { asm volatile ("movq $0x04, %%rax; movq %0, %%rcx; int $0x21" : : "r"(attr) : "rax", "rcx"); }
void ConClear() { asm volatile ("movq $0x05, %%rax; int $0x21" : : : "rax"); }
void ConSetCursor(uint64_t x, uint64_t y) { asm volatile ("movq $0x06, %%rax; movq %0, %%rcx; movq %1, %%rdx; int $0x21" : : "r"(x), "r"(y) : "rax", "rcx", "rdx"); }
void ConEnableCursor(uint8_t enable) { asm volatile ("movq $0x07, %%rax; movq %0, %%rcx; int $0x21" : : "r"((uint64_t)enable) : "rax", "rcx"); }

// ==========================================
// INT 0x22: KEYBOARD
// ==========================================
uint64_t KbdGetKey() { uint64_t r; asm volatile ("movq $0x01, %%rax; int $0x22; movq %%rax, %0" : "=r"(r) : : "rax"); return r; }
uint64_t KbdHasKey() { uint64_t r; asm volatile ("movq $0x02, %%rax; int $0x22; movq %%rax, %0" : "=r"(r) : : "rax"); return r; }
void KbdReset() { asm volatile ("movq $0x03, %%rax; int $0x22" : : : "rax"); }

// ==========================================
// INT 0x23: STORAGE / FILE SYSTEM API
// ==========================================
uint64_t FsReadFileByPath(const CHAR16* path, EC16* out) { uint64_t r; asm volatile ("movq $0x01, %%rax; movq %1, %%rcx; movq %2, %%rdx; int $0x23; movq %%rax, %0" : "=r"(r) : "r"(path), "r"(out) : "rax", "rcx", "rdx"); return r; }
uint64_t FsSetCurrentDisk(CHAR16 letter) { uint64_t r; asm volatile ("movq $0x02, %%rax; movq %1, %%rcx; int $0x23; movq %%rax, %0" : "=r"(r) : "r"((uint64_t)letter) : "rax", "rcx"); return r; }
uint64_t FsWriteFile(const CHAR16* filename, const uint16_t* data, uint64_t len) { uint64_t r; asm volatile ("movq $0x03, %%rax; movq %1, %%rcx; movq %2, %%rdx; movq %3, %%r8; int $0x23; movq %%rax, %0" : "=r"(r) : "r"(filename), "r"(data), "r"(len) : "rax", "rcx", "rdx", "r8"); return r; }
uint64_t FsCreateFile(const CHAR16* name) { uint64_t r; asm volatile ("movq $0x04, %%rax; movq %1, %%rcx; int $0x23; movq %%rax, %0" : "=r"(r) : "r"(name) : "rax", "rcx"); return r; }
uint64_t FsDeleteFile(const CHAR16* name) { uint64_t r; asm volatile ("movq $0x05, %%rax; movq %1, %%rcx; int $0x23; movq %%rax, %0" : "=r"(r) : "r"(name) : "rax", "rcx"); return r; }
uint64_t FsGetFileSize(const CHAR16* filename, uint64_t* filesize) { uint64_t r; asm volatile ("movq $0x06, %%rax; movq %1, %%rcx; movq %2, %%rdx; int $0x23; movq %%rax, %0" : "=r"(r) : "r"(filename), "r"(filesize) : "rax", "rcx", "rdx"); return r; }
uint64_t FsChangeDir(const CHAR16* path) { uint64_t r; asm volatile ("movq $0x07, %%rax; int $0x23; movq %1, %%rcx; movq %%rax, %0" : "=r"(r) : "r"(path) : "rax", "rcx"); return r; }
CHAR16* FsListDir(CHAR16* path) { uint64_t result; asm volatile ("movq $0x08, %%rax; movq %1, %%rcx; int $0x23; movq %%rax, %0" : "=r"(result) : "r"(path) : "rax", "rcx"); return (CHAR16*)result; }
CHAR16* FsListDisks() { uint64_t res; asm volatile ("movq $0x09, %%rax; int $0x23; movq %%rax, %0" : "=r"(res) : : "rax"); return (CHAR16*)res; }
bool FsFileExists(const CHAR16* path) { uint64_t r; asm volatile ("movq $0x0A, %%rax; movq %1, %%rcx; int $0x23; movq %%rax, %0" : "=r"(r) : "r"(path) : "rax", "rcx"); return (bool)r; }
bool FsDirExists(const CHAR16* path) { uint64_t r; asm volatile ("movq $0x0B, %%rax; movq %1, %%rcx; int $0x23; movq %%rax, %0" : "=r"(r) : "r"(path) : "rax", "rcx"); return (bool)r; }
uint64_t FsCreateDir(const CHAR16* name) { uint64_t r; asm volatile ("movq $0x0C, %%rax; movq %1, %%rcx; int $0x23; movq %%rax, %0" : "=r"(r) : "r"(name) : "rax", "rcx"); return r; }
uint64_t FsDeleteDir(const CHAR16* name) { uint64_t r; asm volatile ("movq $0x0D, %%rax; movq %1, %%rcx; int $0x23; movq %%rax, %0" : "=r"(r) : "r"(name) : "rax", "rcx"); return r; }
uint64_t FsMoveFile(const CHAR16* src, const CHAR16* dst) { uint64_t r; asm volatile ("movq $0x0E, %%rax; movq %1, %%rcx; movq %2, %%rdx; int $0x23; movq %%rax, %0" : "=r"(r) : "r"(src), "r"(dst) : "rax", "rcx", "rdx"); return r; }
uint64_t FsCopyFile(const CHAR16* src, const CHAR16* dst) { uint64_t r; asm volatile ("movq $0x0F, %%rax; movq %1, %%rcx; movq %2, %%rdx; int $0x23; movq %%rax, %0" : "=r"(r) : "r"(src), "r"(dst) : "rax", "rcx", "rdx"); return r; }
EC16 FsReadFile(const CHAR16* filename) { EC16 res; uint64_t st, msg, sz; asm volatile ("movq $0x11, %%rax; movq %3, %%rcx; int $0x23; movq %%rax, %0; movq %%rdx, %1; movq %%r8, %2" : "=r"(st), "=r"(msg), "=r"(sz) : "r"(filename) : "rax", "rcx", "rdx", "r8"); res.Status = st; res.Message = (CHAR16*)msg; res.FileSize = sz; return res; }
const CHAR16* FsGetCurrentPath(void) { const CHAR16* r; asm volatile ("movq $0x12, %%rax; int $0x23; movq %%rax, %0" : "=r"(r) : : "rax"); return r; }
uint64_t FsPathUp(void) { uint64_t r; asm volatile ("movq $0x13, %%rax; int $0x23; movq %%rax, %0" : "=r"(r) : : "rax"); return r; }
void FsRegistersDisk(void) { asm volatile ("movq $0x14, %%rax; int $0x23" : : : "rax"); }

// ==========================================
// INT 0x24: GRAPHICS / VIDEO
// ==========================================
void GfxClear(uint32_t color) { asm volatile ("movq $0x01, %%rax; movq %0, %%rcx; int $0x24" : : "r"((uint64_t)color) : "rax", "rcx"); }
void GfxPutPixel(int32_t x, int32_t y, uint32_t color) { asm volatile ("movq $0x02, %%rax; movq %0, %%rcx; movq %1, %%rdx; movq %2, %%r8; int $0x24" : : "r"((uint64_t)x), "r"((uint64_t)y), "r"((uint64_t)color) : "rax", "rcx", "rdx", "r8"); }
void GfxDrawLine(int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint32_t color) { asm volatile ("movq $0x03, %%rax; movq %0, %%rcx; movq %1, %%rdx; movq %2, %%r8; movq %3, %%r9; movq %4, %%r10; int $0x24" : : "r"((uint64_t)x1), "r"((uint64_t)y1), "r"((uint64_t)x2), "r"((uint64_t)y2), "r"((uint64_t)color) : "rax", "rcx", "rdx", "r8", "r9", "r10"); }
void GfxDrawBitmap(uint32_t* data, int32_t x, int32_t y, int32_t w, int32_t h) { asm volatile ("movq $0x04, %%rax; movq %0, %%rcx; movq %1, %%rdx; movq %2, %%r8; movq %3, %%r9; movq %4, %%r10; int $0x24" : : "r"(data), "r"((uint64_t)x), "r"((uint64_t)y), "r"((uint64_t)w), "r"((uint64_t)h) : "rax", "rcx", "rdx", "r8", "r9", "r10"); }
uint64_t GfxLoadFont(const CHAR16* path, const CHAR16* name) { uint64_t r; asm volatile ("movq $0x05, %%rax; movq %1, %%rcx; movq %2, %%rdx; int $0x24; movq %%rax, %0" : "=r"(r) : "r"(path), "r"(name) : "rax", "rcx", "rdx"); return r; }
void GfxSetFont(const CHAR16* name) { _api_font = name; }
void GfxDrawChar(int32_t x, int32_t y, int32_t size, uint32_t color, CHAR16 c) { asm volatile ("movq $0x06, %%rax; movq %0, %%rcx; movq %1, %%rdx; movq %2, %%r8; movq %3, %%r9; movq %4, %%r10; movq %5, %%r11; int $0x24" : : "r"(_api_font), "r"((uint64_t)x), "r"((uint64_t)y), "r"((uint64_t)size), "r"((uint64_t)color), "r"((uint64_t)c) : "rax", "rcx", "rdx", "r8", "r9", "r10", "r11"); }
void GfxPrint(int32_t x, int32_t y, int32_t size, uint32_t color, const CHAR16* text) { asm volatile ("movq $0x08, %%rax; movq %0, %%rcx; movq %1, %%rdx; movq %2, %%r8; movq %3, %%r9; movq %4, %%r10; movq %5, %%r11; int $0x24" : : "r"(_api_font), "r"((uint64_t)x), "r"((uint64_t)y), "r"((uint64_t)size), "r"((uint64_t)color), "r"(text) : "rax", "rcx", "rdx", "r8", "r9", "r10", "r11"); }
void GfxGetScreenSize(int32_t* w, int32_t* h) { uint64_t width, height; asm volatile ("movq $0x09, %%rax; int $0x24; movq %%rax, %0; movq %%rbx, %1" : "=r"(width), "=r"(height) : : "rax", "rbx", "rcx"); *w = (int32_t)width; *h = (int32_t)height; }
uint32_t GfxGetPixel(int32_t x, int32_t y) { uint64_t color; asm volatile ("movq $0x0A, %%rax; movq %1, %%rcx; movq %2, %%rdx; int $0x24; movq %%rax, %0" : "=r"(color) : "r"((uint64_t)x), "r"((uint64_t)y) : "rax", "rcx", "rdx"); return (uint32_t)color; }
void SB() { asm volatile ("movq $0x0C, %%rax; int $0x24" : : : "rax"); }
void GpuUploadShader(void* shader, uint64_t size, uint64_t id) { asm volatile ("movq $0x0D, %%rax; movq %0, %%rcx; movq %1, %%rdx; movq %2, %%r8; int $0x24" : : "r"(shader), "r"(size), "r"(id) : "rax", "rcx", "rdx", "r8"); }
void GpuRunCompute(uint64_t id, uint32_t workgroups) { asm volatile ("movq $0x0E, %%rax; movq %0, %%rcx; movq %1, %%rdx; int $0x24" : : "r"(id), "r"((uint64_t)workgroups) : "rax", "rcx", "rdx"); }
CHAR16* GfxGetVideoStatus() { uint64_t r; asm volatile ("movq $0x0F, %%rax; int $0x24; movq %%rax, %0" : "=r"(r) : : "rax"); return (CHAR16*)r; }

// ==========================================
// INT 0x25: MULTITASKING (Process Manager)
// ==========================================
void TaskCreate(int32_t id, void (*entry)(void)) { asm volatile ("movq $0x01, %%rax; movq %0, %%rcx; movq %1, %%rdx; int $0x25" : : "r"((uint64_t)id), "r"(entry) : "rax", "rcx", "rdx"); }
void TaskCreateWithArg(int32_t id, void (*entry)(void*), void* arg) { asm volatile ("movq $0x02, %%rax; movq %0, %%rcx; movq %1, %%rdx; movq %2, %%r8; int $0x25" : : "r"((uint64_t)id), "r"(entry), "r"(arg) : "rax", "rcx", "rdx", "r8"); }
void TaskYield() { asm volatile ("movq $0x03, %%rax; int $0x25" : : : "rax"); }
void TaskExit() { asm volatile ("movq $0x04, %%rax; int $0x25" : : : "rax"); }
uint64_t TaskGetCurrent() { uint64_t r; asm volatile ("movq $0x05, %%rax; int $0x25; movq %%rax, %0" : "=r"(r) : : "rax"); return r; }
void TaskStartFirst() { asm volatile ("movq $0x06, %%rax; int $0x25" : : : "rax"); }
void TaskStopAndRun(int32_t id) { asm volatile ("movq $0x07, %%rax; movq %0, %%rcx; int $0x25" : : "r"((uint64_t)id) : "rax", "rcx"); }
void TaskExitX(int32_t id) { asm volatile ("movq $0x08, %%rax; movq %0, %%rcx; int $0x25" : : "r"((uint64_t)id) : "rax", "rcx"); }
uint64_t PexRun(const CHAR16* path, struct Process* p_info) { uint64_t r; asm volatile ("movq $0x09, %%rax; movq %1, %%rcx; movq %2, %%rdx; int $0x25; movq %%rax, %0" : "=r"(r) : "r"(path), "r"(p_info) : "rax", "rcx", "rdx"); return r; }
struct Process* GetCurrentProcess() { uint64_t r; asm volatile ("movq $0x0A, %%rax; int $0x25; movq %%rax, %0" : "=r"(r) : : "rax"); return (struct Process*)r; }
uint64_t ProcessExit(int32_t id) { uint64_t r; asm volatile ("movq $0x0B, %%rax; movq %1, %%rcx; int $0x25; movq %%rax, %0" : "=r"(r) : "r"((uint64_t)id) : "rax", "rcx"); return r; }
uint64_t TaskGetById(int32_t id) { uint64_t r; asm volatile ("movq $0x0C, %%rax; movq %1, %%rcx; int $0x25; movq %%rax, %0" : "=r"(r) : "r"((uint64_t)id) : "rax", "rcx"); return r; }
static inline uint64_t DrawBmp(uint8_t* data, uint64_t size, int32_t x, int32_t y) {
    uint64_t r;
    asm volatile (
        "movq $0x11, %%rax;"
        "movq %1, %%rcx;" // Указатель
        "movq %2, %%rdx;" // Размер
        "movq %3, %%r8;"  // X
        "movq %4, %%r9;"  // Y
        "int $0x80;"      // Или ваш номер вектора прерывания
        "movq %%rax, %0"
        : "=r"(r)
        : "r"((uint64_t)data), "r"(size), "r"((uint64_t)x), "r"((uint64_t)y)
        : "rax", "rcx", "rdx", "r8", "r9"
    );
    return r;
}
// ==========================================
// INT 0x26: KERNEL SERVICES
// ==========================================
void SysRegisterHandler(uint8_t vector, void* handler) { asm volatile ("movq $0x01, %%rax; movq %0, %%rbx; movq %1, %%rcx; int $0x26" : : "r"((uint64_t)vector), "r"(handler) : "rax", "rbx", "rcx"); }
uint64_t SysRegisterDriver(void* driver) { uint64_t r; asm volatile ("movq $0x02, %%rax; movq %1, %%rcx; int $0x26; movq %%rax, %0" : "=r"(r) : "r"(driver) : "rax", "rcx"); return r; }
void SysGetHandles(void** img_handle, void** sys_table) { uint64_t i, s; asm volatile ("movq $0x03, %%rax; int $0x26; movq %%rcx, %0; movq %%rdx, %1" : "=r"(i), "=r"(s) : : "rax", "rcx", "rdx"); if (img_handle) *img_handle = (void*)i; if (sys_table) *sys_table = (void*)s; }
void SysReboot() { asm volatile ("movq $0x04, %%rax; int $0x26" : : : "rax"); }
void SysShutdown() { asm volatile ("movq $0x05, %%rax; int $0x26" : : : "rax"); }
void SysInitDrivers() { asm volatile ("movq $0x06, %%rax; int $0x26" : : : "rax"); }

// ==========================================
// INT 0x27: NETWORK
// ==========================================
uint64_t NetInit(const CHAR16* ip, const CHAR16* mask) { uint64_t r; asm volatile ("movq $0x01, %%rax; movq %1, %%rcx; movq %2, %%rdx; int $0x27; movq %%rax, %0" : "=r"(r) : "r"(ip), "r"(mask) : "rax", "rcx", "rdx"); return r; }
uint64_t NetTcpConnect(const CHAR16* ip, uint16_t port) { uint64_t r; asm volatile ("movq $0x02, %%rax; movq %1, %%rcx; movq %2, %%rdx; int $0x27; movq %%rax, %0" : "=r"(r) : "r"(ip), "r"((uint64_t)port) : "rax", "rcx", "rdx"); return r; }
uint64_t NetTcpSend(uint8_t* data, uint64_t size) { uint64_t r; asm volatile ("movq $0x03, %%rax; movq %1, %%rcx; movq %2, %%rdx; int $0x27; movq %%rax, %0" : "=r"(r) : "r"(data), "r"(size) : "rax", "rcx", "rdx"); return r; }
uint64_t NetTcpReceive(uint8_t* buffer, uint64_t* size_out) { uint64_t r; asm volatile ("movq $0x04, %%rax; movq %1, %%rcx; movq %2, %%rdx; int $0x27; movq %%rax, %0" : "=r"(r) : "r"(buffer), "r"(size_out) : "rax", "rcx", "rdx"); return r; }
uint64_t NetTcpDisconnect() { uint64_t r; asm volatile ("movq $0x05, %%rax; int $0x27; movq %%rax, %0" : "=r"(r) : : "rax"); return r; }
uint64_t NetDnsLookup(const CHAR16* host, CHAR16* out_ip) { uint64_t r; asm volatile ("movq $0x06, %%rax; movq %1, %%rcx; movq %2, %%rdx; int $0x27; movq %%rax, %0" : "=r"(r) : "r"(host), "r"(out_ip) : "rax", "rcx", "rdx"); return r; }

// ==========================================
// INT 0x28: AUDIO
// ==========================================
void AudioBeep(uint32_t freq, uint32_t ms) { asm volatile ("movq $0x01, %%rax; movq %0, %%rcx; movq %1, %%rdx; int $0x28" : : "r"((uint64_t)freq), "r"((uint64_t)ms) : "rax", "rcx", "rdx"); }
uint64_t AudioPlay(uint8_t* data, uint64_t size) { uint64_t r; asm volatile ("movq $0x02, %%rax; movq %1, %%rcx; movq %2, %%rdx; int $0x28; movq %%rax, %0" : "=r"(r) : "r"(data), "r"(size) : "rax", "rcx", "rdx"); return r; }

// ==========================================
// INT 0x29: MOUSE
// ==========================================
uint64_t MouseInit() { uint64_t r; asm volatile ("movq $0x01, %%rax; int $0x29; movq %%rax, %0" : "=r"(r) : : "rax"); return r; }
uint64_t MouseGetState(int32_t* x, int32_t* y, uint8_t* b1, uint8_t* b2) { uint64_t r; asm volatile ("movq $0x02, %%rax; movq %1, %%rcx; movq %2, %%rdx; movq %3, %%r8; movq %4, %%r9; int $0x29; movq %%rax, %0" : "=r"(r) : "r"(x), "r"(y), "r"(b1), "r"(b2) : "rax", "rcx", "rdx", "r8", "r9"); return r; }

// ==========================================
// INT 0x2A: MEMORY ALLOCATION
// ==========================================
void* MemAlloc(uint64_t size) { uint64_t r; asm volatile ("movq $0x01, %%rax; movq %1, %%rcx; int $0x2A; movq %%rax, %0" : "=r"(r) : "r"(size) : "rax", "rcx"); return (void*)r; }
void MemFree(void* ptr) { asm volatile ("movq $0x02, %%rax; movq %0, %%rcx; int $0x2A" : : "r"(ptr) : "rax", "rcx"); }

#endif