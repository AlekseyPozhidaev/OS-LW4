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

    std::string logName = "Reader_" + std::to_string(GetCurrentProcessId()) + ".csv";
    std::ofstream log(logName);

    for (int i = 0; i < ITERATIONS; i++) {
        int page = rand() % PAGE_COUNT;
        char mutName[64], semName[64];
        sprintf_s(mutName, "%s%d", MUTEX_PREFIX, page);
        sprintf_s(semName, "%s%d", SEM_PREFIX, page);
        
        HANDLE hCountMut = CreateMutexA(NULL, FALSE, mutName);
        HANDLE hWriteSem = CreateSemaphoreA(NULL, 1, 1, semName);

        // 1. WAIT
        LogCsv(log, 0, page);

        WaitForSingleObject(hCountMut, INFINITE);
        header->readers_on_page[page]++;
        if (header->readers_on_page[page] == 1) {
            WaitForSingleObject(hWriteSem, INFINITE);
        }
        ReleaseMutex(hCountMut);

        // 2. ACTIVE
        LogCsv(log, 1, page);

                char* pagePtr = (char*)pBuf + sizeof(ControlHeader) + page * si.dwPageSize;
        char readBuffer[256];
        strncpy_s(readBuffer, pagePtr, sizeof(readBuffer) - 1);
        readBuffer[255] = '\0';
        std::cout << "Reader " << GetCurrentProcessId() << " read from page " << page << ": " << readBuffer << std::endl;

        RandomSleep();

        // 3. RELEASE
        LogCsv(log, 2, page); // LOG BEFORE RELEASING

        WaitForSingleObject(hCountMut, INFINITE);
        header->readers_on_page[page]--;
        if (header->readers_on_page[page] == 0) {
            ReleaseSemaphore(hWriteSem, 1, NULL);
        }
        ReleaseMutex(hCountMut);

        CloseHandle(hCountMut);
        CloseHandle(hWriteSem);
        Sleep(100);
    }

    UnmapViewOfFile(pBuf);
    CloseHandle(hMapFile);
    CloseHandle(hFile); // Не забудьте закрыть хендл файла
    return 0;
}