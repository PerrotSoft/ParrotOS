#include "ParrotOS_API.hpp"

using namespace ParrotOS;

// ============================================================
//  УТИЛИТЫ И БЕЗОПАСНЫЕ ФУНКЦИИ
// ============================================================

static void u64_to_str(uint64_t v, CHAR16* buf, int buf_size) {
    if (v == 0) { buf[0] = u'0'; buf[1] = 0; return; }
    CHAR16 tmp[32]; int i = 0;
    while (v > 0 && i < buf_size - 1) { tmp[i++] = u'0' + (CHAR16)(v % 10); v /= 10; }
    int j = 0;
    while (i > 0) buf[j++] = tmp[--i];
    buf[j] = 0;
}

static void str_cat(CHAR16* dst, const CHAR16* src) {
    while (*dst) dst++;
    while (*src) *dst++ = *src++;
    *dst = 0;
}

static int str_len(const CHAR16* s) {
    int n = 0; while (*s++) n++; return n;
}

// ============================================================
//  CSV-ОТЧЕТ
// ============================================================

static CHAR16   g_report[8192];
static uint64_t g_report_len = 0;

static void report_append(const CHAR16* s) {
    while (*s) g_report[g_report_len++] = *s++;
    g_report[g_report_len] = 0;
}

// ============================================================
//  ГЛОБАЛЬНЫЕ ПЕРЕМЕННЫЕ ЭКРАНА
// ============================================================

static int32_t SCR_W = 1280, SCR_H = 720;

// ============================================================
//  ГРАФИЧЕСКИЕ ХЕЛПЕРЫ
// ============================================================

static void draw_text(int32_t x, int32_t y, int32_t sz, uint32_t col, const CHAR16* s) {
    Graphics::Print(x, y, sz, col, s);
}

static void draw_rect(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t col) {
    for (int32_t row = 0; row < h; row++)
        Graphics::DrawLine(x, y + row, x + w - 1, y + row, col);
}

// ============================================================
//  ПРОДВИНУТЫЙ ВЫЧИСЛИТЕЛЬ КУРСОРА (ВИРТУАЛЬНЫЙ + ФИЗИЧЕСКИЙ)
// ============================================================

static int32_t g_cur_x      = 0;
static int32_t g_cur_y      = 0;
static bool    g_cur_click  = false; // ЛКМ (Q)
static bool    g_cur_rclick = false; // ПКМ (E)

#define STEP_S 12 // Скорость перемещения курсора

static void cursor_init() {
    Mouse::Init();
    g_cur_x = SCR_W / 2;
    g_cur_y = SCR_H / 2;
    g_cur_click = false;
    g_cur_rclick = false;
}

static void cursor_update() {
    g_cur_click = false;
    g_cur_rclick = false;

    int32_t mx = 0, my = 0;
    uint8_t mb1 = 0, mb2 = 0;
    Mouse::GetState(mx, my, mb1, mb2);
    
    if (mx != 0 || my != 0) {
        g_cur_x += mx;
        g_cur_y += my;
    }
    if (mb1) g_cur_click = true;
    if (mb2) g_cur_rclick = true;

    if (Keyboard::HasKey()) {
        uint64_t raw  = Keyboard::GetKey();
        uint16_t ch   = (uint16_t)(raw & 0xFFFF);
        uint16_t scan = (uint16_t)((raw >> 16) & 0xFFFF);

        if      (ch == u'w' || ch == u'W' || scan == Keys::Up)    g_cur_y -= STEP_S;
        else if (ch == u's' || ch == u'S' || scan == Keys::Down)  g_cur_y += STEP_S;
        else if (ch == u'a' || ch == u'A' || scan == Keys::Left)  g_cur_x -= STEP_S;
        else if (ch == u'd' || ch == u'D' || scan == Keys::Right) g_cur_x += STEP_S;
        else if (ch == u'q' || ch == u'Q' || ch == Keys::CarriageReturn || ch == Keys::Linefeed) {
            g_cur_click = true;
        }
        else if (ch == u'e' || ch == u'E') {
            g_cur_rclick = true;
        }
    }

    if (g_cur_x < 0)      g_cur_x = 0;
    if (g_cur_x >= SCR_W) g_cur_x = SCR_W - 1;
    if (g_cur_y < 0)      g_cur_y = 0;
    if (g_cur_y >= SCR_H) g_cur_y = SCR_H - 1;
}

