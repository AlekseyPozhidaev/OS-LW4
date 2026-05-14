#include <windows.h>
#include <iostream>
#include <string>

using namespace std;

#define PIPE_NAME L"\\\\.\\pipe\\MyLabPipe"

int main() {
    HANDLE hPipe = INVALID_HANDLE_VALUE;
    HANDLE hEvent = NULL;
    OVERLAPPED ol = { 0 };
    string inputData;

    while (true) {
        cout << "\n--- Server Menu ---\n";
        cout << "1. Create Named Pipe\n";
        cout << "2. Wait for Client Connection\n";
        cout << "3. Send Data (Asynchronous)\n";
        cout << "4. Disconnect Client\n";
        cout << "0. Exit\n";
        cout << "Choice: ";
        int choice;
        cin >> choice;

        if (choice == 0) break;

        switch (choice) {
            case 1:
                if (hPipe != INVALID_HANDLE_VALUE) {
                    cout << "Pipe already exists!\n";
                    break;
                }
                hPipe = CreateNamedPipeW(
                    PIPE_NAME,
                    PIPE_ACCESS_OUTBOUND | FILE_FLAG_OVERLAPPED, 
                    PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
                    1, 1024, 1024, 0, NULL);
                    
                if (hPipe == INVALID_HANDLE_VALUE) {
                    cout << "CreatePipe failed. Error: " << GetLastError() << "\n";
                } else {
                    cout << "Pipe created successfully.\n";
                }
                break;

            case 2:
                if (hPipe == INVALID_HANDLE_VALUE) {
                    cout << "Create pipe first!\n";
                    break;
                }
                cout << "Waiting for client...\n";
                if (ConnectNamedPipe(hPipe, NULL) ? TRUE : (GetLastError() == ERROR_PIPE_CONNECTED)) {
                    cout << "Client connected!\n";
                } else {
                    cout << "Connect failed. Error: " << GetLastError() << "\n";
                }
                break;

            case 3:
                if (hPipe == INVALID_HANDLE_VALUE) {
                    cout << "Pipe not initialized!\n";
                    break;
                }
                if (hEvent == NULL) {
                    hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
                    ol.hEvent = hEvent;
                }
                
                cout << "Enter message: ";
                cin.ignore();
                getline(cin, inputData);
                
                ResetEvent(hEvent);
                {
                    DWORD bytesWritten = 0;
                    BOOL result = WriteFile(hPipe, inputData.c_str(), (DWORD)inputData.length() + 1, NULL, &ol);
                    
                    if (!result && GetLastError() == ERROR_IO_PENDING) {
                        cout << "IO Pending. Waiting for completion...\n";
                        WaitForSingleObject(hEvent, INFINITE);
                        GetOverlappedResult(hPipe, &ol, &bytesWritten, FALSE);
                        cout << "Write completed. Bytes sent: " << bytesWritten << "\n";
                    } else if (result) {
                        cout << "Write completed immediately.\n";
                    } else {
                        cout << "Write failed. Error: " << GetLastError() << "\n";
                    }
                }
                break;

            case 4:
                if (hPipe != INVALID_HANDLE_VALUE) {
                    DisconnectNamedPipe(hPipe);
                    cout << "Client disconnected.\n";
                }
                break;

            default:
                cout << "Invalid choice.\n";
        }
    }

    if (hEvent) CloseHandle(hEvent);
    if (hPipe != INVALID_HANDLE_VALUE) CloseHandle(hPipe);
    return 0;
}