// SPDX-License-Identifier: GPL-3.0-or-later
#include <stdio.h>
#include <string.h>
#include <windows.h>
#include <process.h>
#include <tlhelp32.h>
#include <locale.h>
#include "misc.h"
#include "compat.h"

BOOL IsAdmin(void)
{
	BOOL b;
	SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
	PSID AdministratorsGroup;
	b = AllocateAndInitializeSid(&NtAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS,
		0, 0, 0, 0, 0, 0, &AdministratorsGroup);
	if (b)
	{
		if (!CheckTokenMembership(NULL, AdministratorsGroup, &b))
			b = FALSE;
		FreeSid(AdministratorsGroup);
	}
	return b;
}

DWORD ObtainPrivileges(LPCTSTR Privilege)
{
	HANDLE hToken;
	TOKEN_PRIVILEGES tkp = { 0 };
	BOOL res;
	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
		return GetLastError();
	res = LookupPrivilegeValue(NULL, Privilege, &tkp.Privileges[0].Luid);
	if (!res)
		return GetLastError();
	tkp.PrivilegeCount = 1;
	tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, (PTOKEN_PRIVILEGES)NULL, 0);
	return GetLastError();
}

int GetRegDword(HKEY Key, LPCSTR SubKey, LPCSTR ValueName, DWORD* pValue)
{
	HKEY hKey;
	DWORD Type;
	DWORD Size;
	LSTATUS lRet;
	DWORD Value = 0;
	lRet = RegOpenKeyExA(Key, SubKey, 0, KEY_QUERY_VALUE, &hKey);
	if (lRet != ERROR_SUCCESS)
		return 1;
	Size = sizeof(Value);
	lRet = RegQueryValueExA(hKey, ValueName, NULL, &Type, (LPBYTE)&Value, &Size);
	*pValue = Value;
	RegCloseKey(hKey);
	return 0;
}

CHAR* GetRegSz(HKEY Key, LPCSTR SubKey, LPCSTR ValueName)
{
	HKEY hKey;
	DWORD Type;
	DWORD Size = 1024;
	LSTATUS lRet;
	CHAR* sRet = NULL;
	lRet = RegOpenKeyExA(Key, SubKey, 0, KEY_QUERY_VALUE, &hKey);
	if (lRet != ERROR_SUCCESS)
		return NULL;
	sRet = malloc(Size);
	if (!sRet)
		return NULL;
	lRet = RegQueryValueExA(hKey, ValueName, NULL, &Type, (LPBYTE)sRet, &Size);
	if (lRet != ERROR_SUCCESS)
	{
		free(sRet);
		return NULL;
	}
	RegCloseKey(hKey);
	return sRet;
}

void TrimString(CHAR* String)
{
	CHAR* Pos1 = String;
	CHAR* Pos2 = String;
	size_t Len = strlen(String);
	while (Len > 0)
	{
		if (String[Len - 1] != ' ' && String[Len - 1] != '\t')
			break;
		String[Len - 1] = 0;
		Len--;
	}
	while (*Pos1 == ' ' || *Pos1 == '\t')
		Pos1++;
	while (*Pos1)
		*Pos2++ = *Pos1++;
	*Pos2++ = 0;
}

const char*
GetBusTypeString(STORAGE_BUS_TYPE Type)
{
	switch (Type)
	{
	case BusTypeScsi: return "SCSI";
	case BusTypeAtapi: return "Atapi";
	case BusTypeAta: return "ATA";
	case BusType1394: return "1394";
	case BusTypeSsa: return "SSA";
	case BusTypeFibre: return "Fibre";
	case BusTypeUsb: return "USB";
	case BusTypeRAID: return "RAID";
	case BusTypeiScsi: return "iSCSI";
	case BusTypeSas: return "SAS";
	case BusTypeSata: return "SATA";
	case BusTypeSd: return "SD";
	case BusTypeMmc: return "MMC";
	case BusTypeVirtual: return "Virtual";
	case BusTypeFileBackedVirtual: return "FileBackedVirtual";
	case BusTypeSpaces: return "Spaces";
	case BusTypeNvme: return "NVMe";
	case BusTypeSCM: return "SCM";
	case BusTypeUfs: return "UFS";
	}
	return "UNKNOWN";
}

