#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/UefiApplicationEntryPoint.h>
#include "include/drivers/DriverManager.h"
#include "include/drivers/Keybord.h"
#include "include/drivers/fat32.h"
#include "include/drivers/Video_Driver.h"
#include "include/bmp.h"

// Ссылаемся на внешние переменные из драйверов
extern VideoMode vmode; 
extern CHAR16 FAT32_DiskLetters[MAX_DISKS];
extern UINTN  FAT32_DiskCount;

EFI_STATUS draw_logo_from_disk(CHAR16 DiskLetter) {
    // Получаем актуальные размеры экрана
    INT32 bmp_x = (INT32)(vmode.width / 2 - 50);
    INT32 bmp_y = (INT32)(vmode.height / 2 - 50);
    SetCurrentDisk(DiskLetter);

    EC16 file;
    EFI_STATUS fileStatus = ReadFileByPath(L"\\ico_100x100.bmp", &file);
    
    if (EFI_ERROR(fileStatus)) {
        Print(L"\n[Error] File \\ico_100x100.bmp not found on disk %c: (%r)\n", DiskLetter, fileStatus);
        return fileStatus;
    }

    CLEAR_SCREEN(0x000000);
    return draw_bmp_from_memory_safe((UINT8*)file.Message, file.FileSize, bmp_x, bmp_y);
}

EFI_STATUS EFIAPI UefiMain (IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE *SystemTable)
{
    INIT(SystemTable);
    init_vd();
    INIT_VIDEO_DRIVER(SystemTable);
    Keyboard_INIT();
    Fat32_Storage_INIT();
    Fat32_RegisterrsDisk(); 
    vmode = *GET_CURRENT_VMODE();

    CLEAR_SCREEN(0x000000);
    Print(L"--- ParrotOS Disk Selector ---\n\n");

    if (FAT32_DiskCount == 0) {
        Print(L"Error: No FAT32 disks found! Halt.\n");
        while(1);
    }
    draw_logo_from_disk('A');
    
    while(1) {
    }

    return EFI_SUCCESS;
}