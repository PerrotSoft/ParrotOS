#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/UefiApplicationEntryPoint.h>
#include "include/drivers/DriverManager.h"
#include "include/drivers/Keybord.h"
#include "include/drivers/fat32.h"
#include "include/drivers/Video_Driver.h"

EFI_STATUS
EFIAPI
UefiMain (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  INIT(SystemTable);         // Инициализация менеджера
  init_vd();                // Инициализация видео драйвера
  INIT_VIDEO_DRIVER(SystemTable);       // ИСПРАВЛЕНО: Вместо init_vd()
  Keyboard_INIT();           // Инициализация клавиатуры
  Fat32_Storage_INIT();      // Инициализация ФС
  CLEAR_SCREEN(0x000000);   // Очистка экрана черным цветом
  
  Print(L"ParrotOS Loaded Successfully!\n");
  
  while(1){
    // Ваш основной цикл
  }
  return EFI_SUCCESS;
}