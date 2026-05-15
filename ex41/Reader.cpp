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

    std::string logName = "Reader_" + std::to_string(GetCurrentProcessId()) + ".csv";
    std::ofstream log(logName);

    for (int i = 0; i < ITERATIONS; i++) {
        int page = -1;

        LogCsv(log, 0, page);                 // WAIT

        do {
            page = AcquireSuitablePage(header, false);  // false = reader
            if (page == -1) Sleep(1);
        } while (page == -1);

        char semName[64];
        sprintf_s(semName, "%s%d", SEM_PREFIX, page);
        HANDLE hWriteSem = CreateSemaphoreA(NULL, 1, 1, semName);

        WaitForSingleObject(hWriteSem, INFINITE);

        LogCsv(log, 1, page);                 // ACTIVE

        char* pagePtr = (char*)pBuf + sizeof(ControlHeader) + page * si.dwPageSize;
        char readBuffer[256];
        strncpy_s(readBuffer, pagePtr, sizeof(readBuffer) - 1);
        readBuffer[255] = '\0';
        std::cout << "Reader " << GetCurrentProcessId() << " read from page " << page << ": " << readBuffer << std::endl;

        RandomSleep();

        // После чтения освобождаем страницу
        HANDLE hControlMutex = GetControlMutex();
        WaitForSingleObject(hControlMutex, INFINITE);
        header->readers_on_page[page]--;
        if (header->readers_on_page[page] == 0) {
            header->page_state[page] = READY_FOR_WRITE;
        }
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