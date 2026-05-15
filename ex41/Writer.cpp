#include "Сommon.h"

int main() {
    // Инициализация генератора случайных чисел
    srand((unsigned int)time(NULL) ^ GetCurrentProcessId());

    SYSTEM_INFO si;
    GetSystemInfo(&si);
    DWORD totalSize;

    HANDLE hFile = NULL;
    HANDLE hMapFile = NULL;

    // Подключение к разделяемой памяти
    void* pBuf = MapSharedFile(hFile, hMapFile, totalSize);
    if (!pBuf) {
        std::cerr << "Mapping failed\n";
        return 1;
    }

    // Очистка памяти при запуске писателя (опционально для первого процесса)
    for (int i = 0; i < PAGE_COUNT; ++i) {
        char* pagePtr = (char*)pBuf + sizeof(ControlHeader) + i * si.dwPageSize;
        memset(pagePtr, 0, si.dwPageSize);
    }

    // Подготовка лог-файла
    std::string logName = "Writer_" + std::to_string(GetCurrentProcessId()) + ".csv";
    std::ofstream log(logName);

    for (int i = 0; i < ITERATIONS; i++) {
        int page = rand() % PAGE_COUNT; // Выбор случайной страницы для записи
        
        char semName[64];
        sprintf_s(semName, "%s%d", SEM_PREFIX, page);
        
        // Создание или открытие бинарного семафора
        HANDLE hWriteSem = CreateSemaphoreA(NULL, 1, 1, semName);

        // --- БЛОК 1: ОЖИДАНИЕ (WAIT) ---
        LogCsv(log, 0, page); // Статус "Ожидаю"
        // Ожидание семафора
        WaitForSingleObject(hWriteSem, INFINITE);

        // --- БЛОК 2: АКТИВНОСТЬ (ACTIVE) ---
        LogCsv(log, 1, page); // Статус "Работаю"

        // Запись
        char* pagePtr = (char*)pBuf + sizeof(ControlHeader) + page * si.dwPageSize;
        char buffer[256];
        DWORD tick = timeGetTime();
        
        sprintf_s(buffer, "Writer %d at %u", GetCurrentProcessId(), tick);
        
        // Прямая запись в память
        strcpy_s(pagePtr, si.dwPageSize, buffer);
        
        std::cout << "Writer " << GetCurrentProcessId() << " wrote to page " << page << ": " << buffer << std::endl;

        RandomSleep();

        // --- БЛОК 3: ОСВОБОЖДЕНИЕ (RELEASE) ---
        LogCsv(log, 2, page); // Статус "Завершил"
        // Увеличение счетчика семафора
        ReleaseSemaphore(hWriteSem, 1, NULL);

        CloseHandle(hWriteSem);
        Sleep(100); // Небольшая пауза, чтобы не захватить ту же страницу мгновенно снова
    }

    UnmapViewOfFile(pBuf);
    CloseHandle(hMapFile);
    CloseHandle(hFile);
    return 0;
}