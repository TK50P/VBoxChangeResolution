#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstdlib> // For system()
#include <windows.h>
#include <shlwapi.h> // For StrStrI()
#pragma comment(lib, "shlwapi.lib") // Link against Shlwapi.lib for StrStrI()
#define _CRT_SECURE_NO_WARNINGS
#pragma warning(disable: 4996)
constexpr auto IDR_VIDEO1 = 102;

bool copyFile(const std::string& srcPath, const std::string& destPath) {
    std::ifstream src(srcPath, std::ios::binary);
    std::ofstream dest(destPath, std::ios::binary);

    if (!src.is_open() || !dest.is_open()) {
        std::cerr << "Failed to open source or destination file.\n";
        return false;
    }

    dest << src.rdbuf();

    src.close();
    dest.close();
    return true;
}

bool extractResourceToFile(int resourceID, const std::string& outputPath) {
    HRSRC hResource = FindResource(NULL, MAKEINTRESOURCE(IDR_VIDEO1), ("Video"));
    if (!hResource) {
        std::cerr << "Failed to find resource.\n";
        return false;
    }
    HGLOBAL hLoadedResource = LoadResource(NULL, hResource);
    if (!hLoadedResource) {
        std::cerr << "Failed to load resource.\n";
        return false;
    }
    LPVOID pLockedResource = LockResource(hLoadedResource);
    if (!pLockedResource) {
        std::cerr << "Failed to lock resource.\n";
        return false;
    }
    DWORD dwResourceSize = SizeofResource(NULL, hResource);
    if (dwResourceSize == 0) {
        std::cerr << "Resource size is zero.\n";
        return false;
    }

    std::ofstream outFile(outputPath, std::ios::binary);
    if (!outFile) {
        std::cerr << "Failed to open output file.\n";
        return false;
    }
    outFile.write(reinterpret_cast<const char*>(pLockedResource), dwResourceSize);
    outFile.close();
}

// Helper function to convert std::string to std::wstring
std::wstring stringToWideString(const std::string& str) {
    if (str.empty()) return std::wstring();
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    std::wstring wstr(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstr[0], size_needed);
    return wstr;
}

bool AddToSystemPath(const std::string& checkPath) {
    // Get the current PATH environment variable
    char* pathEnv = getenv("PATH");
    if (pathEnv == nullptr) {
        std::cerr << "Failed to get PATH environment variable.\n";
        return false;
    }

    std::string currentPath = pathEnv;
    std::wstring currentPathW = stringToWideString(currentPath);
    std::wstring checkPathW = stringToWideString(checkPath);

    // Check if the path is already in the PATH environment variable
    if (currentPath.find(checkPath) == std::string::npos) {
        std::cout << "The path is not in the system PATH.\n";
        std::cout << "Do you want to add it? (yes/no): ";
        std::string response;
        std::getline(std::cin, response);

        if (response == "yes") {
            std::wstring newPath = currentPathW + L";" + checkPathW;
            if (!SetEnvironmentVariableW(L"PATH", newPath.c_str())) {
                std::cerr << "Failed to set PATH environment variable.\n";
                return false;
            }

            // Notify the system of the change
            SendMessageTimeout(HWND_BROADCAST, WM_SETTINGCHANGE, 0, (LPARAM)L"Environment", SMTO_ABORTIFHUNG, 5000, NULL);
            std::cout << "The path was successfully added to the system PATH.\n";
        }
    }
    else {
        NULL;
    }

    return true;
}

