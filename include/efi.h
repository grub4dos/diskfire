// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef _EFI_HEADER
#define _EFI_HEADER 1

#include "compat.h"

#pragma warning(disable:4200)

#define GRUB_EFI_GV_GUID "{8BE4DF61-93CA-11D2-AA0D-00E098032B8C}"

#define GRUB_EFI_VARIABLE_NON_VOLATILE			0x00000001
#define GRUB_EFI_VARIABLE_BOOTSERVICE_ACCESS	0x00000002
#define GRUB_EFI_VARIABLE_RUNTIME_ACCESS		0x00000004

typedef char grub_efi_boolean_t;
typedef grub_ssize_t grub_efi_intn_t;
typedef grub_size_t grub_efi_uintn_t;
typedef grub_int8_t grub_efi_int8_t;
typedef grub_uint8_t grub_efi_uint8_t;
typedef grub_int16_t grub_efi_int16_t;
typedef grub_uint16_t grub_efi_uint16_t;
typedef grub_int32_t grub_efi_int32_t;
typedef grub_uint32_t grub_efi_uint32_t;
typedef grub_int64_t grub_efi_int64_t;
typedef grub_uint64_t grub_efi_uint64_t;
typedef grub_uint8_t grub_efi_char8_t;
typedef grub_uint16_t grub_efi_char16_t;

typedef grub_efi_uint64_t grub_efi_lba_t;
typedef grub_uint8_t grub_efi_mac_address_t[32];
typedef grub_uint8_t grub_efi_ipv4_address_t[4];
typedef grub_uint16_t grub_efi_ipv6_address_t[8];
typedef grub_efi_uint64_t grub_efi_physical_address_t;
typedef grub_efi_uint64_t grub_efi_virtual_address_t;

typedef grub_packed_guid_t grub_efi_packed_guid_t;

PRAGMA_BEGIN_PACKED
struct grub_efi_device_path
{
	grub_efi_uint8_t type;
	grub_efi_uint8_t subtype;
	grub_efi_uint16_t length;
};
typedef struct grub_efi_device_path grub_efi_device_path_t;

#define GRUB_EFI_DEVICE_PATH_TYPE(dp)		((dp)->type & 0x7f)
#define GRUB_EFI_DEVICE_PATH_SUBTYPE(dp)	((dp)->subtype)
#define GRUB_EFI_DEVICE_PATH_LENGTH(dp)		((dp)->length)
#define GRUB_EFI_DEVICE_PATH_VALID(dp)		((dp) != NULL && GRUB_EFI_DEVICE_PATH_LENGTH (dp) >= 4)

/* The End of Device Path nodes.  */
#define GRUB_EFI_END_DEVICE_PATH_TYPE			(0xff & 0x7f)

#define GRUB_EFI_END_ENTIRE_DEVICE_PATH_SUBTYPE		0xff
#define GRUB_EFI_END_THIS_DEVICE_PATH_SUBTYPE		0x01

#define GRUB_EFI_END_ENTIRE_DEVICE_PATH(dp)	\
	(!GRUB_EFI_DEVICE_PATH_VALID (dp) || \
		(GRUB_EFI_DEVICE_PATH_TYPE (dp) == GRUB_EFI_END_DEVICE_PATH_TYPE \
		&& (GRUB_EFI_DEVICE_PATH_SUBTYPE (dp) \
		== GRUB_EFI_END_ENTIRE_DEVICE_PATH_SUBTYPE)))

#define GRUB_EFI_NEXT_DEVICE_PATH(dp)	\
	(GRUB_EFI_DEVICE_PATH_VALID (dp) \
		? ((grub_efi_device_path_t *) \
		((char *) (dp) + GRUB_EFI_DEVICE_PATH_LENGTH (dp))) \
		: NULL)

/* Hardware Device Path.  */
#define GRUB_EFI_HARDWARE_DEVICE_PATH_TYPE		1

#define GRUB_EFI_PCI_DEVICE_PATH_SUBTYPE		1

struct grub_efi_pci_device_path
{
	grub_efi_device_path_t header;
	grub_efi_uint8_t function;
	grub_efi_uint8_t device;
};
typedef struct grub_efi_pci_device_path grub_efi_pci_device_path_t;

