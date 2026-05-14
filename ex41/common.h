#pragma once
#include <windows.h>
#include <iostream>
#include <fstream>
#include <string>
#include <mmsystem.h>

#pragma comment(lib, "winmm.lib")

const int PAGE_COUNT = 16; // Сумма цифр ID 431417 без первой (3+1+4+1+7) 
const int MIN_DELAY = 500;
const int MAX_DELAY = 1500;
const int ITERATIONS = 15;

const char* SHM_NAME = "Local\\SharedBuffer_Lab4";
const char* BACKING_FILE = "buffer_storage.bin"; // Имя физического файла 
const char* MUTEX_PREFIX = "Local\\PageMutex_";
const char* SEM_PREFIX = "Local\\PageWriteSem_";

struct ControlHeader {
    int readers_on_page[PAGE_COUNT];
};

// Функция для маппинга файла
void* MapSharedFile(HANDLE& hFile, HANDLE& hMap, DWORD& size) {
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    size = sizeof(ControlHeader) + (PAGE_COUNT * si.dwPageSize);

    // 1. Создаем или открываем физический файл 
    hFile = CreateFileA(BACKING_FILE, GENERIC_READ | GENERIC_WRITE, 
                        FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, 
                        OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    if (hFile == INVALID_HANDLE_VALUE) return nullptr;

    // 2. Создаем проекцию файла (указываем hFile вместо INVALID_HANDLE_VALUE)
    hMap = CreateFileMappingA(hFile, NULL, PAGE_READWRITE, 0, size, SHM_NAME);
    if (!hMap) return nullptr;

    // 3. Отображаем в адресное пространство
    void* pBuf = MapViewOfFile(hMap, FILE_MAP_ALL_ACCESS, 0, 0, size);
    
    // 4. Блокируем страницы в ОЗУ 
    if (pBuf) {
        if (!VirtualLock(pBuf, size)) {
            std::cerr << "VirtualLock failed. Error: " << GetLastError() << "\n";
            // В некоторых системах требует настройки рабочих наборов, но для ЛР обычно проходит
        }
    }

    return pBuf;
}

void LogCsv(std::ofstream& log, int state, int page) {
    log << timeGetTime() << "," << state << "," << page << "\n";
}

void RandomSleep() {
    Sleep(MIN_DELAY + rand() % (MAX_DELAY - MIN_DELAY + 1));
}