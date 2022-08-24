// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef _MISC_HEADER
#define _MISC_HEADER 1

#include <windows.h>

#define CHECK_CLOSE_HANDLE(Handle) \
{\
	if (Handle == 0) \
		Handle = INVALID_HANDLE_VALUE; \
    if (Handle != INVALID_HANDLE_VALUE) \
    {\
        CloseHandle(Handle); \
        Handle = INVALID_HANDLE_VALUE; \
    }\
}

typedef struct PHY_DRIVE_INFO
{
	DWORD PhyDrive;
	UINT64 SizeInBytes;
	BYTE DeviceType;
	BOOL RemovableMedia;
	CHAR VendorId[128];
	CHAR ProductId[128];
	STORAGE_BUS_TYPE BusType;
	CHAR DriveLetters[26];
}PHY_DRIVE_INFO;

BOOL IsAdmin(void);
DWORD ObtainPrivileges(LPCTSTR Privilege);
int GetRegDword(HKEY Key, LPCSTR SubKey, LPCSTR ValueName, DWORD* pValue);
CHAR* GetRegSz(HKEY Key, LPCSTR SubKey, LPCSTR ValueName);
void TrimString(CHAR* String);
const char* GetBusTypeString(STORAGE_BUS_TYPE Type);
DWORD GetDriveCount(void);
HANDLE GetHandleByLetter(char Letter);
HANDLE GetHandleById(DWORD Id);
BOOL GetDriveByLetter(char Letter, DWORD* pDrive);
UINT64 GetDriveSize(HANDLE hDisk);
BOOL GetDriveInfoList(PHY_DRIVE_INFO** pDriveList, DWORD* pDriveCount);
HANDLE LockDriveByLetter(char Letter);
HANDLE* LockDriveById(DWORD Id);
void UnlockDrive(HANDLE* hList);
BOOL ManageService(PCSTR pService, BOOL bStop);
void KillProcessByName(WCHAR* pName, UINT uExitCode);
void KillProcessById(DWORD dwProcessId, UINT uExitCode);

void SetDebug(const char* cond);

extern PHY_DRIVE_INFO* gDriveList;
extern DWORD gDriveCount;

/* Callback command types (some errorcode were filled from HPUSBFW V2.2.3 and their
   designation from docs.microsoft.com/windows/win32/api/vds/nf-vds-ivdsvolumemf2-formatex */
typedef enum
{
	FCC_PROGRESS,
	FCC_DONE_WITH_STRUCTURE,
	FCC_UNKNOWN2,
	FCC_INCOMPATIBLE_FILE_SYSTEM,
	FCC_UNKNOWN4,
	FCC_UNKNOWN5,
	FCC_ACCESS_DENIED,
	FCC_MEDIA_WRITE_PROTECTED,
	FCC_VOLUME_IN_USE,
	FCC_CANT_QUICK_FORMAT,
	FCC_UNKNOWNA,
	FCC_DONE,
	FCC_BAD_LABEL,
	FCC_UNKNOWND,
	FCC_OUTPUT,
	FCC_STRUCTURE_PROGRESS,
	FCC_CLUSTER_SIZE_TOO_SMALL,
	FCC_CLUSTER_SIZE_TOO_BIG,
	FCC_VOLUME_TOO_SMALL,
	FCC_VOLUME_TOO_BIG,
	FCC_NO_MEDIA_IN_DRIVE,
	FCC_UNKNOWN15,
	FCC_UNKNOWN16,
	FCC_UNKNOWN17,
	FCC_DEVICE_NOT_READY,
	FCC_CHECKDISK_PROGRESS,
	FCC_UNKNOWN1A,
	FCC_UNKNOWN1B,
	FCC_UNKNOWN1C,
	FCC_UNKNOWN1D,
	FCC_UNKNOWN1E,
	FCC_UNKNOWN1F,
	FCC_READ_ONLY_MODE,
	FCC_UNKNOWN21,
	FCC_UNKNOWN22,
	FCC_UNKNOWN23,
	FCC_UNKNOWN24,
	FCC_ALIGNMENT_VIOLATION,
} FILE_SYSTEM_CALLBACK_COMMAND;

typedef struct
{
	DWORD Lines;
	CHAR* Output;
} TEXTOUTPUT, * PTEXTOUTPUT;

typedef BOOLEAN(__stdcall* FILE_SYSTEM_CALLBACK)(FILE_SYSTEM_CALLBACK_COMMAND Command, ULONG Action, PVOID pData);

BOOL
FmifsFormatEx(CONST WCHAR* DriveRoot, WCHAR* FileSystemTypeName,
	WCHAR* Label, BOOL QuickFormat, BOOL EnableCompression,
	ULONG DesiredUnitAllocationSize,
	FILE_SYSTEM_CALLBACK Callback);

#endif
