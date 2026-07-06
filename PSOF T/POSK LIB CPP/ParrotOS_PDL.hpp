#ifndef PARROT_API_HPP
#define PARROT_API_HPP
#pragma once

#include <cstdint>
#include <cstddef>

// Нативный тип для 16-битных символов в C++
using CHAR16 = char16_t;

namespace ParrotOS {

    inline const CHAR16* api_font = u"SysFont";

    struct SystemContextX64 {
        uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
        uint64_t rbp, rsi, rdi, rdx, rcx, rbx, rax;
        uint64_t int_no, err_code;
        uint64_t rip, cs, rflags, rsp, ss;
    };

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

    struct EC16 {
        uint64_t Status;
        CHAR16* Message;
        uint64_t FileSize;
    };

    // Сканкоды клавиатуры (constexpr вместо макросов)
    namespace Keys {
        constexpr uint16_t Backspace      = 0x0008;
        constexpr uint16_t Tab            = 0x0009;
        constexpr uint16_t Linefeed       = 0x000A;
        constexpr uint16_t CarriageReturn = 0x000D;
        constexpr uint16_t Null           = 0x0000;
        constexpr uint16_t Up             = 0x0100;
        constexpr uint16_t Down           = 0x0200;
        constexpr uint16_t Right          = 0x0300;
        constexpr uint16_t Left           = 0x0400;
        constexpr uint16_t Esc            = 0x1700;
    }

    // ==========================================
    // INT 0x20: SYSTEM TIME & INFO
    // ==========================================
    namespace System {
        inline uint64_t GetTickCount() { uint64_t r; asm volatile ("movq $0x01, %%rax; int $0x20; movq %%rax, %0" : "=r"(r) : : "rax"); return r; }
        inline void Stall(uint64_t usec) { asm volatile ("movq $0x02, %%rax; movq %0, %%rcx; int $0x20" : : "r"(usec) : "rax", "rcx"); }
        inline uint64_t GetBuild() { uint64_t r; asm volatile ("movq $0x03, %%rax; int $0x20; movq %%rax, %0" : "=r"(r) : : "rax"); return r; }
        inline uint64_t GetVersion() { uint64_t r; asm volatile ("movq $0x04, %%rax; int $0x20; movq %%rax, %0" : "=r"(r) : : "rax"); return r; }
    }

    // Размер образа ТЕКУЩЕЙ программы (.pex) в памяти.
    // Требует символ __pex_end в конце SECTIONS в app.ld (см. pdl.ld —
    // там аналогичный __pdl_end уже добавлен). Т.к. образ грузится с
    // адреса 0, &__pex_end == размер файла Main.bin в байтах.
    extern "C" uint8_t __pex_end;
    inline uint64_t GetImageSize() { return (uint64_t)&__pex_end; }

    // ==========================================
    // INT 0x21: CONSOLE IO
    // ==========================================
    namespace Console {
        inline void PrintChar(CHAR16 c) { asm volatile ("movq $0x01, %%rax; movq %0, %%rcx; int $0x21" : : "r"((uint64_t)c) : "rax", "rcx"); }
        inline void Print(const CHAR16* msg) { asm volatile ("movq $0x02, %%rax; movq %0, %%rcx; int $0x21" : : "r"(msg) : "rax", "rcx"); }
        inline void SetAttribute(uint64_t attr) { asm volatile ("movq $0x04, %%rax; movq %0, %%rcx; int $0x21" : : "r"(attr) : "rax", "rcx"); }
        inline void Clear() { asm volatile ("movq $0x05, %%rax; int $0x21" : : : "rax"); }
        inline void SetCursor(uint64_t x, uint64_t y) { asm volatile ("movq $0x06, %%rax; movq %0, %%rcx; movq %1, %%rdx; int $0x21" : : "r"(x), "r"(y) : "rax", "rcx", "rdx"); }
        inline void EnableCursor(bool enable) { asm volatile ("movq $0x07, %%rax; movq %0, %%rcx; int $0x21" : : "r"((uint64_t)enable) : "rax", "rcx"); }
    }

