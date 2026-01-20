#ifndef PARROT_API_H
#define PARROT_API_H

#include <stdint.h>

typedef uint16_t CHAR16;

// --- ТОЧКА ВХОДА (CRT0) ---
extern void main();

__attribute__((section(".text.boot")))
void _start() {
    main();
    // Int 25h, AX=0x03 (Завершение задачи после выхода из main)
    asm volatile ("movq $0x03, %%rax; int $0x25" : : : "rax");
}

// --- INT 21h: CONSOLE IO ---
void p_print_char(const CHAR16 c) {
    asm volatile ("movq $0x01, %%rax; movq %0, %%rcx; int $0x21" : : "r"((uint64_t)c) : "rax", "rcx");
}

void p_print(const CHAR16* msg) {
    asm volatile ("movq $0x02, %%rax; movq %0, %%rcx; int $0x21" : : "r"(msg) : "rax", "rcx");
}

void p_set_color(uint64_t color) {
    asm volatile ("movq $0x04, %%rax; movq %0, %%rcx; int $0x21" : : "r"(color) : "rax", "rcx");
}

void p_clear_cmd() {
    asm volatile ("movq $0x05, %%rax; int $0x21" : : : "rax");
}

void p_set_cursor(uint64_t x, uint64_t y) {
    asm volatile ("movq $0x06, %%rax; movq %0, %%rcx; movq %1, %%rdx; int $0x21" : : "r"(x), "r"(y) : "rax", "rcx", "rdx");
}

// --- INT 22h: KEYBOARD ---
CHAR16 p_get_key() {
    uint64_t res;
    asm volatile ("movq $0x01, %%rax; int $0x22; movq %%rax, %0" : "=r"(res) : : "rax");
    return (CHAR16)res;
}

uint64_t p_has_key() {
    uint64_t res;
    asm volatile ("movq $0x02, %%rax; int $0x22; movq %%rax, %0" : "=r"(res) : : "rax");
    return res;
}

void p_kbd_reset() {
    asm volatile ("movq $0x03, %%rax; int $0x22" : : : "rax");
}

// --- INT 23h: STORAGE ---
uint64_t p_read_file(const CHAR16* path, void* buffer) {
    uint64_t res;
    asm volatile ("movq $0x01, %%rax; movq %1, %%rcx; movq %2, %%rdx; int $0x23; movq %%rax, %0" 
        : "=r"(res) : "r"(path), "r"(buffer) : "rax", "rcx", "rdx");
    return res;
}

uint64_t p_set_disk(CHAR16 letter) {
    uint64_t res;
    asm volatile ("movq $0x02, %%rax; movq %1, %%rcx; int $0x23; movq %%rax, %0" 
        : "=r"(res) : "r"((uint64_t)letter) : "rax", "rcx");
    return res;
}

uint64_t p_write_file(const CHAR16* path, void* data, uint64_t size) {
    uint64_t res;
    asm volatile ("movq $0x03, %%rax; movq %1, %%rcx; movq %2, %%rdx; movq %3, %%r8; int $0x23; movq %%rax, %0" 
        : "=r"(res) : "r"(path), "r"(data), "r"(size) : "rax", "rcx", "rdx", "r8");
    return res;
}

uint64_t p_create_file(const CHAR16* path) {
    uint64_t res;
    asm volatile ("movq $0x04, %%rax; movq %1, %%rcx; int $0x23; movq %%rax, %0" 
        : "=r"(res) : "r"(path) : "rax", "rcx");
    return res;
}

uint64_t p_delete_file(const CHAR16* path) {
    uint64_t res;
    asm volatile ("movq $0x05, %%rax; movq %1, %%rcx; int $0x23; movq %%rax, %0" 
        : "=r"(res) : "r"(path) : "rax", "rcx");
    return res;
}

uint64_t p_get_file_size(const CHAR16* path, uint64_t* size_ptr) {
    uint64_t res;
    asm volatile ("movq $0x06, %%rax; movq %1, %%rcx; movq %2, %%rdx; int $0x23; movq %%rax, %0" 
        : "=r"(res) : "r"(path), "r"(size_ptr) : "rax", "rcx", "rdx");
    return res;
}

