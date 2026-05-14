#include "Common.h"

int main() {
    srand((unsigned int)time(NULL) ^ GetCurrentProcessId());

    SYSTEM_INFO si;
    GetSystemInfo(&si);
    DWORD totalSize = sizeof(ControlHeader) + (PAGE_COUNT * si.dwPageSize);

    HANDLE hFile = NULL;
    HANDLE hMapFile = NULL;

    // Вместо старого CreateFileMappingA и MapViewOfFile:
    void* pBuf = MapSharedFile(hFile, hMapFile, totalSize);
    if (!pBuf) {
        std::cerr << "Mapping failed\n";
        return 1;
    }

    VirtualLock(pBuf, totalSize);
    ControlHeader* header = (ControlHeader*)pBuf;

    for (int i = 0; i < PAGE_COUNT; ++i) {
        char* pagePtr = (char*)pBuf + sizeof(ControlHeader) + i * si.dwPageSize;
        memset(pagePtr, 0, si.dwPageSize);
    }

    std::string logName = "Writer_" + std::to_string(GetCurrentProcessId()) + ".csv";
    std::ofstream log(logName);

    for (int i = 0; i < ITERATIONS; i++) {
        int page = rand() % PAGE_COUNT;
        char semName[64];
        sprintf_s(semName, "%s%d", SEM_PREFIX, page);
        HANDLE hWriteSem = CreateSemaphoreA(NULL, 1, 1, semName);

        // 1. WAIT
        LogCsv(log, 0, page);
        WaitForSingleObject(hWriteSem, INFINITE);

        // 2. ACTIVE
        LogCsv(log, 1, page);

                char* pagePtr = (char*)pBuf + sizeof(ControlHeader) + page * si.dwPageSize;
        char buffer[256];
        DWORD tick = timeGetTime();
        sprintf_s(buffer, "Writer %d at %u", GetCurrentProcessId(), tick);
        strcpy_s(pagePtr, si.dwPageSize, buffer);
        std::cout << "Writer " << GetCurrentProcessId() << " wrote to page " << page << ": " << buffer << std::endl;

        RandomSleep();

        // 3. RELEASE (IMPORTANT: Log BEFORE releasing)
        LogCsv(log, 2, page);
        ReleaseSemaphore(hWriteSem, 1, NULL);

        CloseHandle(hWriteSem);
        Sleep(100);
    }

    UnmapViewOfFile(pBuf);
    CloseHandle(hMapFile);
    CloseHandle(hFile); // Не забудьте закрыть хендл файла
    return 0;
}