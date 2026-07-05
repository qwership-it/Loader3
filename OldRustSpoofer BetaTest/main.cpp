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

#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "advapi32.lib")

namespace fs = std::filesystem;

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

    static bool ProcessExists(const std::string& processName) {
        HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (hSnapshot == INVALID_HANDLE_VALUE) return false;

        PROCESSENTRY32W pe;
        pe.dwSize = sizeof(PROCESSENTRY32W);
        bool exists = false;

        int wideSize = MultiByteToWideChar(CP_ACP, 0, processName.c_str(), -1, nullptr, 0);
        std::wstring wideProcessName(wideSize, L'\0');
        MultiByteToWideChar(CP_ACP, 0, processName.c_str(), -1, &wideProcessName[0], wideSize);

        if (Process32FirstW(hSnapshot, &pe)) {
            do {
                if (_wcsicmp(pe.szExeFile, wideProcessName.c_str()) == 0) {
                    exists = true;
                    break;
                }
            } while (Process32NextW(hSnapshot, &pe));
        }
        CloseHandle(hSnapshot);
        return exists;
    }

    static bool HideProcess(const std::string& processName) {
        return true;
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
        std::string cmd = "rmdir /s /q \"" + path + "\"";
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

    static bool CopyFolder(const std::string& src, const std::string& dst) {
        std::string cmd = "xcopy /E /Y \"" + src + "\\*\" \"" + dst + "\\\" >nul";
        return system(cmd.c_str()) == 0;
    }
};

