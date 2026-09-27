#pragma once
#include <windows.h>

namespace MadMultiplayer {
    class CrashHandler {
    public:
        static void Install();
        static LONG WINAPI ExceptionFilter(EXCEPTION_POINTERS* pException);
    };
}
