#include "../include/drivers/Video_Driver.h"
#include <Library/UefiLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>

VideoMode vmode = { 0 };
static EFI_GRAPHICS_OUTPUT_PROTOCOL *s_Gop = NULL;

// Оставляем только эти две затычки, иначе DriverManager.c крашнет систему (Null Pointer Exception)
void upload_shader(VOID *Code, UINTN Size, UINT64 Offset) { (void)Code; (void)Size; (void)Offset; }
void run_compute(UINT64 Offset, UINT32 Threads)           { (void)Offset; (void)Threads; }

const CHAR8 *get_driver_type(VOID)                        { return "ParrotOS Accumulation GOP"; }
VideoMode *get_current_vmode(VOID)                        { return &vmode; }
static inline INT32 abs_i(INT32 v) { return v < 0 ? -v : v; }

static inline UINT32 rgb24_to_pixel32(UINT32 rgb24) {
    if (__builtin_expect(vmode.pixel_format == PixelBlueGreenRedReserved8BitPerColor, 1)) {
        return rgb24;
    } else if (vmode.pixel_format == PixelRedGreenBlueReserved8BitPerColor) {
        UINT8 r = (rgb24 >> 16) & 0xFF;
        UINT8 g = (rgb24 >>  8) & 0xFF;
        UINT8 b = (rgb24      ) & 0xFF;
        return ((UINT32)b << 16) | ((UINT32)g << 8) | r;
    }
    return rgb24;
}

/* ─────────────────────────────────────────────────────────────
   ОСНОВНЫЕ ФУНКЦИИ (Пишут ТОЛЬКО в саббуфер)
   ───────────────────────────────────────────────────────────── */

void put_pixel(INT32 x, INT32 y, UINT32 rgb24) {
    if (!vmode.back_buffer) return;
    if ((UINT32)x >= vmode.width || (UINT32)y >= vmode.height) return;
    *(UINT32 *)(vmode.back_buffer + (UINTN)y * vmode.pitch + (UINTN)x * 4) = rgb24_to_pixel32(rgb24);
}

UINT32 get_pixel(INT32 x, INT32 y) {
    if (!vmode.back_buffer) return 0;
    if ((UINT32)x >= vmode.width || (UINT32)y >= vmode.height) return 0;
    
    // Максимальная оптимизация: читаем из RAM, а не из VRAM
    UINT32 raw = *(volatile UINT32 *)(vmode.back_buffer + (UINTN)y * vmode.pitch + (UINTN)x * 4);
    
    if (vmode.pixel_format == PixelBlueGreenRedReserved8BitPerColor) {
        return raw & 0x00FFFFFF;
    } else {
        UINT8 r = (raw      ) & 0xFF;
        UINT8 g = (raw >>  8) & 0xFF;
        UINT8 b = (raw >> 16) & 0xFF;
        return ((UINT32)r << 16) | ((UINT32)g << 8) | b;
    }
}

void clear_screen(UINT32 rgb24) {
    if (!vmode.back_buffer) return;
    UINT32 color = rgb24_to_pixel32(rgb24);
    
    // Команда очистки ПРОСТО заливает саббуфер. На экране ничего не изменится до SWAP.
    for (UINT32 y = 0; y < vmode.height; ++y) {
        SetMem32((VOID *)(vmode.back_buffer + (UINTN)y * vmode.pitch), vmode.width * 4, color);
    }
}

void fill_rect(INT32 x, INT32 y, INT32 w, INT32 h, UINT32 rgb24) {
    if (!vmode.back_buffer) return;
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > (INT32)vmode.width)  w = (INT32)vmode.width  - x;
    if (y + h > (INT32)vmode.height) h = (INT32)vmode.height - y;
    if (w <= 0 || h <= 0) return;

    UINT32 color = rgb24_to_pixel32(rgb24);
    for (INT32 i = 0; i < h; i++) {
        SetMem32((VOID *)(vmode.back_buffer + (UINTN)(y + i) * vmode.pitch + (UINTN)x * 4), (UINTN)w * 4, color);
    }
}

