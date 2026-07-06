#include <Library/UefiBootServicesTableLib.h>
#include "../include/drivers/DriverManager.h"
#include "../include/drivers/Audiodrv.h"

// =====================================================================
// 1. КОНСТАНТЫ И РЕГИСТРЫ INTEL HIGH DEFINITION AUDIO (HDA)
// =====================================================================
#define PCI_CONFIG_ADDRESS  0xCF8
#define PCI_CONFIG_DATA     0xCFC

// Регистры Intel HDA контроллера (смещение относительно BAR0)
#define HDA_REG_GCAP        0x00 // Global Capabilities
#define HDA_REG_GCTL        0x08 // Global Control
#define HDA_REG_STATESTS    0x0E // State Status
#define HDA_REG_INTCTL      0x20 // Interrupt Control
#define HDA_REG_INTSTS      0x24 // Interrupt Status

// Смещение для выходного потока (Output Stream 0)
#define HDA_SD_BASE         0x80 
#define HDA_SD_CTL          0x00 // Stream Control (3 байта)
#define HDA_SD_STS          0x03 // Stream Status
#define HDA_SD_LPIB         0x04 // Link Position In Buffer
#define HDA_SD_CBL          0x08 // Cyclic Buffer Length
#define HDA_SD_LVI          0x0C // Last Valid Index
#define HDA_SD_FIFOSIZE     0x10 // FIFO Size
#define HDA_SD_FORMAT       0x12 // Stream Format
#define HDA_SD_BDPL         0x18 // Buffer Descriptor List Pointer Lower
#define HDA_SD_BDPU         0x1C // Buffer Descriptor List Pointer Upper

// Структура дескриптора буфера (BDL Entry) для DMA передачи
typedef struct {
    UINT32 AddressLow;
    UINT32 AddressHigh;
    UINT32 Length;
    UINT32 Flags; // Bit 0 = Interrupt on Completion
} __attribute__((packed)) HDA_BDL_ENTRY;

// Выделяем статический BDL лист и структуру управления
static HDA_BDL_ENTRY g_BdlList[2] __attribute__((aligned(128)));
static UINT64 g_HdaBar0 = 0;
static BOOLEAN g_HdaAvailable = FALSE;

// =====================================================================
// 2. ВЗАИМОДЕЙСТВИЕ С ШИНОЙ PCI И ПОРТАМИ
// =====================================================================
static inline void outl(UINT16 port, UINT32 val) {
    __asm__ volatile ("outl %0, %1" : : "a"(val), "Nd"(port));
}

