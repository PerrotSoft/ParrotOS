#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/UefiApplicationEntryPoint.h>
#include "include/drivers/DriverManager.h"
#include "include/drivers/Keybord.h"
#include "include/drivers/fat32.h"

EFI_STATUS
EFIAPI
UefiMain (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  INIT(SystemTable);
  Keyboard_INIT();
  
  Fat32_Storage_INIT();
  RegisterrsDisk();
  Print(L"%c", GetKey());
  WriteFile(L"test.txt", (UINT16*)L"Hello from ParrotOS!", 21);
  Print(ReadFile(L"test.txt").Message);
  Print(L"Test ParrotOS\nHello, world\n");
  while(1){}
  return EFI_SUCCESS;
}