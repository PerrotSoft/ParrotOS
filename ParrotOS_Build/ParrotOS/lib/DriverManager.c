#include "../include/drivers/DriverManager.h"

#define MAX_DRIVERS 32

static DRIVER Drivers[MAX_DRIVERS];
static UINTN  DriversCount = 0;
static EFI_SYSTEM_TABLE* SystemTables = NULL;

// Кэшированные указатели на активные интерфейсы для О(1) доступа
static KEY_DRIVER_IF* ActiveKeyboard = NULL;
static STORAGE_DRIVER_IF* ActiveStorage  = NULL;
static VIDEO_DRIVER_IF* ActiveVideo    = NULL;
static NETWORK_DRIVER_IF* ActiveNetwork  = NULL;
static MOUSE_DRIVER_IF* ActiveMouse    = NULL;
static AUDIO_DRIVER_IF* ActiveAudio    = NULL;

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

BOOLEAN RegisterDriver(DRIVER* Driver)
{
    if (DriversCount >= MAX_DRIVERS || Driver == NULL)
        return FALSE;

    Drivers[DriversCount++] = *Driver;
    
    // Обновляем кэш только при регистрации нового драйвера
    DRIVER* Best = GetBestDriver(Driver->Type);
    if (Best && Best->Interface) {
        switch (Best->Type) {
            case DRIVER_TYPE_KEYBOARD: ActiveKeyboard = (KEY_DRIVER_IF*)Best->Interface; break;
            case DRIVER_TYPE_STORAGE:  ActiveStorage  = (STORAGE_DRIVER_IF*)Best->Interface; break;
            case DRIVER_TYPE_VIDEO:    ActiveVideo    = (VIDEO_DRIVER_IF*)Best->Interface; break;
            case DRIVER_TYPE_NETWORK:  ActiveNetwork  = (NETWORK_DRIVER_IF*)Best->Interface; break;
            case DRIVER_TYPE_MOUSE:    ActiveMouse    = (MOUSE_DRIVER_IF*)Best->Interface; break;
            case DRIVER_TYPE_AUDIO:    ActiveAudio    = (AUDIO_DRIVER_IF*)Best->Interface; break;
            default: break;
        }
    }
    
    return TRUE;
}

VOID INIT(EFI_SYSTEM_TABLE *SytemTables)
{
    SystemTables = SytemTables;
}


// ==============================================================================
// KEYBOARD
// ==============================================================================

CHAR16 GetKey(VOID) {
    return (ActiveKeyboard && SystemTables) ? ActiveKeyboard->GetKey(SystemTables) : 0;
}

BOOLEAN HasKey(VOID) {
    return (ActiveKeyboard && SystemTables) ? ActiveKeyboard->HasKey(SystemTables) : FALSE;
}

VOID Reset(VOID) {
    if (ActiveKeyboard) ActiveKeyboard->Reset(SystemTables);
}


// ==============================================================================
// STORAGE
// ==============================================================================

EFI_STATUS ReadFileByPath(CHAR16 *path_in, EC16 *out) {
    return ActiveStorage ? ActiveStorage->ReadFileByPath(path_in, out) : EFI_NOT_FOUND;
}

EFI_STATUS SetCurrentDisk(CHAR16 Letter) {
    return ActiveStorage ? ActiveStorage->SetCurrentDisk(Letter) : EFI_NOT_FOUND;
}

const CHAR16* GetCurrentPath(VOID) {
    return ActiveStorage ? ActiveStorage->GetCurrentPath() : NULL;
}

EFI_STATUS PathUp(VOID) {
    return ActiveStorage ? ActiveStorage->PathUp() : EFI_NOT_FOUND;
}

EC16 ListDir(VOID) {
    if (!ActiveStorage) { 
        EC16 error = { EFI_NOT_FOUND, NULL, 0 }; 
        return error; 
    }
    return ActiveStorage->ListDir();
}

EFI_STATUS ChangeDir(CHAR16 *path) {
    return ActiveStorage ? ActiveStorage->ChangeDir(path) : EFI_NOT_FOUND;
}

EFI_STATUS CreateFile(CHAR16 *name) {
    return ActiveStorage ? ActiveStorage->CreateFile(name) : EFI_NOT_FOUND;
}

EFI_STATUS DeleteFile(CHAR16 *name) {
    return ActiveStorage ? ActiveStorage->DeleteFile(name) : EFI_NOT_FOUND;
}

EC16 ReadFile(CHAR16 *filename) {
    if (!ActiveStorage) { 
        EC16 error = { EFI_NOT_FOUND, NULL, 0 }; 
        return error; 
    }
    return ActiveStorage->ReadFile(filename);
}

EFI_STATUS WriteFile(CHAR16 *filename, UINT16 *data, UINTN len) {
    return ActiveStorage ? ActiveStorage->WriteFile(filename, data, len) : EFI_NOT_FOUND;
}

EFI_STATUS GetFileSize(CHAR16 *filename, UINT64 *filesize) {
    return ActiveStorage ? ActiveStorage->GetFileSize(filename, filesize) : EFI_NOT_FOUND;
}

VOID RegisterrsDisk() {
    if (ActiveStorage) ActiveStorage->RegisterrsDisk();
}

EC16 ListDisks() {
    if (!ActiveStorage) { 
        EC16 error_res = { EFI_NOT_FOUND, NULL, 0 }; 
        return error_res; 
    }
    return ActiveStorage->ListDisks();
}

BOOLEAN FileExists(CHAR16 *path) {
    return ActiveStorage ? ActiveStorage->ExistsFile(path) : FALSE;
}

BOOLEAN DirExists(CHAR16 *path) {
    return ActiveStorage ? ActiveStorage->ExistsDir(path) : FALSE;
}