static inline UINT32 inl(UINT16 port) {
    UINT32 ret;
    __asm__ volatile ("inl %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void outb(UINT16 port, UINT8 val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline UINT8 inb(UINT16 port) {
    UINT8 ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

// Чтение конфигурационного пространства PCI
UINT32 PciReadConfigDword(UINT8 bus, UINT8 slot, UINT8 func, UINT8 offset) {
    UINT32 address = (UINT32)((UINT32)bus << 16) | ((UINT32)slot << 11) |
                     ((UINT32)func << 8) | (offset & 0xFC) | ((UINT32)0x80000000);
    outl(PCI_CONFIG_ADDRESS, address);
    return inl(PCI_CONFIG_DATA);
}

// Запись в конфигурационное пространство PCI
void PciWriteConfigDword(UINT8 bus, UINT8 slot, UINT8 func, UINT8 offset, UINT32 val) {
    UINT32 address = (UINT32)((UINT32)bus << 16) | ((UINT32)slot << 11) |
                     ((UINT32)func << 8) | (offset & 0xFC) | ((UINT32)0x80000000);
    outl(PCI_CONFIG_ADDRESS, address);
    outl(PCI_CONFIG_DATA, val);
}

// =====================================================================
// 3. УПРАВЛЕНИЕ PC SPEAKER (ПИЩАЛКА)
// =====================================================================
VOID AudioBeepStart(UINT32 Freq) {
    if (Freq == 0) return;
    UINT32 Div = 1193180 / Freq;
    outb(0x43, 0xB6);
    outb(0x42, (UINT8)(Div & 0xFF));
    outb(0x42, (UINT8)((Div >> 8) & 0xFF));
    UINT8 tmp = inb(0x61);
    if (tmp != (tmp | 3)) outb(0x61, tmp | 3);
}

VOID AudioBeepStop() {
    outb(0x61, inb(0x61) & 0xFC);
}

VOID AudioBeepImp(UINT32 Freq, UINT32 DurationMs) {
    AudioBeepStart(Freq);
    if (gBS && gBS->Stall) {
        gBS->Stall(DurationMs * 1000);
        AudioBeepStop();
    }
}

// =====================================================================
// 4. ДРАЙВЕР ВОСПРОИЗВЕДЕНИЯ МАССИВОВ (Intel HDA DMA Engine)
// =====================================================================

// Поиск Intel HDA контроллера на шине PCI
BOOLEAN FindHdaController() {
    for (UINT16 bus = 0; bus < 256; bus++) {
        for (UINT8 slot = 0; slot < 32; slot++) {
            for (UINT8 func = 0; func < 8; func++) {
                UINT32 id = PciReadConfigDword((UINT8)bus, slot, func, 0x00);
                if (id == 0xFFFFFFFF) continue;

                UINT32 class_rev = PciReadConfigDword((UINT8)bus, slot, func, 0x08);
                UINT8 base_class = (class_rev >> 24) & 0xFF;
                UINT8 sub_class  = (class_rev >> 16) & 0xFF;

                // Базовый класс 0x04 (Multimedia), Подкласс 0x03 (Audio Device)
                if (base_class == 0x04 && sub_class == 0x03) {
                    // Читаем BAR0 (Базовый адрес памяти контроллера)
                    UINT32 bar0_low = PciReadConfigDword((UINT8)bus, slot, func, 0x10);
                    g_HdaBar0 = bar0_low & 0xFFFFFFF0; // Маскируем флаги
                    
                    // Включаем Bus Mastering и Memory Space в командном регистре PCI
                    UINT32 cmd = PciReadConfigDword((UINT8)bus, slot, func, 0x04);
                    PciWriteConfigDword((UINT8)bus, slot, func, 0x04, cmd | 0x06);
                    
                    return TRUE;
                }
            }
        }
    }
    return FALSE;
}

// Функция прямой записи в MMIO регистры HDA
static inline void HdaWrite32(UINT32 reg, UINT32 val) {
    *(volatile UINT32*)(g_HdaBar0 + reg) = val;
}
static inline void HdaWrite16(UINT32 reg, UINT16 val) {
    *(volatile UINT16*)(g_HdaBar0 + reg) = val;
}
static inline void HdaWrite8(UINT32 reg, UINT8 val) {
    *(volatile UINT8*)(g_HdaBar0 + reg) = val;
}
static inline UINT32 HdaRead32(UINT32 reg) {
    return *(volatile UINT32*)(g_HdaBar0 + reg);
}

// Функция воспроизведения сырого PCM массива байт
EFI_STATUS DriverPlayRaw(UINT8 *Buffer, UINTN Size) {
    if (!g_HdaAvailable || !Buffer || Size == 0) {
        return EFI_UNSUPPORTED;
    }

    UINT64 stream_reg_base = HDA_SD_BASE; // Output Stream 0

    // 1. Останавливаем поток, если он запущен
    HdaWrite8(stream_reg_base + HDA_SD_CTL, 0);

    // 2. Настраиваем BDL (список описателей буфера) для DMA
    g_BdlList[0].AddressLow  = (UINT32)((UINTN)Buffer & 0xFFFFFFFF);
    g_BdlList[0].AddressHigh = (UINT32)(((UINTN)Buffer >> 32) & 0xFFFFFFFF);
    g_BdlList[0].Length      = (UINT32)Size;
    g_BdlList[0].Flags       = 1; // Прерывание по завершению фрагмента

    // 3. Передаем адрес BDL листа контроллеру
    UINTN bdl_addr = (UINTN)&g_BdlList[0];
    HdaWrite32(stream_reg_base + HDA_SD_BDPL, (UINT32)(bdl_addr & 0xFFFFFFFF));
    HdaWrite32(stream_reg_base + HDA_SD_BDPU, (UINT32)((bdl_addr >> 32) & 0xFFFFFFFF));

    // 4. Задаем параметры потока
    HdaWrite32(stream_reg_base + HDA_SD_CBL, (UINT32)Size); // Длина циклического буфера
    HdaWrite16(stream_reg_base + HDA_SD_LVI, 0);            // Последний валидный индекс дескриптора (у нас 1 элемент, индекс 0)

    // Формат аудио: 44100 Гц, 16 бит, Стерео (Стандартный PCM) -> маска 0x4011
    HdaWrite16(stream_reg_base + HDA_SD_FORMAT, 0x4011);

    // Устанавливаем ID потока равным 1
    UINT8 ctl = HdaRead32(stream_reg_base + HDA_SD_CTL) & 0xFF;
    ctl |= (1 << 4); 
    HdaWrite8(stream_reg_base + HDA_SD_CTL, ctl);

    // 5. Запускаем DMA стрим (Бит RUN = 1)
    HdaWrite8(stream_reg_base + HDA_SD_CTL, HdaRead32(stream_reg_base + HDA_SD_CTL) | 0x02);

    return EFI_SUCCESS;
}

VOID InitSimpleAudio() {
    if (FindHdaController()) {
        g_HdaAvailable = TRUE;

        // ПРАВИЛЬНЫЙ АППАРАТНЫЙ СБРОС (Hardware Reset Sequence)
        UINT32 gctl = HdaRead32(HDA_REG_GCTL);
        HdaWrite32(HDA_REG_GCTL, gctl & ~0x01); // Опускаем бит Reset (уводим в сброс)
        
        // Ждем, пока железо реально не уйдет в сброс
        while (HdaRead32(HDA_REG_GCTL) & 0x01) {
            if (gBS) gBS->Stall(100); 
        }

        if (gBS) gBS->Stall(10000); // Даем железу 10 мс передохнуть

        HdaWrite32(HDA_REG_GCTL, HdaRead32(HDA_REG_GCTL) | 0x01); // Поднимаем бит Reset (включаем)
        
        // Ждем, пока железо не проснется
        UINT32 timeout = 500; // 50 мс таймаут
        while ((HdaRead32(HDA_REG_GCTL) & 0x01) == 0 && timeout > 0) {
            if (gBS) gBS->Stall(100);
            timeout--;
        }
    }

    static AUDIO_DRIVER_IF audio_if;
    audio_if.Beep = AudioBeepImp;
    audio_if.PlayRaw = DriverPlayRaw;

    DRIVER d;
    d.Type = DRIVER_TYPE_AUDIO;
    d.Priority = 5;
    d.Interface = &audio_if;
    
    RegisterDriver(&d);
}