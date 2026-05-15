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
const int ITERATIONS = 50;

const char* SHM_NAME = "Local\\SharedBuffer_Lab4";
const char* BACKING_FILE = "buffer_storage.bin";
const char* MUTEX_PREFIX = "Local\\PageMutex_";
const char* SEM_PREFIX = "Local\\PageWriteSem_";

// Состояния страницы
const int READY_FOR_WRITE = 0;
const int READY_FOR_READ = 1;

struct ControlHeader {
    int readers_on_page[PAGE_COUNT];
    int page_state[PAGE_COUNT];
    int writer_active[PAGE_COUNT];
    int initialized; // маркер инициализации (0x1234)
};

// Глобальный мьютекс для доступа к ControlHeader
inline HANDLE GetControlMutex() {
    static HANDLE hMutex = nullptr;
    if (!hMutex) {
        hMutex = CreateMutexA(NULL, FALSE, "Local\\ControlMutex");
    }
    return hMutex;
}

// Функция для маппинга файла
void* MapSharedFile(HANDLE& hFile, HANDLE& hMap, DWORD& size) {
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    size = sizeof(ControlHeader) + (PAGE_COUNT * si.dwPageSize);

    hFile = CreateFileA(BACKING_FILE, GENERIC_READ | GENERIC_WRITE,
                        FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                        OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return nullptr;

    hMap = CreateFileMappingA(hFile, NULL, PAGE_READWRITE, 0, size, SHM_NAME);
    if (!hMap) return nullptr;

    void* pBuf = MapViewOfFile(hMap, FILE_MAP_ALL_ACCESS, 0, 0, size);
    if (pBuf) {
        if (!VirtualLock(pBuf, size)) {
            std::cerr << "VirtualLock failed. Error: " << GetLastError() << "\n";
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

// Выбор подходящей страницы с чередованием (писатель/читатель)
inline int AcquireSuitablePage(ControlHeader* header, bool isWriter) {
    HANDLE hMutex = GetControlMutex();
    WaitForSingleObject(hMutex, INFINITE);

    int found = -1;
    for (int i = 0; i < PAGE_COUNT; ++i) {
        if (isWriter) {
            if (header->page_state[i] == READY_FOR_WRITE &&
                header->writer_active[i] == 0 &&
                header->readers_on_page[i] == 0) {
                header->writer_active[i] = 1;
                found = i;
                break;
            }
        } else {
            if (header->page_state[i] == READY_FOR_READ &&
                header->writer_active[i] == 0 &&
                header->readers_on_page[i] == 0) {
                header->readers_on_page[i] = 1; // только один читатель
                found = i;
                break;
            }
        }
    }
    ReleaseMutex(hMutex);
    return found;
}