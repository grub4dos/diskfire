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
