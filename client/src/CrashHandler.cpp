#include "../include/CrashHandler.h"
#include <stdio.h>
#include <time.h>
#include <dbghelp.h>

#pragma comment(lib, "dbghelp.lib")

namespace MadMultiplayer {

static void WriteMiniDump(EXCEPTION_POINTERS* pException, const char* dumpPath) {
    HANDLE hFile = CreateFileA(dumpPath, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile != INVALID_HANDLE_VALUE) {
        MINIDUMP_EXCEPTION_INFORMATION mei;
        mei.ThreadId = GetCurrentThreadId();
        mei.ExceptionPointers = pException;
        mei.ClientPointers = FALSE;

        MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hFile, MiniDumpNormal, &mei, NULL, NULL);
        CloseHandle(hFile);
    }
}

LONG WINAPI CrashHandler::ExceptionFilter(EXCEPTION_POINTERS* pException)
{
    if (!pException || !pException->ExceptionRecord) return EXCEPTION_CONTINUE_SEARCH;

    DWORD code = pException->ExceptionRecord->ExceptionCode;
    PVOID addr = pException->ExceptionRecord->ExceptionAddress;
    CONTEXT* ctx = pException->ContextRecord;

    char moduleName[MAX_PATH] = "Unknown Module";
    HMODULE hMod = NULL;
    if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCSTR)addr, &hMod)) {
        GetModuleFileNameA(hMod, moduleName, MAX_PATH);
    }

    uintptr_t offset = (uintptr_t)addr - (uintptr_t)hMod;

    FILE* f = nullptr;
    fopen_s(&f, "crash_report.log", "w");
    if (f) {
        time_t now = time(NULL);
        char timeBuf[64];
        ctime_s(timeBuf, sizeof(timeBuf), &now);

        fprintf(f, "==================================================================\n");
        fprintf(f, " MADAGASCAR MULTIPLAYER - CRASH REPORT\n");
        fprintf(f, " Time: %s", timeBuf);
        fprintf(f, " Exception Code:    0x%08X\n", code);
        fprintf(f, " Fault Address:     0x%p (Module: %s + 0x%X)\n", addr, moduleName, (unsigned int)offset);
        
        if (code == EXCEPTION_ACCESS_VIOLATION && pException->ExceptionRecord->NumberParameters >= 2) {
            ULONG_PTR accessType = pException->ExceptionRecord->ExceptionInformation[0];
            ULONG_PTR targetAddr = pException->ExceptionRecord->ExceptionInformation[1];
            fprintf(f, " Details:           Access Violation attempting to %s address 0x%p\n",
                (accessType == 0 ? "READ from" : (accessType == 1 ? "WRITE to" : "EXECUTE")), (void*)targetAddr);
        }

        if (ctx) {
            fprintf(f, "\nCPU REGISTERS:\n");
            fprintf(f, " EAX: 0x%08X  EBX: 0x%08X  ECX: 0x%08X  EDX: 0x%08X\n", ctx->Eax, ctx->Ebx, ctx->Ecx, ctx->Edx);
            fprintf(f, " ESI: 0x%08X  EDI: 0x%08X  EBP: 0x%08X  ESP: 0x%08X\n", ctx->Esi, ctx->Edi, ctx->Ebp, ctx->Esp);
            fprintf(f, " EIP: 0x%08X  EFLAGS: 0x%08X\n", ctx->Eip, ctx->EFlags);
        }

        fprintf(f, "==================================================================\n");
        fclose(f);
    }

    WriteMiniDump(pException, "crash_dump.dmp");
    fflush(stdout);

    return EXCEPTION_CONTINUE_SEARCH;
}

void CrashHandler::Install() {
    SetUnhandledExceptionFilter(CrashHandler::ExceptionFilter);
}

} // namespace MadMultiplayer
