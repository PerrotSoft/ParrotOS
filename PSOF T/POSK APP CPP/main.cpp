#include "ParrotOS_API.hpp"
using namespace ParrotOS;
void main(struct ParrotOS::Process* pr) {
    Graphics::SetFont((CHAR16*)L"SysFont");
    Graphics::Print(100,100,40,0xFFFFFF,(const CHAR16*)L"Hello, World!");
    Graphics::SwapBuffers();
}