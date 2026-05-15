#include "Common.h"

int main() {
    srand((unsigned int)time(NULL) ^ GetCurrentProcessId());

    SYSTEM_INFO si;
    GetSystemInfo(&si);
    DWORD totalSize;

    HANDLE hFile = NULL;
    HANDLE hMapFile = NULL;

    void* pBuf = MapSharedFile(hFile, hMapFile, totalSize);
    if (!pBuf) {
        std::cerr << "Mapping failed\n";
        return 1;
    }

    // Заголовок находится в начале разделяемой памяти
    ControlHeader* header = (ControlHeader*)pBuf;

    std::string logName = "Reader_" + std::to_string(GetCurrentProcessId()) + ".csv";
    std::ofstream log(logName);

    for (int i = 0; i < ITERATIONS; i++) {
        int page = rand() % PAGE_COUNT;
        
        char mutName[64], semName[64];
        sprintf_s(mutName, "%s%d", MUTEX_PREFIX, page);
        sprintf_s(semName, "%s%d", SEM_PREFIX, page);
        
        // Мьютекс переменной-счетчика
        HANDLE hCountMut = CreateMutexA(NULL, FALSE, mutName);
        // Семафор читателей
        HANDLE hWriteSem = CreateSemaphoreA(NULL, 1, 1, semName);

        // --- БЛОК 1: ОЖИДАНИЕ (WAIT) ---
        LogCsv(log, 0, page);

        // Ждём счётчик читателей
        WaitForSingleObject(hCountMut, INFINITE);
        header->readers_on_page[page]++;
        
        // блокировка доступа писателям
        if (header->readers_on_page[page] == 1) {
            WaitForSingleObject(hWriteSem, INFINITE);
        }
        ReleaseMutex(hCountMut); // Выход из защиты счетчика

        // --- БЛОК 2: АКТИВНОСТЬ (ACTIVE) ---
        LogCsv(log, 1, page);

        // Вычисление адреса для чтения
        char* pagePtr = (char*)pBuf + sizeof(ControlHeader) + page * si.dwPageSize;
        char readBuffer[256];
        
        // Безопасное копирование данных из разделяемой памяти в локальный буфер
        strncpy_s(readBuffer, pagePtr, sizeof(readBuffer) - 1);
        readBuffer[255] = '\0';
        
        std::cout << "Reader " << GetCurrentProcessId() << " read from page " << page << ": " << readBuffer << std::endl;

        RandomSleep();

        // --- БЛОК 3: ОСВОБОЖДЕНИЕ (RELEASE) ---
        LogCsv(log, 2, page); 

        // Освобождаем счётчик чистателей
        WaitForSingleObject(hCountMut, INFINITE);
        header->readers_on_page[page]--;
        
        // Разрешение писателям заходить на страницу
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
    CloseHandle(hFile);
    return 0;
}