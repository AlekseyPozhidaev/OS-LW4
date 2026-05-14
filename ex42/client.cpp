#include <windows.h>
#include <iostream>

using namespace std;

#define PIPE_NAME L"\\\\.\\pipe\\MyLabPipe"

VOID WINAPI FileIOCompletionRoutine(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED lpOverlapped) {
    if (dwErrorCode == 0) {
        cout << "\n[SYSTEM] Data received successfully (" << dwNumberOfBytesTransfered << " bytes).\n";
    } else {
        cout << "\n[SYSTEM] Read failed. Error: " << dwErrorCode << "\n";
    }
}

int main() {
    HANDLE hFile = INVALID_HANDLE_VALUE;
    OVERLAPPED ol = { 0 };
    char buffer[1024];

    while (true) {
        cout << "\n--- Client Menu ---\n";
        cout << "1. Connect to Pipe\n";
        cout << "2. Read Data (Asynchronous)\n";
        cout << "3. Close Connection\n";
        cout << "0. Exit\n";
        cout << "Choice: ";
        int choice;
        cin >> choice;

        if (choice == 0) break;

        switch (choice) {
            case 1:
                if (hFile != INVALID_HANDLE_VALUE) {
                    cout << "Already connected!\n";
                    break;
                }
                hFile = CreateFileW(
                    PIPE_NAME,
                    GENERIC_READ,
                    0,
                    NULL,
                    OPEN_EXISTING,
                    FILE_FLAG_OVERLAPPED, 
                    NULL);
                    
                if (hFile == INVALID_HANDLE_VALUE) {
                    cout << "Connection failed. Error: " << GetLastError() << "\n";
                } else {
                    cout << "Connected to server.\n";
                }
                break;

            case 2:
                if (hFile == INVALID_HANDLE_VALUE) {
                    cout << "Connect to server first!\n";
                    break;
                }
                memset(buffer, 0, sizeof(buffer));
                ZeroMemory(&ol, sizeof(OVERLAPPED));
                
                cout << "Starting async read...\n";
                if (ReadFileEx(hFile, buffer, sizeof(buffer), &ol, FileIOCompletionRoutine)) {
                    cout << "Waiting for data (thread in alertable state)...\n";
                    // SleepEx with TRUE flag allows the Completion Routine to run
                    SleepEx(INFINITE, TRUE);
                    cout << "Message content: " << buffer << "\n";
                } else {
                    cout << "ReadFileEx failed. Error: " << GetLastError() << "\n";
                }
                break;

            case 3:
                if (hFile != INVALID_HANDLE_VALUE) {
                    CloseHandle(hFile);
                    hFile = INVALID_HANDLE_VALUE;
                    cout << "Connection closed.\n";
                }
                break;

            default:
                cout << "Invalid choice.\n";
        }
    }

    if (hFile != INVALID_HANDLE_VALUE) CloseHandle(hFile);
    return 0;
}