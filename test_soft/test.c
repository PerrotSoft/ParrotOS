typedef unsigned short CHAR16;

void _start() {
    // 1. Вывод строки через твое прерывание Int 21h, AX=0x02
    const CHAR16* msg = L"PSA Test: Raw binary execution success!\r\n";
    asm volatile (
        "movq $0x02, %%rax \n"
        "movq %0, %%rcx \n"
        "int $0x21 \n"
        : : "r"(msg) : "rax", "rcx"
    );

    // 2. Завершение задачи через Int 25h, AX=0x03
    asm volatile (
        "movq $0x03, %%rax \n"
        "int $0x25 \n"
        : : : "rax"
    );
}