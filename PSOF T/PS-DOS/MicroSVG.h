#include <stdint.h>
#include <stddef.h>
void* malloc(size_t size) {
    uint64_t ptr;
    // RAX = 0x01 (Выделить память)
    // RCX = размер
    asm volatile (
        "movq $0x01, %%rax;"
        "movq %1, %%rcx;"
        "int $0x2A;"
        "movq %%rax, %0;"
        : "=r"(ptr)
        : "r"((uint64_t)size)
        : "rax", "rcx"
    );
    return (void*)ptr; // Если вернулся 0 (NULL), значит памяти нет
}

void free(void* ptr) {
    if (!ptr) return;
    // RAX = 0x02 (Освободить память)
    // RCX = указатель на память
    asm volatile (
        "movq $0x02, %%rax;"
        "movq %0, %%rcx;"
        "int $0x2A;"
        :
        : "r"((uint64_t)ptr)
        : "rax", "rcx"
    );
}

// Твои функции из API (заголовки)
extern void GfxPutPixel(int32_t x, int32_t y, uint32_t color);
extern void GfxDrawLine(int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint32_t color);

// --- Структура "Скомпилированного" SVG (Растр) ---
typedef struct {
    int32_t width;
    int32_t height;
    uint32_t* pixels;
} RasterImage;

// =====================================================================
// 1. УНИВЕРСАЛЬНАЯ КИСТЬ (Рисует на экран ИЛИ в память)
// =====================================================================
typedef struct {
    uint32_t* buffer; // Если NULL -> рисуем на экран. Если есть -> рисуем в память
    int32_t buf_w;
    int32_t buf_h;
    int32_t offset_x;
    int32_t offset_y;
} RenderTarget;

static void SvgPutPixel(RenderTarget* target, int32_t x, int32_t y, uint32_t color) {
    int32_t tx = x + target->offset_x;
    int32_t ty = y + target->offset_y;
    
    // Режим КОМПИЛЯЦИИ (в память)
    if (target->buffer) {
        if (tx >= 0 && tx < target->buf_w && ty >= 0 && ty < target->buf_h) {
            target->buffer[ty * target->buf_w + tx] = color;
        }
    } 
    // Режим ИНТЕРПРЕТАЦИИ (сразу на экран через твой API)
    else {
        GfxPutPixel(tx, ty, color);
    }
}

