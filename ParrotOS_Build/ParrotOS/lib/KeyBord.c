#include <Uefi.h>
#include "../include/drivers/Keybord.h"

EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL*
GetKeyboard(EFI_SYSTEM_TABLE* SystemTable)
{
    EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL* Keyboard = NULL;

    SystemTable->BootServices->LocateProtocol(
        &gEfiSimpleTextInputExProtocolGuid,
        NULL,
        (VOID**)&Keyboard
    );

    return Keyboard;
}
BOOLEAN Keyboard_HasKey(EFI_SYSTEM_TABLE* SystemTable)
{
    EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL* Keyboard = GetKeyboard(SystemTable);
    if (!Keyboard)
        return FALSE;

    EFI_KEY_DATA KeyData;
    EFI_STATUS Status = Keyboard->ReadKeyStrokeEx(Keyboard, &KeyData);

    return !EFI_ERROR(Status);
}
CHAR16 Keyboard_GetKey(EFI_SYSTEM_TABLE* SystemTable)
{
    EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL* Keyboard = GetKeyboard(SystemTable);
    if (!Keyboard)
        return 0;

    EFI_KEY_DATA KeyData;
    UINTN Index;
    SystemTable->BootServices->WaitForEvent(
        1,
        &Keyboard->WaitForKeyEx,
        &Index
    );

    if (!EFI_ERROR(Keyboard->ReadKeyStrokeEx(Keyboard, &KeyData)))
        return KeyData.Key.UnicodeChar;

    return 0;
}
VOID Keyboard_Reset(EFI_SYSTEM_TABLE* SystemTable)
{
    EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL* Keyboard = GetKeyboard(SystemTable);
    if (!Keyboard)
        return;

    Keyboard->Reset(Keyboard, FALSE);
}
VOID Keyboard_INIT(VOID)
{
    RegisterDriver(&(DRIVER){
        .Type = DRIVER_TYPE_KEYBOARD,
        .Priority = 1,
        .Interface = &(KEY_DRIVER_IF){
            .GetKey = Keyboard_GetKey,
            .HasKey = Keyboard_HasKey,
            .Reset = Keyboard_Reset
        }
    });
}