static void cursor_draw() {
    int32_t x = g_cur_x, y = g_cur_y;
    
    for (int dx = -1; dx <= 1; dx++)
        for (int dy = -1; dy <= 9; dy++)
            Graphics::PutPixel(x + dx, y + dy, 0x000000);
            
    uint32_t c_color = (g_cur_click || g_cur_rclick) ? 0xFFFF00 : 0xFFFFFF;
    
    for (int i = 0; i < 9; i++) Graphics::PutPixel(x, y + i, c_color);
    Graphics::PutPixel(x + 1, y + 4, c_color);
    Graphics::PutPixel(x + 2, y + 4, c_color);
    Graphics::PutPixel(x + 3, y + 4, c_color);
    Graphics::PutPixel(x + 1, y + 5, c_color);
    Graphics::PutPixel(x + 2, y + 6, c_color);
    Graphics::PutPixel(x + 3, y + 7, c_color);
    Graphics::PutPixel(x + 4, y + 8, c_color);
}

static bool cursor_over(int32_t bx, int32_t by, int32_t bw, int32_t bh) {
    return g_cur_x >= bx && g_cur_x < bx + bw &&
           g_cur_y >= by && g_cur_y < by + bh;
}

// ============================================================
//  КНОПКА UI
// ============================================================

struct Button { int32_t x, y, w, h; const CHAR16* label; };

static bool button_draw(const Button& b) {
    bool hover   = cursor_over(b.x, b.y, b.w, b.h);
    bool clicked = hover && g_cur_click; 

    uint32_t bg  = hover  ? 0x2255CC : 0x1133AA;
    uint32_t brd = hover  ? 0x88AAFF : 0x3355BB;

    draw_rect(b.x, b.y, b.w, b.h, bg);

    Graphics::DrawLine(b.x, b.y, b.x + b.w - 1, b.y, brd);
    Graphics::DrawLine(b.x, b.y + b.h - 1, b.x + b.w - 1, b.y + b.h - 1, brd);
    Graphics::DrawLine(b.x, b.y, b.x, b.y + b.h - 1, brd);
    Graphics::DrawLine(b.x + b.w - 1, b.y, b.x + b.w - 1, b.y + b.h - 1, brd);

    int32_t tx = b.x + (b.w - str_len(b.label) * 12) / 2;
    int32_t ty = b.y + (b.h - 22) / 2;
    draw_text(tx, ty, 22, 0xFFFFFF, b.label);

    return clicked;
}

// ============================================================
//  СТАРТОВЫЙ ЭКРАН
// ============================================================

static void start_screen() {
    Button btn = { SCR_W / 2 - 160, SCR_H / 2 - 40, 320, 80, u"[ START BENCHMARK ]" };

    for (;;) {
        cursor_update();

        Graphics::Clear(0x05050F);

        draw_text(SCR_W / 2 - 295, 55,  34, 0xFFD700, u"ParrotOS Benchmark Suite");
        draw_text(SCR_W / 2 - 240, 102, 20, 0x5577AA, u"Total ticks for N operations  —  results saved to CSV");

        Graphics::DrawLine(60, 140, SCR_W - 60, 140, 0x223355);

        draw_text(30, SCR_H - 118, 18, 0x445566, u"Cursor:   W/A/S/D or Arrows  — Move Pointer");
        draw_text(30, SCR_H - 94,  18, 0x445566, u"Clicks:   Q / Enter — Left Click (Select)");
        draw_text(30, SCR_H - 70,  18, 0x556677, u"          E         — Right Click");

        bool clicked = button_draw(btn);

        cursor_draw();
        Graphics::SwapBuffers();

        if (clicked) return;
        
         
    }
}

// ============================================================
//  ПРОГРЕСС-БАР
// ============================================================