#define GRUB_EFI_PCCARD_DEVICE_PATH_SUBTYPE		2

struct grub_efi_pccard_device_path
{
	grub_efi_device_path_t header;
	grub_efi_uint8_t function;
};
typedef struct grub_efi_pccard_device_path grub_efi_pccard_device_path_t;

#define GRUB_EFI_MEMORY_MAPPED_DEVICE_PATH_SUBTYPE	3

struct grub_efi_memory_mapped_device_path
{
	grub_efi_device_path_t header;
	grub_efi_uint32_t memory_type;
	grub_efi_physical_address_t start_address;
	grub_efi_physical_address_t end_address;
};
typedef struct grub_efi_memory_mapped_device_path grub_efi_memory_mapped_device_path_t;

#define GRUB_EFI_VENDOR_DEVICE_PATH_SUBTYPE		4

struct grub_efi_vendor_device_path
{
	grub_efi_device_path_t header;
	grub_efi_packed_guid_t vendor_guid;
	grub_efi_uint8_t vendor_defined_data[0];
};
typedef struct grub_efi_vendor_device_path grub_efi_vendor_device_path_t;

#define GRUB_EFI_CONTROLLER_DEVICE_PATH_SUBTYPE		5

struct grub_efi_controller_device_path
{
	grub_efi_device_path_t header;
	grub_efi_uint32_t controller_number;
};
typedef struct grub_efi_controller_device_path grub_efi_controller_device_path_t;

/* ACPI Device Path.  */
#define GRUB_EFI_ACPI_DEVICE_PATH_TYPE			2

#define GRUB_EFI_ACPI_DEVICE_PATH_SUBTYPE		1

struct grub_efi_acpi_device_path
{
	grub_efi_device_path_t header;
	grub_efi_uint32_t hid;
	grub_efi_uint32_t uid;
};
typedef struct grub_efi_acpi_device_path grub_efi_acpi_device_path_t;

#define GRUB_EFI_EXPANDED_ACPI_DEVICE_PATH_SUBTYPE	2

struct grub_efi_expanded_acpi_device_path
{
	grub_efi_device_path_t header;
	grub_efi_uint32_t hid;
	grub_efi_uint32_t uid;
	grub_efi_uint32_t cid;
	char hidstr[0];
};
typedef struct grub_efi_expanded_acpi_device_path grub_efi_expanded_acpi_device_path_t;

#define GRUB_EFI_EXPANDED_ACPI_HIDSTR(dp)	\
  (((grub_efi_expanded_acpi_device_path_t *) dp)->hidstr)
#define GRUB_EFI_EXPANDED_ACPI_UIDSTR(dp)	\
  (GRUB_EFI_EXPANDED_ACPI_HIDSTR(dp) \
   + grub_strlen (GRUB_EFI_EXPANDED_ACPI_HIDSTR(dp)) + 1)
#define GRUB_EFI_EXPANDED_ACPI_CIDSTR(dp)	\
  (GRUB_EFI_EXPANDED_ACPI_UIDSTR(dp) \
   + grub_strlen (GRUB_EFI_EXPANDED_ACPI_UIDSTR(dp)) + 1)

/* Messaging Device Path.  */
#define GRUB_EFI_MESSAGING_DEVICE_PATH_TYPE		3

#define GRUB_EFI_ATAPI_DEVICE_PATH_SUBTYPE		1

struct grub_efi_atapi_device_path
{
	grub_efi_device_path_t header;
	grub_efi_uint8_t primary_secondary;
	grub_efi_uint8_t slave_master;
	grub_efi_uint16_t lun;
};
typedef struct grub_efi_atapi_device_path grub_efi_atapi_device_path_t;

#define GRUB_EFI_SCSI_DEVICE_PATH_SUBTYPE		2

struct grub_efi_scsi_device_path
{
	grub_efi_device_path_t header;
	grub_efi_uint16_t pun;
	grub_efi_uint16_t lun;
};
typedef struct grub_efi_scsi_device_path grub_efi_scsi_device_path_t;

#define GRUB_EFI_FIBRE_CHANNEL_DEVICE_PATH_SUBTYPE	3

