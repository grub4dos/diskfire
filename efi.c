// SPDX-License-Identifier: GPL-3.0-or-later
#include <windows.h>
#include "compat.h"
#include "charset.h"

static DWORD
NTGetFirmwareEnvironmentVariable(LPCSTR lpName, LPCSTR lpGuid, PVOID pBuffer, DWORD nSize, PDWORD pdwAttribubutes)
{
	DWORD(WINAPI * NT6GetFirmwareEnvironmentVariable)
		(LPCSTR lpName, LPCSTR lpGuid, PVOID pBuffer, DWORD nSize) = NULL;
	DWORD(WINAPI * NT6GetFirmwareEnvironmentVariableEx)
		(LPCSTR lpName, LPCSTR lpGuid, PVOID pBuffer, DWORD nSize, PDWORD pdwAttribubutes) = NULL;
	HMODULE hMod = GetModuleHandleA("kernel32");

	if (hMod)
	{
		*(FARPROC*)&NT6GetFirmwareEnvironmentVariable = GetProcAddress(hMod, "GetFirmwareEnvironmentVariableA");
		*(FARPROC*)&NT6GetFirmwareEnvironmentVariableEx = GetProcAddress(hMod, "GetFirmwareEnvironmentVariableExA");
	}
	if (NT6GetFirmwareEnvironmentVariableEx)
		return NT6GetFirmwareEnvironmentVariableEx(lpName, lpGuid, pBuffer, nSize, pdwAttribubutes);
	if (NT6GetFirmwareEnvironmentVariable)
		return NT6GetFirmwareEnvironmentVariable(lpName, lpGuid, pBuffer, nSize);
	SetLastError(ERROR_INVALID_FUNCTION);
	return 0;
}

static DWORD
NTSetFirmwareEnvironmentVariable(LPCSTR lpName, LPCSTR lpGuid, PVOID pBuffer, DWORD nSize, DWORD dwAttributes)
{
	DWORD(WINAPI * NT6SetFirmwareEnvironmentVariable)
		(LPCSTR lpName, LPCSTR lpGuid, PVOID pBuffer, DWORD nSize) = NULL;
	DWORD(WINAPI * NT6SetFirmwareEnvironmentVariableEx)
		(LPCSTR lpName, LPCSTR lpGuid, PVOID pBuffer, DWORD nSize, DWORD dwAttributes) = NULL;
	HMODULE hMod = GetModuleHandleA("kernel32");

	if (hMod)
	{
		*(FARPROC*)&NT6SetFirmwareEnvironmentVariable = GetProcAddress(hMod, "SetFirmwareEnvironmentVariableA");
		*(FARPROC*)&NT6SetFirmwareEnvironmentVariableEx = GetProcAddress(hMod, "SetFirmwareEnvironmentVariableExA");
	}

	if (NT6SetFirmwareEnvironmentVariableEx)
		return NT6SetFirmwareEnvironmentVariableEx(lpName, lpGuid, pBuffer, nSize, dwAttributes);
	if (NT6SetFirmwareEnvironmentVariable)
		return NT6SetFirmwareEnvironmentVariable(lpName, lpGuid, pBuffer, nSize);
	SetLastError(ERROR_INVALID_FUNCTION);
	return 0;
}

#define MAX_VAR_SIZE 65536

grub_err_t
grub_efi_get_variable(const char* var,
	const char* guid,
	grub_size_t* datasize_out,
	void** data_out,
	grub_uint32_t* attributes)
{
	grub_size_t datasize = MAX_VAR_SIZE;
	void* data;
	DWORD len = 0;
	DWORD attr = 0;

	*data_out = NULL;
	*datasize_out = 0;

	data = grub_zalloc(datasize);
	if (!data)
		return grub_error(GRUB_ERR_OUT_OF_MEMORY, "out of memory");

	len = NTGetFirmwareEnvironmentVariable(var, guid, data, datasize, &attr);

	if (len > 0)
	{
		*data_out = data;
		*datasize_out = len;
		if (attributes)
			*attributes = attr;
		return GRUB_ERR_NONE;
	}

	grub_free(data);
	return grub_error(GRUB_ERR_ACCESS_DENIED, "access denied");
}

#define GRUB_EFI_VARIABLE_NON_VOLATILE			0x00000001
#define GRUB_EFI_VARIABLE_BOOTSERVICE_ACCESS	0x00000002
#define GRUB_EFI_VARIABLE_RUNTIME_ACCESS		0x00000004

grub_err_t
grub_efi_set_variable(const char* var,
	const char* guid,
	grub_size_t datasize,
	void* data,
	const grub_uint32_t* attributes)
{
	DWORD attr;
	DWORD ret;
	if (!attributes)
		attr = GRUB_EFI_VARIABLE_NON_VOLATILE | GRUB_EFI_VARIABLE_BOOTSERVICE_ACCESS | GRUB_EFI_VARIABLE_RUNTIME_ACCESS;
	else
		attr = *attributes;
	ret = NTSetFirmwareEnvironmentVariable(var, guid, data, datasize, attr);
	if (ret > 0)
		return GRUB_ERR_NONE;
	return grub_error(GRUB_ERR_ACCESS_DENIED, "access denied");
}