void draw_line(INT32 x0, INT32 y0, INT32 x1, INT32 y1, UINT32 rgb24) {
    if (!vmode.back_buffer) return;
    UINT32 color = rgb24_to_pixel32(rgb24);
    INT32 dx = abs_i(x1 - x0), sx = x0 < x1 ? 1 : -1;
    INT32 dy = -abs_i(y1 - y0), sy = y0 < y1 ? 1 : -1;
    INT32 err = dx + dy, e2;
    for (;;) {
        if ((UINT32)x0 < vmode.width && (UINT32)y0 < vmode.height) {
            *(UINT32 *)(vmode.back_buffer + (UINTN)y0 * vmode.pitch + (UINTN)x0 * 4) = color;
        }
        if (x0 == x1 && y0 == y1) break;
        e2 = err << 1;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

void draw_bitmap32(const UINT32 *bmp, INT32 bmp_w, INT32 bmp_h, INT32 x0, INT32 y0) {
    if (!bmp || !vmode.back_buffer) return;
    if (bmp_h < 0) bmp_h = -bmp_h;
    if (bmp_w < 0) bmp_w = -bmp_w;

    INT32 sx = (x0 < 0) ? -x0 : 0;
    INT32 sy = (y0 < 0) ? -y0 : 0;
    INT32 ex = (x0 + bmp_w > (INT32)vmode.width)  ? (INT32)vmode.width  - x0 : bmp_w;
    INT32 ey = (y0 + bmp_h > (INT32)vmode.height) ? (INT32)vmode.height - y0 : bmp_h;
    if (sx >= ex || sy >= ey) return;

    INT32 draw_w = ex - sx;
    for (INT32 row = sy; row < ey; row++) {
        const UINT32 *src = &bmp[row * bmp_w + sx];
        UINT32 *dst = (UINT32 *)(vmode.back_buffer + (UINTN)(y0 + row) * vmode.pitch + (UINTN)(x0 + sx)  * 4);
        for (INT32 i = 0; i < draw_w; i++) {
            if (src[i] != 0x00000000) dst[i] = rgb24_to_pixel32(src[i]);
        }
    }
}

void scroll_screen_up(int speed_scroll) {
    if (!vmode.back_buffer || speed_scroll <= 0) return;
    if ((UINT32)speed_scroll >= vmode.height) {
        clear_screen(0x000000);
        return;
    }
    UINTN rows_to_move = vmode.height - speed_scroll;
    CopyMem(vmode.back_buffer, vmode.back_buffer + (UINTN)speed_scroll * vmode.pitch, rows_to_move * vmode.pitch);
    for(UINT32 y = rows_to_move; y < vmode.height; y++) {
        SetMem32((VOID *)(vmode.back_buffer + (UINTN)y * vmode.pitch), vmode.width * 4, 0);
    }
}

/* ─────────────────────────────────────────────────────────────
   ГЛАВНАЯ МАГИЯ СВАПА (Никаких очисток!)
   ───────────────────────────────────────────────────────────── */
void swap_buffers(VOID) {
    if (!vmode.fb || !vmode.back_buffer) return;
    
    // Перемещаем кадр на экран максимально быстро. 
    // Буфер остается НЕ ТРОНУТЫМ (память прошлого кадра).
    CopyMem((VOID *)vmode.fb, (VOID *)vmode.back_buffer, (UINTN)vmode.height * vmode.pitch);
}

/* ─────────────────────────────────────────────────────────────
   ИНИЦИАЛИЗАЦИЯ И РЕГИСТРАЦИЯ
   ───────────────────────────────────────────────────────────── */
static EFI_GRAPHICS_OUTPUT_PROTOCOL *FindBestGop(VOID) {
    EFI_GUID gopGuid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
    UINTN HandleCount = 0;
    EFI_HANDLE *Handles = NULL;

    EFI_STATUS Status = gBS->LocateHandleBuffer(ByProtocol, &gopGuid, NULL, &HandleCount, &Handles);
    if (EFI_ERROR(Status) || HandleCount == 0) {
        EFI_GRAPHICS_OUTPUT_PROTOCOL *Gop = NULL;
        gBS->LocateProtocol(&gopGuid, NULL, (VOID **)&Gop);
        return Gop;
    }

    EFI_GRAPHICS_OUTPUT_PROTOCOL *Best = NULL;
    for (UINTN i = 0; i < HandleCount; i++) {
        EFI_GRAPHICS_OUTPUT_PROTOCOL *Gop = NULL;
        Status = gBS->HandleProtocol(Handles[i], &gopGuid, (VOID **)&Gop);
        if (EFI_ERROR(Status) || !Gop || !Gop->Mode) continue;
        if (Gop->Mode->FrameBufferBase == 0 || Gop->Mode->FrameBufferSize == 0) continue;

        if (Best == NULL ||
            (UINT64)Gop->Mode->Info->HorizontalResolution * Gop->Mode->Info->VerticalResolution >
            (UINT64)Best->Mode->Info->HorizontalResolution * Best->Mode->Info->VerticalResolution) {
            Best = Gop;
        }
    }
    gBS->FreePool(Handles);
    if (!Best) gBS->LocateProtocol(&gopGuid, NULL, (VOID **)&Best);
    return Best;
}

static EFI_STATUS ApplyCurrentGopMode(EFI_GRAPHICS_OUTPUT_PROTOCOL *Gop) {
    if (!Gop || !Gop->Mode || !Gop->Mode->Info) return EFI_INVALID_PARAMETER;
    if (gST->ConOut) gST->ConOut->EnableCursor(gST->ConOut, FALSE);

    vmode.width        = Gop->Mode->Info->HorizontalResolution;
    vmode.height       = Gop->Mode->Info->VerticalResolution;
    vmode.bpp          = 32;
    vmode.pitch        = Gop->Mode->Info->PixelsPerScanLine * 4;
    vmode.fb           = (volatile UINT8 *)(UINTN)Gop->Mode->FrameBufferBase;
    vmode.pixel_format = Gop->Mode->Info->PixelFormat;

    if (!vmode.fb || vmode.width == 0 || vmode.height == 0) return EFI_UNSUPPORTED;

    // Безопасное выделение памяти для саббуфера
    if (vmode.back_buffer) {
        gBS->FreePool(vmode.back_buffer);
        vmode.back_buffer = NULL;
    }
    
    UINTN buffer_size = (UINTN)vmode.height * vmode.pitch;
    EFI_STATUS Status = gBS->AllocatePool(EfiLoaderData, buffer_size, (VOID **)&vmode.back_buffer);
    
    if (!EFI_ERROR(Status) && vmode.back_buffer) {
        SetMem(vmode.back_buffer, buffer_size, 0); // Чистим только при первом запуске
    }

    return EFI_SUCCESS;
}

EFI_STATUS init_gop_driver(EFI_SYSTEM_TABLE *SystemTable) {
    (void)SystemTable;
    s_Gop = FindBestGop();
    if (!s_Gop) return EFI_NOT_FOUND;

    EFI_STATUS Status = ApplyCurrentGopMode(s_Gop);
    if (EFI_ERROR(Status)) return Status;

    return EFI_SUCCESS;
}

EFI_STATUS SetVideoMode(UINT32 Width, UINT32 Height) {
    if (!s_Gop) {
        s_Gop = FindBestGop();
        if (!s_Gop) return EFI_NOT_FOUND;
    }

    BOOLEAN want_exact = (Width != 0 && Height != 0);
    UINT32  best_mode  = s_Gop->Mode->Mode;
    UINT32  best_w = 0, best_h = 0;
    EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *Info;
    UINTN InfoSize;

    for (UINT32 i = 0; i < s_Gop->Mode->MaxMode; i++) {
        if (EFI_ERROR(s_Gop->QueryMode(s_Gop, i, &InfoSize, &Info))) continue;
        if (Info->PixelFormat == PixelBltOnly || Info->HorizontalResolution == 0) continue;

        UINT32 w = Info->HorizontalResolution;
        UINT32 h = Info->VerticalResolution;

        if (want_exact) {
            if (w == Width && h == Height) { best_mode = i; break; }
        } else {
            if ((UINT64)w * h > (UINT64)best_w * best_h) {
                best_w = w; best_h = h; best_mode = i;
            }
        }
    }

    if (EFI_ERROR(s_Gop->SetMode(s_Gop, best_mode))) return EFI_UNSUPPORTED;
    return ApplyCurrentGopMode(s_Gop);
}

void init_vd(void) {
    static VIDEO_DRIVER_IF vd_if = {
        .Init          = init_gop_driver,
        .ClearScreen   = clear_screen,
        .PutPixel      = put_pixel,
        .DrawLine      = draw_line,
        .DrawBitmap32  = draw_bitmap32,
        .GetVideoMode  = get_current_vmode,
        .Get_Pixel     = get_pixel,
        .SwapBuffers   = swap_buffers,
        .UploadShader  = upload_shader, // Пустышка (защита от краша)
        .RunCompute    = run_compute,   // Пустышка (защита от краша)
        .GetDriverType = get_driver_type,
        .SetVideoMode  = SetVideoMode,
    };
    DRIVER vd_driver = {
        .Type      = DRIVER_TYPE_VIDEO,
        .Priority  = 2,
        .Interface = (VOID *)&vd_if,
    };
    RegisterDriver(&vd_driver);
}