EFI_STATUS CreateDir(CHAR16 *name) {
    return ActiveStorage ? ActiveStorage->CreateDir(name) : EFI_NOT_FOUND;
}

EFI_STATUS DeleteDir(CHAR16 *name) {
    return ActiveStorage ? ActiveStorage->DeleteDir(name) : EFI_NOT_FOUND;
}

EFI_STATUS MoveFile(CHAR16 *src, CHAR16 *dst) {
    return ActiveStorage ? ActiveStorage->MoveFile(src, dst) : EFI_NOT_FOUND;
}

EFI_STATUS CopyFile(CHAR16 *src, CHAR16 *dst) {
    return ActiveStorage ? ActiveStorage->CopyFile(src, dst) : EFI_NOT_FOUND;
}


// ==============================================================================
// VIDEO
// ==============================================================================

EFI_STATUS INIT_VIDEO_DRIVER(EFI_SYSTEM_TABLE *SystemTable) {
    return ActiveVideo ? ActiveVideo->Init(SystemTable) : EFI_NOT_FOUND;
}

VOID CLEAR_SCREEN(UINT32 rgb24) {
    if (ActiveVideo) ActiveVideo->ClearScreen(rgb24);
}

VOID PUT_PIXEL(INT32 x, INT32 y, UINT32 rgb24) {
    if (ActiveVideo) ActiveVideo->PutPixel(x, y, rgb24);
}

VOID DRAW_LINE(INT32 x0, INT32 y0, INT32 x1, INT32 y1, UINT32 rgb24) {
    if (ActiveVideo) ActiveVideo->DrawLine(x0, y0, x1, y1, rgb24);
}

VOID DRAW_BITMAP32(const UINT32* bmp, INT32 bmp_w, INT32 bmp_h, INT32 x0, INT32 y0) {
    if (ActiveVideo) ActiveVideo->DrawBitmap32(bmp, bmp_w, bmp_h, x0, y0);
}

VideoMode* GET_CURRENT_VMODE(VOID) {
    return ActiveVideo ? ActiveVideo->GetVideoMode() : NULL;
}

UINT32 GET_PIXEL(INT32 x, INT32 y) {
    return ActiveVideo ? ActiveVideo->Get_Pixel(x, y) : 0;
}

VOID SWAP_BUFFERS(VOID) {
    if (ActiveVideo) ActiveVideo->SwapBuffers();
}

VOID GPU_UPLOAD_SHADER(VOID* Code, UINTN Size, UINT64 Offset) {
    if (ActiveVideo) ActiveVideo->UploadShader(Code, Size, Offset);
}

VOID GPU_RUN_COMPUTE(UINT64 Offset, UINT32 Threads) {
    if (ActiveVideo) ActiveVideo->RunCompute(Offset, Threads);
}

const CHAR8* GET_VIDEO_STATUS_STR(VOID) {
    return ActiveVideo ? ActiveVideo->GetDriverType() : "No Video Driver";
}

EFI_STATUS SET_VIDEO_MODE(UINT32 Width, UINT32 Height) {
    return ActiveVideo ? ActiveVideo->SetVideoMode(Width, Height) : EFI_NOT_FOUND;
}


// ==============================================================================
// NETWORK
// ==============================================================================

EFI_STATUS INIT_NETWORK_DRIVER(CHAR16 *NicName, CHAR16 *Password) {
    return ActiveNetwork ? ActiveNetwork->Init(SystemTables, NicName, Password) : EFI_NOT_FOUND;
}

EFI_STATUS NETWORK_TCP_CONNECT(CHAR16 *Ip, UINT16 Port) {
    return ActiveNetwork ? ActiveNetwork->TcpConnect(Ip, Port) : EFI_NOT_FOUND;
}

EFI_STATUS NETWORK_TCP_SEND(UINT8 *Data, UINTN Len) {
    return ActiveNetwork ? ActiveNetwork->TcpSend(Data, Len) : EFI_NOT_FOUND;
}

EFI_STATUS NETWORK_TCP_RECEIVE(UINT8 *Buffer, UINTN *Len) {
    return ActiveNetwork ? ActiveNetwork->TcpReceive(Buffer, Len) : EFI_NOT_FOUND;
}

EFI_STATUS NETWORK_TCP_DISCONNECT(VOID) {
    return ActiveNetwork ? ActiveNetwork->TcpDisconnect() : EFI_NOT_FOUND;
}

EFI_STATUS NETWORK_DNS_LOOKUP(CHAR16 *DomainName, CHAR16 *OutIpStr) {
    return ActiveNetwork ? ActiveNetwork->DnsLookup(DomainName, OutIpStr) : EFI_NOT_FOUND;
}


// ==============================================================================
// MOUSE
// ==============================================================================

EFI_STATUS INIT_MOUSE(VOID) {
    return ActiveMouse ? ActiveMouse->Init() : EFI_NOT_FOUND;
}

EFI_STATUS GET_MOUSE_STATE(INT32 *x, INT32 *y, BOOLEAN *lb, BOOLEAN *rb) {
    return ActiveMouse ? ActiveMouse->GetState(x, y, lb, rb) : EFI_NOT_FOUND;
}


// ==============================================================================
// AUDIO
// ==============================================================================

EFI_STATUS INIT_AUDIO(VOID) {
    return ActiveAudio ? ActiveAudio->Init() : EFI_NOT_FOUND;
}

VOID AudioBeep(UINT32 Freq, UINT32 Dur) {
    if (ActiveAudio) ActiveAudio->Beep(Freq, Dur);
}

EFI_STATUS AudioPlay(UINT8 *Data, UINTN Size) {
    return ActiveAudio ? ActiveAudio->PlayRaw(Data, Size) : EFI_NOT_FOUND;
}