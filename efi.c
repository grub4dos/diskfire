// SPDX-License-Identifier: GPL-3.0-or-later
#include <windows.h>
#include "compat.h"
#include "charset.h"
#include "efi.h"

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

static void
dump_vendor_path(const char* type, grub_efi_vendor_device_path_t* vendor)
{
	grub_uint32_t vendor_data_len = vendor->header.length - sizeof(*vendor);
	grub_printf("/%sVendor(%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x)[%x: ",
		type,
		(unsigned)vendor->vendor_guid.data1,
		(unsigned)vendor->vendor_guid.data2,
		(unsigned)vendor->vendor_guid.data3,
		(unsigned)vendor->vendor_guid.data4[0],
		(unsigned)vendor->vendor_guid.data4[1],
		(unsigned)vendor->vendor_guid.data4[2],
		(unsigned)vendor->vendor_guid.data4[3],
		(unsigned)vendor->vendor_guid.data4[4],
		(unsigned)vendor->vendor_guid.data4[5],
		(unsigned)vendor->vendor_guid.data4[6],
		(unsigned)vendor->vendor_guid.data4[7],
		vendor_data_len);
	if (vendor->header.length > sizeof(*vendor))
	{
		grub_uint32_t i;
		for (i = 0; i < vendor_data_len; i++)
			grub_printf("%02x ", vendor->vendor_defined_data[i]);
	}
	grub_printf("]");
}


