#include "../include/drivers/Video_Driver.h"
#include "../include/drivers/DriverManager.h"
#include <Library/UefiLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/GraphicsOutput.h>

// Определяем константы шрифта локально, если они не подтянулись из заголовков
#ifndef CHAR_W
#define CHAR_W 8
#endif
#ifndef CHAR_H
#define CHAR_H 16
#endif
#ifndef CHAR_SPACING
#define CHAR_SPACING 1
#endif

VideoMode vmode = { 0 };

static inline void pack_rgb24_to_fb(UINT32 rgb24, volatile UINT8 *dst, UINT32 bpp_bytes, EFI_GRAPHICS_PIXEL_FORMAT fmt)
{
    UINT8 r = (rgb24 >> 16) & 0xFF;
    UINT8 g = (rgb24 >> 8) & 0xFF;
    UINT8 b = (rgb24) & 0xFF;

    if (bpp_bytes == 1) {
        dst[0] = b;
    } else if (bpp_bytes == 2) {
        UINT16 p = (UINT16)(((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3));
        dst[0] = (UINT8)(p & 0xFF);
        dst[1] = (UINT8)(p >> 8);
    } else if (bpp_bytes == 3) {
        if (fmt == PixelRedGreenBlueReserved8BitPerColor) {
            dst[0] = r; dst[1] = g; dst[2] = b;
        } else {
            dst[0] = b; dst[1] = g; dst[2] = r;
        }
    } else {
        if (fmt == PixelRedGreenBlueReserved8BitPerColor) {
            dst[0] = r; dst[1] = g; dst[2] = b; dst[3] = 0;
        } else {
            dst[0] = b; dst[1] = g; dst[2] = r; dst[3] = 0;
        }
    }
}

void put_pixel(INT32 x, INT32 y, UINT32 rgb24)
{
    if (!vmode.fb) return;
    if (x < 0 || y < 0) return;
    if ((UINT32)x >= vmode.width || (UINT32)y >= vmode.height) return;

    UINT32 bpp_bytes = vmode.bpp / 8;
    volatile UINT8 *dst = vmode.fb + (UINT64)y * vmode.pitch + (UINT64)x * bpp_bytes;
    pack_rgb24_to_fb(rgb24, (UINT8*)dst, bpp_bytes, vmode.pixel_format);
}

void scroll_screen_up(int speed_scroll) {
    if (!vmode.fb) return;

    const UINT32 shift_px = (CHAR_H + CHAR_SPACING) * speed_scroll; 
    UINT32 height = vmode.height;
    UINT32 pitch  = vmode.pitch;
    UINT32 width  = vmode.width;
    UINT32 bpp_bytes = vmode.bpp / 8;

    if (pitch == 0 || height == 0 || width == 0 || bpp_bytes == 0) return;
    
    if (shift_px >= height) {
        clear_screen(0x000000);
        return;
    }

    // В UEFI используем CopyMem вместо memcpy
    VOID* dst_addr = (VOID*)vmode.fb;
    VOID* src_addr = (VOID*)(vmode.fb + (UINT64)shift_px * pitch);
    UINTN size_to_copy = (UINTN)(height - shift_px) * pitch;
    
    CopyMem(dst_addr, src_addr, size_to_copy);

    // Очистка появившейся пустой области снизу
    for (UINT32 y = height - shift_px; y < height; y++) {
        volatile UINT8 *line = vmode.fb + (UINT64)y * pitch;
        for (UINT32 x = 0; x < width; x++) {
            pack_rgb24_to_fb(0x000000, (UINT8*)(line + (UINT64)x * bpp_bytes), bpp_bytes, vmode.pixel_format);
        }
    }
}

static inline INT32 abs_i(INT32 v) { return v < 0 ? -v : v; }

void draw_line(INT32 x0, INT32 y0, INT32 x1, INT32 y1, UINT32 rgb24)
{
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

void clear_screen(UINT32 rgb24)
{
    if (!vmode.fb) return;
    UINT32 bpp_bytes = vmode.bpp / 8;
    for (UINT32 y = 0; y < vmode.height; ++y) {
        volatile UINT8 *line = vmode.fb + (UINT64)y * vmode.pitch;
        for (UINT32 x = 0; x < vmode.width; ++x) {
            pack_rgb24_to_fb(rgb24, (UINT8*)(line + (UINT64)x * bpp_bytes), bpp_bytes, vmode.pixel_format);
        }
    }
}

EFI_STATUS init_gop_from_protocol(EFI_GRAPHICS_OUTPUT_PROTOCOL *Gop)
{
    if (!Gop) return EFI_INVALID_PARAMETER;

    vmode.width = Gop->Mode->Info->HorizontalResolution;
    vmode.height = Gop->Mode->Info->VerticalResolution;
    vmode.bpp = 32; 
    vmode.pitch = Gop->Mode->Info->PixelsPerScanLine * (vmode.bpp / 8);
    vmode.fb = (volatile UINT8*)(UINTN)Gop->Mode->FrameBufferBase;
    vmode.pixel_format = Gop->Mode->Info->PixelFormat;

    return EFI_SUCCESS;
}

EFI_STATUS init_gop_driver(EFI_SYSTEM_TABLE *SystemTable)
{
    if (!SystemTable) return EFI_INVALID_PARAMETER;

    EFI_STATUS Status;
    EFI_GRAPHICS_OUTPUT_PROTOCOL *Gop = NULL;
    EFI_GUID gopGuid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;

    Status = SystemTable->BootServices->LocateProtocol(&gopGuid, NULL, (VOID**)&Gop);
    if (EFI_ERROR(Status)) return Status;

    return init_gop_from_protocol(Gop);
}

void draw_char(INT32 x, INT32 y, const UINT8 *bitmap, UINT32 fg) {
    for (INT32 row = 0; row < CHAR_H; row++) {
        UINT8 bits = bitmap[row];
        for (INT32 col = 0; col < CHAR_W; col++) {
            if (bits & (1u << (CHAR_W - 1 - col))) {
                put_pixel(x + col, y + row, fg);
            }
        }
    }
}

void draw_bitmap32(const UINT32* bmp, INT32 bmp_w, INT32 bmp_h, INT32 x0, INT32 y0) {
    for (INT32 y = 0; y < bmp_h; y++) {
        for (INT32 x = 0; x < bmp_w; x++) {
            UINT32 rgb = bmp[y * bmp_w + x];
            put_pixel(x0 + x, y0 + y, rgb);
        }
    }
}

void init_vd()
{
    static VIDEO_DRIVER_IF vd_if = {
        .Init = init_gop_driver,
        .ClearScreen = clear_screen,
        .PutPixel = put_pixel,
        .DrawLine = draw_line,
        .DrawBitmap32 = draw_bitmap32
    };

    DRIVER vd_driver = {
        .Type = DRIVER_TYPE_VIDEO,
        .Priority = 10,
        .Interface = (VOID*)&vd_if
    };
    RegisterDriver(&vd_driver);
}