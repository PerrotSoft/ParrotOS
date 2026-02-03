#include "../include/drivers/Video_Driver.h"
#include <Library/UefiLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>

VideoMode vmode = { 0 };

static UINT8* back_buffer = NULL;
static UINTN  back_buffer_size = 0;

static inline INT32 abs_i(INT32 v) { return v < 0 ? -v : v; }

static inline UINT32 convert_color(UINT32 rgb24, EFI_GRAPHICS_PIXEL_FORMAT fmt) {
    UINT8 r = (rgb24 >> 16) & 0xFF;
    UINT8 g = (rgb24 >> 8) & 0xFF;
    UINT8 b = (rgb24) & 0xFF;
    if (fmt == PixelBlueGreenRedReserved8BitPerColor) return (UINT32)(b | (g << 8) | (r << 16));
    return (UINT32)(r | (g << 8) | (b << 16));
}

void put_pixel(INT32 x, INT32 y, UINT32 rgb24) {
    if (!back_buffer) return;
    if (x < 0 || y < 0 || (UINT32)x >= vmode.width || (UINT32)y >= vmode.height) return;
    UINT32 bpp_bytes = vmode.bpp / 8;
    UINT64 offset = (UINT64)y * vmode.pitch + (UINT64)x * bpp_bytes;
    if (bpp_bytes == 4) {
        *((UINT32*)(back_buffer + offset)) = convert_color(rgb24, vmode.pixel_format);
    } else {
        UINT8* dst = back_buffer + offset;
        UINT8 r = (rgb24 >> 16) & 0xFF, g = (rgb24 >> 8) & 0xFF, b = rgb24 & 0xFF;
        if (vmode.pixel_format == PixelBlueGreenRedReserved8BitPerColor) { dst[0] = b; dst[1] = g; dst[2] = r; }
        else { dst[0] = r; dst[1] = g; dst[2] = b; }
    }
}

UINT32 get_pixel(INT32 x, INT32 y) {
    if (!back_buffer || x < 0 || y < 0 || (UINT32)x >= vmode.width || (UINT32)y >= vmode.height) return 0;
    UINT32 bpp_bytes = vmode.bpp / 8;
    UINT8* src = back_buffer + (UINT64)y * vmode.pitch + (UINT64)x * bpp_bytes;
    if (bpp_bytes == 4) {
        UINT32 raw = *((UINT32*)src);
        if (vmode.pixel_format == PixelBlueGreenRedReserved8BitPerColor)
            return (UINT32)(((raw >> 16) & 0xFF) << 16 | ((raw >> 8) & 0xFF) << 8 | (raw & 0xFF));
        return raw;
    }
    return 0;
}

void fill_rect(INT32 x, INT32 y, INT32 w, INT32 h, UINT32 rgb24) {
    for (INT32 i = y; i < y + h; i++)
        for (INT32 j = x; j < x + w; j++) put_pixel(j, i, rgb24);
}

void draw_line(INT32 x0, INT32 y0, INT32 x1, INT32 y1, UINT32 rgb24) {
    INT32 dx = abs_i(x1 - x0), sx = x0 < x1 ? 1 : -1;
    INT32 dy = -abs_i(y1 - y0), sy = y0 < y1 ? 1 : -1;
    INT32 err = dx + dy, e2;
    for (;;) {
        put_pixel(x0, y0, rgb24);
        if (x0 == x1 && y0 == y1) break;
        e2 = err << 1;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

void clear_screen(UINT32 rgb24) {
    if (!back_buffer) return;
    if (rgb24 == 0) { ZeroMem(back_buffer, back_buffer_size); return; }
    UINT32 color32 = convert_color(rgb24, vmode.pixel_format);
    UINT32* dst32 = (UINT32*)back_buffer;
    for (UINTN i = 0; i < back_buffer_size / 4; i++) dst32[i] = color32;
}

void draw_bitmap32(const UINT32* bmp, INT32 bmp_w, INT32 bmp_h, INT32 x0, INT32 y0) {
    if (!bmp) return;
    for (INT32 y = 0; y < bmp_h; y++)
        for (INT32 x = 0; x < bmp_w; x++) put_pixel(x0 + x, y0 + y, bmp[y * bmp_w + x]);
}

void swap_buffers(VOID) {
    if (back_buffer && vmode.fb) CopyMem((VOID*)vmode.fb, (VOID*)back_buffer, back_buffer_size);
}

void upload_shader(VOID* Code, UINTN Size, UINT64 Offset) { (VOID)Code; (VOID)Size; (VOID)Offset; }
void run_compute(UINT64 Offset, UINT32 Threads) { (VOID)Offset; (VOID)Threads; }
const CHAR8* get_driver_type(VOID) { return "UEFI Soft-Rasterizer (Double Buffered)"; }
VideoMode* get_current_vmode() { return &vmode; }

EFI_STATUS init_gop_driver(EFI_SYSTEM_TABLE *SystemTable) {
    EFI_GRAPHICS_OUTPUT_PROTOCOL *Gop = NULL;
    EFI_GUID gopGuid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
    EFI_STATUS Status = SystemTable->BootServices->LocateProtocol(&gopGuid, NULL, (VOID**)&Gop);
    if (EFI_ERROR(Status)) return Status;
    vmode.width = Gop->Mode->Info->HorizontalResolution;
    vmode.height = Gop->Mode->Info->VerticalResolution;
    vmode.pixel_format = Gop->Mode->Info->PixelFormat;
    vmode.pitch = Gop->Mode->Info->PixelsPerScanLine * 4;
    vmode.bpp = 32;
    vmode.fb = (volatile UINT8*)(UINTN)Gop->Mode->FrameBufferBase;
    back_buffer_size = (UINTN)vmode.height * vmode.pitch;
    Status = gBS->AllocatePool(EfiLoaderData, back_buffer_size, (VOID**)&back_buffer);
    if (EFI_ERROR(Status)) return Status;
    ZeroMem(back_buffer, back_buffer_size);
    vmode.back_buffer = back_buffer;
    return EFI_SUCCESS;
}

void init_vd() {
    static VIDEO_DRIVER_IF vd_if = {
        .Init = init_gop_driver, .ClearScreen = clear_screen, .PutPixel = put_pixel,
        .DrawLine = draw_line, .DrawBitmap32 = draw_bitmap32, .GetVideoMode = get_current_vmode,
        .Get_Pixel = get_pixel, .SwapBuffers = swap_buffers, .UploadShader = upload_shader,
        .RunCompute = run_compute, .GetDriverType = get_driver_type
    };
    DRIVER vd_driver = { .Type = DRIVER_TYPE_VIDEO, .Priority = 10, .Interface = (VOID*)&vd_if };
    RegisterDriver(&vd_driver);
}