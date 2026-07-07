#include <iostream>
#include <windows.h>
#include <tlhelp32.h>
#include <shlwapi.h>
#include <string>
#include <vector>
#include <sstream>
#include <fstream>
#include <filesystem>
#include <cstdlib>
#include <ctime>
#include <thread>
#include <chrono>
#include <algorithm>
#include <random>
#include <winhttp.h>
#include <comdef.h>
#include <Wbemidl.h>
#include <evntprov.h>
#include <dxgi.h>
#include <d3d11.h>
#include <iphlpapi.h>

#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "wbemuuid.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "iphlpapi.lib")

namespace fs = std::filesystem;

class UltimateCleaner {
private:
    std::string steamPath;
    std::string userProfile;
    std::string systemDrive;
    std::string fakeHWID;
    std::string backupPath;

public:
    UltimateCleaner() {
        userProfile = getEnvVar("USERPROFILE");
        systemDrive = getEnvVar("SYSTEMDRIVE");
        if (systemDrive.empty()) systemDrive = "C:";
        backupPath = userProfile + "\\AppData\\Local\\Temp\\cleaner_backup_" + std::to_string(time(nullptr));
        detectSteamPath();
        fakeHWID = generateFakeHWID();
    }

    std::string getEnvVar(const std::string& name) {
        char* buffer = nullptr;
        size_t size = 0;
        if (_dupenv_s(&buffer, &size, name.c_str()) == 0 && buffer != nullptr) {
            std::string result(buffer);
            free(buffer);
            return result;
        }
        return "";
    }

    void detectSteamPath() {
        HKEY hKey;
        if (RegOpenKeyExA(HKEY_CURRENT_USER, "Software\\Valve\\Steam", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
            char buffer[MAX_PATH];
            DWORD size = sizeof(buffer);
            if (RegQueryValueExA(hKey, "SteamPath", nullptr, nullptr, (LPBYTE)buffer, &size) == ERROR_SUCCESS) {
                steamPath = buffer;
            }
            RegCloseKey(hKey);
        }
        if (steamPath.empty()) {
            steamPath = "C:\\Program Files (x86)\\Steam";
        }
    }

    std::string generateFakeHWID() {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, 15);

        std::string hwid = "HWID-";
        for (int i = 0; i < 8; i++) {
            int val = dis(gen);
            hwid += (val < 10) ? ('0' + val) : ('A' + val - 10);
            if (i % 2 == 1 && i != 7) hwid += '-';
        }
        return hwid;
    }

    std::string generateRandomGUID() {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, 15);