// Простейшая линия для рендера в память (Брезенхем)
static void SvgDrawLine(RenderTarget* target, int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint32_t color) {
    int dx = (x1 > x0 ? x1 - x0 : x0 - x1), sx = x0 < x1 ? 1 : -1;
    int dy = -(y1 > y0 ? y1 - y0 : y0 - y1), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy, e2;
    for (;;) {
        SvgPutPixel(target, x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

// Вертикальный градиент (от цвета 1 к цвету 2)
static uint32_t InterpolateColor(uint32_t c1, uint32_t c2, float t) {
    uint8_t r1 = (c1 >> 16) & 0xFF, g1 = (c1 >> 8) & 0xFF, b1 = c1 & 0xFF;
    uint8_t r2 = (c2 >> 16) & 0xFF, g2 = (c2 >> 8) & 0xFF, b2 = c2 & 0xFF;
    uint8_t r = r1 + (r2 - r1) * t;
    uint8_t g = g1 + (g2 - g1) * t;
    uint8_t b = b1 + (b2 - b1) * t;
    return (r << 16) | (g << 8) | b;
}

// =====================================================================
// 2. ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ ПАРСЕРА ТЕКСТА
// =====================================================================
// Очень простой поиск подстроки
static const char* StrFind(const char* str, const char* sub) {
    if (!*sub) return str;
    const char* p1; const char* p2; const char* p1_adv = str;
    while (*++p1_adv) {
        p1 = str; p2 = sub;
        while (*p1 && *p2 && *p1 == *p2) { p1++; p2++; }
        if (!*p2) return str;
        str++;
    }
    return NULL;
}

// Парсинг атрибутов вида width="100"
static int32_t ParseIntAttr(const char* tag_text, const char* attr_name) {
    const char* p = StrFind(tag_text, attr_name);
    if (!p) return 0;
    p += 0; // Смещаемся за имя (нужно написать нормальный strlen)
    while (*p && *p != '"') p++;
    if (!*p) return 0;
    p++; // пропускаем кавычку
    
    int32_t val = 0;
    while (*p >= '0' && *p <= '9') {
        val = val * 10 + (*p - '0');
        p++;
    }
    return val;
}

// Парсинг цвета вида fill="#FF0000"
static uint32_t ParseHexColor(const char* tag_text, const char* attr_name) {
    const char* p = StrFind(tag_text, attr_name);
    if (!p) return 0xFFFFFF; // Белый по умолчанию
    while (*p && *p != '"') p++;
    if (!*p) return 0xFFFFFF;
    p++;
    if (*p == '#') p++;
    
    uint32_t color = 0;
    for (int i = 0; i < 6 && *p; i++, p++) {
        color <<= 4;
        if (*p >= '0' && *p <= '9') color |= (*p - '0');
        else if (*p >= 'a' && *p <= 'f') color |= (*p - 'a' + 10);
        else if (*p >= 'A' && *p <= 'F') color |= (*p - 'A' + 10);
    }
    return color;
}

// =====================================================================
// 3. ОСНОВНОЙ ДВИЖОК MICRO-SVG
// =====================================================================
static void ProcessSVGText(const char* svg_text, RenderTarget* target) {
    const char* p = svg_text;
    
    while (*p) {
        // Ищем начало тега
        if (*p == '<') {
            // ПРЯМОУГОЛЬНИК И ГРАДИЕНТ
            if (p[1] == 'r' && p[2] == 'e' && p[3] == 'c' && p[4] == 't') {
                int32_t x = ParseIntAttr(p, "x=");
                int32_t y = ParseIntAttr(p, "y=");
                int32_t w = ParseIntAttr(p, "width=");
                int32_t h = ParseIntAttr(p, "height=");
                uint32_t color = ParseHexColor(p, "fill=");
                
                // Проверяем наличие атрибута градиента grad="..." (кастомная фича)
                uint32_t grad_color = ParseHexColor(p, "grad=");
                int has_grad = StrFind(p, "grad=") != NULL;

                for (int32_t iy = 0; iy < h; iy++) {
                    uint32_t row_color = color;
                    if (has_grad) { // Считаем линейный градиент сверху вниз
                        float t = (float)iy / (float)(h > 1 ? h - 1 : 1);
                        row_color = InterpolateColor(color, grad_color, t);
                    }
                    for (int32_t ix = 0; ix < w; ix++) {
                        SvgPutPixel(target, x + ix, y + iy, row_color);
                    }
                }
            }
            // КРУГ
            else if (p[1] == 'c' && p[2] == 'i' && p[3] == 'r' && p[4] == 'c') {
                int32_t cx = ParseIntAttr(p, "cx=");
                int32_t cy = ParseIntAttr(p, "cy=");
                int32_t r = ParseIntAttr(p, "r=");
                uint32_t color = ParseHexColor(p, "fill=");
                
                // Простая заливка круга
                for (int32_t iy = -r; iy <= r; iy++) {
                    for (int32_t ix = -r; ix <= r; ix++) {
                        if (ix*ix + iy*iy <= r*r) {
                            SvgPutPixel(target, cx + ix, cy + iy, color);
                        }
                    }
                }
            }
            // ЛИНИЯ
            else if (p[1] == 'l' && p[2] == 'i' && p[3] == 'n' && p[4] == 'e') {
                int32_t x1 = ParseIntAttr(p, "x1=");
                int32_t y1 = ParseIntAttr(p, "y1=");
                int32_t x2 = ParseIntAttr(p, "x2=");
                int32_t y2 = ParseIntAttr(p, "y2=");
                uint32_t color = ParseHexColor(p, "stroke=");
                SvgDrawLine(target, x1, y1, x2, y2, color);
            }
        }
        p++;
    }
}

// =====================================================================
// API ДЛЯ ОС (РЕЖИМ 1: ИНТЕРПРЕТАЦИЯ)
// =====================================================================
void SvgDrawRealtime(const char* svg_text, int32_t screen_x, int32_t screen_y) {
    RenderTarget rt = {0};
    rt.buffer = NULL; // NULL значит рисуем прямо на экран
    rt.offset_x = screen_x;
    rt.offset_y = screen_y;
    
    ProcessSVGText(svg_text, &rt);
}

// =====================================================================
// API ДЛЯ ОС (РЕЖИМ 2: КОМПИЛЯЦИЯ В РАСТР)
// =====================================================================
RasterImage SvgCompileToBitmap(const char* svg_text, int32_t out_w, int32_t out_h) {
    RasterImage img;
    img.width = out_w;
    img.height = out_h;
    img.pixels = (uint32_t*)malloc(out_w * out_h * sizeof(uint32_t)); // Выделяем память
    
    // Заливаем прозрачным/черным фоном по умолчанию
    for(int i=0; i<out_w*out_h; i++) img.pixels[i] = 0x00000000; 

    RenderTarget rt = {0};
    rt.buffer = img.pixels; // Направляем кисть в выделенную память!
    rt.buf_w = out_w;
    rt.buf_h = out_h;
    rt.offset_x = 0;
    rt.offset_y = 0;
    
    ProcessSVGText(svg_text, &rt);
    return img;
}