/* Print the chain of Device Path nodes. This is mainly for debugging. */
void
grub_efi_print_device_path(grub_efi_device_path_t* dp)
{
	while (GRUB_EFI_DEVICE_PATH_VALID(dp))
	{
		grub_efi_uint8_t type = GRUB_EFI_DEVICE_PATH_TYPE(dp);
		grub_efi_uint8_t subtype = GRUB_EFI_DEVICE_PATH_SUBTYPE(dp);
		grub_efi_uint16_t len = GRUB_EFI_DEVICE_PATH_LENGTH(dp);

		switch (type)
		{
		case GRUB_EFI_END_DEVICE_PATH_TYPE:
			switch (subtype)
			{
			case GRUB_EFI_END_ENTIRE_DEVICE_PATH_SUBTYPE:
				grub_printf("/EndEntire\n");
				break;
			case GRUB_EFI_END_THIS_DEVICE_PATH_SUBTYPE:
				grub_printf("/EndThis\n");
				break;
			default:
				grub_printf("/EndUnknown(%x)\n", (unsigned)subtype);
				break;
			}
			break;

		case GRUB_EFI_HARDWARE_DEVICE_PATH_TYPE:
			switch (subtype)
			{
			case GRUB_EFI_PCI_DEVICE_PATH_SUBTYPE:
			{
				grub_efi_pci_device_path_t* pci
					= (grub_efi_pci_device_path_t*)dp;
				grub_printf("/PCI(%x,%x)",
					(unsigned)pci->function, (unsigned)pci->device);
			}
			break;
			case GRUB_EFI_PCCARD_DEVICE_PATH_SUBTYPE:
			{
				grub_efi_pccard_device_path_t* pccard
					= (grub_efi_pccard_device_path_t*)dp;
				grub_printf("/PCCARD(%x)",
					(unsigned)pccard->function);
			}
			break;
			case GRUB_EFI_MEMORY_MAPPED_DEVICE_PATH_SUBTYPE:
			{
				grub_efi_memory_mapped_device_path_t* mmapped
					= (grub_efi_memory_mapped_device_path_t*)dp;
				grub_printf("/MMap(%x,%llx,%llx)",
					(unsigned)mmapped->memory_type,
					(unsigned long long) mmapped->start_address,
					(unsigned long long) mmapped->end_address);
			}
			break;
			case GRUB_EFI_VENDOR_DEVICE_PATH_SUBTYPE:
				dump_vendor_path("Hardware",
					(grub_efi_vendor_device_path_t*)dp);
				break;
			case GRUB_EFI_CONTROLLER_DEVICE_PATH_SUBTYPE:
			{
				grub_efi_controller_device_path_t* controller
					= (grub_efi_controller_device_path_t*)dp;
				grub_printf("/Ctrl(%x)",
					(unsigned)controller->controller_number);
			}
			break;
			default:
				grub_printf("/UnknownHW(%x)", (unsigned)subtype);
				break;
			}
			break;

		case GRUB_EFI_ACPI_DEVICE_PATH_TYPE:
			switch (subtype)
			{
			case GRUB_EFI_ACPI_DEVICE_PATH_SUBTYPE:
			{
				grub_efi_acpi_device_path_t* acpi
					= (grub_efi_acpi_device_path_t*)dp;
				grub_printf("/ACPI(%x,%x)",
					(unsigned)acpi->hid,
					(unsigned)acpi->uid);
			}
			break;
			case GRUB_EFI_EXPANDED_ACPI_DEVICE_PATH_SUBTYPE:
			{
				grub_efi_expanded_acpi_device_path_t* eacpi
					= (grub_efi_expanded_acpi_device_path_t*)dp;
				grub_printf("/ACPI(");

				if (GRUB_EFI_EXPANDED_ACPI_HIDSTR(dp)[0] == '\0')
					grub_printf("%x,", (unsigned)eacpi->hid);
				else
					grub_printf("%s,", GRUB_EFI_EXPANDED_ACPI_HIDSTR(dp));

				if (GRUB_EFI_EXPANDED_ACPI_UIDSTR(dp)[0] == '\0')
					grub_printf("%x,", (unsigned)eacpi->uid);
				else
					grub_printf("%s,", GRUB_EFI_EXPANDED_ACPI_UIDSTR(dp));

				if (GRUB_EFI_EXPANDED_ACPI_CIDSTR(dp)[0] == '\0')
					grub_printf("%x)", (unsigned)eacpi->cid);
				else
					grub_printf("%s)", GRUB_EFI_EXPANDED_ACPI_CIDSTR(dp));
			}
			break;
			default:
				grub_printf("/UnknownACPI(%x)", (unsigned)subtype);
				break;
			}
			break;

		case GRUB_EFI_MESSAGING_DEVICE_PATH_TYPE:
			switch (subtype)
			{
			case GRUB_EFI_ATAPI_DEVICE_PATH_SUBTYPE:
			{
				grub_efi_atapi_device_path_t* atapi
					= (grub_efi_atapi_device_path_t*)dp;
				grub_printf("/ATAPI(%x,%x,%x)",
					(unsigned)atapi->primary_secondary,
					(unsigned)atapi->slave_master,
					(unsigned)atapi->lun);
			}
			break;
			case GRUB_EFI_SCSI_DEVICE_PATH_SUBTYPE:
			{
				grub_efi_scsi_device_path_t* scsi
					= (grub_efi_scsi_device_path_t*)dp;
				grub_printf("/SCSI(%x,%x)",
					(unsigned)scsi->pun,
					(unsigned)scsi->lun);
			}
			break;
			case GRUB_EFI_FIBRE_CHANNEL_DEVICE_PATH_SUBTYPE:
			{
				grub_efi_fibre_channel_device_path_t* fc
					= (grub_efi_fibre_channel_device_path_t*)dp;
				grub_printf("/FibreChannel(%llx,%llx)",
					(unsigned long long) fc->wwn,
					(unsigned long long) fc->lun);
			}
			break;
			case GRUB_EFI_1394_DEVICE_PATH_SUBTYPE:
			{
				grub_efi_1394_device_path_t* firewire
					= (grub_efi_1394_device_path_t*)dp;
				grub_printf("/1394(%llx)",
					(unsigned long long) firewire->guid);
			}
			break;
			case GRUB_EFI_USB_DEVICE_PATH_SUBTYPE:
			{
				grub_efi_usb_device_path_t* usb
					= (grub_efi_usb_device_path_t*)dp;
				grub_printf("/USB(%x,%x)",
					(unsigned)usb->parent_port_number,
					(unsigned)usb->usb_interface);
			}
			break;
			case GRUB_EFI_USB_CLASS_DEVICE_PATH_SUBTYPE:
			{
				grub_efi_usb_class_device_path_t* usb_class
					= (grub_efi_usb_class_device_path_t*)dp;
				grub_printf("/USBClass(%x,%x,%x,%x,%x)",
					(unsigned)usb_class->vendor_id,
					(unsigned)usb_class->product_id,
					(unsigned)usb_class->device_class,
					(unsigned)usb_class->device_subclass,
					(unsigned)usb_class->device_protocol);
			}
			break;
			case GRUB_EFI_I2O_DEVICE_PATH_SUBTYPE:
			{
				grub_efi_i2o_device_path_t* i2o
					= (grub_efi_i2o_device_path_t*)dp;
				grub_printf("/I2O(%x)", (unsigned)i2o->tid);
			}
			break;
			case GRUB_EFI_MAC_ADDRESS_DEVICE_PATH_SUBTYPE:
			{
				grub_efi_mac_address_device_path_t* mac
					= (grub_efi_mac_address_device_path_t*)dp;
				grub_printf("/MacAddr(%02x:%02x:%02x:%02x:%02x:%02x,%x)",
					(unsigned)mac->mac_address[0],
					(unsigned)mac->mac_address[1],
					(unsigned)mac->mac_address[2],
					(unsigned)mac->mac_address[3],
					(unsigned)mac->mac_address[4],
					(unsigned)mac->mac_address[5],
					(unsigned)mac->if_type);
			}
			break;
			case GRUB_EFI_IPV4_DEVICE_PATH_SUBTYPE:
			{
				grub_efi_ipv4_device_path_t* ipv4
					= (grub_efi_ipv4_device_path_t*)dp;
				grub_printf("/IPv4(%u.%u.%u.%u,%u.%u.%u.%u,%u,%u,%x,%x)",
					(unsigned)ipv4->local_ip_address[0],
					(unsigned)ipv4->local_ip_address[1],
					(unsigned)ipv4->local_ip_address[2],
					(unsigned)ipv4->local_ip_address[3],
					(unsigned)ipv4->remote_ip_address[0],
					(unsigned)ipv4->remote_ip_address[1],
					(unsigned)ipv4->remote_ip_address[2],
					(unsigned)ipv4->remote_ip_address[3],
					(unsigned)ipv4->local_port,
					(unsigned)ipv4->remote_port,
					(unsigned)ipv4->protocol,
					(unsigned)ipv4->static_ip_address);
			}
			break;
			case GRUB_EFI_IPV6_DEVICE_PATH_SUBTYPE:
			{
				grub_efi_ipv6_device_path_t* ipv6
					= (grub_efi_ipv6_device_path_t*)dp;
				grub_printf("/IPv6(%x:%x:%x:%x:%x:%x:%x:%x,%x:%x:%x:%x:%x:%x:%x:%x,%u,%u,%x,%x)",
					(unsigned)ipv6->local_ip_address[0],
					(unsigned)ipv6->local_ip_address[1],
					(unsigned)ipv6->local_ip_address[2],
					(unsigned)ipv6->local_ip_address[3],
					(unsigned)ipv6->local_ip_address[4],
					(unsigned)ipv6->local_ip_address[5],
					(unsigned)ipv6->local_ip_address[6],
					(unsigned)ipv6->local_ip_address[7],
					(unsigned)ipv6->remote_ip_address[0],
					(unsigned)ipv6->remote_ip_address[1],
					(unsigned)ipv6->remote_ip_address[2],
					(unsigned)ipv6->remote_ip_address[3],
					(unsigned)ipv6->remote_ip_address[4],
					(unsigned)ipv6->remote_ip_address[5],
					(unsigned)ipv6->remote_ip_address[6],
					(unsigned)ipv6->remote_ip_address[7],
					(unsigned)ipv6->local_port,
					(unsigned)ipv6->remote_port,
					(unsigned)ipv6->protocol,
					(unsigned)ipv6->static_ip_address);
			}
			break;
			case GRUB_EFI_INFINIBAND_DEVICE_PATH_SUBTYPE:
			{
				grub_efi_infiniband_device_path_t* ib
					= (grub_efi_infiniband_device_path_t*)dp;
				grub_printf("/InfiniBand(%x,%llx,%llx,%llx)",
					(unsigned)ib->port_gid[0], /* XXX */
					(unsigned long long) ib->remote_id,
					(unsigned long long) ib->target_port_id,
					(unsigned long long) ib->device_id);
			}
			break;
			case GRUB_EFI_UART_DEVICE_PATH_SUBTYPE:
			{
				grub_efi_uart_device_path_t* uart
					= (grub_efi_uart_device_path_t*)dp;
				grub_printf("/UART(%llu,%u,%x,%x)",
					(unsigned long long) uart->baud_rate,
					uart->data_bits,
					uart->parity,
					uart->stop_bits);
			}
			break;
			case GRUB_EFI_SATA_DEVICE_PATH_SUBTYPE:
			{
				grub_efi_sata_device_path_t* sata;
				sata = (grub_efi_sata_device_path_t*)dp;
				grub_printf("/Sata(%x,%x,%x)",
					sata->hba_port,
					sata->multiplier_port,
					sata->lun);
			}
			break;

			case GRUB_EFI_VENDOR_MESSAGING_DEVICE_PATH_SUBTYPE:
				dump_vendor_path("Messaging",
					(grub_efi_vendor_device_path_t*)dp);
				break;
			default:
				grub_printf("/UnknownMessaging(%x)", (unsigned)subtype);
				break;
			}
			break;

		case GRUB_EFI_MEDIA_DEVICE_PATH_TYPE:
			switch (subtype)
			{
			case GRUB_EFI_HARD_DRIVE_DEVICE_PATH_SUBTYPE:
			{
				grub_efi_hard_drive_device_path_t* hd = (grub_efi_hard_drive_device_path_t*)
					dp;
				grub_printf("/HD(%u,%llx,%llx,%02x%02x%02x%02x%02x%02x%02x%02x,%x,%x)",
					hd->partition_number,
					(unsigned long long) hd->partition_start,
					(unsigned long long) hd->partition_size,
					(unsigned)hd->partition_signature[0],
					(unsigned)hd->partition_signature[1],
					(unsigned)hd->partition_signature[2],
					(unsigned)hd->partition_signature[3],
					(unsigned)hd->partition_signature[4],
					(unsigned)hd->partition_signature[5],
					(unsigned)hd->partition_signature[6],
					(unsigned)hd->partition_signature[7],
					(unsigned)hd->partmap_type,
					(unsigned)hd->signature_type);
			}
			break;
			case GRUB_EFI_CDROM_DEVICE_PATH_SUBTYPE:
			{
				grub_efi_cdrom_device_path_t* cd
					= (grub_efi_cdrom_device_path_t*)dp;
				grub_printf("/CD(%u,%llx,%llx)",
					cd->boot_entry,
					(unsigned long long) cd->partition_start,
					(unsigned long long) cd->partition_size);
			}
			break;
			case GRUB_EFI_VENDOR_MEDIA_DEVICE_PATH_SUBTYPE:
				dump_vendor_path("Media",
					(grub_efi_vendor_device_path_t*)dp);
				break;
			case GRUB_EFI_FILE_PATH_DEVICE_PATH_SUBTYPE:
			{
				grub_efi_file_path_device_path_t* fp;
				grub_uint8_t* buf;
				fp = (grub_efi_file_path_device_path_t*)dp;
				buf = grub_malloc(2ULL * (len - 4) + 1);
				if (buf)
				{
					grub_efi_char16_t* dup_name = grub_malloc(len - 4);
					if (!dup_name)
					{
						grub_errno = GRUB_ERR_NONE;
						grub_printf("/File((null))");
						grub_free(buf);
						break;
					}
					*grub_utf16_to_utf8(buf, grub_memcpy(dup_name, fp->path_name, len - 4),
						(len - 4) / sizeof(grub_efi_char16_t))
						= '\0';
					grub_free(dup_name);
				}
				else
					grub_errno = GRUB_ERR_NONE;
				grub_printf("/File(%s)", buf);
				grub_free(buf);
			}
			break;
			case GRUB_EFI_PROTOCOL_DEVICE_PATH_SUBTYPE:
			{
				grub_efi_protocol_device_path_t* proto
					= (grub_efi_protocol_device_path_t*)dp;
				grub_printf("/Protocol(%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x)",
					(unsigned)proto->guid.data1,
					(unsigned)proto->guid.data2,
					(unsigned)proto->guid.data3,
					(unsigned)proto->guid.data4[0],
					(unsigned)proto->guid.data4[1],
					(unsigned)proto->guid.data4[2],
					(unsigned)proto->guid.data4[3],
					(unsigned)proto->guid.data4[4],
					(unsigned)proto->guid.data4[5],
					(unsigned)proto->guid.data4[6],
					(unsigned)proto->guid.data4[7]);
			}
			break;
			default:
				grub_printf("/UnknownMedia(%x)", (unsigned)subtype);
				break;
			}
			break;

		case GRUB_EFI_BIOS_DEVICE_PATH_TYPE:
			switch (subtype)
			{
			case GRUB_EFI_BIOS_DEVICE_PATH_SUBTYPE:
			{
				grub_efi_bios_device_path_t* bios
					= (grub_efi_bios_device_path_t*)dp;
				grub_printf("/BIOS(%x,%x,%s)",
					(unsigned)bios->device_type,
					(unsigned)bios->status_flags,
					(char*)(dp + 1));
			}
			break;
			default:
				grub_printf("/UnknownBIOS(%x)", (unsigned)subtype);
				break;
			}
			break;

		default:
			grub_printf("/UnknownType(%x,%x)\n",
				(unsigned)type,
				(unsigned)subtype);
			return;
			break;
		}

		if (GRUB_EFI_END_ENTIRE_DEVICE_PATH(dp))
			break;

		dp = (grub_efi_device_path_t*)((char*)dp + len);
	}
}

