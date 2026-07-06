// console.c
#include "console.h"

static COLORED_CHAR screen_buffer[MAX_BUFFER_ROWS][MAX_BUFFER_COLS];
static int cursor_x = 0;
static int cursor_y = 0;

static uint32_t global_fg = 0xCCCCCC;
static uint32_t global_bg = 0x050505;

static int console_cols = 80;
static int console_rows = 25;
static int char_w = 4;
static int char_h = 8;
static int font_size = 10;

#define OFFSET_X 10
#define OFFSET_Y 10

// -------------------------------------------------------------------
// Скан-коды Shift (стандарт PS/2 Set 1, то что обычно приходит в UEFI)
// Left Shift  = 0x2A,  Right Shift = 0x36
// При отпускании приходит 0x80 | скан-код:
//   Left Shift release  = 0xAA
//   Right Shift release = 0xB6
// Если твой KbdGetKey возвращает что-то другое — замени эти константы.
// -------------------------------------------------------------------
#define KEY_LSHIFT         0x2A
#define KEY_RSHIFT         0x36
#define KEY_LSHIFT_RELEASE 0xAA
#define KEY_RSHIFT_RELEASE 0xB6

static int shift_pressed = 0;

// Таблица перевода символа при зажатом Shift.
// Покрывает стандартную US-раскладку.
static CHAR16 ApplyShift(CHAR16 c) {
    // Цифры → символы над ними
    switch (c) {
        case L'1': return L'!';
        case L'2': return L'@';
        case L'3': return L'#';
        case L'4': return L'$';
        case L'5': return L'%';
        case L'6': return L'^';
        case L'7': return L'&';
        case L'8': return L'*';
        case L'9': return L'(';
        case L'0': return L')';
        case L'-': return L'_';
        case L'=': return L'+';
        case L'[': return L'{';
        case L']': return L'}';
        case L'\\':return L'|';
        case L';': return L':';
        case L'\'':return L'"';
        case L',': return L'<';
        case L'.': return L'>';
        case L'/': return L'?';
        case L'`': return L'~';
    }
    // Буквы → верхний регистр
    if (c >= L'a' && c <= L'z') return c - L'a' + L'A';
    // Всё остальное — без изменений
    return c;
}

void ConsoleInit(uint32_t screen_width, uint32_t screen_height) {
    if (screen_width >= 1920) {
        font_size = 24;
        char_w = 14;
        char_h = 28;
    } else if (screen_width >= 1024) {
        font_size = 16;
        char_w = 9;
        char_h = 18;
    } else {
        font_size = 10;
        char_w = 6;
        char_h = 12;
    }

    console_cols = (screen_width  - (OFFSET_X * 2)) / char_w;
    console_rows = (screen_height - (OFFSET_Y * 2)) / char_h;

    if (console_cols > MAX_BUFFER_COLS) console_cols = MAX_BUFFER_COLS;
    if (console_rows > MAX_BUFFER_ROWS) console_rows = MAX_BUFFER_ROWS;

    ConsoleClear();
}

void ConsoleClear() {
    for (int y = 0; y < console_rows; y++) {
        for (int x = 0; x < console_cols; x++) {
            screen_buffer[y][x].Char = 0;
            screen_buffer[y][x].FG   = global_fg;
            screen_buffer[y][x].BG   = global_bg;
        }
    }
    cursor_x = 0;
    cursor_y = 0;
}

void ScrollUp() {
    for (int y = 0; y < console_rows - 1; y++) {
        for (int x = 0; x < console_cols; x++) {
            screen_buffer[y][x] = screen_buffer[y + 1][x];
        }
    }
    for (int x = 0; x < console_cols; x++) {
        screen_buffer[console_rows - 1][x].Char = 0;
        screen_buffer[console_rows - 1][x].FG   = global_fg;
        screen_buffer[console_rows - 1][x].BG   = global_bg;
    }
    cursor_y = console_rows - 1;
}

void PrintString(const CHAR16* str, uint32_t fg, uint32_t bg) {
    while (*str) PrintChar(*str++, fg, bg);
}