    // ==========================================
    // INT 0x22: KEYBOARD
    // ==========================================
    namespace Keyboard {
        // 0x01: Неблокирующее чтение (возвращает код клавиши с маркером 0xFF00 или 0, если пусто)
        inline char16_t GetKeyNonBlocking() { 
            register uint64_t rax asm("rax") = 0x01;
            asm volatile ("int $0x22" : "+r"(rax) : : "rcx", "rdx", "rbx", "memory"); 
            return static_cast<char16_t>(rax); 
        }
        
        // 0x02: Выгрузить весь текущий буфер в массив программы (возвращает количество скопированных клавиш)
        inline uint64_t GetBuffer(char16_t* buffer, uint64_t maxLength) { 
            register uint64_t rax asm("rax") = 0x02;
            register uint64_t rcx asm("rcx") = reinterpret_cast<uint64_t>(buffer);
            register uint64_t rdx asm("rdx") = maxLength;
            asm volatile ("int $0x22" : "+r"(rax), "+r"(rcx), "+r"(rdx) : : "rbx", "memory"); 
            return rax;
        }
        
        // 0x03: Быстрая очистка программного буфера
        inline void Flush() { 
            register uint64_t rax asm("rax") = 0x03;
            asm volatile ("int $0x22" : "+r"(rax) : : "rcx", "rdx", "rbx", "memory"); 
        }
        
        // 0x04: Блокирующее чтение (программа спит, пока пользователь не нажмёт клавишу)
        inline char16_t GetKey() { 
            register uint64_t rax asm("rax") = 0x04;
            asm volatile ("int $0x22" : "+r"(rax) : : "rcx", "rdx", "rbx", "memory"); 
            return static_cast<char16_t>(rax);
        }
        
        // 0x05: Проверка наличия клавиши (true — если есть клавиша в буфере, false — если пусто)
        inline bool HasKey() { 
            register uint64_t rax asm("rax") = 0x05;
            asm volatile ("int $0x22" : "+r"(rax) : : "rcx", "rdx", "rbx", "memory"); 
            return (rax != 0);
        }

        // 0x06: Получить текущее количество клавиш, ожидающих в буфере
        inline uint64_t GetBufferSize() { 
            register uint64_t rax asm("rax") = 0x06;
            asm volatile ("int $0x22" : "+r"(rax) : : "rcx", "rdx", "rbx", "memory"); 
            return rax;
        }