int main() {
    std::string vmName;
    int resolutionChoice;
    std::string resolution;
    std::string command;  // Declare 'command' here to ensure it's accessible throughout main()

    const std::string vboxPath = "C:\\Program Files\\Oracle\\VirtualBox";

    if (!AddToSystemPath(vboxPath)) {
        std::cerr << "An error occurred.\n";
        return 1;
    }

    // Set the title of the console window
    SetConsoleTitle(TEXT("VBoxChangeResolution"));

    // Prompt for VM name
    do {
        std::cout << "Enter the name of the VM: ";
        std::getline(std::cin, vmName);

        std::string command = "VBoxManage showvminfo \"" + vmName + "\" 2>&1 | findstr /c:\"VBOX_E_OBJECT_NOT_FOUND\" >nul";
        if (system(command.c_str()) == 0) {
            std::cout << "Invalid VM name: \"" << vmName << "\". Make sure you checked case sensitivity.\n";
            continue;
        }

        command = "VBoxManage showvminfo \"" + vmName + "\" | findstr /c:\"Firmware\" | findstr /c:\"EFI\" >nul";
        if (system(command.c_str()) != 0) {
            std::cout << "The VM \"" << vmName << "\" is not using UEFI.\n";
            continue;
        }

        break;
    } while (true);

    // Choose resolution
    do {
        std::cout << "\nChoose the Resolution:\n";
        std::cout << "1. 1152x864\n";
        std::cout << "2. 1366x768\n";
        std::cout << "3. 1440x900\n";
        std::cout << "4. 1920x1080\n";
        std::cout << "5. Custom resolution\n";
        std::cout << "Enter the number (1-5): ";
        std::cin >> resolutionChoice;
        std::cin.ignore(); // Ignore the newline after integer input

        switch (resolutionChoice) {
        case 1: resolution = "1152x864"; break;
        case 2: resolution = "1366x768"; break;
        case 3: resolution = "1440x900"; break;
        case 4: resolution = "1920x1080"; break;
        case 5:
            std::cout << "Enter the custom resolution (e.g., 1600x900): ";
            std::getline(std::cin, resolution);
            break;
        case 39: { // Easter egg case
            std::cout << "Easter Egg Activated: Hatsune Miku Time!\n";
            // Extract the video file to the %temp% directory
            char tempPath[MAX_PATH];
            GetTempPath(MAX_PATH, tempPath);
            std::string tempFilePath = std::string(tempPath) + "Miku Miku!.mp4";
            extractResourceToFile(IDR_VIDEO1, tempFilePath);
            // Check if the file was successfully extracted
            std::ifstream inFile(tempFilePath, std::ios::binary);
            if (!inFile) {
                std::cerr << "Failed to extract video resource.\n";
                break;
            }
            inFile.close();
            // Play the temporary file using VLC
            std::string command = "start /B \"\" \"" + std::string("C:\\Program Files\\VideoLAN\\VLC\\vlc.exe") + "\" \"" + tempFilePath + "\"";
            system(command.c_str());
            for (int i = 0; i < 100; ++i) { // Spam the console
                std::cout << "Miku, Miku!\n";
            }
            // Handle user input for saving or deleting the video
            std::cout << "Do you want to save the video? (Y/N): ";
            char choice;
            std::cin >> choice; // Ensure this captures user input correctly

            if (tolower(choice) == 'y') {
                std::string savePath;
                std::cout << "Enter full path to save the video, including filename (e.g., C:\\Videos\\Miku.mp4): ";
                std::cin >> savePath; // Make sure to capture the full path correctly

                if (!copyFile(tempFilePath, savePath)) { // Assuming copyFile returns a bool indicating success/failure
                    std::cerr << "Failed to save video resource.\n";
                }
                else {
                    std::cout << "Video saved to " << savePath << "\n";
                }
            }
            else {
                std::cout << "Video not saved.\n";
            }
            // Cleanup
            DeleteFile(tempFilePath.c_str());
            break;
        }
        default:
            std::cout << "Invalid choice. Please enter a number between 1 and 5.\n";
            continue;
        }
        break;
    } while (true);

    // Execute VBoxManage command
    command = "VBoxManage setextradata \"" + vmName + "\" VBoxInternal2/EfiGraphicsResolution " + resolution;
    system(command.c_str());
    std::cout << "\nResolution \"" << resolution << "\" has successfully changed for \"" << vmName << "\".\n\n";

    system("pause");
    return 0;
}