class HWIDManager {
public:
    static std::string GenerateFakeHWID() {
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

    static bool SpoofHWID() {
        std::string fakeHwid = GenerateFakeHWID();
        RegistryManager::SetValue(HKEY_LOCAL_MACHINE,
            "SYSTEM\\CurrentControlSet\\Control\\IDConfigDB\\Hardware Profiles\\0001",
            "HwProfileGuid", fakeHwid);

        
        SetEnvironmentVariableA("FAKE_HWID", fakeHwid.c_str());
        return true;
    }
};

class UltimateCleaner {
private:
    std::string steamPath;
    std::string userProfile;
    std::string systemDrive;
    std::string fakeHWID;

public:
    UltimateCleaner() {
        userProfile = getEnvVar("USERPROFILE");
        systemDrive = getEnvVar("SYSTEMDRIVE");
        if (systemDrive.empty()) systemDrive = "C:";
        detectSteamPath();
        fakeHWID = HWIDManager::GenerateFakeHWID();
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

    void printHeader(const std::string& title) {
        std::cout << "\n=====================================\n";
        std::cout << "      " << title << "\n";
        std::cout << "=====================================\n\n";
    }

    void safeRunOldRust() {
        printHeader("ЗАПУСК OLDRUST В БЕЗОПАСНОМ РЕЖИМЕ");

        std::string rustFolder = findRustFolder();
        if (rustFolder.empty()) {
            std::cout << "[-] Папка OldRust не найдена!\n";
            system("pause");
            return;
        }

        std::string exePath = rustFolder + "\\RustClient.exe";
        if (!fs::exists(exePath)) {
            std::cout << "[-] RustClient.exe не найден!\n";
            system("pause");
            return;
        }

        std::cout << "[+] Папка найдена: " << rustFolder << "\n";
        std::cout << "[+] Генерация фейкового HWID: " << fakeHWID << "\n";
        std::cout << "[+] Применение spoof HWID в реестре...\n";

        HWIDManager::SpoofHWID();

        SetEnvironmentVariableA("COMPUTERNAME", "FAKE-PC");
        SetEnvironmentVariableA("USERNAME", "User");
        SetEnvironmentVariableA("FAKE_HWID", fakeHWID.c_str());

        std::cout << "[+] Запуск RustClient.exe в скрытом режиме...\n";
        ProcessManager::RunHidden(exePath);

        std::cout << "\n[OK] OldRust запущен в безопасном режиме!\n";
        std::cout << "[INFO] HWID эмулирован: " << fakeHWID << "\n";
        std::cout << "[INFO] Процесс скрыт от античитов (частично)\n";
        system("pause");
    }

    void bypassClean() {
        printHeader("ЗАПУСК BYPASS ОЧИСТКИ");

        std::cout << "[1/15] Закрытие Steam...\n";
        ProcessManager::KillProcess("steam.exe");
        ProcessManager::KillProcess("steamwebhelper.exe");
        Sleep(2000);

        std::cout << "[2/15] Очистка реестра Rust...\n";
        RegistryManager::DeleteKey(HKEY_CURRENT_USER, "Software\\Facepunch Studios LTD");
        RegistryManager::DeleteKey(HKEY_CURRENT_USER, "Software\\OldDevblogRust");
        RegistryManager::DeleteKey(HKEY_CURRENT_USER, "Software\\MYRUST240DEVBLOG");

        std::cout << "[3/15] Очистка истории Win+R...\n";
        RegistryManager::DeleteKey(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\RunMRU");

        std::cout << "[4/15] Очистка истории браузера...\n";
        system("rundll32.exe inetcpl.cpl,ClearMyTracksByProcess 1");

        std::cout << "[5/15] Очистка FeatureUsage...\n";
        RegistryManager::DeleteKey(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\FeatureUsage\\AppSwitched");

        std::cout << "[6/15] Удаление ssfn файлов...\n";
        if (!steamPath.empty()) {
            std::string pattern = steamPath + "\\ssfn*";
            FileManager::DeleteFilesByPattern(pattern);
        }

        std::cout << "[7/15] Сброс loginusers.vdf...\n";
        if (!steamPath.empty()) {
            std::string loginPath = steamPath + "\\config\\loginusers.vdf";
            if (fs::exists(loginPath)) {
                SetFileAttributesA(loginPath.c_str(), FILE_ATTRIBUTE_NORMAL);
                std::ofstream ofs(loginPath);
                ofs.close();
                SetFileAttributesA(loginPath.c_str(), FILE_ATTRIBUTE_READONLY);
            }
        }

        std::cout << "[8/15] Удаление avatarcache и userdata...\n";
        if (!steamPath.empty()) {
            std::string avaPath = steamPath + "\\config\\avatarcache";
            if (fs::exists(avaPath)) FileManager::DeleteFolder(avaPath);

            std::string userPath = steamPath + "\\userdata";
            if (fs::exists(userPath)) {
                try {
                    for (const auto& entry : fs::directory_iterator(userPath)) {
                        if (entry.is_directory()) {
                            fs::remove_all(entry.path());
                        }
                    }
                }
                catch (...) {}
            }
        }

        std::cout << "[9/15] Удаление coplay файлов...\n";
        if (!steamPath.empty()) {
            std::string pattern = steamPath + "\\config\\coplay_*.vdf";
            FileManager::DeleteFilesByPattern(pattern);
        }

        std::cout << "[10/15] Очистка реестра Steam...\n";
        RegistryManager::DeleteValue(HKEY_CURRENT_USER, "Software\\Valve\\Steam", "AutoLoginUser");
        RegistryManager::DeleteValue(HKEY_CURRENT_USER, "Software\\Valve\\Steam", "RememberPassword");
        RegistryManager::DeleteValue(HKEY_CURRENT_USER, "Software\\Valve\\Steam", "MostRecentUser");

        std::cout << "[11/15] Очистка SRU...\n";
        system("net stop DPS >nul 2>&1");
        std::string sruPath = getEnvVar("windir") + "\\System32\\sru\\*";
        system(("del /f /s /q /a \"" + sruPath + "\" >nul 2>&1").c_str());
        system("net start DPS >nul 2>&1");

        std::cout << "[12/15] Очистка TEMP папок...\n";
        std::string tempPath = getEnvVar("TEMP");
        system(("del /f /s /q \"" + tempPath + "\\*\" >nul 2>&1").c_str());
        system(("for /d %i in (\"" + tempPath + "\\*\") do rmdir /s /q \"%i\" >nul 2>&1").c_str());

        std::string winTemp = getEnvVar("windir") + "\\Temp";
        system(("del /f /s /q \"" + winTemp + "\\*\" >nul 2>&1").c_str());
        system(("for /d %i in (\"" + winTemp + "\\*\") do rmdir /s /q \"%i\" >nul 2>&1").c_str());


        std::cout << "[13/15] Удаление папки OldRust...\n";
        std::string oldRustPath = systemDrive + "\\OldRust";
        if (fs::exists(oldRustPath)) FileManager::DeleteFolder(oldRustPath);


        std::cout << "[14/15] Удаление EAC с рабочего стола...\n";
        std::string eacPath = userProfile + "\\Desktop\\OldRust\\RustClient_Data\\Plugins\\x86_64\\EasyAntiCheat.dll";
        if (fs::exists(eacPath)) FileManager::DeleteFileWithAttrib(eacPath);

        std::cout << "[15/15] Очистка Prefetch (extreme/injector)...\n";
        std::string prefetchPath = getEnvVar("windir") + "\\Prefetch";
        if (fs::exists(prefetchPath)) {
            std::vector<std::string> patterns = {
                "\\*extreme*", "\\*injector*", "\\EXTREME*.PF", "\\INJECTOR*.PF"
            };
            for (const auto& p : patterns) {
                std::string pattern = prefetchPath + p;
                FileManager::DeleteFilesByPattern(pattern);
            }
        }

        // Запуск Steam
        if (!steamPath.empty() && fs::exists(steamPath + "\\steam.exe")) {
            std::cout << "\nЗапуск Steam...\n";
            ShellExecuteA(nullptr, "open", (steamPath + "\\steam.exe").c_str(), nullptr, nullptr, SW_SHOW);
        }

        std::cout << "\n=====================================\n";
        std::cout << "         BYPASS ОЧИСТКА ЗАВЕРШЕНА\n";
        std::cout << "=====================================\n\n";
        system("pause");
    }
    void rustManager() {
        while (true) {
            system("cls");
            printHeader("RUST МЕНЕДЖЕР");
            std::cout << "[1] - OldRust (запуск + мониторинг DLL)\n";
            std::cout << "[2] - Резервное копирование всех DLL\n";
            std::cout << "[3] - Удаление новых DLL (интерактивно)\n";
            std::cout << "[4] - Удаление EasyAntiCheat.dll\n";
            std::cout << "[5] - БЕЗОПАСНЫЙ ЗАПУСК (HWID Spoof + Hidden)\n";
            std::cout << "[0] - Назад в главное меню\n\n";
            std::cout << "Выберите [0-5]: ";

            int choice;
            std::cin >> choice;
            std::cin.ignore();

            switch (choice) {
            case 0: return;
            case 1: rustOpen(); break;
            case 2: rustBackup(); break;
            case 3: rustDeleteNew(); break;
            case 4: rustDeleteEAC(); break;
            case 5: safeRunOldRust(); break;
            default: std::cout << "Неверный выбор!\n"; system("pause");
            }
        }
    }

    void rustOpen() {
        system("cls");
        printHeader("ЗАПУСК OLDRUST + МОНИТОРИНГ");

        std::string rustFolder = findRustFolder();
        if (rustFolder.empty()) {
            std::cout << "Папка OldRust не найдена!\n";
            system("pause");
            return;
        }

        std::cout << "Найдена папка: " << rustFolder << "\n";
        std::cout << "Запуск RustClient.exe (скрыто)...\n";

        std::string cmd = "start /b \"\" \"" + rustFolder + "\\RustClient.exe\"";
        system(cmd.c_str());

        std::cout << "\nЗапуск мониторинга DLL (с фильтром)...\n";

        std::string psCmd =
            "$p = Get-Process -Name RustClient -ErrorAction SilentlyContinue; "
            "if ($p) { "
            "  $modules = $p.Modules | Where-Object { "
            "    $_.FileName -notlike 'C:\\Windows\\*' -and "
            "    $_.FileName -notlike 'C:\\Program Files\\*' -and "
            "    $_.FileName -notlike 'C:\\Program Files (x86)\\*' -and "
            "    $_.FileName -notlike 'C:\\ProgramData\\*' -and "
            "    $_.FileName -notlike \"" + userProfile + "\\Desktop\\OldRust\\BepInEx\\*\" "
            "  }; "
            "  $modules | Format-Table ModuleName, FileName, Size -AutoSize "
            "} else { Write-Host 'Ожидание RustClient.exe...' }; "
            "while ($true) { "
            "  Start-Sleep -Seconds 5; cls; "
            "  $p = Get-Process -Name RustClient -ErrorAction SilentlyContinue; "
            "  if ($p) { "
            "    $modules = $p.Modules | Where-Object { "
            "      $_.FileName -notlike 'C:\\Windows\\*' -and "
            "      $_.FileName -notlike 'C:\\Program Files\\*' -and "
            "      $_.FileName -notlike 'C:\\Program Files (x86)\\*' -and "
            "      $_.FileName -notlike 'C:\\ProgramData\\*' -and "
            "      $_.FileName -notlike \"" + userProfile + "\\Desktop\\OldRust\\BepInEx\\*\" "
            "    }; "
            "    $modules | Format-Table ModuleName, FileName, Size -AutoSize "
            "  } else { Write-Host 'RustClient.exe не запущен' } "
            "}";

        std::string psFullCmd = "powershell -NoExit -Command \"" + psCmd + "\"";
        ShellExecuteA(nullptr, "open", "powershell.exe", psFullCmd.c_str(), nullptr, SW_SHOW);

        std::cout << "\n[OK] OldRust запущен + открыто окно мониторинга.\n";
        std::cout << "Мониторинг обновляется каждые 5 секунд.\n";
        system("pause");
    }

    void rustBackup() {
        system("cls");
        printHeader("РЕЗЕРВНОЕ КОПИРОВАНИЕ DLL");

        std::string rustFolder = findRustFolder();
        if (rustFolder.empty()) {
            std::cout << "Папка OldRust не найдена!\n";
            system("pause");
            return;
        }

        std::string backupDir = userProfile + "\\Documents\\OldRust_Backup";
        if (!fs::exists(backupDir)) fs::create_directories(backupDir);

        std::cout << "Копирование DLL из " << rustFolder << " в " << backupDir << "...\n";
        std::string cmd = "xcopy /E /Y \"" + rustFolder + "\\*.dll\" \"" + backupDir + "\\\" >nul";
        system(cmd.c_str());

        std::cout << "[OK] Резервная копия создана.\n";
        system("pause");
    }

    void rustDeleteNew() {
        system("cls");
        printHeader("УДАЛЕНИЕ НОВЫХ DLL");

        std::string rustFolder = findRustFolder();
        if (rustFolder.empty()) {
            std::cout << "Папка OldRust не найдена!\n";
            system("pause");
            return;
        }

        std::string backupDir = userProfile + "\\Documents\\OldRust_Backup";
        if (!fs::exists(backupDir)) {
            std::cout << "Резервная копия не найдена! Сначала выполните пункт 2.\n";
            system("pause");
            return;
        }

        std::vector<std::string> newDlls;
        try {
            for (const auto& entry : fs::directory_iterator(rustFolder)) {
                if (entry.is_regular_file() && entry.path().extension() == ".dll") {
                    std::string filename = entry.path().filename().string();
                    std::string backupPath = backupDir + "\\" + filename;
                    if (!fs::exists(backupPath)) {
                        newDlls.push_back(filename);
                    }
                }
            }
        }
        catch (...) {}

        if (newDlls.empty()) {
            std::cout << "Новых DLL не найдено.\n";
            system("pause");
            return;
        }

        std::cout << "Найдено " << newDlls.size() << " новых DLL:\n\n";
        for (size_t i = 0; i < newDlls.size(); ++i) {
            std::cout << "  " << (i + 1) << " - " << newDlls[i] << "\n";
        }

        std::cout << "\nВведите номера для удаления (например, 1,2,3) или 'all': ";
        std::string input;
        std::getline(std::cin, input);

        if (input == "all") {
            for (const auto& dll : newDlls) {
                std::string path = rustFolder + "\\" + dll;
                FileManager::DeleteFileWithAttrib(path);
                std::cout << "Удалён: " << dll << "\n";
            }
        }
        else {
            std::stringstream ss(input);
            std::string token;
            std::vector<int> indices;
            while (std::getline(ss, token, ',')) {
                try {
                    int idx = std::stoi(token);
                    if (idx >= 1 && idx <= (int)newDlls.size()) {
                        indices.push_back(idx - 1);
                    }
                }
                catch (...) {}
            }
            for (int idx : indices) {
                std::string path = rustFolder + "\\" + newDlls[idx];
                FileManager::DeleteFileWithAttrib(path);
                std::cout << "Удалён: " << newDlls[idx] << "\n";
            }
        }
        std::cout << "[OK] Готово.\n";
        system("pause");
    }
    void rustDeleteEAC() {
        system("cls");
        printHeader("УДАЛЕНИЕ EASYANTICHEAT.DLL");
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
        std::string foundPath;
        for (const auto& p : paths) {
            if (fs::exists(p)) {
                foundPath = p;
                break;
            }
        }
        if (foundPath.empty()) {
            std::cout << "EasyAntiCheat.dll не найден ни в одном из известных путей.\n";
            std::cout << "[Fix] Переместите Oldrust на рабочий стол и проверьте его название Oldrust\n";
            system("pause");
            return;
        }
        std::cout << "Найден EAC: " << foundPath << "\n";
        std::cout << "Удалить EasyAntiCheat.dll? (Y/N): ";
        char confirm;
        std::cin >> confirm;
        std::cin.ignore();

        if (toupper(confirm) == 'Y') {
            std::cout << "Удаление " << foundPath << "...\n";
            if (FileManager::DeleteFileWithAttrib(foundPath)) {
                std::cout << "[OK] EasyAntiCheat.dll удалён.\n";
            }
            else {
                std::cout << "Не удалось удалить (файл может использоваться закройте rust и повторите попытку).\n";
            }
        }
        else {
            std::cout << "Отменено.\n";
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
            std::cout << "[1] - BYPASS (ПОЛНАЯ ОЧИСТКА)\n";
            std::cout << "[2] - RUST (Менеджер OldRust)\n";
            std::cout << "[3] - БЕЗОПАСНЫЙ ЗАПУСК OLDRUST (HWID Spoof)\n";
            std::cout << "[0] - ВЫХОД\n\n";
            std::cout << "Выберите [0-3]: ";

            int choice;
            std::cin >> choice;
            std::cin.ignore();

            switch (choice) {
            case 0:
                std::cout << "Выход...\n";
                return;
            case 1:
                bypassClean();
                break;
            case 2:
                rustManager();
                break;
            case 3:
                safeRunOldRust();
                break;
            default:
                std::cout << "Неверный выбор!\n";
                system("pause");
            }
        }
    }
};
int main() {
    SetConsoleOutputCP(1251);
    SetConsoleCP(1251);
    SetConsoleTitleA("SJC Ultimate Spoofer");
    HANDLE hToken;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
        std::cout << "Запрос прав администратора...\n";
        char szPath[MAX_PATH];
        GetModuleFileNameA(nullptr, szPath, MAX_PATH);
        SHELLEXECUTEINFOA sei = { sizeof(sei) };
        sei.lpVerb = "runas";
        sei.lpFile = szPath;
        sei.nShow = SW_SHOW;
        if (!ShellExecuteExA(&sei)) {
            std::cout << "Не удалось получить права администратора.\n";
            system("pause");
            return 1;
        }
        return 0;
    }
    CloseHandle(hToken);
    UltimateCleaner cleaner;
    cleaner.run();
    return 0;
}