static void show_progress(int bench, int round) {
    Graphics::Clear(0x0A0A1A);
    draw_text(20, 18, 28, 0xFFD700, u"=== ParrotOS Benchmark Suite ===");

    CHAR16 buf[128]; buf[0] = 0;
    str_cat(buf, u"Benchmark ");
    CHAR16 nb[8]; u64_to_str((uint64_t)bench, nb, 8); str_cat(buf, nb);
    str_cat(buf, u" / 10     Round ");
    u64_to_str((uint64_t)round, nb, 8); str_cat(buf, nb);
    str_cat(buf, u" / 10");
    draw_text(20, 64, 24, 0x00FFAA, buf);

    int32_t total = 100;
    int32_t done  = (bench - 1) * 10 + round;
    int32_t bw    = SCR_W - 80;
    
    draw_rect(40, 106, bw, 22, 0x1A1A2E);
    draw_rect(40, 106, bw * done / total, 22, 0x3399FF);
    
    Graphics::DrawLine(40, 106, 40 + bw - 1, 106, 0x4466AA);
    Graphics::DrawLine(40, 128, 40 + bw - 1, 128, 0x4466AA);

    CHAR16 pct[16]; pct[0] = 0;
    u64_to_str((uint64_t)(done * 100 / total), pct, 16);
    str_cat(pct, u"%");
    draw_text(40 + bw / 2 - 20, 108, 18, 0xFFFFFF, pct);

    draw_text(20, 145, 20, 0x8899AA, u"Please wait — running benchmark...");
    Graphics::SwapBuffers();
}

// ============================================================
//  БЕНЧМАРКИ
// ============================================================

static uint32_t g_sprite[64 * 64];

static void init_sprite() {
    for (int y = 0; y < 64; y++)
        for (int x = 0; x < 64; x++)
            g_sprite[y * 64 + x] = (uint32_t)(x * 4) | ((uint32_t)(y * 4) << 8) | 0x3F0000;
}

static uint64_t bench_gfx_clear() {
    const int N = 20000;
    uint64_t t0 = System::GetTickCount();
    for (int i = 0; i < N; i++) Graphics::Clear(0x1A2B3C + (uint32_t)i * 0x010101);
    return System::GetTickCount() - t0;
}

static uint64_t bench_put_pixel() {
    const int N = 20000;
    uint64_t t0 = System::GetTickCount();
    for (int i = 0; i < N; i++)
        Graphics::PutPixel((int32_t)(i % SCR_W), (int32_t)((i / SCR_W) % SCR_H), 0xFF0000 + (uint32_t)i);
    return System::GetTickCount() - t0;
}

static uint64_t bench_draw_line() {
    const int N = 20000;
    uint64_t t0 = System::GetTickCount();
    for (int i = 0; i < N; i++) {
        int32_t y = (int32_t)((i * 7) % SCR_H);
        Graphics::DrawLine(0, y, SCR_W - 1, SCR_H - 1 - y, 0x00FF00 + (uint32_t)(i * 3));
    }
    return System::GetTickCount() - t0;
}

static uint64_t bench_gfx_print() {
    const int N = 20000;
    int32_t max_x = (SCR_W - 200 > 1) ? SCR_W - 200 : 1;
    int32_t max_y = (SCR_H - 40 > 1)  ? SCR_H - 40  : 1;

    uint64_t t0 = System::GetTickCount();
    for (int i = 0; i < N; i++)
        Graphics::Print((int32_t)((i * 17) % max_x), (int32_t)((i * 13) % max_y), 20, 0xFFFFFF, u"BenchmarkText");
    return System::GetTickCount() - t0;
}

static uint64_t bench_swap_buffers() {
    const int N = 20000;
    uint64_t t0 = System::GetTickCount();
    for (int i = 0; i < N; i++) {
        Graphics::Clear((uint32_t)(i & 0xF) * 0x101010);
        Graphics::SwapBuffers();
    }
    return System::GetTickCount() - t0;
}