char*
grub_efi_get_filename(grub_efi_device_path_t* dp0)
{
	char* name = 0, * p, * pi;
	grub_size_t filesize = 0;
	grub_efi_device_path_t* dp;

	if (!dp0)
		return NULL;

	dp = dp0;

	while (dp)
	{
		grub_efi_uint8_t type = GRUB_EFI_DEVICE_PATH_TYPE(dp);
		grub_efi_uint8_t subtype = GRUB_EFI_DEVICE_PATH_SUBTYPE(dp);

		if (type == GRUB_EFI_END_DEVICE_PATH_TYPE)
			break;
		if (type == GRUB_EFI_MEDIA_DEVICE_PATH_TYPE
			&& subtype == GRUB_EFI_FILE_PATH_DEVICE_PATH_SUBTYPE)
		{
			grub_efi_uint16_t len = GRUB_EFI_DEVICE_PATH_LENGTH(dp);

			if (len < 4)
			{
				grub_error(GRUB_ERR_OUT_OF_RANGE,
					"malformed EFI Device Path node has length=%d", len);
				return NULL;
			}
			len = (len - 4) / sizeof(grub_efi_char16_t);
			filesize += GRUB_MAX_UTF8_PER_UTF16 * len + 2;
		}

		dp = GRUB_EFI_NEXT_DEVICE_PATH(dp);
	}

	if (!filesize)
		return NULL;

	dp = dp0;

	p = name = grub_malloc(filesize);
	if (!name)
		return NULL;

	while (dp)
	{
		grub_efi_uint8_t type = GRUB_EFI_DEVICE_PATH_TYPE(dp);
		grub_efi_uint8_t subtype = GRUB_EFI_DEVICE_PATH_SUBTYPE(dp);

		if (type == GRUB_EFI_END_DEVICE_PATH_TYPE)
			break;
		else if (type == GRUB_EFI_MEDIA_DEVICE_PATH_TYPE
			&& subtype == GRUB_EFI_FILE_PATH_DEVICE_PATH_SUBTYPE)
		{
			grub_efi_file_path_device_path_t* fp;
			grub_efi_uint16_t len;
			grub_efi_char16_t* dup_name;

			*p++ = '/';

			len = GRUB_EFI_DEVICE_PATH_LENGTH(dp);
			if (len < 4)
			{
				grub_error(GRUB_ERR_OUT_OF_RANGE,
					"malformed EFI Device Path node has length=%d", len);
				grub_free(name);
				return NULL;
			}

			len = (len - 4) / sizeof(grub_efi_char16_t);
			fp = (grub_efi_file_path_device_path_t*)dp;
			/* According to EFI spec Path Name is NULL terminated */
			while (len > 0 && fp->path_name[len - 1] == 0)
				len--;

			dup_name = grub_calloc(len, sizeof(*dup_name));
			if (!dup_name)
			{
				grub_free(name);
				return NULL;
			}
			p = (char*)grub_utf16_to_utf8((unsigned char*)p,
				grub_memcpy(dup_name, fp->path_name, len * sizeof(*dup_name)),
				len);
			grub_free(dup_name);
		}

		dp = GRUB_EFI_NEXT_DEVICE_PATH(dp);
	}

	*p = '\0';

	for (pi = name, p = name; *pi;)
	{
		/* EFI breaks paths with backslashes.  */
		if (*pi == '\\' || *pi == '/')
		{
			*p++ = '/';
			while (*pi == '\\' || *pi == '/')
				pi++;
			continue;
		}
		*p++ = *pi++;
	}
	*p = '\0';

	return name;
}

