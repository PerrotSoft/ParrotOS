#include "../include/drivers/DriverManager.h"

#define MAX_DRIVERS 32

static DRIVER Drivers[MAX_DRIVERS];
static UINTN  DriversCount = 0;
static EFI_SYSTEM_TABLE* SystemTables;

BOOLEAN RegisterDriver(DRIVER* Driver)
{
    if (DriversCount >= MAX_DRIVERS || Driver == NULL)
        return FALSE;

    Drivers[DriversCount++] = *Driver;
    GetBestDriver(Driver->Type);
    return TRUE;
}

DRIVER* GetBestDriver(DRIVER_TYPE Type)
{
    DRIVER* Best = NULL;

    for (UINTN i = 0; i < DriversCount; i++) {
        if (Drivers[i].Type != Type)
            continue;

        if (Best == NULL || Drivers[i].Priority > Best->Priority)
            Best = &Drivers[i];
    }

    return Best;
}

VOID INIT(EFI_SYSTEM_TABLE *SytemTables)
{
    SystemTables = SytemTables;
}

CHAR16 GetKey(VOID)
{
    DRIVER* drv = GetBestDriver(DRIVER_TYPE_KEYBOARD);
    if (!drv || !drv->Interface)
        return 0;

    KEY_DRIVER_IF* key = (KEY_DRIVER_IF*)drv->Interface;
    return key->GetKey(SystemTables);
}

BOOLEAN HasKey(VOID)
{
    DRIVER* drv = GetBestDriver(DRIVER_TYPE_KEYBOARD);
    if (!drv || !drv->Interface)
        return FALSE;

    KEY_DRIVER_IF* key = (KEY_DRIVER_IF*)drv->Interface;
    return key->HasKey(SystemTables);
}

VOID Reset(VOID)
{
    DRIVER* drv = GetBestDriver(DRIVER_TYPE_KEYBOARD);
    if (!drv || !drv->Interface)
        return;

    KEY_DRIVER_IF* key = (KEY_DRIVER_IF*)drv->Interface;
    key->Reset(SystemTables);
}



EFI_STATUS ReadFileByPath(CHAR16 *path_in, EC16 *out)
{
    DRIVER* drv = GetBestDriver(DRIVER_TYPE_STORAGE);
    if (!drv || !drv->Interface)
        return EFI_NOT_FOUND;

    STORAGE_DRIVER_IF* storage = (STORAGE_DRIVER_IF*)drv->Interface;
    return storage->ReadFileByPath(path_in, out);
}
EFI_STATUS SetCurrentDisk(CHAR16 Letter)
{
    DRIVER* drv = GetBestDriver(DRIVER_TYPE_STORAGE);
    if (!drv || !drv->Interface)
        return EFI_NOT_FOUND;

    STORAGE_DRIVER_IF* storage = (STORAGE_DRIVER_IF*)drv->Interface;
    return storage->SetCurrentDisk(Letter);
}
const CHAR16* GetCurrentPath(VOID)
{
    DRIVER* drv = GetBestDriver(DRIVER_TYPE_STORAGE);
    if (!drv || !drv->Interface)
        return NULL;

    STORAGE_DRIVER_IF* storage = (STORAGE_DRIVER_IF*)drv->Interface;
    return storage->GetCurrentPath();
}
EFI_STATUS PathUp(VOID)
{
    DRIVER* drv = GetBestDriver(DRIVER_TYPE_STORAGE);
    if (!drv || !drv->Interface)
        return EFI_NOT_FOUND;

    STORAGE_DRIVER_IF* storage = (STORAGE_DRIVER_IF*)drv->Interface;
    return storage->PathUp();
}
EC16 ListDir(VOID)
{
    DRIVER* drv = GetBestDriver(DRIVER_TYPE_STORAGE);
    if (!drv || !drv->Interface) {
        EC16 error = { EFI_NOT_FOUND, NULL, 0 };
        return error;
    }

    STORAGE_DRIVER_IF* storage = (STORAGE_DRIVER_IF*)drv->Interface;
    return storage->ListDir();
}
EFI_STATUS ChangeDir(CHAR16 *path)
{
    DRIVER* drv = GetBestDriver(DRIVER_TYPE_STORAGE);
    if (!drv || !drv->Interface)
        return EFI_NOT_FOUND;

    STORAGE_DRIVER_IF* storage = (STORAGE_DRIVER_IF*)drv->Interface;
    return storage->ChangeDir(path);
}
EFI_STATUS CreateFile(CHAR16 *name)
{
    DRIVER* drv = GetBestDriver(DRIVER_TYPE_STORAGE);
    if (!drv || !drv->Interface)
        return EFI_NOT_FOUND;

    STORAGE_DRIVER_IF* storage = (STORAGE_DRIVER_IF*)drv->Interface;
    return storage->CreateFile(name);
}
EFI_STATUS DeleteFile(CHAR16 *name)
{
    DRIVER* drv = GetBestDriver(DRIVER_TYPE_STORAGE);
    if (!drv || !drv->Interface)
        return EFI_NOT_FOUND;

    STORAGE_DRIVER_IF* storage = (STORAGE_DRIVER_IF*)drv->Interface;
    return storage->DeleteFile(name);
}
EC16 ReadFile(CHAR16 *filename)
{
    DRIVER* drv = GetBestDriver(DRIVER_TYPE_STORAGE);
    if (!drv || !drv->Interface) {
        EC16 error = { EFI_NOT_FOUND, NULL, 0 };
        return error;
    }

    STORAGE_DRIVER_IF* storage = (STORAGE_DRIVER_IF*)drv->Interface;
    return storage->ReadFile(filename);
}
EFI_STATUS WriteFile(CHAR16 *filename, UINT16 *data, UINTN len)
{
    DRIVER* drv = GetBestDriver(DRIVER_TYPE_STORAGE);
    if (!drv || !drv->Interface)
        return EFI_NOT_FOUND;

    STORAGE_DRIVER_IF* storage = (STORAGE_DRIVER_IF*)drv->Interface;
    return storage->WriteFile(filename, data, len);
}
EFI_STATUS GetFileSize(CHAR16 *filename, UINT64 *filesize)
{
    DRIVER* drv = GetBestDriver(DRIVER_TYPE_STORAGE);
    if (!drv || !drv->Interface)
        return EFI_NOT_FOUND;

    STORAGE_DRIVER_IF* storage = (STORAGE_DRIVER_IF*)drv->Interface;
    return storage->GetFileSize(filename, filesize);
}
VOID RegisterrsDisk()
{
    DRIVER* drv = GetBestDriver(DRIVER_TYPE_STORAGE);
    if (!drv || !drv->Interface)
        return;

    STORAGE_DRIVER_IF* storage = (STORAGE_DRIVER_IF*)drv->Interface;
    storage->RegisterrsDisk();
}