struct grub_efi_fibre_channel_device_path
{
	grub_efi_device_path_t header;
	grub_efi_uint32_t reserved;
	grub_efi_uint64_t wwn;
	grub_efi_uint64_t lun;
};
typedef struct grub_efi_fibre_channel_device_path grub_efi_fibre_channel_device_path_t;

#define GRUB_EFI_1394_DEVICE_PATH_SUBTYPE		4

struct grub_efi_1394_device_path
{
	grub_efi_device_path_t header;
	grub_efi_uint32_t reserved;
	grub_efi_uint64_t guid;
};
typedef struct grub_efi_1394_device_path grub_efi_1394_device_path_t;

#define GRUB_EFI_USB_DEVICE_PATH_SUBTYPE		5

struct grub_efi_usb_device_path
{
	grub_efi_device_path_t header;
	grub_efi_uint8_t parent_port_number;
	grub_efi_uint8_t usb_interface;
};
typedef struct grub_efi_usb_device_path grub_efi_usb_device_path_t;

#define GRUB_EFI_USB_CLASS_DEVICE_PATH_SUBTYPE		15

struct grub_efi_usb_class_device_path
{
	grub_efi_device_path_t header;
	grub_efi_uint16_t vendor_id;
	grub_efi_uint16_t product_id;
	grub_efi_uint8_t device_class;
	grub_efi_uint8_t device_subclass;
	grub_efi_uint8_t device_protocol;
};
typedef struct grub_efi_usb_class_device_path grub_efi_usb_class_device_path_t;

#define GRUB_EFI_I2O_DEVICE_PATH_SUBTYPE		6

struct grub_efi_i2o_device_path
{
	grub_efi_device_path_t header;
	grub_efi_uint32_t tid;
};
typedef struct grub_efi_i2o_device_path grub_efi_i2o_device_path_t;

#define GRUB_EFI_MAC_ADDRESS_DEVICE_PATH_SUBTYPE	11

struct grub_efi_mac_address_device_path
{
	grub_efi_device_path_t header;
	grub_efi_mac_address_t mac_address;
	grub_efi_uint8_t if_type;
};
typedef struct grub_efi_mac_address_device_path grub_efi_mac_address_device_path_t;

#define GRUB_EFI_IPV4_DEVICE_PATH_SUBTYPE		12

struct grub_efi_ipv4_device_path
{
	grub_efi_device_path_t header;
	grub_efi_ipv4_address_t local_ip_address;
	grub_efi_ipv4_address_t remote_ip_address;
	grub_efi_uint16_t local_port;
	grub_efi_uint16_t remote_port;
	grub_efi_uint16_t protocol;
	grub_efi_uint8_t static_ip_address;
};
typedef struct grub_efi_ipv4_device_path grub_efi_ipv4_device_path_t;

#define GRUB_EFI_IPV6_DEVICE_PATH_SUBTYPE		13

struct grub_efi_ipv6_device_path
{
	grub_efi_device_path_t header;
	grub_efi_ipv6_address_t local_ip_address;
	grub_efi_ipv6_address_t remote_ip_address;
	grub_efi_uint16_t local_port;
	grub_efi_uint16_t remote_port;
	grub_efi_uint16_t protocol;
	grub_efi_uint8_t static_ip_address;
};
typedef struct grub_efi_ipv6_device_path grub_efi_ipv6_device_path_t;

#define GRUB_EFI_INFINIBAND_DEVICE_PATH_SUBTYPE		9

struct grub_efi_infiniband_device_path
{
	grub_efi_device_path_t header;
	grub_efi_uint32_t resource_flags;
	grub_efi_uint8_t port_gid[16];
	grub_efi_uint64_t remote_id;
	grub_efi_uint64_t target_port_id;
	grub_efi_uint64_t device_id;
};
typedef struct grub_efi_infiniband_device_path
grub_efi_infiniband_device_path_t;

#define GRUB_EFI_UART_DEVICE_PATH_SUBTYPE		14