DWORD GetDriveCount(void)
{
	DWORD Value = 0;
	if (GetRegDword(HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Services\\disk\\Enum", "Count", &Value) != 0)
		Value = 0;
	return Value;
}

HANDLE GetHandleByLetter(char Letter)
{
	char PhyPath[] = "\\\\.\\A:";
	snprintf(PhyPath, sizeof(PhyPath), "\\\\.\\%C:", Letter);
	return CreateFileA(PhyPath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_EXISTING, 0, 0);
}

HANDLE GetHandleById(DWORD Id)
{
	char PhyPath[] = "\\\\.\\PhysicalDrive4294967295";
	snprintf(PhyPath, sizeof(PhyPath), "\\\\.\\PhysicalDrive%u", Id);
	return CreateFileA(PhyPath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_EXISTING, 0, 0);
}

BOOL GetDriveByLetter(char Letter, DWORD* pDrive)
{
	BOOL Ret;
	DWORD dwSize = 0;
	VOLUME_DISK_EXTENTS DiskExtents = { 0 };
	HANDLE Handle = GetHandleByLetter(Letter);
	if (Handle == INVALID_HANDLE_VALUE)
		return FALSE;
	Ret = DeviceIoControl(Handle, IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS,
		NULL, 0, &DiskExtents, (DWORD)(sizeof(DiskExtents)), &dwSize, NULL);
	if (!Ret || DiskExtents.NumberOfDiskExtents == 0)
	{
		CHECK_CLOSE_HANDLE(Handle);
		return FALSE;
	}
	CHECK_CLOSE_HANDLE(Handle);
	*pDrive = DiskExtents.Extents[0].DiskNumber;
	return TRUE;
}

UINT64
GetDriveSize(HANDLE hDisk)
{
	DWORD dwBytes;
	UINT64 Size = 0;
	GET_LENGTH_INFORMATION LengthInfo = { 0 };
	if (DeviceIoControl(hDisk, IOCTL_DISK_GET_LENGTH_INFO, NULL, 0,
		&LengthInfo, sizeof(LengthInfo), &dwBytes, NULL))
		Size = LengthInfo.Length.QuadPart;
	return Size;
}

static void AddLetter(CHAR* LogLetter, char Letter)
{
	if (Letter < 'A' || Letter > 'Z')
		return;
	for (int i = 0; i < 26; i++)
	{
		if (LogLetter[i] == 0)
		{
			LogLetter[i] = Letter;
			break;
		}
	}
}

struct DRV_LETTER_LIST
{
	CHAR List[26];
};

BOOL GetDriveInfoList(PHY_DRIVE_INFO** pDriveList, DWORD* pDriveCount)
{
	DWORD i;
	DWORD Count;
	DWORD id;
	char Letter = 'A';
	BOOL  bRet;
	DWORD dwBytes;
	HANDLE Handle = INVALID_HANDLE_VALUE;
	PHY_DRIVE_INFO* CurDrive;
	GET_LENGTH_INFORMATION LengthInfo = { 0 };
	STORAGE_PROPERTY_QUERY Query = { 0 };
	STORAGE_DESCRIPTOR_HEADER DevDescHeader = { 0 };
	STORAGE_DEVICE_DESCRIPTOR* pDevDesc;
	struct DRV_LETTER_LIST* LogLetter = NULL;

	Count = GetDriveCount();
	if (Count == 0)
		return FALSE;
	LogLetter = calloc (Count, sizeof(struct DRV_LETTER_LIST));
	if (!LogLetter)
		return FALSE;
	*pDriveList = calloc(Count, sizeof(PHY_DRIVE_INFO));
	if (!*pDriveList)
	{
		free(LogLetter);
		return FALSE;
	}
	CurDrive = *pDriveList;

	dwBytes = GetLogicalDrives();

	while (dwBytes)
	{
		if (dwBytes & 0x01)
		{
			if (GetDriveByLetter(Letter, &id) && id < Count)
			{
				AddLetter(LogLetter[id].List, Letter);
			}
		}
		Letter++;
		dwBytes >>= 1;
	}

	for (i = 0; i < Count; i++)
	{
		CHECK_CLOSE_HANDLE(Handle);

		Handle = GetHandleById(i);

		if (!Handle || Handle == INVALID_HANDLE_VALUE)
			continue;

		bRet = DeviceIoControl(Handle, IOCTL_DISK_GET_LENGTH_INFO, NULL, 0,
			&LengthInfo, sizeof(LengthInfo), &dwBytes, NULL);
		if (!bRet)
			continue;

		Query.PropertyId = StorageDeviceProperty;
		Query.QueryType = PropertyStandardQuery;

		bRet = DeviceIoControl(Handle, IOCTL_STORAGE_QUERY_PROPERTY, &Query, sizeof(Query),
			&DevDescHeader, sizeof(STORAGE_DESCRIPTOR_HEADER), &dwBytes, NULL);
		if (!bRet)
			continue;

		if (DevDescHeader.Size < sizeof(STORAGE_DEVICE_DESCRIPTOR))
			continue;

		pDevDesc = (STORAGE_DEVICE_DESCRIPTOR*)malloc(DevDescHeader.Size);
		if (!pDevDesc)
			continue;

		bRet = DeviceIoControl(Handle, IOCTL_STORAGE_QUERY_PROPERTY, &Query, sizeof(Query),
			pDevDesc, DevDescHeader.Size, &dwBytes, NULL);
		if (!bRet)
		{
			free(pDevDesc);
			continue;
		}

		CurDrive->PhyDrive = i;
		CurDrive->SizeInBytes = LengthInfo.Length.QuadPart;
		CurDrive->DeviceType = pDevDesc->DeviceType;
		CurDrive->RemovableMedia = pDevDesc->RemovableMedia;
		CurDrive->BusType = pDevDesc->BusType;

		if (pDevDesc->VendorIdOffset)
		{
			strcpy_s(CurDrive->VendorId, sizeof(CurDrive->VendorId),
				(char*)pDevDesc + pDevDesc->VendorIdOffset);
			TrimString(CurDrive->VendorId);
		}

		if (pDevDesc->ProductIdOffset)
		{
			strcpy_s(CurDrive->ProductId, sizeof(CurDrive->ProductId),
				(char*)pDevDesc + pDevDesc->ProductIdOffset);
			TrimString(CurDrive->ProductId);
		}

		memcpy(CurDrive->DriveLetters, LogLetter[CurDrive->PhyDrive].List, 26);

		CurDrive++;

		free(pDevDesc);

		CHECK_CLOSE_HANDLE(Handle);
	}

	*pDriveCount = Count;
	free(LogLetter);
	return TRUE;
}

HANDLE LockDriveByLetter(char Letter)
{
	BOOL bRet;
	HANDLE hVolume = INVALID_HANDLE_VALUE;
	char PhyPath[] = "\\\\.\\A:";
	DWORD dwReturn;
	snprintf(PhyPath, sizeof(PhyPath), "\\\\.\\%C:", Letter);
	hVolume = CreateFileA(PhyPath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_EXISTING, 0, 0);
	bRet = DeviceIoControl(hVolume, FSCTL_LOCK_VOLUME, NULL, 0, NULL, 0, &dwReturn, NULL);
	if (!bRet)
		CHECK_CLOSE_HANDLE(hVolume);
	return hVolume;
}

HANDLE* LockDriveById(DWORD Id)
{
	HANDLE* hVolList = NULL;
	int i;
	if (Id > gDriveCount || !gDriveList[Id].DriveLetters[0])
		return NULL;
	hVolList = grub_calloc(26, sizeof(HANDLE));
	if (!hVolList)
		return NULL;
	for (i = 0; i < 26; i++)
	{
		if (!gDriveList[Id].DriveLetters[i])
			break;
		hVolList[i] = LockDriveByLetter(gDriveList[Id].DriveLetters[i]);
	}
	return hVolList;
}

void UnlockDrive(HANDLE* hList)
{
	int i;
	if (!hList)
		return;
	for (i = 0; i < 26; i++)
	{
		CHECK_CLOSE_HANDLE(hList[i]);
	}
	grub_free(hList);
}

BOOL ManageService(PCSTR pService, BOOL bStop)
{
	BOOL bResult = FALSE;
	SC_HANDLE hManager = OpenSCManagerA(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (hManager)
	{
		SC_HANDLE hService = OpenServiceA(hManager, pService, SERVICE_START | SERVICE_STOP);
		if (hService)
		{
			if (bStop)
			{
				SERVICE_STATUS sStatus;
				bResult = ControlService(hService, SERVICE_CONTROL_STOP, &sStatus);
			}
			else
			{
				bResult = StartServiceA(hService, 0, NULL);
			}

			CloseServiceHandle(hService);
		}
		CloseServiceHandle(hManager);
	}
	return bResult;
}

void
KillProcessByName(WCHAR* pName, UINT uExitCode)
{
	HANDLE hSnapShot = CreateToolhelp32Snapshot(TH32CS_SNAPALL, 0);
	PROCESSENTRY32W pEntry = { .dwSize = sizeof(pEntry) };
	BOOL bRet;
	if (hSnapShot == INVALID_HANDLE_VALUE)
		return;
	bRet = Process32FirstW(hSnapShot, &pEntry);
	while (bRet)
	{
		if (_wcsicmp(pEntry.szExeFile, pName) == 0)
		{
			HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, 0, pEntry.th32ProcessID);
			if (hProcess != NULL)
			{
				TerminateProcess(hProcess, uExitCode);
				CloseHandle(hProcess);
			}
		}
		bRet = Process32NextW(hSnapShot, &pEntry);
	}
	CloseHandle(hSnapShot);
}

void
KillProcessById(DWORD dwProcessId, UINT uExitCode)
{
	HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, dwProcessId);
	TerminateProcess(hProcess, uExitCode);
	CloseHandle(hProcess);
}

/* http://msdn.microsoft.com/en-us/library/windows/desktop/aa383357.aspx */
typedef enum
{
	FPF_COMPRESSED = 0x01
} FILE_SYSTEM_PROP_FLAG;

static MEDIA_TYPE
GetMediaType(CONST WCHAR* DriveRoot)
{
	HANDLE hDrive = INVALID_HANDLE_VALUE;
	DWORD dwReturn = sizeof(DISK_GEOMETRY);
	DISK_GEOMETRY diskGeometry = { 0 };
	WCHAR* wDrive = NULL;
	size_t wDriveLen = wcslen(DriveRoot) + 5;
	wDrive = calloc(wDriveLen, sizeof(WCHAR));
	if (!wDrive)
		return Unknown;
	swprintf(wDrive, wDriveLen, L"\\\\.\\%s", DriveRoot);
	hDrive = CreateFileW(wDrive, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_EXISTING, 0, 0);
	if (hDrive == INVALID_HANDLE_VALUE)
		goto fail;
	DeviceIoControl(hDrive, IOCTL_DISK_GET_DRIVE_GEOMETRY, NULL, 0, &diskGeometry, dwReturn, &dwReturn, NULL);
fail:
	if (hDrive != INVALID_HANDLE_VALUE)
		CloseHandle(hDrive);
	free(wDrive);
	return diskGeometry.MediaType;
}

BOOL
FmifsFormatEx(CONST WCHAR* DriveRoot, WCHAR* FileSystemTypeName,
	WCHAR* Label, BOOL QuickFormat, BOOL EnableCompression,
	ULONG DesiredUnitAllocationSize,
	FILE_SYSTEM_CALLBACK Callback)
{
	BOOL bRet = FALSE;
	WCHAR* wDrive = NULL;
	size_t szDriveLen = wcslen(DriveRoot);
	VOID(WINAPI * NtFormatEx)(WCHAR * DriveRoot,
		MEDIA_TYPE MediaType, WCHAR * FileSystemTypeName,
		WCHAR * Label, BOOL QuickFormat, ULONG DesiredUnitAllocationSize,
		FILE_SYSTEM_CALLBACK Callback) = NULL;
	BOOLEAN(WINAPI * NtEnableVolumeCompression)(WCHAR * DriveRoot, ULONG CompressionFlags) = NULL;
	// LoadLibrary("fmifs.dll") appears to changes the locale, which can lead to
	// problems with tolower(). Make sure we restore the locale. For more details,
	// see https://sourceforge.net/p/mingw/mailman/message/29269040/
	char* locale = setlocale(LC_ALL, NULL);
	HMODULE hL = LoadLibraryW(L"fmifs.dll");
	setlocale(LC_ALL, locale);
	if (hL)
	{
		*(FARPROC*)&NtFormatEx = GetProcAddress(hL, "FormatEx");
		*(FARPROC*)&NtEnableVolumeCompression = GetProcAddress(hL, "EnableVolumeCompression");
	}
	if (!NtFormatEx || !NtEnableVolumeCompression)
	{
		grub_error(GRUB_ERR_ACCESS_DENIED, "fmifs.dll load error");
		return FALSE;
	}
	wDrive = calloc(szDriveLen + 3, sizeof(WCHAR));
	if (!wDrive)
	{
		grub_error(GRUB_ERR_OUT_OF_MEMORY, "out of memory");
		return FALSE;
	}
	if (szDriveLen < 2)
	{
		/* 'X' -> 'X:' */
		swprintf(wDrive, szDriveLen + 3, L"%c:", DriveRoot[0]);
		szDriveLen += 1;
	}
	else if (DriveRoot[szDriveLen - 1] == L'\\')
	{
		/* 'X:\' -> 'X:' */
		wcsncpy_s(wDrive, szDriveLen + 3, DriveRoot, szDriveLen);
		wDrive[szDriveLen - 1] = L'\0';
		szDriveLen -= 1;
	}
	else
	{
		wcsncpy_s(wDrive, szDriveLen + 3, DriveRoot, szDriveLen + 1);
	}

	if (DesiredUnitAllocationSize < 0x200)
	{
		// 0 is FormatEx's value for default, which we need to use for UDF
		DesiredUnitAllocationSize = 0;
	}

	NtFormatEx(wDrive, GetMediaType(wDrive), FileSystemTypeName, Label, QuickFormat, DesiredUnitAllocationSize, Callback);
	if (EnableCompression)
	{
		wDrive[szDriveLen] = L'\\';
		wDrive[szDriveLen + 1] = L'\0';
		if (NtEnableVolumeCompression(wDrive, FPF_COMPRESSED))
			bRet = TRUE;
		else
			grub_error(GRUB_ERR_BAD_DEVICE, "cannot enable NTFS compression");
	}
	free(wDrive);
	return bRet;
}