/* Return the device path node right before the end node.  */
grub_efi_device_path_t*
grub_efi_find_last_device_path(const grub_efi_device_path_t* dp)
{
	grub_efi_device_path_t* next, * p;

	if (GRUB_EFI_END_ENTIRE_DEVICE_PATH(dp))
		return 0;

	for (p = (grub_efi_device_path_t*)dp, next = GRUB_EFI_NEXT_DEVICE_PATH(p);
		!GRUB_EFI_END_ENTIRE_DEVICE_PATH(next);
		p = next, next = GRUB_EFI_NEXT_DEVICE_PATH(next))
		;

	return p;
}

/* Duplicate a device path.  */
grub_efi_device_path_t*
grub_efi_duplicate_device_path(const grub_efi_device_path_t* dp)
{
	grub_efi_device_path_t* p;
	grub_size_t total_size = 0;

	for (p = (grub_efi_device_path_t*)dp;
		;
		p = GRUB_EFI_NEXT_DEVICE_PATH(p))
	{
		grub_size_t len = GRUB_EFI_DEVICE_PATH_LENGTH(p);

		/*
		 * In the event that we find a node that's completely garbage, for
		 * example if we get to 0x7f 0x01 0x02 0x00 ... (EndInstance with a size
		 * of 2), GRUB_EFI_END_ENTIRE_DEVICE_PATH() will be true and
		 * GRUB_EFI_NEXT_DEVICE_PATH() will return NULL, so we won't continue,
		 * and neither should our consumers, but there won't be any error raised
		 * even though the device path is junk.
		 *
		 * This keeps us from passing junk down back to our caller.
		 */
		if (len < 4)
		{
			grub_error(GRUB_ERR_OUT_OF_RANGE,
				"malformed EFI Device Path node has length=%" PRIuGRUB_SIZE, len);
			return NULL;
		}

		total_size += len;
		if (GRUB_EFI_END_ENTIRE_DEVICE_PATH(p))
			break;
	}

	p = grub_malloc(total_size);
	if (!p)
		return 0;

	grub_memcpy(p, dp, total_size);
	return p;
}