struct grub_efi_uart_device_path
{
	grub_efi_device_path_t header;
	grub_efi_uint32_t reserved;
	grub_efi_uint64_t baud_rate;
	grub_efi_uint8_t data_bits;
	grub_efi_uint8_t parity;
	grub_efi_uint8_t stop_bits;
};
typedef struct grub_efi_uart_device_path grub_efi_uart_device_path_t;

#define GRUB_EFI_SATA_DEVICE_PATH_SUBTYPE		18

struct grub_efi_sata_device_path
{
	grub_efi_device_path_t header;
	grub_efi_uint16_t hba_port;
	grub_efi_uint16_t multiplier_port;
	grub_efi_uint16_t lun;
};
typedef struct grub_efi_sata_device_path grub_efi_sata_device_path_t;

#define GRUB_EFI_VENDOR_MESSAGING_DEVICE_PATH_SUBTYPE	10

/* Media Device Path.  */
#define GRUB_EFI_MEDIA_DEVICE_PATH_TYPE			4

#define GRUB_EFI_HARD_DRIVE_DEVICE_PATH_SUBTYPE		1

struct grub_efi_hard_drive_device_path
{
	grub_efi_device_path_t header;
	grub_efi_uint32_t partition_number;
	grub_efi_lba_t partition_start;
	grub_efi_lba_t partition_size;
	grub_efi_uint8_t partition_signature[16];
	grub_efi_uint8_t partmap_type;
	grub_efi_uint8_t signature_type;
};
typedef struct grub_efi_hard_drive_device_path grub_efi_hard_drive_device_path_t;

#define GRUB_EFI_CDROM_DEVICE_PATH_SUBTYPE		2

struct grub_efi_cdrom_device_path
{
	grub_efi_device_path_t header;
	grub_efi_uint32_t boot_entry;
	grub_efi_lba_t partition_start;
	grub_efi_lba_t partition_size;
};
typedef struct grub_efi_cdrom_device_path grub_efi_cdrom_device_path_t;

#define GRUB_EFI_VENDOR_MEDIA_DEVICE_PATH_SUBTYPE	3

struct grub_efi_vendor_media_device_path
{
	grub_efi_device_path_t header;
	grub_efi_packed_guid_t vendor_guid;
	grub_efi_uint8_t vendor_defined_data[0];
};
typedef struct grub_efi_vendor_media_device_path grub_efi_vendor_media_device_path_t;

#define GRUB_EFI_FILE_PATH_DEVICE_PATH_SUBTYPE		4

struct grub_efi_file_path_device_path
{
	grub_efi_device_path_t header;
	grub_efi_char16_t path_name[0];
};
typedef struct grub_efi_file_path_device_path grub_efi_file_path_device_path_t;

#define GRUB_EFI_PROTOCOL_DEVICE_PATH_SUBTYPE		5

struct grub_efi_protocol_device_path
{
	grub_efi_device_path_t header;
	grub_efi_packed_guid_t guid;
};
typedef struct grub_efi_protocol_device_path grub_efi_protocol_device_path_t;

#define GRUB_EFI_PIWG_DEVICE_PATH_SUBTYPE		6

struct grub_efi_piwg_device_path
{
	grub_efi_device_path_t header;
	grub_efi_packed_guid_t guid;
};
typedef struct grub_efi_piwg_device_path grub_efi_piwg_device_path_t;

/* BIOS Boot Specification Device Path.  */
#define GRUB_EFI_BIOS_DEVICE_PATH_TYPE			5

#define GRUB_EFI_BIOS_DEVICE_PATH_SUBTYPE		1

struct grub_efi_bios_device_path
{
	grub_efi_device_path_t header;
	grub_efi_uint16_t device_type;
	grub_efi_uint16_t status_flags;
	char description[0];
};
typedef struct grub_efi_bios_device_path grub_efi_bios_device_path_t;

PRAGMA_END_PACKED

void
grub_efi_print_device_path(grub_efi_device_path_t* dp);

grub_err_t
grub_efi_get_variable(const char* var,
	const char* guid,
	grub_size_t* datasize_out,
	void** data_out,
	grub_uint32_t* attributes);

grub_err_t
grub_efi_set_variable(const char* var,
	const char* guid,
	grub_size_t datasize,
	void* data,
	const grub_uint32_t* attributes);

#endif