        std::string guid = "{";
        for (int i = 0; i < 8; i++) {
            int val = dis(gen);
            guid += (val < 10) ? ('0' + val) : ('A' + val - 10);
        }
        guid += "-";
        for (int section = 0; section < 3; section++) {
            for (int i = 0; i < 4; i++) {
                int val = dis(gen);
                guid += (val < 10) ? ('0' + val) : ('A' + val - 10);
            }
            guid += "-";
        }
        for (int i = 0; i < 12; i++) {
            int val = dis(gen);
            guid += (val < 10) ? ('0' + val) : ('A' + val - 10);
        }
        guid += "}";
        return guid;
    }

    std::string generateRandomMachineName() {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(1000, 9999);

        return "PC-" + std::to_string(dis(gen));
    }

    bool spoofMachineGUIDOnly() {
        std::cout << "[*] Spoofing Machine GUID...\n";

        std::string newMachineName = generateRandomMachineName();
        std::string newGUID = generateRandomGUID();

        RegistryManager::SetValue(HKEY_LOCAL_MACHINE,
            "SYSTEM\\CurrentControlSet\\Control\\ComputerName\\ComputerName",
            "ComputerName", newMachineName);
        RegistryManager::SetValue(HKEY_LOCAL_MACHINE,
            "SYSTEM\\CurrentControlSet\\Control\\ComputerName\\ActiveComputerName",
            "ComputerName", newMachineName);
        RegistryManager::SetValue(HKEY_LOCAL_MACHINE,
            "SYSTEM\\CurrentControlSet\\Control\\SystemInfo",
            "ComputerName", newMachineName);

        RegistryManager::SetValue(HKEY_LOCAL_MACHINE,
            "SOFTWARE\\Microsoft\\Cryptography",
            "MachineGuid", newGUID);

        SetEnvironmentVariableA("COMPUTERNAME", newMachineName.c_str());

        std::cout << "[+] Machine renamed to: " << newMachineName << "\n";
        std::cout << "[+] Machine GUID changed\n";
        return true;
    }

    bool spoofUserIdentityOnly() {
        std::cout << "[*] Spoofing User Identity...\n";

        std::string newUser = "User" + std::to_string(rand() % 1000);
        SetEnvironmentVariableA("USERNAME", newUser.c_str());

        std::cout << "[+] User identity spoofed to: " << newUser << "\n";
        return true;
    }

    bool spoofHardwareProfilesOnly() {
        std::cout << "[*] Spoofing Hardware Profiles...\n";

        std::string newHwProfile = generateRandomGUID();

        RegistryManager::SetValue(HKEY_LOCAL_MACHINE,
            "SYSTEM\\CurrentControlSet\\Control\\IDConfigDB\\Hardware Profiles\\0001",
            "HwProfileGuid", newHwProfile);

        SetEnvironmentVariableA("FAKE_HWID", newHwProfile.c_str());

        std::cout << "[+] Hardware profile spoofed\n";
        return true;
    }

    bool spoofBIOSandProcessorOnly() {
        std::cout << "[*] Spoofing BIOS and Processor Info...\n";

        std::string fakeBIOS = "VER" + std::to_string(rand() % 10000);
        std::string fakeManufacturer = "Generic Systems";
        std::string fakeProcessor = "Intel(R) Core(TM) i" + std::to_string((rand() % 9) + 3) +
            "-" + std::to_string((rand() % 9000) + 1000) + " CPU";

        RegistryManager::SetValue(HKEY_LOCAL_MACHINE,
            "HARDWARE\\DESCRIPTION\\System\\BIOS",
            "BIOSVersion", fakeBIOS);
        RegistryManager::SetValue(HKEY_LOCAL_MACHINE,
            "HARDWARE\\DESCRIPTION\\System\\BIOS",
            "SystemManufacturer", fakeManufacturer);
        RegistryManager::SetValue(HKEY_LOCAL_MACHINE,
            "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0",
            "ProcessorNameString", fakeProcessor);

        std::cout << "[+] BIOS and Processor info spoofed\n";
        return true;
    }

    bool performSafeHWIDSpoof() {
        std::cout << "\n=====================================\n";
        std::cout << "      SAFE HWID SPOOF\n";
        std::cout << "=====================================\n\n";

        bool success = true;

        success &= spoofMachineGUIDOnly();
        Sleep(300);

        success &= spoofUserIdentityOnly();
        Sleep(300);

        success &= spoofHardwareProfilesOnly();
        Sleep(300);

        success &= spoofBIOSandProcessorOnly();
        Sleep(300);

        if (success) {
            std::cout << "\n[OK] Safe HWID spoof completed!\n";
        }
        else {
            std::cout << "\n[-] Some spoofing operations failed!\n";
        }

        system("pause");
        return success;
    }

    bool launchRustClient(const std::string& folder) {
        std::string exePath = folder + "\\RustClient.exe";
        if (!fs::exists(exePath)) {
            std::cout << "[-] RustClient.exe not found at: " << exePath << "\n";
            return false;
        }

        std::cout << "[*] Setting working directory to: " << folder << "\n";
        SetCurrentDirectoryA(folder.c_str());

        STARTUPINFOA si = { sizeof(si) };
        PROCESS_INFORMATION pi = { 0 };

        std::string cmdLine = "\"" + exePath + "\"";

        std::cout << "[*] Launch command: " << cmdLine << "\n";

        if (CreateProcessA(
            exePath.c_str(),           // Application name
            (LPSTR)cmdLine.c_str(),    // Command line
            nullptr,                   // Process security attributes
            nullptr,                   // Thread security attributes
            FALSE,                     // Inherit handles
            CREATE_NEW_CONSOLE,        // Creation flags
            nullptr,                   // Environment
            folder.c_str(),            // Current directory
            &si,                       // Startup info
            &pi)) {                    // Process information

            std::cout << "[+] Process started successfully\n";
            std::cout << "[+] Process ID: " << pi.dwProcessId << "\n";

            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);

            return true;
        }
        else {
            DWORD error = GetLastError();
            std::cout << "[-] Failed to create process. Error code: " << error << "\n";
            std::cout << "[*] Trying alternative launch method...\n";
            if (ShellExecuteA(nullptr, "open", exePath.c_str(), nullptr, folder.c_str(), SW_SHOW) > (HINSTANCE)32) {
                std::cout << "[+] Alternative launch successful\n";
                return true;
            }
            else {
                std::cout << "[-] Alternative launch also failed\n";
                return false;
            }
        }
    }

    void safeRunOldRust() {
        printHeader("SAFE RUN RUST CLIENT");

        std::cout << "[*] Searching for OldRust folder...\n";
        std::string rustFolder = findRustFolder();

        if (rustFolder.empty()) {
            std::cout << "[-] OldRust folder not found!\n";
            std::cout << "[*] Please ensure OldRust is installed correctly\n";
            system("pause");
            return;
        }

        std::cout << "[+] Found OldRust at: " << rustFolder << "\n";

        std::string exePath = rustFolder + "\\RustClient.exe";
        if (!fs::exists(exePath)) {
            std::cout << "[-] RustClient.exe not found!\n";
            system("pause");
            return;
        }

        std::cout << "[+] Generating fake HWID: " << fakeHWID << "\n";
        std::cout << "[+] Applying minimal spoof (no network)...\n";

        spoofHWID();
        spoofMachineGUIDOnly();
        spoofUserIdentityOnly();

        SetEnvironmentVariableA("COMPUTERNAME", "FAKE-PC");
        SetEnvironmentVariableA("USERNAME", "User");
        SetEnvironmentVariableA("FAKE_HWID", fakeHWID.c_str());

        std::cout << "[+] Starting RustClient.exe...\n";

        if (launchRustClient(rustFolder)) {
            std::cout << "\n[OK] Rust started successfully!\n";
            std::cout << "[INFO] Check if game window appears\n";
        }
        else {
            std::cout << "\n[-] Failed to start Rust!\n";
            std::cout << "[*] Check if file is corrupted or blocked\n";
        }

        system("pause");
    }


    bool spoofHWID() {
        RegistryManager::SetValue(HKEY_LOCAL_MACHINE,
            "SYSTEM\\CurrentControlSet\\Control\\IDConfigDB\\Hardware Profiles\\0001",
            "HwProfileGuid", fakeHWID);
        SetEnvironmentVariableA("FAKE_HWID", fakeHWID.c_str());
        return true;
    }

    bool spoofWMIProperties() {
        std::string mofContent = R"(
#pragma namespace ("\\\\.\\root\\cimv2")
instance of Win32_Processor {
    DeviceID = "CPU0001";
    Name = "Intel(R) Core(TM) i7-9700K CPU @ 3.60GHz";
    Manufacturer = "GenuineIntel";
    MaxClockSpeed = 3600;
};
instance of Win32_VideoController {
    Name = "NVIDIA GeForce RTX 5090";
    PNPDeviceID = "PCI\\VEN_10DE&DEV_1F07&SUBSYS_376C1462&REV_A1";
    AdapterRAM = 8589934592;
};
)";

        char* tempPath = nullptr;
        size_t size = 0;
        if (_dupenv_s(&tempPath, &size, "TEMP") != 0 || tempPath == nullptr) {
            return false;
        }

        std::string mofPath = std::string(tempPath) + "\\spoof.mof";
        free(tempPath);

        std::ofstream mof(mofPath);
        if (!mof.is_open()) return false;
        mof << mofContent;
        mof.close();

        std::string cmd = "mofcomp \"" + mofPath + "\" >nul 2>&1";
        system(cmd.c_str());

        DeleteFileA(mofPath.c_str());
        return true;
    }

    bool generateFakeEventLogs() {
        HANDLE hEventLog = RegisterEventSourceA(nullptr, "Application");
        if (!hEventLog) return false;

        const char* msgs[] = {
            "Successfully loaded graphics driver version 27.21.14.6589",
            "Driver installation completed for NVIDIA GeForce RTX 2070",
            "System performance optimized for gaming applications"
        };

        for (int i = 0; i < 3; i++) {
            ReportEventA(hEventLog, EVENTLOG_INFORMATION_TYPE, 0, 0, nullptr, 1, 0, &msgs[i], nullptr);
            Sleep(100);
        }
        DeregisterEventSource(hEventLog);
        return true;
    }

    bool spoofDirectXAdapters() {
        HMODULE hDXGI = LoadLibraryA("dxgi.dll");
        if (!hDXGI) return false;

        typedef HRESULT(WINAPI* tCreateDXGIFactory)(REFIID riid, void** ppFactory);
        tCreateDXGIFactory pCreateDXGIFactory = (tCreateDXGIFactory)GetProcAddress(hDXGI, "CreateDXGIFactory");

        if (pCreateDXGIFactory) {
            IDXGIFactory* pFactory = nullptr;
            if (SUCCEEDED(pCreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&pFactory))) {
                pFactory->Release();
            }
        }
        FreeLibrary(hDXGI);
        return true;
    }

    bool protectFromBanMarks() {
       
        RegistryManager::SetValue(HKEY_CURRENT_USER,
            "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\FeatureUsage\\AppSwitched",
            "RustClient.exe", "1600000000");

       
        std::string logPath = userProfile + "\\AppData\\Local\\Rust\\logs";
        if (fs::exists(logPath)) {
            try {
                for (const auto& entry : fs::directory_iterator(logPath)) {
                    if (entry.is_regular_file()) {
                        std::ofstream ofs(entry.path(), std::ios::trunc);
                        ofs.close();
                    }
                }
            }
            catch (...) {}
        }
        std::string configPath = userProfile + "\\AppData\\Local\\Rust\\cfg.dat";
        std::ofstream cfg(configPath, std::ios::binary);
        if (cfg.is_open()) {
            cfg.write("FAKE_CONFIG_DATA_ANTI_BAN", 24);
            cfg.close();
        }

        return true;
    }

    void emulateLegitStartup() {

        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(100, 300);

        for (int i = 0; i < 5; i++) {
            INPUT input = { 0 };
            input.type = INPUT_MOUSE;
            input.mi.dx = dis(gen);
            input.mi.dy = dis(gen);
            input.mi.dwFlags = MOUSEEVENTF_MOVE;
            SendInput(1, &input, sizeof(INPUT));
            Sleep(dis(gen));
        }
        INPUT keys[2] = { 0 };
        keys[0].type = INPUT_KEYBOARD;
        keys[0].ki.wVk = VK_SHIFT;
        keys[1].type = INPUT_KEYBOARD;
        keys[1].ki.wVk = VK_SHIFT;
        keys[1].ki.dwFlags = KEYEVENTF_KEYUP;

        SendInput(2, keys, sizeof(INPUT));
        Sleep(500);
    }

    void randomizeTiming() {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        static std::uniform_int_distribution<> dis(500, 2000);
        Sleep(dis(gen));
    }

    class RegistryManager {
    public:
        static bool DeleteKey(HKEY hRoot, const std::string& subKey) {
            return RegDeleteKeyA(hRoot, subKey.c_str()) == ERROR_SUCCESS;
        }

        static bool DeleteValue(HKEY hRoot, const std::string& subKey, const std::string& value) {
            HKEY hKey;
            if (RegOpenKeyExA(hRoot, subKey.c_str(), 0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS) {
                LONG result = RegDeleteValueA(hKey, value.c_str());
                RegCloseKey(hKey);
                return result == ERROR_SUCCESS;
            }
            return false;
        }

        static bool SetValue(HKEY hRoot, const std::string& subKey, const std::string& value, const std::string& data) {
            HKEY hKey;
            if (RegOpenKeyExA(hRoot, subKey.c_str(), 0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS) {
                LONG result = RegSetValueExA(hKey, value.c_str(), 0, REG_SZ, (const BYTE*)data.c_str(), (DWORD)data.size() + 1);
                RegCloseKey(hKey);
                return result == ERROR_SUCCESS;
            }
            return false;
        }
    };

    class ProcessManager {
    public:
        static bool KillProcess(const std::string& processName) {
            HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
            if (hSnapshot == INVALID_HANDLE_VALUE) return false;

            PROCESSENTRY32W pe;
            pe.dwSize = sizeof(PROCESSENTRY32W);
            bool killed = false;

            int wideSize = MultiByteToWideChar(CP_ACP, 0, processName.c_str(), -1, nullptr, 0);
            std::wstring wideProcessName(wideSize, L'\0');
            MultiByteToWideChar(CP_ACP, 0, processName.c_str(), -1, &wideProcessName[0], wideSize);

            if (Process32FirstW(hSnapshot, &pe)) {
                do {
                    if (_wcsicmp(pe.szExeFile, wideProcessName.c_str()) == 0) {
                        HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pe.th32ProcessID);
                        if (hProcess) {
                            TerminateProcess(hProcess, 0);
                            CloseHandle(hProcess);
                            killed = true;
                        }
                    }
                } while (Process32NextW(hSnapshot, &pe));
            }
            CloseHandle(hSnapshot);
            return killed;
        }

        static bool RunHidden(const std::string& path) {
            STARTUPINFOA si = { sizeof(si) };
            PROCESS_INFORMATION pi;
            si.dwFlags = STARTF_USESHOWWINDOW;
            si.wShowWindow = SW_HIDE;

            std::string cmd = "\"" + path + "\"";
            if (CreateProcessA(nullptr, (LPSTR)cmd.c_str(), nullptr, nullptr, FALSE,
                CREATE_NO_WINDOW | CREATE_SUSPENDED, nullptr, nullptr, &si, &pi)) {
                ResumeThread(pi.hThread);
                CloseHandle(pi.hProcess);
                CloseHandle(pi.hThread);
                return true;
            }
            return false;
        }
    };

    class FileManager {
    public:
        static bool DeleteFileWithAttrib(const std::string& path) {
            SetFileAttributesA(path.c_str(), FILE_ATTRIBUTE_NORMAL);
            return DeleteFileA(path.c_str()) != 0;
        }

        static bool DeleteFolder(const std::string& path) {
            SetFileAttributesA(path.c_str(), FILE_ATTRIBUTE_DIRECTORY);
            std::string cmd = "rmdir /s /q \"" + path + "\" >nul 2>&1";
            return system(cmd.c_str()) == 0;
        }

        static bool DeleteFilesByPattern(const std::string& pattern) {
            WIN32_FIND_DATAA ffd;
            HANDLE hFind = FindFirstFileA(pattern.c_str(), &ffd);
            if (hFind == INVALID_HANDLE_VALUE) return false;

            bool deleted = false;
            do {
                if (!(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                    std::string filePath = pattern.substr(0, pattern.find_last_of("\\/") + 1) + ffd.cFileName;
                    DeleteFileWithAttrib(filePath);
                    deleted = true;
                }
            } while (FindNextFileA(hFind, &ffd) != 0);
            FindClose(hFind);
            return deleted;
        }
    };

    void printHeader(const std::string& title) {
        std::cout << "\n=====================================\n";
        std::cout << "      " << title << "\n";
        std::cout << "=====================================\n\n";
    }

    void bypassClean() {
        printHeader("FULL SYSTEM CLEAN");

        std::cout << "[1/15] Closing Steam processes...\n";
        ProcessManager::KillProcess("steam.exe");
        ProcessManager::KillProcess("steamwebhelper.exe");
        ProcessManager::KillProcess("RustClient.exe");
        Sleep(3000);

        std::cout << "[2/15] Cleaning Rust registry entries...\n";
        RegistryManager::DeleteKey(HKEY_CURRENT_USER, "Software\\Facepunch Studios LTD");
        RegistryManager::DeleteKey(HKEY_CURRENT_USER, "Software\\OldDevblogRust");
        RegistryManager::DeleteKey(HKEY_CURRENT_USER, "Software\\MYRUST240DEVBLOG");

        std::cout << "[3/15] Cleaning Run dialog history...\n";
        RegistryManager::DeleteKey(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\RunMRU");

        std::cout << "[4/15] Clearing browser history...\n";
        system("rundll32.exe inetcpl.cpl,ClearMyTracksByProcess 255 >nul 2>&1");

        std::cout << "[5/15] Cleaning FeatureUsage logs...\n";
        RegistryManager::DeleteKey(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\FeatureUsage\\AppSwitched");
        RegistryManager::DeleteKey(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\FeatureUsage\\ShowJumpView");

        std::cout << "[6/15] Removing Steam ssfn files...\n";
        if (!steamPath.empty()) {
            std::string pattern = steamPath + "\\ssfn*";
            FileManager::DeleteFilesByPattern(pattern);
        }
        std::cout << "[7/15] Resetting Steam login cache...\n";
        if (!steamPath.empty()) {
            std::string loginPath = steamPath + "\\config\\loginusers.vdf";
            if (fs::exists(loginPath)) {
                SetFileAttributesA(loginPath.c_str(), FILE_ATTRIBUTE_NORMAL);
                std::ofstream ofs(loginPath, std::ios::trunc);
                ofs.close();
                SetFileAttributesA(loginPath.c_str(), FILE_ATTRIBUTE_READONLY);
            }
        }
        std::cout << "[8/15] Removing avatar cache and user data...\n";
        if (!steamPath.empty()) {
            std::string avaPath = steamPath + "\\config\\avatarcache";
            if (fs::exists(avaPath)) FileManager::DeleteFolder(avaPath);

            std::string userPath = steamPath + "\\userdata";
            if (fs::exists(userPath)) {
                try {
                    for (const auto& entry : fs::directory_iterator(userPath)) {
                        if (entry.is_directory()) {
                            FileManager::DeleteFolder(entry.path().string());
                        }
                    }
                }
                catch (...) {}
            }
        }
        std::cout << "[9/15] Removing coplay files...\n";
        if (!steamPath.empty()) {
            std::string pattern = steamPath + "\\config\\coplay_*.vdf";
            FileManager::DeleteFilesByPattern(pattern);
        }
        std::cout << "[10/15] Cleaning Steam registry settings...\n";
        RegistryManager::DeleteValue(HKEY_CURRENT_USER, "Software\\Valve\\Steam", "AutoLoginUser");
        RegistryManager::DeleteValue(HKEY_CURRENT_USER, "Software\\Valve\\Steam", "RememberPassword");
        RegistryManager::DeleteValue(HKEY_CURRENT_USER, "Software\\Valve\\Steam", "MostRecentUser");
        std::cout << "[11/15] Cleaning SRU diagnostic logs (please wait)...\n";
        system("net stop DPS >nul 2>&1");
        Sleep(2000);
        std::string sruPath = getEnvVar("windir") + "\\System32\\sru";
        if (fs::exists(sruPath)) {
            try {
                for (const auto& entry : fs::directory_iterator(sruPath)) {
                    if (entry.is_regular_file()) {
                        SetFileAttributesA(entry.path().string().c_str(), FILE_ATTRIBUTE_NORMAL);
                        DeleteFileA(entry.path().string().c_str());
                    }
                }
            }
            catch (...) {}
        }
        Sleep(1000);
        system("net start DPS >nul 2>&1");
        std::cout << "[12/15] Cleaning temporary files...\n";
        std::string tempPath = getEnvVar("TEMP");
        system(("del /f /s /q \"" + tempPath + "\\*\" >nul 2>&1").c_str());
        system(("for /d %i in (\"" + tempPath + "\\*\") do rmdir /s /q \"%i\" >nul 2>&1").c_str());
        std::string winTemp = getEnvVar("windir") + "\\Temp";
        system(("del /f /s /q \"" + winTemp + "\\*\" >nul 2>&1").c_str());
        system(("for /d %i in (\"" + winTemp + "\\*\") do rmdir /s /q \"%i\" >nul 2>&1").c_str());
        std::cout << "[13/15] Removing OldRust folder...\n";
        std::string oldRustPath = systemDrive + "\\OldRust";
        if (fs::exists(oldRustPath)) FileManager::DeleteFolder(oldRustPath);
        std::cout << "[14/15] Removing EasyAntiCheat...\n";
        std::string eacPath = userProfile + "\\Desktop\\OldRust\\RustClient_Data\\Plugins\\x86_64\\EasyAntiCheat.dll";
        if (fs::exists(eacPath)) FileManager::DeleteFileWithAttrib(eacPath);
        std::cout << "[15/15] Cleaning Prefetch cache...\n";
        std::string prefetchPath = getEnvVar("windir") + "\\Prefetch";
        if (fs::exists(prefetchPath)) {
            std::vector<std::string> patterns = {
                "\\RUSTCLIENT*.pf", "\\STEAM*.pf", "\\EASYANTICHEAT*.pf"
            };
            for (const auto& p : patterns) {
                std::string pattern = prefetchPath + p;
                FileManager::DeleteFilesByPattern(pattern);
            }
        }

        if (!steamPath.empty() && fs::exists(steamPath + "\\steam.exe")) {
            std::cout << "\nStarting Steam...\n";
            ShellExecuteA(nullptr, "open", (steamPath + "\\steam.exe").c_str(), nullptr, nullptr, SW_SHOW);
        }

        std::cout << "\n=====================================\n";
        std::cout << "         CLEANING COMPLETED\n";
        std::cout << "=====================================\n\n";
        system("pause");
    }

    void rustManager() {
        while (true) {
            system("cls");
            printHeader("RUST MANAGER");
            std::cout << "[1] - Delete EasyAntiCheat.dll\n";
            std::cout << "[2] - SAFE RUN (HWID Spoof + Hidden)\n";
            std::cout << "[3] - COMPLETE HWID SPOOF\n";
            std::cout << "[4] - NETWORK SPOOF (MAC/IP)\n";
            std::cout << "[0] - Back\n\n";
            std::cout << "Select [0-4]: ";

            int choice;
            std::cin >> choice;
            std::cin.ignore();

            switch (choice) {
            case 0: return;
            case 1: deleteEAC(); break;
            case 2: safeRunOldRust(); break;
            case 3: performSafeHWIDSpoof(); break;
            case 4: performNetworkSpoof(); break;
            default: std::cout << "Invalid choice!\n"; system("pause");
            }
        }
    }

    bool performNetworkSpoof() {
        std::cout << "\n=====================================\n";
        std::cout << "      NETWORK SPOOF (ADVANCED)\n";
        std::cout << "=====================================\n\n";

        std::cout << "[WARNING] This will restart network services!\n";
        std::cout << "[?] Continue? (y/n): ";

        char confirm;
        std::cin >> confirm;
        std::cin.ignore();

        if (tolower(confirm) != 'y') {
            std::cout << "[-] Operation cancelled\n";
            system("pause");
            return false;
        }

        std::cout << "[*] Stopping network services...\n";
        system("net stop Dhcp >nul 2>&1");
        system("net stop NlaSvc >nul 2>&1");
        Sleep(2000);

        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, 255);

        char newMAC[18];
        sprintf_s(newMAC, "%02X%02X%02X%02X%02X%02X",
            dis(gen) & 0xFE, dis(gen), dis(gen),
            dis(gen), dis(gen), dis(gen));

        std::cout << "[*] Spoofing network adapters...\n";

        for (int i = 0; i < 30; i++) {
            std::string subKey = "SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e972-e325-11ce-bfc1-08002be10318}\\";
            if (i < 10) subKey += "000" + std::to_string(i);
            else subKey += "00" + std::to_string(i);

            HKEY hKey;
            if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, subKey.c_str(), 0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS) {
                RegSetValueExA(hKey, "NetworkAddress", 0, REG_SZ, (BYTE*)newMAC, strlen(newMAC) + 1);
                RegCloseKey(hKey);
            }
        }

        std::cout << "[*] Restarting network services...\n";
        Sleep(3000);
        system("net start Dhcp >nul 2>&1");
        system("net start NlaSvc >nul 2>&1");

        std::cout << "[+] Network adapters spoofed with MAC: " << newMAC << "\n";
        std::cout << "[+] Network spoof completed!\n";
        system("pause");
        return true;
    }

    void deleteEAC() {
        system("cls");
        printHeader("DELETE EASYANTICHEAT");

        std::vector<std::string> paths = {
            userProfile + "\\Desktop\\OldRust\\RustClient_Data\\Plugins\\x86_64\\EasyAntiCheat.dll",
            userProfile + "\\Desktop\\oldrust\\RustClient_Data\\Plugins\\x86_64\\EasyAntiCheat.dll",
            userProfile + "\\OneDrive\\Desktop\\OldRust\\RustClient_Data\\Plugins\\x86_64\\EasyAntiCheat.dll",
            systemDrive + "\\OldRust\\RustClient_Data\\Plugins\\x86_64\\EasyAntiCheat.dll",
            userProfile + "\\Downloads\\OldRust\\RustClient_Data\\Plugins\\x86_64\\EasyAntiCheat.dll",
            "C:\\Program Files (x86)\\Steam\\steamapps\\common\\Rust\\RustClient_Data\\Plugins\\x86_64\\EasyAntiCheat.dll",
            "C:\\Program Files (x86)\\EasyAntiCheat\\EasyAntiCheat.dll",
            userProfile + "\\AppData\\Local\\EasyAntiCheat\\EasyAntiCheat.dll"
        };
        bool found = false;
        for (const auto& p : paths) {
            if (fs::exists(p)) {
                std::cout << "Found EAC: " << p << "\n";
                if (FileManager::DeleteFileWithAttrib(p)) {
                    std::cout << "[OK] EasyAntiCheat.dll deleted.\n";
                }
                else {
                    std::cout << "[ERROR] Failed to delete file.\n";
                }
                found = true;
            }
        }
        if (!found) {
            std::cout << "EasyAntiCheat.dll not found in common locations.\n";
        }
        system("pause");
    }
    std::string findRustFolder() {
        std::vector<std::string> paths = {
            userProfile + "\\Desktop\\OldRust",
            userProfile + "\\Desktop\\oldrust",
            userProfile + "\\OneDrive\\Desktop\\OldRust",
            userProfile + "\\OneDrive\\Desktop\\oldrust",
            userProfile + "\\Downloads\\OldRust",
            userProfile + "\\Downloads\\oldrust",
            systemDrive + "\\OldRust"
        };
        for (const auto& p : paths) {
            if (fs::exists(p + "\\RustClient.exe")) {
                return p;
            }
        }
        return "";
    }
    void run() {
        while (true) {
            system("cls");
            printHeader("SJC ULTIMATE CLEANER");
            std::cout << "[1] - FULL SYSTEM CLEAN\n";
            std::cout << "[2] - RUST MANAGER\n";
            std::cout << "[0] - EXIT\n\n";
            std::cout << "Select [0-2]: ";
            int choice;
            std::cin >> choice;
            std::cin.ignore();
            switch (choice) {
            case 0:
                std::cout << "Exiting...\n";
                return;
            case 1:
                bypassClean();
                break;
            case 2:
                rustManager();
                break;
            default:
                std::cout << "Invalid choice!\n";
                system("pause");
            }
        }
    }
};
bool RequestAdminRights() {
    HANDLE hToken;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
        return false;
    }
    CloseHandle(hToken);
    return true;
}
int main() {
    SetConsoleTitleA("SJC Ultimate Cleaner v2.1");
    srand(static_cast<unsigned int>(time(nullptr)));
    HANDLE hToken;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
        std::cout << "[*] Requesting administrator privileges...\n";
        char szPath[MAX_PATH];
        GetModuleFileNameA(nullptr, szPath, MAX_PATH);
        SHELLEXECUTEINFOA sei = { sizeof(sei) };
        sei.lpVerb = "runas";
        sei.lpFile = szPath;
        sei.nShow = SW_SHOW;
        if (!ShellExecuteExA(&sei)) {
            std::cout << "[-] Failed to get administrator rights.\n";
            std::cout << "[*] Some functions may not work properly.\n";
            system("pause");
        }
        else {
            return 0; 
        }
    }
    else {
        CloseHandle(hToken);
        std::cout << "[+] Running with administrator privileges\n";
        Sleep(1000);
    }
    UltimateCleaner cleaner;
    cleaner.run();
    return 0;
}