int
grub_efi_compare_device_paths(const grub_efi_device_path_t* dp1,
	const grub_efi_device_path_t* dp2)
{
	if (!dp1 || !dp2)
		/* Return non-zero.  */
		return 1;

	if (dp1 == dp2)
		return 0;

	while (GRUB_EFI_DEVICE_PATH_VALID(dp1) && GRUB_EFI_DEVICE_PATH_VALID(dp2))
	{
		grub_efi_uint8_t type1, type2;
		grub_efi_uint8_t subtype1, subtype2;
		grub_efi_uint16_t len1, len2;
		int ret;

		type1 = GRUB_EFI_DEVICE_PATH_TYPE(dp1);
		type2 = GRUB_EFI_DEVICE_PATH_TYPE(dp2);

		if (type1 != type2)
			return (int)type2 - (int)type1;

		subtype1 = GRUB_EFI_DEVICE_PATH_SUBTYPE(dp1);
		subtype2 = GRUB_EFI_DEVICE_PATH_SUBTYPE(dp2);

		if (subtype1 != subtype2)
			return (int)subtype1 - (int)subtype2;

		len1 = GRUB_EFI_DEVICE_PATH_LENGTH(dp1);
		len2 = GRUB_EFI_DEVICE_PATH_LENGTH(dp2);

		if (len1 != len2)
			return (int)len1 - (int)len2;

		ret = grub_memcmp(dp1, dp2, len1);
		if (ret != 0)
			return ret;

		if (GRUB_EFI_END_ENTIRE_DEVICE_PATH(dp1))
			break;

		dp1 = (grub_efi_device_path_t*)((char*)dp1 + len1);
		dp2 = (grub_efi_device_path_t*)((char*)dp2 + len2);
	}

	/*
	 * There's no "right" answer here, but we probably don't want to call a valid
	 * dp and an invalid dp equal, so pick one way or the other.
	 */
	if (GRUB_EFI_DEVICE_PATH_VALID(dp1) && !GRUB_EFI_DEVICE_PATH_VALID(dp2))
		return 1;
	else if (!GRUB_EFI_DEVICE_PATH_VALID(dp1) && GRUB_EFI_DEVICE_PATH_VALID(dp2))
		return -1;

	return 0;
}

