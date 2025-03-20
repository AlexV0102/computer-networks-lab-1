#include <windows.h>
#include <iostream>
#include <sstream>
#include <vector>

#define MAX_PROCESSES 10
#define MAX_CONCURRENT_PROCESSES 3

HANDLE hNamedMutex;       
HANDLE hSemaphore;        
HANDLE hTimer;            
std::vector<HANDLE> processes; 


void CreateChildProcess(int processNumber, HANDLE hMutexDup) {
    STARTUPINFO si = { sizeof(si) };
    PROCESS_INFORMATION pi;

    std::ostringstream args;
    args << processNumber << " " << (long long)hMutexDup;

    std::string sArgs = args.str(); 
    std::wstring wArgs(sArgs.begin(), sArgs.end()); 
    std::wstring command = L"lab1.exe " + wArgs; 

    std::vector<wchar_t> cmdLine(command.begin(), command.end());
    cmdLine.push_back(0); 

    if (!CreateProcess(NULL, cmdLine.data(), NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi)) {
        std::cerr << "Failed to create process " << processNumber << "! Error: " << GetLastError() << std::endl;
    }
    else {
        std::cout << "Process " << processNumber << " started." << std::endl;
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }
}



void CreateProcesses() {
    HANDLE hMutex = CreateMutex(NULL, FALSE, NULL); 

    HANDLE hMutexDup;
    DuplicateHandle(GetCurrentProcess(), hMutex, GetCurrentProcess(), &hMutexDup, 0, TRUE, DUPLICATE_SAME_ACCESS);

    for (int i = 1; i <= MAX_PROCESSES; i++) {
        CreateChildProcess(i, hMutexDup);
    }

    CloseHandle(hMutex);
}

void ChildProcess(int processNumber, HANDLE hMutex) {

    WaitForSingleObject(hSemaphore, INFINITE);
    std::cout << "Process " << processNumber << " acquired semaphore." << std::endl;

    WaitForSingleObject(hMutex, INFINITE);
    std::cout << "Process " << processNumber << " acquired mutex." << std::endl;
    Sleep(2000);
    ReleaseMutex(hMutex);
    std::cout << "Process " << processNumber << " released mutex." << std::endl;

    ReleaseSemaphore(hSemaphore, 1, NULL);
    std::cout << "Process " << processNumber << " released semaphore." << std::endl;
}

void ParentProcess() {
    std::cout << "Main process started." << std::endl;


    hNamedMutex = CreateMutex(NULL, TRUE, L"Global\\SingleInstanceMutex");
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        std::cout << "Another instance is already running. Exiting..." << std::endl;
        return;
    }

   
    hSemaphore = CreateSemaphore(NULL, MAX_CONCURRENT_PROCESSES, MAX_CONCURRENT_PROCESSES, NULL);
    if (!hSemaphore) {
        std::cerr << "Failed to create semaphore! Error: " << GetLastError() << std::endl;
        return;
    }


    CreateProcesses();


    hTimer = CreateWaitableTimer(NULL, TRUE, NULL);
    LARGE_INTEGER li;
    li.QuadPart = -50000000LL; 
    SetWaitableTimer(hTimer, &li, 0, NULL, NULL, FALSE);
    WaitForSingleObject(hTimer, INFINITE);
    std::cout << "5 seconds passed, checking process states..." << std::endl;

    WaitForMultipleObjects(processes.size(), processes.data(), TRUE, INFINITE);
    std::cout << "All processes completed." << std::endl;

    CloseHandle(hNamedMutex);
    CloseHandle(hSemaphore);
    CloseHandle(hTimer);
}

int main(int argc, char* argv[]) {
    if (argc == 1) {
        ParentProcess();
    }
    else {
        int processNumber = std::atoi(argv[1]);
        HANDLE hMutex = (HANDLE)std::atoll(argv[2]);
        ChildProcess(processNumber, hMutex);
    }
    return 0;
}
