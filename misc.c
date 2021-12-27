// SPDX-License-Identifier: GPL-3.0-or-later
#include <stdio.h>
#include <string.h>
#include <windows.h>
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
	return CreateFileA(PhyPath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
}

HANDLE GetHandleById(DWORD Id)
{
	char PhyPath[] = "\\\\.\\PhysicalDrive4294967295";
	snprintf(PhyPath, sizeof(PhyPath), "\\\\.\\PhysicalDrive%u", Id);
	return CreateFileA(PhyPath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
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