static grub_err_t
copy_file_path(grub_efi_file_path_device_path_t* fp,
	const char* str, grub_efi_uint16_t len)
{
	grub_efi_char16_t* p, * path_name;
	grub_efi_uint16_t size;

	fp->header.type = GRUB_EFI_MEDIA_DEVICE_PATH_TYPE;
	fp->header.subtype = GRUB_EFI_FILE_PATH_DEVICE_PATH_SUBTYPE;

	path_name = grub_calloc(len, GRUB_MAX_UTF16_PER_UTF8 * sizeof(*path_name));
	if (!path_name)
		return grub_error(GRUB_ERR_OUT_OF_MEMORY, "failed to allocate path buffer");

	size = grub_utf8_to_utf16(path_name, len * GRUB_MAX_UTF16_PER_UTF8,
		(const grub_uint8_t*)str, len, 0);
	for (p = path_name; p < path_name + size; p++)
		if (*p == '/')
			*p = '\\';

	grub_memcpy(fp->path_name, path_name, size * sizeof(*fp->path_name));
	/* File Path is NULL terminated */
	fp->path_name[size++] = '\0';
	fp->header.length = size * sizeof(grub_efi_char16_t) + sizeof(*fp);
	grub_free(path_name);
	return GRUB_ERR_NONE;
}

