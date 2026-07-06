#include <stdint.h>
#include <stddef.h>

#define PDL_MAGIC       0x4C44505F // '_PDL'
#define PDL_ARCH_X86_64 0x8664     // Архитектура x86_64

typedef uint16_t CHAR16;

typedef struct {
    uint32_t magic;
    uint16_t version;
    uint16_t architecture;
    uint32_t export_count;
    uint32_t export_table_offset;
} PDL_Header;

typedef struct {
    char     name[32];
    uint64_t offset;
} PDL_ExportEntry;

// Прототипы функций
void LibDrawBox(void);
void LibWriteValue(CHAR16* filename, uint16_t val);
uint16_t LibReadValue(CHAR16* filename);

// 1. Заголовок библиотеки (секция .pdl_header гарантирует, что он будет в самом начале файла)
// aligned(1) ОБЯЗАТЕЛЕН: без него линковщик может вставить паддинг между
// .pdl_header и .pdl_exports (например, выровняв массив exports[] на 32 байта),
// из-за чего export_table_offset перестанет соответствовать реальному
// расположению данных в файле.
__attribute__((section(".pdl_header"), aligned(1)))
const PDL_Header header = {
    .magic               = PDL_MAGIC,
    .version             = 1,
    .architecture        = PDL_ARCH_X86_64,
    .export_count        = 3,
    .export_table_offset = sizeof(PDL_Header)
};

// 2. Таблица экспорта (идет сразу после заголовка, без паддинга)
__attribute__((section(".pdl_exports"), aligned(1)))
const PDL_ExportEntry exports[3] = {
    { .name = "LibDrawBox",    .offset = (uint64_t)&LibDrawBox },
    { .name = "LibWriteValue", .offset = (uint64_t)&LibWriteValue },
    { .name = "LibReadValue",  .offset = (uint64_t)&LibReadValue }
};

// 3. Реализация функций (компилируются с -fPIC в относительные адреса)

// Рисует белый пиксель/блок в координатах (10, 10) через INT 0x24
void LibDrawBox(void) {
    asm volatile (
        "movq $0x02, %%rax\n\t"    // Запрос PUT_PIXEL
        "movq $10, %%rcx\n\t"      // X = 10
        "movq $10, %%rdx\n\t"      // Y = 10
        "movq $0xFFFFFF, %%r8\n\t" // Цвет = Белый
        "int $0x24"
        : : : "rax", "rcx", "rdx", "r8"
    );
}

// Создает файл и записывает в него 2 байта через INT 0x23
void LibWriteValue(CHAR16* filename, uint16_t val) {
    // КРИТИЧЕСКИЙ ФИКС: без этой volatile-копии компилятор может НЕ записать
    // реальное значение val в память по адресу &val перед тем, как мы отдадим
    // этот адрес ядру через int 0x23. С точки зрения GCC, "r"(&val) — это
    // просто число (адрес) в регистре; компилятор не видит, что данные по
    // этому адресу будут прочитаны снаружи (в обработчике прерывания), и
    // поэтому не обязан физически сохранять val в память — он может держать
    // val только в регистре. В итоге по &val могли лежать нули/мусор вместо 42,
    // что и объясняет запись нулевого значения в файл.
    volatile uint16_t val_storage = val;

    asm volatile ("movq $0x04, %%rax; movq %0, %%rcx; int $0x23" : : "r"(filename) : "rax", "rcx", "memory");
    asm volatile (
        "movq $0x03, %%rax\n\t"
        "movq %0, %%rcx\n\t"
        "movq %1, %%rdx\n\t"
        "movq $2, %%r8\n\t"
        "int $0x23"
        : : "r"(filename), "r"(&val_storage) : "rax", "rcx", "rdx", "r8", "memory"
    );
}

// Читает 2 байта из файла через INT 0x23
uint16_t LibReadValue(CHAR16* filename) {
    uint64_t status, msg_ptr, size;
    asm volatile (
        "movq $0x11, %%rax\n\t"
        "movq %3, %%rcx\n\t"
        "int $0x23\n\t"
        "movq %%rax, %0\n\t"
        "movq %%rdx, %1\n\t"
        "movq %%r8, %2"
        : "=r"(status), "=r"(msg_ptr), "=r"(size)
        : "r"(filename)
        : "rax", "rcx", "rdx", "r8"
    );

    if (status == 0 && msg_ptr != 0 && size >= 2) {
        return *((uint16_t*)msg_ptr);
    }
    return 0;
}