        // 0x07: Полный аппаратный сброс контроллера клавиатуры + очистка очереди
        inline void Reset() { 
            register uint64_t rax asm("rax") = 0x07;
            asm volatile ("int $0x22" : "+r"(rax) : : "rcx", "rdx", "rbx", "memory"); 
        }
    }
    // ==========================================
    // INT 0x23: STORAGE / FILE SYSTEM API
    // ==========================================
    namespace FileSystem {
        inline uint64_t ReadFileByPath(const CHAR16* path, EC16* out) { uint64_t r; asm volatile ("movq $0x01, %%rax; movq %1, %%rcx; movq %2, %%rdx; int $0x23; movq %%rax, %0" : "=r"(r) : "r"(path), "r"(out) : "rax", "rcx", "rdx"); return r; }
        inline uint64_t SetCurrentDisk(CHAR16 letter) { uint64_t r; asm volatile ("movq $0x02, %%rax; movq %1, %%rcx; int $0x23; movq %%rax, %0" : "=r"(r) : "r"((uint64_t)letter) : "rax", "rcx"); return r; }
        inline uint64_t WriteFile(const CHAR16* filename, const uint16_t* data, uint64_t len) { uint64_t r; asm volatile ("movq $0x03, %%rax; movq %1, %%rcx; movq %2, %%rdx; movq %3, %%r8; int $0x23; movq %%rax, %0" : "=r"(r) : "r"(filename), "r"(data), "r"(len) : "rax", "rcx", "rdx", "r8"); return r; }
        inline uint64_t CreateFile(const CHAR16* name) { uint64_t r; asm volatile ("movq $0x04, %%rax; movq %1, %%rcx; int $0x23; movq %%rax, %0" : "=r"(r) : "r"(name) : "rax", "rcx"); return r; }
        inline uint64_t DeleteFile(const CHAR16* name) { uint64_t r; asm volatile ("movq $0x05, %%rax; movq %1, %%rcx; int $0x23; movq %%rax, %0" : "=r"(r) : "r"(name) : "rax", "rcx"); return r; }
        inline uint64_t GetFileSize(const CHAR16* filename, uint64_t* filesize) { uint64_t r; asm volatile ("movq $0x06, %%rax; movq %1, %%rcx; movq %2, %%rdx; int $0x23; movq %%rax, %0" : "=r"(r) : "r"(filename), "r"(filesize) : "rax", "rcx", "rdx"); return r; }
        inline uint64_t ChangeDir(const CHAR16* path) { uint64_t r; asm volatile ("movq $0x07, %%rax; int $0x23; movq %1, %%rcx; movq %%rax, %0" : "=r"(r) : "r"(path) : "rax", "rcx"); return r; }
        inline CHAR16* ListDir(CHAR16* path) { uint64_t result; asm volatile ("movq $0x08, %%rax; movq %1, %%rcx; int $0x23; movq %%rax, %0" : "=r"(result) : "r"(path) : "rax", "rcx"); return (CHAR16*)result; }
        inline CHAR16* ListDisks() { uint64_t res; asm volatile ("movq $0x09, %%rax; int $0x23; movq %%rax, %0" : "=r"(res) : : "rax"); return (CHAR16*)res; }
        inline bool FileExists(const CHAR16* path) { uint64_t r; asm volatile ("movq $0x0A, %%rax; movq %1, %%rcx; int $0x23; movq %%rax, %0" : "=r"(r) : "r"(path) : "rax", "rcx"); return (bool)r; }
        inline bool DirExists(const CHAR16* path) { uint64_t r; asm volatile ("movq $0x0B, %%rax; movq %1, %%rcx; int $0x23; movq %%rax, %0" : "=r"(r) : "r"(path) : "rax", "rcx"); return (bool)r; }
        inline uint64_t CreateDir(const CHAR16* name) { uint64_t r; asm volatile ("movq $0x0C, %%rax; movq %1, %%rcx; int $0x23; movq %%rax, %0" : "=r"(r) : "r"(name) : "rax", "rcx"); return r; }
        inline uint64_t DeleteDir(const CHAR16* name) { uint64_t r; asm volatile ("movq $0x0D, %%rax; movq %1, %%rcx; int $0x23; movq %%rax, %0" : "=r"(r) : "r"(name) : "rax", "rcx"); return r; }
        inline uint64_t MoveFile(const CHAR16* src, const CHAR16* dst) { uint64_t r; asm volatile ("movq $0x0E, %%rax; movq %1, %%rcx; movq %2, %%rdx; int $0x23; movq %%rax, %0" : "=r"(r) : "r"(src), "r"(dst) : "rax", "rcx", "rdx"); return r; }
        inline uint64_t CopyFile(const CHAR16* src, const CHAR16* dst) { uint64_t r; asm volatile ("movq $0x0F, %%rax; movq %1, %%rcx; movq %2, %%rdx; int $0x23; movq %%rax, %0" : "=r"(r) : "r"(src), "r"(dst) : "rax", "rcx", "rdx"); return r; }
        inline EC16 ReadFile(const CHAR16* filename) { EC16 res; uint64_t st, msg, sz; asm volatile ("movq $0x11, %%rax; movq %3, %%rcx; int $0x23; movq %%rax, %0; movq %%rdx, %1; movq %%r8, %2" : "=r"(st), "=r"(msg), "=r"(sz) : "r"(filename) : "rax", "rcx", "rdx", "r8"); res.Status = st; res.Message = (CHAR16*)msg; res.FileSize = sz; return res; }
        inline const CHAR16* GetCurrentPath() { const CHAR16* r; asm volatile ("movq $0x12, %%rax; int $0x23; movq %%rax, %0" : "=r"(r) : : "rax"); return r; }
        inline uint64_t PathUp() { uint64_t r; asm volatile ("movq $0x13, %%rax; int $0x23; movq %%rax, %0" : "=r"(r) : : "rax"); return r; }
        inline void RegistersDisk() { asm volatile ("movq $0x14, %%rax; int $0x23" : : : "rax"); }
    }