uint64_t p_cd(const CHAR16* path) {
    uint64_t res;
    asm volatile ("movq $0x07, %%rax; movq %1, %%rcx; int $0x23; movq %%rax, %0" 
        : "=r"(res) : "r"(path) : "rax", "rcx");
    return res;
}

// --- INT 24h: GRAPHICS ---
void p_graphics_clear(uint32_t color) {
    asm volatile ("movq $0x01, %%rax; movq %0, %%rcx; int $0x24" : : "r"((uint64_t)color) : "rax", "rcx");
}

void p_put_pixel(int32_t x, int32_t y, uint32_t color) {
    asm volatile ("movq $0x02, %%rax; movq %0, %%rcx; movq %1, %%rdx; movq %2, %%r8; int $0x24" 
        : : "r"((uint64_t)x), "r"((uint64_t)y), "r"((uint64_t)color) : "rax", "rcx", "rdx", "r8");
}

void p_draw_line(int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint32_t color) {
    asm volatile ("movq $0x03, %%rax; movq %0, %%rcx; movq %1, %%rdx; movq %2, %%r8; movq %3, %%r9; movq %4, %%r10; int $0x24" 
        : : "r"((uint64_t)x1), "r"((uint64_t)y1), "r"((uint64_t)x2), "r"((uint64_t)y2), "r"((uint64_t)color) 
        : "rax", "rcx", "rdx", "r8", "r9", "r10");
}

void p_draw_bitmap(uint32_t* data, int32_t x, int32_t y, int32_t w, int32_t h) {
    asm volatile ("movq $0x04, %%rax; movq %0, %%rcx; movq %1, %%rdx; movq %2, %%r8; movq %3, %%r9; movq %4, %%r10; int $0x24" 
        : : "r"(data), "r"((uint64_t)x), "r"((uint64_t)y), "r"((uint64_t)w), "r"((uint64_t)h) 
        : "rax", "rcx", "rdx", "r8", "r9", "r10");
}

// --- INT 25h: MULTITASKING & SYSTEM ---
void p_task_create(int32_t priority, void (*entry)()) {
    asm volatile ("movq $0x01, %%rax; movq %0, %%rcx; movq %1, %%rdx; int $0x25" : : "r"((uint64_t)priority), "r"(entry) : "rax", "rcx", "rdx");
}

void p_task_yield() {
    asm volatile ("movq $0x02, %%rax; int $0x25" : : : "rax");
}

void p_task_exit() {
    asm volatile ("movq $0x03, %%rax; int $0x25" : : : "rax");
}

uint64_t p_get_current_task() {
    uint64_t res;
    asm volatile ("movq $0x04, %%rax; int $0x25; movq %%rax, %0" : "=r"(res) : : "rax");
    return res;
}

void p_task_start_first() {
    asm volatile ("movq $0x05, %%rax; int $0x25" : : : "rax");
}

uint64_t p_exec(const CHAR16* path) {
    uint64_t res;
    asm volatile ("movq $0x06, %%rax; movq %1, %%rcx; int $0x25; movq %%rax, %0" : "=r"(res) : "r"(path) : "rax", "rcx");
    return res;
}
void p_print_gfx(const CHAR16* text, int32_t x, int32_t y, uint32_t color) {
    asm volatile (
        "movq $0x08, %%rax; "    // Номер функции: draw_string
        "movq %0, %%rcx; "       // Шрифт (L"SysFont")
        "movq %1, %%rdx; "       // X
        "movq %2, %%r8; "        // Y
        "movq $14, %%r9; "       // Размер (Size)
        "movq %3, %%r10; "       // Цвет
        "movq %4, %%r11; "       // Текст
        "int $0x24"
        : 
        : "r"((uint64_t)L"SysFont"), "r"((uint64_t)x), "r"((uint64_t)y), "r"((uint64_t)color), "r"((uint64_t)text)
        : "rax", "rcx", "rdx", "r8", "r9", "r10", "r11"
    );
}
#endif