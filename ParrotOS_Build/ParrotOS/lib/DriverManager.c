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

EFI_STATUS INIT_VIDEO_DRIVER(EFI_SYSTEM_TABLE *SystemTable)
{
    DRIVER* drv = GetBestDriver(DRIVER_TYPE_VIDEO);
    if (!drv || !drv->Interface)
        return EFI_NOT_FOUND;

    VIDEO_DRIVER_IF* video = (VIDEO_DRIVER_IF*)drv->Interface;
    return video->Init(SystemTable);
}
VOID CLEAR_SCREEN(UINT32 rgb24)
{
    DRIVER* drv = GetBestDriver(DRIVER_TYPE_VIDEO);
    if (!drv || !drv->Interface)
        return;

    VIDEO_DRIVER_IF* video = (VIDEO_DRIVER_IF*)drv->Interface;
    video->ClearScreen(rgb24);
}
VOID PUT_PIXEL(INT32 x, INT32 y, UINT32 rgb24)
{
    DRIVER* drv = GetBestDriver(DRIVER_TYPE_VIDEO);
    if (!drv || !drv->Interface)
        return;

    VIDEO_DRIVER_IF* video = (VIDEO_DRIVER_IF*)drv->Interface;
    video->PutPixel(x, y, rgb24);
}
VOID DRAW_LINE(INT32 x0, INT32 y0, INT32 x1, INT32 y1, UINT32 rgb24)
{
    DRIVER* drv = GetBestDriver(DRIVER_TYPE_VIDEO);
    if (!drv || !drv->Interface)
        return;

    VIDEO_DRIVER_IF* video = (VIDEO_DRIVER_IF*)drv->Interface;
    video->DrawLine(x0, y0, x1, y1, rgb24);
}
VOID DRAW_BITMAP32(const UINT32* bmp, INT32 bmp_w, INT32 bmp_h, INT32 x0, INT32 y0)
{
    DRIVER* drv = GetBestDriver(DRIVER_TYPE_VIDEO);
    if (!drv || !drv->Interface)
        return;

    VIDEO_DRIVER_IF* video = (VIDEO_DRIVER_IF*)drv->Interface;
    video->DrawBitmap32(bmp, bmp_w, bmp_h, x0, y0);
}
VideoMode* GET_CURRENT_VMODE(VOID)
{
    DRIVER* drv = GetBestDriver(DRIVER_TYPE_VIDEO);
    if (!drv || !drv->Interface)
        return NULL;

    VIDEO_DRIVER_IF* video = (VIDEO_DRIVER_IF*)drv->Interface;
    return video->GetVideoMode();
}
