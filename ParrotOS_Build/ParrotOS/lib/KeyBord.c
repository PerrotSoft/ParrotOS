#include <Uefi.h>
#include "../include/drivers/Keybord.h"
#include "../include/drivers/DriverManager.h"

#define BUFFER_SIZE 256

static CHAR16 KeyBuffer[BUFFER_SIZE];
static UINTN BufferHead = 0;
static UINTN BufferTail = 0;

static KEY_DRIVER_IF KeyboardInterface;

static DRIVER KeyboardDriver = {
    .Type = DRIVER_TYPE_KEYBOARD,
    .Priority = 10,
    .Interface = &KeyboardInterface
};

EFI_SIMPLE_TEXT_INPUT_PROTOCOL* GetKeyboard(EFI_SYSTEM_TABLE* SystemTable) {
    if (!SystemTable || !SystemTable->ConIn) return NULL;
    return SystemTable->ConIn;
}

/*
 * Безопасное вычитывание аппаратного буфера UEFI в наш кольцевой буфер.
 * Работает БЕЗ CheckEvent(WaitForKey), что гарантирует отсутствие зависаний
 * на реальном железе и в QEMU.
 */
static VOID FetchKeyToBuffer(EFI_SYSTEM_TABLE* SystemTable) {
    EFI_SIMPLE_TEXT_INPUT_PROTOCOL* Keyboard = GetKeyboard(SystemTable);
    if (!Keyboard) return;

    EFI_INPUT_KEY Key;
    UINTN Iterations = 0;

    // Вычитываем нажатия напрямую из прошивки.
    // Лимит Iterations < BUFFER_SIZE защищает от бесконечного цикла,
    // если BIOS забагует и начнет отдавать мусор непрерывно.
    while (Iterations < BUFFER_SIZE && Keyboard->ReadKeyStroke(Keyboard, &Key) == EFI_SUCCESS) {
        Iterations++;

        CHAR16 FinalCode;
        if (Key.UnicodeChar != 0) {
            FinalCode = Key.UnicodeChar;
        } else if (Key.ScanCode != 0) {
            // Кодируем спецклавиши (Стрелки, F1-F12, Home, End) в диапазон 0xFFxx
            FinalCode = (CHAR16)(Key.ScanCode + 0xFF00);
        } else {
            continue; // Игнорируем пустые/нулевые нажатия
        }

        UINTN NextHead = (BufferHead + 1) % BUFFER_SIZE;

        // Если буфер переполнен, сдвигаем хвост, затирая самое старое нажатие,
        // чтобы система ввода никогда не блокировалась
        if (NextHead == BufferTail) {
            BufferTail = (BufferTail + 1) % BUFFER_SIZE;
        }

        KeyBuffer[BufferHead] = FinalCode;
        BufferHead = NextHead;
    }
}

BOOLEAN Keyboard_HasKey(EFI_SYSTEM_TABLE* SystemTable) {
    FetchKeyToBuffer(SystemTable);
    return (BufferHead != BufferTail);
}

CHAR16 Keyboard_GetKeyNonBlocking(EFI_SYSTEM_TABLE* SystemTable) {
    FetchKeyToBuffer(SystemTable);
    
    if (BufferHead == BufferTail) {
        return 0; 
    }

    CHAR16 Key = KeyBuffer[BufferTail];
    BufferTail = (BufferTail + 1) % BUFFER_SIZE;
    return Key;
}

CHAR16 Keyboard_GetKey(EFI_SYSTEM_TABLE* SystemTable) {
    // Блокирующий режим: ждём, пока в кольцевом буфере не появится клавиша
    while (!Keyboard_HasKey(SystemTable)) {
        SystemTable->BootServices->Stall(1000); // 1 мс пауза, чтобы не жарить процессор
    }

    CHAR16 Key = KeyBuffer[BufferTail];
    BufferTail = (BufferTail + 1) % BUFFER_SIZE;
    return Key;
}

UINTN Keyboard_GetBuffer(EFI_SYSTEM_TABLE* SystemTable, CHAR16* UserBuffer, UINTN MaxLength) {
    if (!UserBuffer || MaxLength == 0) return 0;

    FetchKeyToBuffer(SystemTable);

    UINTN Count = 0;
    while (BufferTail != BufferHead && Count < MaxLength) {
        UserBuffer[Count] = KeyBuffer[BufferTail];
        BufferTail = (BufferTail + 1) % BUFFER_SIZE;
        Count++;
    }
    
    return Count;
}

UINTN Keyboard_GetBufferSize(EFI_SYSTEM_TABLE* SystemTable) {
    FetchKeyToBuffer(SystemTable);
    return (BufferHead >= BufferTail)
           ? (BufferHead - BufferTail)
           : (BUFFER_SIZE - BufferTail + BufferHead);
}

VOID Keyboard_FlushBuffer(EFI_SYSTEM_TABLE* SystemTable) {
    EFI_SIMPLE_TEXT_INPUT_PROTOCOL* Keyboard = GetKeyboard(SystemTable);
    EFI_INPUT_KEY Key;
    
    // Аппаратная сброска остатков из BIOS (максимум 64 итерации от зависания)
    if (Keyboard) {
        UINTN Limit = 0;
        while (Limit < 64 && Keyboard->ReadKeyStroke(Keyboard, &Key) == EFI_SUCCESS) {
            Limit++;
        }
    }
    
    // Очищаем наш программный кольцевой буфер
    BufferHead = 0;
    BufferTail = 0;
}

VOID Keyboard_Reset(EFI_SYSTEM_TABLE* SystemTable) {
    EFI_SIMPLE_TEXT_INPUT_PROTOCOL* Keyboard = GetKeyboard(SystemTable);
    if (Keyboard) {
        Keyboard->Reset(Keyboard, TRUE);
    }
    BufferHead = 0;
    BufferTail = 0;
}

static KEY_DRIVER_IF KeyboardInterface = {
    .HasKey             = Keyboard_HasKey,
    .GetKey             = Keyboard_GetKey,
    .Reset              = Keyboard_Reset,
    .GetKeyNonBlocking  = Keyboard_GetKeyNonBlocking,
    .GetBuffer          = Keyboard_GetBuffer,
    .GetBufferSize      = Keyboard_GetBufferSize,
    .FlushBuffer        = Keyboard_FlushBuffer
};

EFI_STATUS Keyboard_INIT(EFI_SYSTEM_TABLE* SystemTable) {
    EFI_SIMPLE_TEXT_INPUT_PROTOCOL* Keyboard = GetKeyboard(SystemTable);
    if (!Keyboard) {
        return EFI_DEVICE_ERROR;
    }

    Keyboard_Reset(SystemTable);

    // Регистрируем наш драйвер в общем менеджере драйверов ParrotOS
    if (!RegisterDriver(&KeyboardDriver)) {
        return EFI_OUT_OF_RESOURCES;
    }

    return EFI_SUCCESS;
}