    // ==========================================
    // INT 0x24: GRAPHICS / VIDEO
    // ==========================================
    namespace Graphics {
        inline void Clear(uint32_t color) { asm volatile ("movq $0x01, %%rax; movq %0, %%rcx; int $0x24" : : "r"((uint64_t)color) : "rax", "rcx"); }
        inline void PutPixel(int32_t x, int32_t y, uint32_t color) { asm volatile ("movq $0x02, %%rax; movq %0, %%rcx; movq %1, %%rdx; movq %2, %%r8; int $0x24" : : "r"((uint64_t)x), "r"((uint64_t)y), "r"((uint64_t)color) : "rax", "rcx", "rdx", "r8"); }
        inline void DrawLine(int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint32_t color) { asm volatile ("movq $0x03, %%rax; movq %0, %%rcx; movq %1, %%rdx; movq %2, %%r8; movq %3, %%r9; movq %4, %%r10; int $0x24" : : "r"((uint64_t)x1), "r"((uint64_t)y1), "r"((uint64_t)x2), "r"((uint64_t)y2), "r"((uint64_t)color) : "rax", "rcx", "rdx", "r8", "r9", "r10"); }
        inline void DrawBitmap(uint32_t* data, int32_t x, int32_t y, int32_t w, int32_t h) { asm volatile ("movq $0x04, %%rax; movq %0, %%rcx; movq %1, %%rdx; movq %2, %%r8; movq %3, %%r9; movq %4, %%r10; int $0x24" : : "r"(data), "r"((uint64_t)x), "r"((uint64_t)y), "r"((uint64_t)w), "r"((uint64_t)h) : "rax", "rcx", "rdx", "r8", "r9", "r10"); }
        inline uint64_t LoadFont(const CHAR16* path, const CHAR16* name) { uint64_t r; asm volatile ("movq $0x05, %%rax; movq %1, %%rcx; movq %2, %%rdx; int $0x24; movq %%rax, %0" : "=r"(r) : "r"(path), "r"(name) : "rax", "rcx", "rdx"); return r; }
        inline void SetFont(const CHAR16* name) { api_font = name; }
        inline void DrawChar(int32_t x, int32_t y, int32_t size, uint32_t color, CHAR16 c) { asm volatile ("movq $0x06, %%rax; movq %0, %%rcx; movq %1, %%rdx; movq %2, %%r8; movq %3, %%r9; movq %4, %%r10; movq %5, %%r11; int $0x24" : : "r"(api_font), "r"((uint64_t)x), "r"((uint64_t)y), "r"((uint64_t)size), "r"((uint64_t)color), "r"((uint64_t)c) : "rax", "rcx", "rdx", "r8", "r9", "r10", "r11"); }
        inline void Print(int32_t x, int32_t y, int32_t size, uint32_t color, const CHAR16* text) { asm volatile ("movq $0x08, %%rax; movq %0, %%rcx; movq %1, %%rdx; movq %2, %%r8; movq %3, %%r9; movq %4, %%r10; movq %5, %%r11; int $0x24" : : "r"(api_font), "r"((uint64_t)x), "r"((uint64_t)y), "r"((uint64_t)size), "r"((uint64_t)color), "r"(text) : "rax", "rcx", "rdx", "r8", "r9", "r10", "r11"); }
        inline void GetScreenSize(int32_t& w, int32_t& h) { uint64_t width, height; asm volatile ("movq $0x09, %%rax; int $0x24; movq %%rax, %0; movq %%rbx, %1" : "=r"(width), "=r"(height) : : "rax", "rbx", "rcx"); w = (int32_t)width; h = (int32_t)height; }
        inline uint32_t GetPixel(int32_t x, int32_t y) { uint64_t color; asm volatile ("movq $0x0A, %%rax; movq %1, %%rcx; movq %2, %%rdx; int $0x24; movq %%rax, %0" : "=r"(color) : "r"((uint64_t)x), "r"((uint64_t)y) : "rax", "rcx", "rdx"); return (uint32_t)color; }
        inline void SwapBuffers() { asm volatile ("movq $0x0C, %%rax; int $0x24" : : : "rax"); }
        inline void UploadShader(void* shader, uint64_t size, uint64_t id) { asm volatile ("movq $0x0D, %%rax; movq %0, %%rcx; movq %1, %%rdx; movq %2, %%r8; int $0x24" : : "r"(shader), "r"(size), "r"(id) : "rax", "rcx", "rdx", "r8"); }
        inline void RunCompute(uint64_t id, uint32_t workgroups) { asm volatile ("movq $0x0E, %%rax; movq %0, %%rcx; movq %1, %%rdx; int $0x24" : : "r"(id), "r"((uint64_t)workgroups) : "rax", "rcx", "rdx"); }
        inline CHAR16* GetVideoStatus() { uint64_t r; asm volatile ("movq $0x0F, %%rax; int $0x24; movq %%rax, %0" : "=r"(r) : : "rax"); return (CHAR16*)r; }
        
