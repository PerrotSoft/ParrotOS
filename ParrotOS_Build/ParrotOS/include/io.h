#pragma once
#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
static inline void outb(UINT16 port, UINT8 val) {
    __asm__ volatile ( "outb %0, %1" : : "a"(val), "Nd"(port) );
}