#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Protocol/SimplePointer.h>
#include "../include/drivers/DriverManager.h"
#include "../include/drivers/Mausedrv.h"

// Поддерживаем до 8 одновременно подключенных мышек/тачпадов
#define MAX_MICE 8
static EFI_SIMPLE_POINTER_PROTOCOL* mMice[MAX_MICE];
static UINTN mMouseCount = 0;

EFI_STATUS Mouse_Init() {
    UINTN HandleCount = 0;
    EFI_HANDLE* Handles = NULL;
    mMouseCount = 0;

    // Ищем все устройства, которые поддерживают протокол мыши
    EFI_STATUS Status = gBS->LocateHandleBuffer(ByProtocol, &gEfiSimplePointerProtocolGuid, NULL, &HandleCount, &Handles);
    if (EFI_ERROR(Status)) return Status;

    // Подключаем все найденные мышки
    for (UINTN i = 0; i < HandleCount && mMouseCount < MAX_MICE; i++) {
        Status = gBS->OpenProtocol(
            Handles[i], 
            &gEfiSimplePointerProtocolGuid, 
            (VOID**)&mMice[mMouseCount], 
            gImageHandle, 
            NULL, 
            EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL
        );
        
        if (!EFI_ERROR(Status)) {
            mMice[mMouseCount]->Reset(mMice[mMouseCount], TRUE); // Сбрасываем мышь (важно для реального железа)
            mMouseCount++;
        }
    }
    
    if (Handles) gBS->FreePool(Handles);
    return (mMouseCount > 0) ? EFI_SUCCESS : EFI_NOT_FOUND;
}

EFI_STATUS Mouse_GetState(INT32* x, INT32* y, BOOLEAN* lb, BOOLEAN* rb) {
    if (mMouseCount == 0) return EFI_NOT_READY;
    
    *x = 0; *y = 0; *lb = FALSE; *rb = FALSE;
    BOOLEAN MovedOrClicked = FALSE;

    // Опрашиваем все мышки (и геймерскую USB, и тачпад)
    for (UINTN i = 0; i < mMouseCount; i++) {
        EFI_SIMPLE_POINTER_STATE State;
        if (mMice[i]->GetState(mMice[i], &State) == EFI_SUCCESS) {
            *x += State.RelativeMovementX;
            *y += State.RelativeMovementY;
            if (State.LeftButton)  *lb = TRUE;
            if (State.RightButton) *rb = TRUE;
            MovedOrClicked = TRUE;
        }
    }

    return MovedOrClicked ? EFI_SUCCESS : EFI_NOT_READY;
}

VOID RegisterMouseDriver(VOID) {
    static MOUSE_DRIVER_IF MouseIf = {
        .Init = Mouse_Init,
        .GetState = Mouse_GetState
    };
    
    DRIVER d;
    d.Type = DRIVER_TYPE_MOUSE;
    d.Priority = 10;
    d.Interface = &MouseIf;
    
    RegisterDriver(&d);
}