static uint64_t bench_mem_alloc() {
    const int N = 20000;
    void* ptrs[64] = {};
    uint64_t t0 = System::GetTickCount();
    for (int i = 0; i < N; i++) {
        int slot = i % 64;
        if (i >= 64 && ptrs[slot]) Memory::Free(ptrs[slot]); 
        ptrs[slot] = Memory::Alloc(1024 + (uint64_t)(i % 512) * 16);
    }
    for (int i = 0; i < 64 && i < N; i++) {
        if (ptrs[i]) Memory::Free(ptrs[i]);
    }
    return System::GetTickCount() - t0;
}

static uint64_t bench_fs_write() {
    const int N = 2000;
    static const CHAR16   fname[]   = u"bench_tmp.dat";
    static const uint16_t payload[] = {
        'B','e','n','c','h','P','a','y','l','o','a','d',
        '0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F',0
    };
    uint64_t t0 = System::GetTickCount();
    for (int i = 0; i < N; i++) {
        FileSystem::CreateFile(fname);
        FileSystem::WriteFile(fname, payload, 27 * sizeof(uint16_t));
        FileSystem::DeleteFile(fname);
    }
    return System::GetTickCount() - t0;
}

static uint64_t bench_syscall() {
    const int N = 20000;
    volatile uint64_t d = 0;
    uint64_t t0 = System::GetTickCount();
    for (int i = 0; i < N; i++) d = System::GetTickCount();
    (void)d;
    return System::GetTickCount() - t0;
}

static uint64_t bench_get_pixel() {
    const int N = 200;
    volatile uint32_t c = 0;
    uint64_t t0 = System::GetTickCount();
    for (int i = 0; i < N; i++)
        c = Graphics::GetPixel((int32_t)(i % SCR_W), (int32_t)((i / SCR_W) % SCR_H));
    (void)c;
    return System::GetTickCount() - t0;
}

static uint64_t bench_draw_bitmap() {
    const int N = 20000;
    int32_t max_x = (SCR_W - 64 > 1) ? SCR_W - 64 : 1;
    int32_t max_y = (SCR_H - 64 > 1) ? SCR_H - 64 : 1;

    uint64_t t0 = System::GetTickCount();
    for (int i = 0; i < N; i++)
        Graphics::DrawBitmap(g_sprite, (int32_t)((i * 19) % max_x), (int32_t)((i * 13) % max_y), 64, 64);
    return System::GetTickCount() - t0;
}

static uint64_t run_benchmark(int b) {
    switch (b) {
        case 0: return bench_gfx_clear();
        case 1: return bench_put_pixel();
        case 2: return bench_draw_line();
        case 3: return bench_gfx_print();
        case 4: return bench_swap_buffers();
        case 5: return bench_mem_alloc();
        case 6: return bench_fs_write();
        case 7: return bench_syscall();
        case 8: return bench_get_pixel();
        case 9: return bench_draw_bitmap();
        default: return 0;
    }
}

static const CHAR16* get_bench_name(int b) {
    switch (b) {
        case 0: return u"GfxClear";
        case 1: return u"PutPixel";
        case 2: return u"DrawLine";
        case 3: return u"GfxPrint";
        case 4: return u"SwapBuffers";
        case 5: return u"MemAllocFree";
        case 6: return u"FS_Write";
        case 7: return u"SyscallTick";
        case 8: return u"GetPixel";
        case 9: return u"DrawBitmap";
        default: return u"Unknown";
    }
}

static const CHAR16 RESULT_FILE[] = u"bench_results.csv";

static void save_results(uint64_t results[10][10]) {
    g_report_len = 0; g_report[0] = 0;

    for (int b = 0; b < 10; b++) {
        report_append(get_bench_name(b));
        report_append(b < 9 ? u"," : u"\r\n");
    }
    for (int r = 0; r < 10; r++) {
        for (int b = 0; b < 10; b++) {
            CHAR16 nb[24]; u64_to_str(results[b][r], nb, 24);
            report_append(nb);
            report_append(b < 9 ? u"," : u"\r\n");
        }
    }
    report_append(u"AVG,");
    for (int b = 0; b < 10; b++) {
        uint64_t sum = 0;
        for (int r = 0; r < 10; r++) sum += results[b][r];
        CHAR16 nb[24]; u64_to_str(sum / 10, nb, 24);
        report_append(nb);
        report_append(b < 9 ? u"," : u"\r\n");
    }

    FileSystem::CreateFile(RESULT_FILE);
    FileSystem::WriteFile(RESULT_FILE, (const uint16_t*)g_report, g_report_len * sizeof(CHAR16));
}

