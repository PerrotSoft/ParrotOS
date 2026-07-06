#include "ParrotOS_API.h"

void main(struct Process* pr) {
    GfxSetFont((CHAR16*)L"SysFont");
    GfxPrint(100,100,40,0xFFFFFF,"Hello, World!");
}
