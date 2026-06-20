#include "../include/drivers/Video_Driver.h"
#include <Library/UefiLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>

VideoMode vmode = { 0 };
static EFI_GRAPHICS_OUTPUT_PROTOCOL *s_Gop = NULL;

void set_render_mode(BOOLEAN fast)                         { (void)fast; }
void upload_shader(VOID *Code, UINTN Size, UINT64 Offset) { (void)Code; (void)Size; (void)Offset; }
void run_compute(UINT64 Offset, UINT32 Threads)            { (void)Offset; (void)Threads; }
const CHAR8 *get_driver_type(VOID)                        { return "ParrotOS GOP Driver"; }
VideoMode *get_current_vmode(VOID)                        { return &vmode; }
static inline INT32 abs_i(INT32 v) { return v < 0 ? -v : v; }


static inline UINT32 rgb24_to_pixel32(UINT32 rgb24)
{
    if (__builtin_expect(
            vmode.pixel_format == PixelBlueGreenRedReserved8BitPerColor, 1))
    {
        return rgb24;
    }
    else if (vmode.pixel_format == PixelRedGreenBlueReserved8BitPerColor)
    {
        /* RGBX — виртуалки. Меняем R и B местами */
        UINT8 r = (rgb24 >> 16) & 0xFF;
        UINT8 g = (rgb24 >>  8) & 0xFF;
        UINT8 b = (rgb24      ) & 0xFF;
        return ((UINT32)b << 16) | ((UINT32)g << 8) | r;
    }
    return rgb24;
}
void put_pixel(INT32 x, INT32 y, UINT32 rgb24)
{
    if (!vmode.fb) return;
    if ((UINT32)x >= vmode.width || (UINT32)y >= vmode.height) return;
    *(UINT32 *)(vmode.fb + (UINTN)y * vmode.pitch + (UINTN)x * 4) = rgb24_to_pixel32(rgb24);
}

UINT32 get_pixel(INT32 x, INT32 y)
{
    if (!vmode.fb) return 0;
    if ((UINT32)x >= vmode.width || (UINT32)y >= vmode.height) return 0;

    UINT32 raw = *(volatile UINT32 *)(vmode.fb + (UINTN)y * vmode.pitch + (UINTN)x * 4);

    if (vmode.pixel_format == PixelBlueGreenRedReserved8BitPerColor) {
        return raw & 0x00FFFFFF;
    } else {
        UINT8 r = (raw      ) & 0xFF;
        UINT8 g = (raw >>  8) & 0xFF;
        UINT8 b = (raw >> 16) & 0xFF;
        return ((UINT32)r << 16) | ((UINT32)g << 8) | b;
    }
}

void clear_screen(UINT32 rgb24)
{
    if (!vmode.fb) return;
    UINT32 color = rgb24_to_pixel32(rgb24);
    UINTN  pitch = vmode.pitch;
    for (UINT32 y = 0; y < vmode.height; ++y) {
        SetMem32((VOID *)(vmode.fb + (UINTN)y * pitch), vmode.width * 4, color);
    }
}

