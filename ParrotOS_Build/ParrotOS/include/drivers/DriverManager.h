#pragma once
#include <Uefi.h>
typedef struct {
    EFI_STATUS Status;
    CHAR16     *Message;
    UINTN      FileSize;
} EC16;
typedef enum {
    DRIVER_TYPE_NONE = 0,
    DRIVER_TYPE_KEYBOARD = 1,
    DRIVER_TYPE_VIDEO = 2,
    DRIVER_TYPE_STORAGE = 3
} DRIVER_TYPE;
typedef struct {
    CHAR16  (*GetKey)(EFI_SYSTEM_TABLE *SytemTables);
    BOOLEAN (*HasKey)(EFI_SYSTEM_TABLE *SytemTables);
    VOID    (*Reset)(EFI_SYSTEM_TABLE *SytemTables);
} KEY_DRIVER_IF;
typedef struct {
    EFI_STATUS (*ReadFileByPath)(CHAR16 *path_in, EC16 *out);
    EFI_STATUS (*SetCurrentDisk)(CHAR16 Letter);
    const CHAR16* (*GetCurrentPath)(VOID);
    EFI_STATUS (*PathUp)(VOID);
    EC16 (*ListDir)(VOID);
    EFI_STATUS (*ChangeDir)(CHAR16 *path);
    EFI_STATUS (*CreateFile)(CHAR16 *name);
    EFI_STATUS (*DeleteFile)(CHAR16 *name);
    EC16 (*ReadFile)(CHAR16 *filename);
    EFI_STATUS (*WriteFile)(CHAR16 *filename, UINT16 *data, UINTN len);
    EFI_STATUS (*GetFileSize)(CHAR16 *filename, UINT64 *filesize);
    void       (*RegisterrsDisk)();
} STORAGE_DRIVER_IF;
typedef struct {
    DRIVER_TYPE Type;
    UINT8       Priority;
    VOID*       Interface;
} DRIVER;

BOOLEAN RegisterDriver(DRIVER* Driver);
DRIVER* GetBestDriver(DRIVER_TYPE Type);
VOID INIT(EFI_SYSTEM_TABLE *SytemTables);

CHAR16 GetKey(VOID);
BOOLEAN HasKey(VOID);
VOID Reset(VOID);

EFI_STATUS ReadFileByPath(CHAR16 *path_in, EC16 *out);
EFI_STATUS SetCurrentDisk(CHAR16 Letter);
const CHAR16* GetCurrentPath(VOID);
EFI_STATUS PathUp(VOID);
EC16 ListDir(VOID);
EFI_STATUS ChangeDir(CHAR16 *path);
EFI_STATUS CreateFile(CHAR16 *name);
EFI_STATUS DeleteFile(CHAR16 *name);
EC16 ReadFile(CHAR16 *filename);
EFI_STATUS WriteFile(CHAR16 *filename, UINT16 *data, UINTN len);
EFI_STATUS GetFileSize(CHAR16 *filename, UINT64 *filesize);
VOID RegisterrsDisk();