// ============================================================
//  ФИНАЛЬНЫЙ ЭКРАН РЕЗУЛЬТАТОВ
// ============================================================

static bool result_screen(uint64_t results[10][10]) {
    Button btn_again = { 80,          SCR_H - 90, 240, 58, u"Run Again" };
    Button btn_exit  = { SCR_W - 320, SCR_H - 90, 240, 58, u"Exit"      };

    for (;;) {
        cursor_update();

        Graphics::Clear(0x050510);
        draw_text(20, 10, 28, 0xFFD700, u"=== RESULTS  (Total Ticks) ===");
        draw_text(20, 48, 18, 0x445566, u"Results saved  ->  bench_results.csv");

        Graphics::DrawLine(20, 76, SCR_W - 20, 76, 0x1A2A3A);

        int32_t col1 = 20, col2 = 220, col3 = SCR_W / 2 + 20, col4 = SCR_W / 2 + 220;
        int32_t y = 88;

        for (int b = 0; b < 10; b++) {
            uint64_t sum = 0;
            for (int r = 0; r < 10; r++) sum += results[b][r];
            uint64_t avg = sum / 10;

            bool left = (b < 5);
            int32_t cx = left ? col1 : col3;
            int32_t vx = left ? col2 : col4;
            int32_t ry = left ? (88 + b * 26) : (88 + (b - 5) * 26);

            if (b % 2 == 0) draw_rect(10, ry - 2, SCR_W - 20, 24, 0x0C0C1C);

            draw_text(cx, ry, 20, 0x88CCFF, get_bench_name(b));

            CHAR16 nb[24]; u64_to_str(avg, nb, 24);
            CHAR16 val[32]; val[0] = 0;
            str_cat(val, nb); str_cat(val, u" tck");
            draw_text(vx, ry, 20, 0x00FFAA, val);
        }

        y = 88 + 5 * 26 + 10;
        Graphics::DrawLine(SCR_W / 2, 80, SCR_W / 2, y, 0x1A2A3A);

        button_draw(btn_again);
        bool again  = btn_again.w > 0 && cursor_over(btn_again.x, btn_again.y, btn_again.w, btn_again.h) && g_cur_click;
        
        button_draw(btn_exit);
        bool quit   = btn_exit.w  > 0 && cursor_over(btn_exit.x,  btn_exit.y,  btn_exit.w,  btn_exit.h)  && g_cur_click;

        cursor_draw();
        Graphics::SwapBuffers();

        if (again) return true;
        if (quit)  return false;
    }
}

// ============================================================
//  ТОЧКА ВХОДА (MAIN)
// ============================================================

void main(struct ParrotOS::Process* /*pr*/) {
    Graphics::SetFont(u"SysFont");
    
    Graphics::GetScreenSize(SCR_W, SCR_H);
    if (SCR_W <= 0) SCR_W = 1024;
    if (SCR_H <= 0) SCR_H = 768;

    init_sprite();
    cursor_init();

    uint64_t results[10][10];

    for (;;) {
        start_screen();

        for (int b = 0; b < 10; b++) {
            for (int r = 0; r < 10; r++) {
                show_progress(b + 1, r + 1);
                results[b][r] = run_benchmark(b); 
            }
        }

        save_results(results);

        bool again = result_screen(results);
        if (!again) break;

        cursor_init();
    }

    Graphics::Clear(0x000000);
    draw_text(SCR_W / 2 - 330, SCR_H / 2 - 14, 22, 0xAABBCC, u"Done. Results saved to bench_results.csv  —  Goodbye!");
    Graphics::SwapBuffers();
    TaskManager::Exit();
}