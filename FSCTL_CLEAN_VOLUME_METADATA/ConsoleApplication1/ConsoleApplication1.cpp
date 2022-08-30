// ConsoleApplication1.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <cstdio>
#include <windows.h>
#include <cstdlib>
#include <iso646.h>

void OCDPuts(_In_z_ wchar_t const* _Buffer) noexcept {
    //_putws: https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/puts-putws?view=msvc-170
    // Returns a nonnegative value if successful. If _putws fails, it returns WEOF.
    const int result = _putws(_Buffer);
    if (result == WEOF) {
        std::abort();
    }
}


void dumpLastError(_In_opt_z_ PCWSTR const optionalMessage) noexcept {
    // GetLastError function (errhandlingapi.h): https://docs.microsoft.com/en-us/windows/win32/api/errhandlingapi/nf-errhandlingapi-getlasterror
    const DWORD lastErr = ::GetLastError();

    // FormatMessage function (winbase.h): https://docs.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-formatmessage
    //  If the function succeeds, the return value is the number of TCHARs stored in the output buffer, excluding the terminating null character.
    //  If the function fails, the return value is zero. To get extended error information, call GetLastError.


    // I guess I could parse the longest error in the system message tables by looking at the comments in winerror.h, but meh, I'm not *THAT* OCD.
    constexpr const rsize_t ERROR_MESSAGE_BUFFER_SIZE = 512u;
    TCHAR errMessageBuffer[ERROR_MESSAGE_BUFFER_SIZE] = {};

    const BOOL formatResult = ::FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM, nullptr, lastErr, 0, errMessageBuffer, ERROR_MESSAGE_BUFFER_SIZE, nullptr);
    if (formatResult == 0) {
        OCDPuts(L"FormatMessageW failed! Not much I can do. I'll try to dump the error code from GetLastError before aborting...\r\n");
        const DWORD formatMessageError = ::GetLastError();
        ::wprintf_s(L"Was handling error %u, FormatMessageW failed with error: %u\r\n", lastErr, formatMessageError);
        std::abort();
        }
    // YES, I'm *THIS* OCD though.
    // wprintf_s: https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/printf-s-printf-s-l-wprintf-s-wprintf-s-l
    //  Returns the number of characters printed, or a negative value if an error occurs.
    const int printResult = ::wprintf_s(L"%s%s", optionalMessage, errMessageBuffer);
    if (printResult < 0) {
        OCDPuts(L"Couldn't print error message.");
        std::abort();
        }
    }


void dumpInfo() noexcept {
    OCDPuts(L"Trying to clean metadata/unused MFT entries. Make sure You've added the MiniNt key manually, and that you then remove it when done.");
    OCDPuts(L"See also: https://grzegorztworek.medium.com/cleaning-ntfs-artifacts-with-fsctl-clean-volume-metadata-bd29afef290c");
    OCDPuts(L"And: https://twitter.com/0gtweet/status/1203719373472575488");
    }

int main() {
    dumpInfo();
    
    constexpr PCWSTR const volumeMountPoint = L"C:\\";
    constexpr const rsize_t MAX_GUID_PATH = 50u;
    wchar_t lpszVolumeName[MAX_GUID_PATH] = {};

    // GetVolumeNameForVolumeMountPointW: https://docs.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-getvolumenameforvolumemountpointw
    //  If the function succeeds, the return value is nonzero.
    //  If the function fails, the return value is zero. To get extended error information, call GetLastError.
    const BOOL volumeNameResult = ::GetVolumeNameForVolumeMountPointW(volumeMountPoint, lpszVolumeName, MAX_GUID_PATH);
    if (volumeNameResult == 0) {
        dumpLastError(L"GetVolumeNameForVolumeMountPointW failed! Details: ");
        return 1;
        }
    const int printVolumeNameResult = ::wprintf_s(L"\r\nvolumeName: '%s'\r\n", lpszVolumeName);
    if (printVolumeNameResult < 0) {
        OCDPuts(L"Failed to print volume name!");
        std::abort();
        }

    PWSTR lastSlash = wcsrchr(lpszVolumeName, L'\\');
    if (lastSlash == nullptr) {
        std::abort();
    }
    *lastSlash = 0;
    OCDPuts(lpszVolumeName);

    // Note to self, opening handle to volume should be similar to my work in altWinDirStat:
    // https://github.com/ariccio/altWinDirStat/blob/41dd067ce29d15113625759836b20911479008cb/WinDirStat/windirstat/directory_enumeration.cpp#L479

    // CreateFileW function (fileapi.h): https://docs.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-createfilew
    // Creates or opens a file or I/O device.
    // If the function succeeds, the return value is an open handle to the specified file, device, named pipe, or mail slot.
    // If the function fails, the return value is INVALID_HANDLE_VALUE. To get extended error information, call GetLastError.
    const HANDLE volumeHandle = ::CreateFileW(lpszVolumeName, GENERIC_READ, (FILE_SHARE_READ bitor FILE_SHARE_WRITE), nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (volumeHandle == INVALID_HANDLE_VALUE) {
        dumpLastError(L"Failed to open handle to the volume. Details: ");
        return 1;
        }
    OCDPuts(L"Got volume handle...");

    // DeviceIoControl function: https://docs.microsoft.com/en-us/windows/win32/api/ioapiset/nf-ioapiset-deviceiocontrol
    const BOOL deviceIOResult = ::DeviceIoControl(volumeHandle, FSCTL_CLEAN_VOLUME_METADATA, nullptr, 0, nullptr, 0, nullptr, nullptr);
    if (deviceIOResult == 0) {
        dumpLastError(L"DeviceIoControl: FSCTL_CLEAN_VOLUME_METADATA returned zero. Details: ");
        }
    else {
        OCDPuts(L"DeviceIoControl: FSCTL_CLEAN_VOLUME_METADATA seems to have suceeded.");
        ::wprintf_s(L"DeviceIoControl returned code: %i", deviceIOResult);
        }
    }