grub_efi_device_path_t*
grub_efi_file_device_path(grub_efi_device_path_t* dp, const char* filename)
{
	char* dir_start;
	char* dir_end;
	grub_size_t size;
	grub_efi_device_path_t* d;
	grub_efi_device_path_t* file_path;

	dir_start = grub_strchr(filename, ')');
	if (!dir_start)
		dir_start = (char*)filename;
	else
		dir_start++;

	dir_end = grub_strrchr(dir_start, '/');
	if (!dir_end)
	{
		grub_error(GRUB_ERR_BAD_FILENAME, "invalid EFI file path");
		return 0;
	}

	size = 0;
	d = dp;
	while (d)
	{
		grub_size_t len = GRUB_EFI_DEVICE_PATH_LENGTH(d);

		if (len < 4)
		{
			grub_error(GRUB_ERR_OUT_OF_RANGE,
				"malformed EFI Device Path node has length=%" PRIuGRUB_SIZE, len);
			return NULL;
		}

		size += len;
		if ((GRUB_EFI_END_ENTIRE_DEVICE_PATH(d)))
			break;
		d = GRUB_EFI_NEXT_DEVICE_PATH(d);
	}

	/* File Path is NULL terminated. Allocate space for 2 extra characters */
	/* FIXME why we split path in two components? */
	file_path = grub_malloc(size
		+ ((grub_strlen(dir_start) + 2)
			* GRUB_MAX_UTF16_PER_UTF8
			* sizeof(grub_efi_char16_t))
		+ sizeof(grub_efi_file_path_device_path_t) * 2);
	if (!file_path)
		return 0;

	grub_memcpy(file_path, dp, size);

	/* Fill the file path for the directory.  */
	d = (grub_efi_device_path_t*)((char*)file_path
		+ ((char*)d - (char*)dp));

	if (copy_file_path((grub_efi_file_path_device_path_t*)d,
		dir_start, dir_end - dir_start) != GRUB_ERR_NONE)
		goto fail;

	/* Fill the file path for the file.  */
	d = GRUB_EFI_NEXT_DEVICE_PATH(d);
	if (copy_file_path((grub_efi_file_path_device_path_t*)d,
		dir_end + 1, grub_strlen(dir_end + 1)) != GRUB_ERR_NONE)
		goto fail;

	/* Fill the end of device path nodes.  */
	d = GRUB_EFI_NEXT_DEVICE_PATH(d);
	if (!d)
		goto fail;
	d->type = GRUB_EFI_END_DEVICE_PATH_TYPE;
	d->subtype = GRUB_EFI_END_ENTIRE_DEVICE_PATH_SUBTYPE;
	d->length = sizeof(*d);

	return file_path;
fail:
	grub_free(file_path);
	return 0;
}

static grub_efi_uintn_t
device_path_node_length(const void* node)
{
	return grub_get_unaligned16((grub_efi_uint16_t*)
		&((grub_efi_device_path_t*)(node))->length);
}

static void
set_device_path_node_length(void* node, grub_efi_uintn_t len)
{
	grub_set_unaligned16((grub_efi_uint16_t*)
		&((grub_efi_device_path_t*)(node))->length,
		(grub_efi_uint16_t)(len));
}

grub_efi_uintn_t
grub_efi_get_dp_size(const grub_efi_device_path_t* dp)
{
	grub_efi_device_path_t* p;
	grub_efi_uintn_t total_size = 0;
	for (p = (grub_efi_device_path_t*)dp; ; p = GRUB_EFI_NEXT_DEVICE_PATH(p))
	{
		total_size += GRUB_EFI_DEVICE_PATH_LENGTH(p);
		if (GRUB_EFI_END_ENTIRE_DEVICE_PATH(p))
			break;
	}
	return total_size;
}

