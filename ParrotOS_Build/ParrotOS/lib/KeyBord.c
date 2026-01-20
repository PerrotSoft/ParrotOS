#include <Uefi.h>
#include "../include/drivers/Keybord.h"

static KEY_DRIVER_IF KeyboardInterface = {
    .HasKey = Keyboard_HasKey,
    .GetKey = Keyboard_GetKey
};
static DRIVER KeyboardDriver = {
    .Type = DRIVER_TYPE_KEYBOARD,
    .Priority = 3,
    .Interface = &KeyboardInterface
};
EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL* GetKeyboard(EFI_SYSTEM_TABLE* SystemTable) {
    static EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL* Keyboard = NULL;
    if (Keyboard) return Keyboard;
    SystemTable->BootServices->LocateProtocol(&gEfiSimpleTextInputExProtocolGuid, NULL, (VOID**)&Keyboard);
    return Keyboard;
}

BOOLEAN Keyboard_HasKey(EFI_SYSTEM_TABLE* SystemTable) {
    EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL* Keyboard = GetKeyboard(SystemTable);
    if (!Keyboard) return FALSE;
    
    EFI_KEY_DATA KeyData;
    EFI_STATUS Status = Keyboard->ReadKeyStrokeEx(Keyboard, &KeyData);
    return (Status == EFI_SUCCESS);
}

CHAR16 Keyboard_GetKey(EFI_SYSTEM_TABLE* SystemTable) {
    EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL* Keyboard = GetKeyboard(SystemTable);
    if (!Keyboard) return 0;

    EFI_KEY_DATA KeyData;
    UINTN Index;

    SystemTable->BootServices->WaitForEvent(1, &Keyboard->WaitForKeyEx, &Index);
    
    if (Keyboard->ReadKeyStrokeEx(Keyboard, &KeyData) == EFI_SUCCESS) {
        return KeyData.Key.UnicodeChar;
    }
    return 0;
}

VOID Keyboard_Reset(EFI_SYSTEM_TABLE* SystemTable) {
    EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL* Keyboard = GetKeyboard(SystemTable);
    if (Keyboard) Keyboard->Reset(Keyboard, FALSE);
}

VOID Keyboard_INIT(VOID) {
    RegisterDriver(&KeyboardDriver);
}