void PrintChar(CHAR16 c, uint32_t fg, uint32_t bg) {
    if (c == (CHAR16)'\r') { cursor_x = 0; return; }

    if (c == (CHAR16)'\n') {
        cursor_x = 0;
        cursor_y++;
        if (cursor_y >= console_rows) ScrollUp();
        return;
    }

    if (c == (CHAR16)'\t') {
        int spaces = 4 - (cursor_x % 4);
        for (int i = 0; i < spaces; i++) PrintChar((CHAR16)' ', fg, bg);
        return;
    }

    if (c == 0x08 || c == 127) {
        if (cursor_x > 0) {
            cursor_x--;
        } else if (cursor_y > 0) {
            cursor_y--;
            cursor_x = console_cols - 1;
        }
        screen_buffer[cursor_y][cursor_x].Char = 0;
        screen_buffer[cursor_y][cursor_x].FG   = global_fg;
        screen_buffer[cursor_y][cursor_x].BG   = global_bg;
        return;
    }

    if (cursor_x >= console_cols) {
        cursor_x = 0;
        cursor_y++;
        if (cursor_y >= console_rows) ScrollUp();
    }

    screen_buffer[cursor_y][cursor_x].Char = c;
    screen_buffer[cursor_y][cursor_x].FG   = fg;
    screen_buffer[cursor_y][cursor_x].BG   = bg;
    cursor_x++;

    if (cursor_x >= console_cols) {
        cursor_x = 0;
        cursor_y++;
        if (cursor_y >= console_rows) ScrollUp();
    }
}

void RenderConsole() {
    GfxClear(0x000000);

    for (int y = 0; y < console_rows; y++) {
        for (int x = 0; x < console_cols; x++) {
            COLORED_CHAR cell = screen_buffer[y][x];

            if (cell.BG != global_bg) {
                for (int h = 0; h < char_h; h++) {
                    GfxDrawLine(OFFSET_X + (x * char_w),
                                OFFSET_Y + (y * char_h) + h,
                                OFFSET_X + ((x + 1) * char_w) - 1,
                                OFFSET_Y + (y * char_h) + h,
                                cell.BG);
                }
            }

            if (cell.Char != 0 && cell.Char != (CHAR16)' ') {
                CHAR16 buf[2] = { cell.Char, 0 };
                GfxPrint(OFFSET_X + (x * char_w),
                         OFFSET_Y + (y * char_h),
                         font_size, cell.FG, buf);
            }
        }
    }

    uint32_t cursor_vis_x = OFFSET_X + (cursor_x * char_w);
    uint32_t cursor_vis_y = OFFSET_Y + (cursor_y * char_h) + (char_h - 2);
    GfxDrawLine(cursor_vis_x, cursor_vis_y,
                cursor_vis_x + char_w - 1, cursor_vis_y, 0xFFFFFF);
    GfxDrawLine(cursor_vis_x, cursor_vis_y + 1,
                cursor_vis_x + char_w - 1, cursor_vis_y + 1, 0xFFFFFF);

    SB();
}

// -------------------------------------------------------------------
// ReadChar — единственное место где обрабатывается Shift.
// Shift не возвращается наружу — он только переключает флаг shift_pressed.
// Все остальные символы при зажатом Shift проходят через ApplyShift().
// -------------------------------------------------------------------
CHAR16 ReadChar() {
    while (1) {
        if (!KbdHasKey()) continue;

        CHAR16 raw = (CHAR16)KbdGetKey();

        // Нажатие Shift
        if (raw == KEY_LSHIFT || raw == KEY_RSHIFT) {
            shift_pressed = 1;
            continue;  // не возвращаем, ждём следующую клавишу
        }

        // Отпускание Shift
        if (raw == KEY_LSHIFT_RELEASE || raw == KEY_RSHIFT_RELEASE) {
            shift_pressed = 0;
            continue;  // не возвращаем
        }

        // Обычная клавиша — применяем Shift если нажат
        return shift_pressed ? ApplyShift(raw) : raw;
    }
}

CHAR16* Read() {
    static CHAR16 buffer[256];
    int index = 0;

    while (1) {
        CHAR16 c = ReadChar();

        if (c == (CHAR16)'\n' || c == (CHAR16)'\r') {
            buffer[index] = 0;
            return buffer;
        }
        else if (c == 0x08 || c == 127) {
            if (index > 0) {
                index--;
                PrintChar(0x08, global_fg, global_bg);
                RenderConsole();
            }
        }
        else if (index < 255) {
            buffer[index++] = c;
            PrintChar(c, global_fg, global_bg);
            RenderConsole();
        }
    }
}

int    GetCursorX()          { return cursor_x; }
int    GetCursorY()          { return cursor_y; }
int    IsShiftPressed()     { return shift_pressed; }

void SetCursorPos(int x, int y) {
    if (x >= 0 && x < console_cols) cursor_x = x;
    if (y >= 0 && y < console_rows) cursor_y = y;
}

CHAR16* ReadLine() {
    CHAR16* line = Read();
    PrintChar((CHAR16)'\n', global_fg, global_bg);
    RenderConsole();
    return line;
}