        inline uint64_t ParseBmp(const void* data, uint64_t size, uint32_t* out_w, uint32_t* out_h, uint32_t** out_buf) {
            uint64_t r; asm volatile ("movq $0x05, %%rax; movq %1, %%rcx; movq %2, %%rdx; movq %3, %%r8; movq %4, %%r9; movq %5, %%r10; int $0x24; movq %%rax, %0"
                : "=r"(r) : "r"(data), "r"(size), "r"(out_w), "r"(out_h), "r"(out_buf) : "rax", "rcx", "rdx", "r8", "r9", "r10");
            return r;
        }
        
        inline uint64_t DrawBmp(uint8_t* data, uint64_t size, int32_t x, int32_t y) {
            uint64_t r; asm volatile ("movq $0x11, %%rax; movq %1, %%rcx; movq %2, %%rdx; movq %3, %%r8; movq %4, %%r9; int $0x80; movq %%rax, %0"
                : "=r"(r) : "r"((uint64_t)data), "r"(size), "r"((uint64_t)x), "r"((uint64_t)y) : "rax", "rcx", "rdx", "r8", "r9");
            return r;
        }
    }

    // ==========================================
    // INT 0x25: MULTITASKING (Process Manager)
    // ==========================================
    namespace TaskManager {
        inline void Create(int32_t id, void (*entry)()) { asm volatile ("movq $0x01, %%rax; movq %0, %%rcx; movq %1, %%rdx; int $0x25" : : "r"((uint64_t)id), "r"(entry) : "rax", "rcx", "rdx"); }
        inline void CreateWithArg(int32_t id, void (*entry)(void*), void* arg) { asm volatile ("movq $0x02, %%rax; movq %0, %%rcx; movq %1, %%rdx; movq %2, %%r8; int $0x25" : : "r"((uint64_t)id), "r"(entry), "r"(arg) : "rax", "rcx", "rdx", "r8"); }
        inline void Yield() { asm volatile ("movq $0x03, %%rax; int $0x25" : : : "rax"); }
        inline void Exit() { asm volatile ("movq $0x04, %%rax; int $0x25" : : : "rax"); }
        inline uint64_t GetCurrent() { uint64_t r; asm volatile ("movq $0x05, %%rax; int $0x25; movq %%rax, %0" : "=r"(r) : : "rax"); return r; }
        inline void StartFirst() { asm volatile ("movq $0x06, %%rax; int $0x25" : : : "rax"); }
        inline void StopAndRun(int32_t id) { asm volatile ("movq $0x07, %%rax; movq %0, %%rcx; int $0x25" : : "r"((uint64_t)id) : "rax", "rcx"); }
        inline void ExitX(int32_t id) { asm volatile ("movq $0x08, %%rax; movq %0, %%rcx; int $0x25" : : "r"((uint64_t)id) : "rax", "rcx"); }
        inline uint64_t RunPex(const CHAR16* path, Process* p_info) { uint64_t r; asm volatile ("movq $0x09, %%rax; movq %1, %%rcx; movq %2, %%rdx; int $0x25; movq %%rax, %0" : "=r"(r) : "r"(path), "r"(p_info) : "rax", "rcx", "rdx"); return r; }
        inline Process* GetCurrentProcess() { uint64_t r; asm volatile ("movq $0x0A, %%rax; int $0x25; movq %%rax, %0" : "=r"(r) : : "rax"); return (Process*)r; }
        inline uint64_t ProcessExit(int32_t id) { uint64_t r; asm volatile ("movq $0x0B, %%rax; movq %1, %%rcx; int $0x25; movq %%rax, %0" : "=r"(r) : "r"((uint64_t)id) : "rax", "rcx"); return r; }
        inline uint64_t GetById(int32_t id) { uint64_t r; asm volatile ("movq $0x0C, %%rax; movq %1, %%rcx; int $0x25; movq %%rax, %0" : "=r"(r) : "r"((uint64_t)id) : "rax", "rcx"); return r; }
    }

