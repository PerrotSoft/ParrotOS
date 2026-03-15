#ifndef PROTOCOLS_H
#define PROTOCOLS_H

#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>

struct Protocol_Col {
    UINT32 ID;              // Программный ID
    UINT8 (*fds)();         // Возвращает 123
    void  (*test)();        // Выводит "test"
    void  (*test1)(CHAR16* g); // Выводит переданную строку
};

// --- Реализация функций для структуры ---

UINT8 Internal_fds() {
    return 123;
}

void Internal_test() {
    Print(L"gfd");
}

void Internal_test1(CHAR16* g) {
    // Временно оставляем пустой.
    if (g != NULL) {
        Print(g);
    }
}

UINT8 Kernel_GetProtocol(INT32 id, UINT32 protocol_id, VOID** out_protocol) {
    if (out_protocol == NULL) {
        return 1; // Ошибка: некуда записывать
    }

    // Выделяем память под структуру протокола
    struct Protocol_Col* new_proto = AllocateZeroPool(sizeof(struct Protocol_Col));
    if (new_proto == NULL) {
        return 2; // Ошибка: нет памяти
    }

    // Заполняем данными
    new_proto->ID = (UINT32)id; 
    new_proto->fds = Internal_fds;
    new_proto->test = Internal_test;
    new_proto->test1 = Internal_test1;

    // Возвращаем результат
    *out_protocol = (VOID*)new_proto;

    return 0; // Успех
}

#endif