void fill_rect(INT32 x, INT32 y, INT32 w, INT32 h, UINT32 rgb24)
{
    if (!vmode.fb) return;
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > (INT32)vmode.width)  w = (INT32)vmode.width  - x;
    if (y + h > (INT32)vmode.height) h = (INT32)vmode.height - y;
    if (w <= 0 || h <= 0) return;

    UINT32 color = rgb24_to_pixel32(rgb24);
    UINTN  pitch = vmode.pitch;
    for (INT32 i = 0; i < h; i++) {
        SetMem32((VOID *)(vmode.fb + (UINTN)(y + i) * pitch + (UINTN)x * 4),
                 (UINTN)w * 4, color);
    }
}
void draw_line(INT32 x0, INT32 y0, INT32 x1, INT32 y1, UINT32 rgb24)
{
    if (!vmode.fb) return;
    UINT32 color = rgb24_to_pixel32(rgb24);
    UINTN  pitch = vmode.pitch;
    UINT32 W = vmode.width, H = vmode.height;

    INT32 dx =  abs_i(x1 - x0), sx = x0 < x1 ? 1 : -1;
    INT32 dy = -abs_i(y1 - y0), sy = y0 < y1 ? 1 : -1;
    INT32 err = dx + dy, e2;

    for (;;) {
        if ((UINT32)x0 < W && (UINT32)y0 < H) {
            *(UINT32 *)(vmode.fb + (UINTN)y0 * pitch + (UINTN)x0 * 4) = color;
        }
        if (x0 == x1 && y0 == y1) break;
        e2 = err << 1;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

void draw_bitmap32(const UINT32 *bmp, INT32 bmp_w, INT32 bmp_h, INT32 x0, INT32 y0)
{
    if (!bmp || !vmode.fb) return;
    if (bmp_h < 0) bmp_h = -bmp_h;
    if (bmp_w < 0) bmp_w = -bmp_w;

    INT32 sx = (x0 < 0) ? -x0 : 0;
    INT32 sy = (y0 < 0) ? -y0 : 0;
    INT32 ex = (x0 + bmp_w > (INT32)vmode.width)  ? (INT32)vmode.width  - x0 : bmp_w;
    INT32 ey = (y0 + bmp_h > (INT32)vmode.height) ? (INT32)vmode.height - y0 : bmp_h;
    if (sx >= ex || sy >= ey) return;

    INT32 draw_w = ex - sx;
    UINTN pitch  = vmode.pitch;

    for (INT32 row = sy; row < ey; row++) {
        const UINT32 *src = &bmp[row * bmp_w + sx];
        UINT32 *dst = (UINT32 *)(vmode.fb +
                      (UINTN)(y0 + row) * pitch +
                      (UINTN)(x0 + sx)  * 4);
        for (INT32 i = 0; i < draw_w; i++) {
            UINT32 c = src[i];
            if (c != 0x00000000) {
                dst[i] = rgb24_to_pixel32(c);
            }
        }
    }
}

void swap_buffers(VOID) { }

/* ─────────────────────────────────────────────────────────────
   FindBestGop — поиск GOP handle с реальным framebuffer.

   ПОЧЕМУ HandleBuffer а не LocateProtocol:
   На NVIDIA/AMD бывает несколько GOP handles — один для UEFI Shell
   (без реального fb, base == 0), один для дисплея.
   LocateProtocol возвращает первый попавшийся — может быть не тот.
   HandleBuffer перебирает все и выбирает нужный.
   ───────────────────────────────────────────────────────────── */
static EFI_GRAPHICS_OUTPUT_PROTOCOL *FindBestGop(VOID)
{
    EFI_GUID gopGuid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
    UINTN HandleCount = 0;
    EFI_HANDLE *Handles = NULL;

    EFI_STATUS Status = gBS->LocateHandleBuffer(
        ByProtocol, &gopGuid, NULL, &HandleCount, &Handles);

    if (EFI_ERROR(Status) || HandleCount == 0) {
        /* Fallback: старый метод */
        EFI_GRAPHICS_OUTPUT_PROTOCOL *Gop = NULL;
        gBS->LocateProtocol(&gopGuid, NULL, (VOID **)&Gop);
        return Gop;
    }

    EFI_GRAPHICS_OUTPUT_PROTOCOL *Best = NULL;

    for (UINTN i = 0; i < HandleCount; i++) {
        EFI_GRAPHICS_OUTPUT_PROTOCOL *Gop = NULL;
        Status = gBS->HandleProtocol(Handles[i], &gopGuid, (VOID **)&Gop);
        if (EFI_ERROR(Status) || !Gop || !Gop->Mode) continue;

        /* Пропускаем handle без реального framebuffer */
        if (Gop->Mode->FrameBufferBase == 0) continue;
        if (Gop->Mode->FrameBufferSize  == 0) continue;

        /* Выбираем GOP с наибольшим разрешением */
        if (Best == NULL ||
            (UINT64)Gop->Mode->Info->HorizontalResolution *
                    Gop->Mode->Info->VerticalResolution >
            (UINT64)Best->Mode->Info->HorizontalResolution *
                    Best->Mode->Info->VerticalResolution)
        {
            Best = Gop;
        }
    }

    gBS->FreePool(Handles);

    if (!Best) {
        /* Последний fallback */
        gBS->LocateProtocol(&gopGuid, NULL, (VOID **)&Best);
    }

    return Best;
}

/* ─────────────────────────────────────────────────────────────
   ApplyCurrentGopMode — читает текущий режим GOP в vmode.
   НЕ вызывает SetMode. НЕ меняет разрешение.
   ───────────────────────────────────────────────────────────── */
static EFI_STATUS ApplyCurrentGopMode(EFI_GRAPHICS_OUTPUT_PROTOCOL *Gop)
{
    if (!Gop || !Gop->Mode || !Gop->Mode->Info) return EFI_INVALID_PARAMETER;

    if (gST->ConOut) {
        gST->ConOut->EnableCursor(gST->ConOut, FALSE);
    }

    vmode.width        = Gop->Mode->Info->HorizontalResolution;
    vmode.height       = Gop->Mode->Info->VerticalResolution;
    vmode.bpp          = 32;
    vmode.pitch        = Gop->Mode->Info->PixelsPerScanLine * 4;
    vmode.fb           = (volatile UINT8 *)(UINTN)Gop->Mode->FrameBufferBase;
    vmode.pixel_format = Gop->Mode->Info->PixelFormat;

    if (!vmode.fb || vmode.width == 0 || vmode.height == 0) {
        return EFI_UNSUPPORTED;
    }

    return EFI_SUCCESS;
}

/* ─────────────────────────────────────────────────────────────
   init_gop_driver — инициализация видеодрайвера.

   КРИТИЧЕСКИ ВАЖНО: эта функция НЕ вызывает SetMode.
   Она только ЧИТАЕТ текущий GOP режим который UEFI уже выставил.

   Если вызвать SetMode здесь — framebuffer сбрасывается,
   и всё что нарисовано после этого (логотип, линия из ParrotOS.c)
   будет потеряно при следующем рисовании.

   Для смены разрешения: вызови SET_VIDEO_MODE(w, h) отдельно.
   ───────────────────────────────────────────────────────────── */
EFI_STATUS init_gop_driver(EFI_SYSTEM_TABLE *SystemTable)
{
    (void)SystemTable;

    s_Gop = FindBestGop();
    if (!s_Gop) return EFI_NOT_FOUND;

    EFI_STATUS Status = ApplyCurrentGopMode(s_Gop);
    if (EFI_ERROR(Status)) return Status;

    clear_screen(0x000000);
    return EFI_SUCCESS;
}

/* ─────────────────────────────────────────────────────────────
   SetVideoMode — явная смена разрешения по запросу.
   Width=0, Height=0 → выбрать максимальное доступное.
   ───────────────────────────────────────────────────────────── */
EFI_STATUS SetVideoMode(UINT32 Width, UINT32 Height)
{
    if (!s_Gop) {
        s_Gop = FindBestGop();
        if (!s_Gop) return EFI_NOT_FOUND;
    }

    BOOLEAN want_exact = (Width != 0 && Height != 0);
    UINT32  best_mode  = s_Gop->Mode->Mode;
    UINT32  best_w     = 0, best_h = 0;

    EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *Info;
    UINTN InfoSize;

    for (UINT32 i = 0; i < s_Gop->Mode->MaxMode; i++) {
        EFI_STATUS s = s_Gop->QueryMode(s_Gop, i, &InfoSize, &Info);
        if (EFI_ERROR(s)) continue;
        if (Info->PixelFormat == PixelBltOnly) continue;
        if (Info->HorizontalResolution == 0)   continue;

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

    EFI_STATUS Status = s_Gop->SetMode(s_Gop, best_mode);
    if (EFI_ERROR(Status)) return Status;

    return ApplyCurrentGopMode(s_Gop);
}

/* ─────────────────────────────────────────────────────────────
   init_vd — регистрация видеодрайвера
   ───────────────────────────────────────────────────────────── */
void init_vd(void)
{
    static VIDEO_DRIVER_IF vd_if = {
        .Init          = init_gop_driver,
        .ClearScreen   = clear_screen,
        .PutPixel      = put_pixel,
        .DrawLine      = draw_line,
        .DrawBitmap32  = draw_bitmap32,
        .GetVideoMode  = get_current_vmode,
        .Get_Pixel     = get_pixel,
        .SwapBuffers   = swap_buffers,
        .UploadShader  = upload_shader,
        .RunCompute    = run_compute,
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
