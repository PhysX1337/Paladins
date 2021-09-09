
#include <TlHelp32.h>
#include "DriverController.h"

DriverController* Controller = nullptr;

struct EnumData {
	DWORD dwProcessId;
	HWND hWnd;
};

BOOL CALLBACK EnumProc(HWND hWnd, LPARAM lParam)
{

	EnumData& ed = *(EnumData*)lParam;
	DWORD dwProcessId = 0x0;

	GetWindowThreadProcessId(hWnd, &dwProcessId);

	if (ed.dwProcessId == dwProcessId) {
		ed.hWnd = hWnd;
		SetLastError(ERROR_SUCCESS);
		return FALSE;
	}
	return TRUE;
}
HWND GetHwndById(DWORD dwProcessId)
{
	EnumData ed = { dwProcessId };
	if (!EnumWindows(EnumProc, (LPARAM)&ed) &&
		(GetLastError() == ERROR_SUCCESS)) {
		return ed.hWnd;
	}
	return NULL;
}
DWORD GetProcessIdByName(const char* processName) {
	PROCESSENTRY32 entry;
	entry.dwSize = sizeof(PROCESSENTRY32);

	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);

	if (Process32First(snapshot, &entry) == TRUE)
	{
		while (Process32Next(snapshot, &entry) == TRUE)
		{
			if (_stricmp(entry.szExeFile, processName) == 0)
			{
				CloseHandle(snapshot);
				return entry.th32ProcessID;
			}
		}
	}

	CloseHandle(snapshot);
	return 0;
}
DWORD64 GetBaseAddress()
{
	return Controller->GetProcessBase();
}


template <typename T>
T read(const uint64_t address)
{
	T buffer{ };
	Controller->ReadProcessMemory(address, &buffer, sizeof(T));

	return buffer;
}

template<typename T>
bool write(uint64_t address, T buffer)
{
	if (address > 0x7FFFFFFFFFFF || address < 1) return 0;

	return Controller->WriteProcessMemory(address, &buffer, sizeof(T));

	return true;
}
bool readwtf(uintptr_t Address, void * Buffer, SIZE_T Size)
{
	if (Address > 0x7FFFFFFFFFFF || Address < 1) return 0;

	Controller->ReadProcessMemory(Address, Buffer, Size);

	return true;
}
template<typename T>
inline bool read_array(uint64_t address, T* array, size_t len) {
	return Controller->ReadProcessMemory(address, array, sizeof(T) * len);
}

#define READ64(addr) read<uint64_t>(addr)