grub_efi_device_path_t*
grub_efi_create_device_node(grub_efi_uint8_t node_type, grub_efi_uintn_t node_subtype,
	grub_efi_uint16_t node_length)
{
	grub_efi_device_path_t* dp;
	if (node_length < sizeof(grub_efi_device_path_t))
		return NULL;
	dp = grub_zalloc(node_length);
	if (dp != NULL)
	{
		dp->type = node_type;
		dp->subtype = node_subtype;
		set_device_path_node_length(dp, node_length);
	}
	return dp;
}

grub_efi_device_path_t*
grub_efi_append_device_path(const grub_efi_device_path_t* dp1,
	const grub_efi_device_path_t* dp2)
{
	grub_efi_uintn_t size;
	grub_efi_uintn_t size1;
	grub_efi_uintn_t size2;
	grub_efi_device_path_t* new_dp;
	grub_efi_device_path_t* tmp_dp;
	// If there's only 1 path, just duplicate it.
	if (dp1 == NULL)
	{
		if (dp2 == NULL)
			return grub_efi_create_device_node(GRUB_EFI_END_DEVICE_PATH_TYPE,
				GRUB_EFI_END_ENTIRE_DEVICE_PATH_SUBTYPE,
				sizeof(grub_efi_device_path_t));
		else
			return grub_efi_duplicate_device_path(dp2);
	}
	if (dp2 == NULL)
		grub_efi_duplicate_device_path(dp1);
	// Allocate space for the combined device path. It only has one end node of
	// length EFI_DEVICE_PATH_PROTOCOL.
	size1 = grub_efi_get_dp_size(dp1);
	size2 = grub_efi_get_dp_size(dp2);
	size = size1 + size2 - sizeof(grub_efi_device_path_t);
	new_dp = grub_malloc(size);

	if (new_dp != NULL)
	{
		new_dp = grub_memcpy(new_dp, dp1, size1);
		// Over write FirstDevicePath EndNode and do the copy
		tmp_dp = (grub_efi_device_path_t*)
			((char*)new_dp + (size1 - sizeof(grub_efi_device_path_t)));
		grub_memcpy(tmp_dp, dp2, size2);
	}
	return new_dp;
}

grub_efi_device_path_t*
grub_efi_append_device_node(const grub_efi_device_path_t* device_path,
	const grub_efi_device_path_t* device_node)
{
	grub_efi_device_path_t* tmp_dp;
	grub_efi_device_path_t* next_node;
	grub_efi_device_path_t* new_dp;
	grub_efi_uintn_t node_length;
	if (device_node == NULL)
	{
		if (device_path == NULL)
			return grub_efi_create_device_node(GRUB_EFI_END_DEVICE_PATH_TYPE,
				GRUB_EFI_END_ENTIRE_DEVICE_PATH_SUBTYPE,
				sizeof(grub_efi_device_path_t));
		else
			return grub_efi_duplicate_device_path(device_path);
	}
	// Build a Node that has a terminator on it
	node_length = device_path_node_length(device_node);

	tmp_dp = grub_malloc(node_length + sizeof(grub_efi_device_path_t));
	if (tmp_dp == NULL)
		return NULL;
	tmp_dp = grub_memcpy(tmp_dp, device_node, node_length);
	// Add and end device path node to convert Node to device path
	next_node = GRUB_EFI_NEXT_DEVICE_PATH(tmp_dp);
	if (!next_node)
		goto fail;
	next_node->type = GRUB_EFI_END_DEVICE_PATH_TYPE;
	next_node->subtype = GRUB_EFI_END_ENTIRE_DEVICE_PATH_SUBTYPE;
	next_node->length = sizeof(grub_efi_device_path_t);
	// Append device paths
	new_dp = grub_efi_append_device_path(device_path, tmp_dp);
	grub_free(tmp_dp);
	return new_dp;
fail:
	grub_free(tmp_dp);
	grub_error(GRUB_ERR_IO, "bad device path");
	return NULL;
}

int
grub_efi_is_child_dp(const grub_efi_device_path_t* child,
	const grub_efi_device_path_t* parent)
{
	grub_efi_device_path_t* dp, * ldp;
	int ret = 0;

	dp = grub_efi_duplicate_device_path(child);
	if (!dp)
		return 0;

	while (!ret)
	{
		ldp = grub_efi_find_last_device_path(dp);
		if (!ldp)
			break;

		ldp->type = GRUB_EFI_END_DEVICE_PATH_TYPE;
		ldp->subtype = GRUB_EFI_END_ENTIRE_DEVICE_PATH_SUBTYPE;
		ldp->length = sizeof(*ldp);

		ret = (grub_efi_compare_device_paths(dp, parent) == 0);
	}

	grub_free(dp);
	return ret;
}