    // ==========================================
    // INT 0x26: KERNEL SERVICES
    // ==========================================
    namespace Kernel {
        inline void RegisterHandler(uint8_t vector, void* handler) { asm volatile ("movq $0x01, %%rax; movq %0, %%rbx; movq %1, %%rcx; int $0x26" : : "r"((uint64_t)vector), "r"(handler) : "rax", "rbx", "rcx"); }
        inline uint64_t RegisterDriver(void* driver) { uint64_t r; asm volatile ("movq $0x02, %%rax; movq %1, %%rcx; int $0x26; movq %%rax, %0" : "=r"(r) : "r"(driver) : "rax", "rcx"); return r; }
        inline void GetHandles(void** img_handle, void** sys_table) { uint64_t i, s; asm volatile ("movq $0x03, %%rax; int $0x26; movq %%rcx, %0; movq %%rdx, %1" : "=r"(i), "=r"(s) : : "rax", "rcx", "rdx"); if (img_handle) *img_handle = (void*)i; if (sys_table) *sys_table = (void*)s; }
        inline void Reboot() { asm volatile ("movq $0x04, %%rax; int $0x26" : : : "rax"); }
        inline void Shutdown() { asm volatile ("movq $0x05, %%rax; int $0x26" : : : "rax"); }
        inline void InitDrivers() { asm volatile ("movq $0x06, %%rax; int $0x26" : : : "rax"); }
    }

