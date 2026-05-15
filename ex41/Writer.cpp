#include "Common.h"

int main() {
    srand((unsigned int)time(NULL) ^ GetCurrentProcessId());

    SYSTEM_INFO si;
    GetSystemInfo(&si);
    DWORD totalSize = sizeof(ControlHeader) + (PAGE_COUNT * si.dwPageSize);

    HANDLE hFile = NULL;
    HANDLE hMapFile = NULL;

    void* pBuf = MapSharedFile(hFile, hMapFile, totalSize);
    if (!pBuf) {
        std::cerr << "Mapping failed\n";
        return 1;
    }

    VirtualLock(pBuf, totalSize);
    ControlHeader* header = (ControlHeader*)pBuf;

    // Инициализация заголовка (только один раз)
    HANDLE hControlMutex = GetControlMutex();
    WaitForSingleObject(hControlMutex, INFINITE);
    if (header->initialized != 0x1234) {
        for (int i = 0; i < PAGE_COUNT; ++i) {
            header->readers_on_page[i] = 0;
            header->page_state[i] = READY_FOR_WRITE;
            header->writer_active[i] = 0;
        }
        header->initialized = 0x1234;
    }
    ReleaseMutex(hControlMutex);

    // Очистка страниц
    for (int i = 0; i < PAGE_COUNT; ++i) {
        char* pagePtr = (char*)pBuf + sizeof(ControlHeader) + i * si.dwPageSize;
        memset(pagePtr, 0, si.dwPageSize);
    }

    std::string logName = "Writer_" + std::to_string(GetCurrentProcessId()) + ".csv";
    std::ofstream log(logName);

    for (int i = 0; i < ITERATIONS; i++) {

        int page = -1;

        LogCsv(log, 0, page);

        do {
            page = AcquireSuitablePage(header, true);   // true = writer
            if (page == -1) Sleep(1);
        } while (page == -1);

        char semName[64];
        sprintf_s(semName, "%s%d", SEM_PREFIX, page);
        HANDLE hWriteSem = CreateSemaphoreA(NULL, 1, 1, semName);

        WaitForSingleObject(hWriteSem, INFINITE);

        LogCsv(log, 1, page);                 // ACTIVE

        char* pagePtr = (char*)pBuf + sizeof(ControlHeader) + page * si.dwPageSize;
        char buffer[256];
        DWORD tick = timeGetTime();
        sprintf_s(buffer, "Writer %d at %u", GetCurrentProcessId(), tick);
        strcpy_s(pagePtr, si.dwPageSize, buffer);
        std::cout << "Writer " << GetCurrentProcessId() << " wrote to page " << page << ": " << buffer << std::endl;

        RandomSleep();

        // После записи меняем состояние страницы
        WaitForSingleObject(hControlMutex, INFINITE);
        header->writer_active[page] = 0;
        header->page_state[page] = READY_FOR_READ;
        ReleaseMutex(hControlMutex);

        LogCsv(log, 2, page);                 // RELEASE
        ReleaseSemaphore(hWriteSem, 1, NULL);

        CloseHandle(hWriteSem);
        Sleep(100);
    }

    UnmapViewOfFile(pBuf);
    CloseHandle(hMapFile);
    CloseHandle(hFile);
    return 0;
}