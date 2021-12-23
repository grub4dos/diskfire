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
const CHAR* GuidToStr(UCHAR Guid[16]);
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

void SetDebug(const char* cond);

extern PHY_DRIVE_INFO* gDriveList;
extern DWORD gDriveCount;

#endif