    // ==========================================
    // INT 0x27: NETWORK
    // ==========================================
    namespace Network {
        inline uint64_t Init(const CHAR16* ip, const CHAR16* mask) { uint64_t r; asm volatile ("movq $0x01, %%rax; movq %1, %%rcx; movq %2, %%rdx; int $0x27; movq %%rax, %0" : "=r"(r) : "r"(ip), "r"(mask) : "rax", "rcx", "rdx"); return r; }
        inline uint64_t TcpConnect(const CHAR16* ip, uint16_t port) { uint64_t r; asm volatile ("movq $0x02, %%rax; movq %1, %%rcx; movq %2, %%rdx; int $0x27; movq %%rax, %0" : "=r"(r) : "r"(ip), "r"((uint64_t)port) : "rax", "rcx", "rdx"); return r; }
        inline uint64_t TcpSend(uint8_t* data, uint64_t size) { uint64_t r; asm volatile ("movq $0x03, %%rax; movq %1, %%rcx; movq %2, %%rdx; int $0x27; movq %%rax, %0" : "=r"(r) : "r"(data), "r"(size) : "rax", "rcx", "rdx"); return r; }
        inline uint64_t TcpReceive(uint8_t* buffer, uint64_t* size_out) { uint64_t r; asm volatile ("movq $0x04, %%rax; movq %1, %%rcx; movq %2, %%rdx; int $0x27; movq %%rax, %0" : "=r"(r) : "r"(buffer), "r"(size_out) : "rax", "rcx", "rdx"); return r; }
        inline uint64_t TcpDisconnect() { uint64_t r; asm volatile ("movq $0x05, %%rax; int $0x27; movq %%rax, %0" : "=r"(r) : : "rax"); return r; }
        inline uint64_t DnsLookup(const CHAR16* host, CHAR16* out_ip) { uint64_t r; asm volatile ("movq $0x06, %%rax; movq %1, %%rcx; movq %2, %%rdx; int $0x27; movq %%rax, %0" : "=r"(r) : "r"(host), "r"(out_ip) : "rax", "rcx", "rdx"); return r; }
    }

    // ==========================================
    // INT 0x28: AUDIO
    // ==========================================
    namespace Audio {
        inline void Beep(uint32_t freq, uint32_t ms) { asm volatile ("movq $0x01, %%rax; movq %0, %%rcx; movq %1, %%rdx; int $0x28" : : "r"((uint64_t)freq), "r"((uint64_t)ms) : "rax", "rcx", "rdx"); }
        inline uint64_t Play(uint8_t* data, uint64_t size) { uint64_t r; asm volatile ("movq $0x02, %%rax; movq %1, %%rcx; movq %2, %%rdx; int $0x28; movq %%rax, %0" : "=r"(r) : "r"(data), "r"(size) : "rax", "rcx", "rdx"); return r; }
    }

    // ==========================================
    // INT 0x29: MOUSE
    // ==========================================
    namespace Mouse {
        inline uint64_t Init() { uint64_t r; asm volatile ("movq $0x01, %%rax; int $0x29; movq %%rax, %0" : "=r"(r) : : "rax"); return r; }
        inline uint64_t GetState(int32_t& x, int32_t& y, uint8_t& b1, uint8_t& b2) { uint64_t r; asm volatile ("movq $0x02, %%rax; movq %1, %%rcx; movq %2, %%rdx; movq %3, %%r8; movq %4, %%r9; int $0x29; movq %%rax, %0" : "=r"(r) : "r"(&x), "r"(&y), "r"(&b1), "r"(&b2) : "rax", "rcx", "rdx", "r8", "r9"); return r; }
    }

    // ==========================================
    // INT 0x2A: MEMORY ALLOCATION
    // ==========================================
    namespace Memory {
        inline void* Alloc(uint64_t size) { uint64_t r; asm volatile ("movq $0x01, %%rax; movq %1, %%rcx; int $0x2A; movq %%rax, %0" : "=r"(r) : "r"(size) : "rax", "rcx"); return (void*)r; }
        inline void Free(void* ptr) { asm volatile ("movq $0x02, %%rax; movq %0, %%rcx; int $0x2A" : : "r"(ptr) : "rax", "rcx"); }

        struct MemoryStats {
            uint64_t TotalBytes;
            uint64_t FreeBytes;
            uint64_t UsedBytes;
        };

        // Общая статистика по RAM всей ОС (сумма по карте памяти UEFI).
        // Возвращает false, если ядро не смогло получить карту памяти.
        inline bool GetStats(MemoryStats& out) {
            uint64_t r;
            asm volatile (
                "movq $0x03, %%rax\n\t"
                "movq %1, %%rcx\n\t"
                "int $0x2A\n\t"
                "movq %%rax, %0"
                : "=r"(r) : "r"(&out) : "rax", "rcx", "memory"
            );
            return r != 0;
        }
    }
}


#endif // PARROT_API_HPP