#include "ParrotOS_API.h"

#define BG_COLOR      0x1A1A1B
#define TEXT_COLOR    0xFFFFFF
#define ACCENT_COLOR  0x00FF7F

// Глобальные переменные для тестов
int32_t sw, sh;
volatile uint32_t completed_tasks = 0;
uint64_t start_time;
uint64_t end_time;

// Чтение тиков процессора
static inline uint64_t rdtsc(void) {
    uint32_t lo, hi;
    __asm__ __volatile__ ("rdtsc" : "=a" (lo), "=d" (hi));
    return ((uint64_t)hi << 32) | lo;
}

// Реализация перевода числа в строку (чтобы не было ошибок линковки)
void int_to_str(int n, CHAR16* str) {
    int i = 0;
    if (n == 0) { str[i++] = L'0'; str[i] = L'\0'; return; }
    
    uint32_t temp;
    if (n < 0) {
        str[i++] = L'-';
        temp = (uint32_t)(-n);
    } else {
        temp = (uint32_t)n;
    }

    CHAR16 reverse[16];
    int j = 0;
    while (temp > 0) {
        reverse[j++] = (temp % 10) + L'0';
        temp /= 10;
    }

    while (j > 0) {
        str[i++] = reverse[--j];
    }
    str[i] = L'\0';
}

// Вычисление Pi (метод Лейбница, 100к итераций для нагрузки)
void calculate_pi_work() {
    double pi = 0;
    for (int i = 0; i < 100000; i++) {
        pi += 1.0 / (i * 2 + 1) * (i % 2 == 0 ? 1 : -1);
    }
}

// Обертка для многопоточного теста
void task_pi_worker() {
    calculate_pi_work();
    __sync_fetch_and_add(&completed_tasks, 1);
    TaskExit();
}

void print_result(int x, int y, uint64_t ticks) {
    CHAR16 buf[32];
    int_to_str((int)ticks, buf);
    GfxPrint(x, y, 14, ACCENT_COLOR, buf);
}

void main() {
    GfxGetScreenSize(&sw, &sh);
    GfxLoadFont(L"Ubuntu.ttf", L"Ubuntu");
    GfxSetFont(L"Ubuntu");
    GfxClear(BG_COLOR);

    GfxPrint(20, 20, 16, TEXT_COLOR, L"Pi Benchmark | 100,000 iterations");

    // --- ТЕСТ 1: ОДНОПОТОК ---
    GfxPrint(20, 60, 14, TEXT_COLOR, L"1. Running Single-thread...");
    SB();
    
    start_time = rdtsc();
    calculate_pi_work();
    end_time = rdtsc();
    
    uint64_t single_ticks = end_time - start_time;
    GfxPrint(20, 80, 14, TEXT_COLOR, L"Single-thread Ticks: ");
    print_result(220, 80, single_ticks);

    // --- ТЕСТ 2: МНОГОПОТОК (15 ЗАДАЧ) ---
    completed_tasks = 0;
    GfxPrint(20, 120, 14, TEXT_COLOR, L"2. Running Multi-thread (15 tasks)...");
    SB();

    start_time = rdtsc();
    for(int i = 0; i < 15; i++) {
        TaskCreate(10, task_pi_worker);
    }

    // Ожидание завершения всех 15 задач
    while(completed_tasks < 15) {
        TaskYield();
    }
    end_time = rdtsc();

    uint64_t multi_ticks = end_time - start_time;
    GfxPrint(20, 140, 14, TEXT_COLOR, L"Multi-thread Ticks: ");
    print_result(220, 140, multi_ticks);

    GfxPrint(20, 180, 14, TEXT_COLOR, L"Done. Press ESC to exit.");
    SB();

    while (1) {
        if (KbdHasKey()) {
            if (KbdGetKey() == 27) break;
        }
        TaskYield();
    }
}