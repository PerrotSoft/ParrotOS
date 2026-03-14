#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/PciIo.h>
#include "../include/drivers/DriverManager.h"
#include "../include/drivers/gpudrv.h"

#define GPU_REG_COMMAND 0x00
#define GPU_REG_STATUS  0x04
#define GPU_REG_PC      0x10

typedef struct {
    EFI_PCI_IO_PROTOCOL *PciIo;
} GPU_HW_CONTEXT;

static GPU_HW_CONTEXT gGpuHw = { NULL };

EFI_STATUS RealGpuInit(VOID) {
    EFI_STATUS Status;
    UINTN HandleCount;
    EFI_HANDLE *HandleBuffer;

    Status = gBS->LocateHandleBuffer(ByProtocol, &gEfiPciIoProtocolGuid, NULL, &HandleCount, &HandleBuffer);
    if (EFI_ERROR(Status)) return Status;

    for (UINTN i = 0; i < HandleCount; i++) {
        EFI_PCI_IO_PROTOCOL *PciIo;
        gBS->OpenProtocol(HandleBuffer[i], &gEfiPciIoProtocolGuid, (VOID**)&PciIo, gImageHandle, NULL, EFI_OPEN_PROTOCOL_GET_PROTOCOL);

        UINT32 ClassCode;
        PciIo->Pci.Read(PciIo, EfiPciIoWidthUint32, 0x08, 1, &ClassCode);

        if ((ClassCode >> 16) == 0x03) { // PCI Display Controller
            gGpuHw.PciIo = PciIo;
            UINT64 Attr = EFI_PCI_IO_ATTRIBUTE_BUS_MASTER | EFI_PCI_IO_ATTRIBUTE_MEMORY;
            PciIo->Attributes(PciIo, EfiPciIoAttributeOperationEnable, Attr, NULL);
            return EFI_SUCCESS;
        }
    }
    return EFI_NOT_FOUND;
}

EFI_STATUS RealGpuSendCommand(GPU_COMMAND* Cmd) {
    if (!gGpuHw.PciIo) return EFI_NOT_READY;

    switch (Cmd->Opcode) {
        case GPU_CMD_UPLOAD_DATA:
        case GPU_CMD_UPLOAD_SHADER:
            return gGpuHw.PciIo->Mem.Write(gGpuHw.PciIo, EfiPciIoWidthUint32, 1, Cmd->GpuAddress, Cmd->Size / 4, (VOID*)Cmd->SystemAddr);

        case GPU_CMD_EXECUTE:
            gGpuHw.PciIo->Mem.Write(gGpuHw.PciIo, EfiPciIoWidthUint32, 0, GPU_REG_PC, 1, &Cmd->GpuAddress);
            UINT32 Start = 0x1;
            gGpuHw.PciIo->Mem.Write(gGpuHw.PciIo, EfiPciIoWidthUint32, 0, GPU_REG_COMMAND, 1, &Start);
            break;

        case GPU_CMD_SYNC: {
            UINT32 Stat = 1;
            while (Stat & 0x1) {
                gGpuHw.PciIo->Mem.Read(gGpuHw.PciIo, EfiPciIoWidthUint32, 0, GPU_REG_STATUS, 1, &Stat);
            }
            break;
        }
    }
    return EFI_SUCCESS;
}

BOOLEAN RealGpuIsBusy() {
    UINT32 Stat = 0;
    if (gGpuHw.PciIo) gGpuHw.PciIo->Mem.Read(gGpuHw.PciIo, EfiPciIoWidthUint32, 0, GPU_REG_STATUS, 1, &Stat);
    return (Stat & 0x1);
}

void RegisterRealGpuDriver() {
    static GPU_DRIVER_IF GpuIF = { RealGpuInit, RealGpuSendCommand, RealGpuIsBusy };
    static DRIVER d = { DRIVER_TYPE_GPU, 100, &GpuIF };
    RegisterDriver(&d);
}