// SPDX-License-Identifier: GPL-3.0-or-later
/*
ImDisk Virtual Disk Driver for Windows NT/2000/XP.
Copyright (C) 2005-2021 Olof Lagerkvist.
Permission is hereby granted, free of charge, to any person
obtaining a copy of this software and associated documentation
files (the "Software"), to deal in the Software without
restriction, including without limitation the rights to use,
copy, modify, merge, publish, distribute, sublicense, and/or
sell copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following
conditions:
The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.
*/

#ifndef _IMDISK_H_
#define _IMDISK_H_ 1

#include <windows.h>

#ifndef __T
#if defined(_NTDDK_) || defined(UNICODE) || defined(_UNICODE)
#define __T(x)  L ## x
#else
#define __T(x)  x
#endif
#endif

#ifndef _T
#define _T(x)   __T(x)
#endif

///
/// Base names for device objects created in \Device
///
#define IMDISK_DEVICE_DIR_NAME         _T("\\Device")
#define IMDISK_DEVICE_BASE_NAME        IMDISK_DEVICE_DIR_NAME  _T("\\ImDisk")
#define IMDISK_CTL_DEVICE_NAME         IMDISK_DEVICE_BASE_NAME _T("Ctl")

///
/// Symlinks created in \DosDevices to device objects
///
#define IMDISK_SYMLNK_NATIVE_DIR_NAME  _T("\\DosDevices")
#define IMDISK_SYMLNK_WIN32_DIR_NAME   _T("\\\\?")
#define IMDISK_SYMLNK_NATIVE_BASE_NAME IMDISK_SYMLNK_NATIVE_DIR_NAME  _T("\\ImDisk")
#define IMDISK_SYMLNK_WIN32_BASE_NAME  IMDISK_SYMLNK_WIN32_DIR_NAME   _T("\\ImDisk")
#define IMDISK_CTL_SYMLINK_NAME        IMDISK_SYMLNK_NATIVE_BASE_NAME _T("Ctl")
#define IMDISK_CTL_DOSDEV_NAME         IMDISK_SYMLNK_WIN32_BASE_NAME  _T("Ctl")

///
/// The driver name and image path
///
#define IMDISK_DRIVER_NAME             _T("ImDisk")
#define IMDISK_DRIVER_PATH             _T("system32\\drivers\\imdisk.sys")

#ifndef AWEALLOC_DRIVER_NAME
#define AWEALLOC_DRIVER_NAME           _T("AWEAlloc")
#endif
#ifndef AWEALLOC_DEVICE_NAME
#define AWEALLOC_DEVICE_NAME           _T("\\Device\\") AWEALLOC_DRIVER_NAME
#endif

///
/// Global refresh event name
///
#define IMDISK_REFRESH_EVENT_NAME      _T("ImDiskRefresh")

///
/// Registry settings. It is possible to specify devices to be mounted
/// automatically when the driver loads.
///
#define IMDISK_CFG_PARAMETER_KEY                  _T("\\Parameters")
#define IMDISK_CFG_MAX_DEVICES_VALUE              _T("MaxDevices")
#define IMDISK_CFG_LOAD_DEVICES_VALUE             _T("LoadDevices")
#define IMDISK_CFG_DISALLOWED_DRIVE_LETTERS_VALUE _T("DisallowedDriveLetters")
#define IMDISK_CFG_IMAGE_FILE_PREFIX              _T("FileName")
#define IMDISK_CFG_SIZE_PREFIX                    _T("Size")
#define IMDISK_CFG_FLAGS_PREFIX                   _T("Flags")
#define IMDISK_CFG_DRIVE_LETTER_PREFIX            _T("DriveLetter")
#define IMDISK_CFG_OFFSET_PREFIX                  _T("ImageOffset")

#define KEY_NAME_HKEY_MOUNTPOINTS  \
  _T("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\MountPoints")
#define KEY_NAME_HKEY_MOUNTPOINTS2  \
  _T("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\MountPoints2")

#define IMDISK_WINVER_MAJOR() (GetVersion() & 0xFF)
#define IMDISK_WINVER_MINOR() ((GetVersion() & 0xFF00) >> 8)

#define IMDISK_WINVER() ((IMDISK_WINVER_MAJOR() << 8) | \
    IMDISK_WINVER_MINOR())

#if defined(NT4_COMPATIBLE) && defined(_M_IX86)
#define IMDISK_GTE_WIN2K() (IMDISK_WINVER_MAJOR() >= 0x05)
#else
#define IMDISK_GTE_WIN2K() TRUE
#endif

#ifdef _M_IX86
#define IMDISK_GTE_WINXP() (IMDISK_WINVER() >= 0x0501)
#else
#define IMDISK_GTE_WINXP() TRUE
#endif

#define IMDISK_GTE_SRV2003() (IMDISK_WINVER() >= 0x0502)

#define IMDISK_GTE_VISTA() (IMDISK_WINVER_MAJOR() >= 0x06)

#ifndef IMDISK_API
#ifdef IMDISK_CPL_EXPORTS
#define IMDISK_API __declspec(dllexport)
#else
#define IMDISK_API __declspec(dllimport)
#endif
#endif

///
/// Base value for the IOCTL's.
///
#define FILE_DEVICE_IMDISK		    0x8372

#define IOCTL_IMDISK_QUERY_VERSION	    ((ULONG) CTL_CODE(FILE_DEVICE_IMDISK, 0x800, METHOD_BUFFERED, 0))
#define IOCTL_IMDISK_CREATE_DEVICE	    ((ULONG) CTL_CODE(FILE_DEVICE_IMDISK, 0x801, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS))
#define IOCTL_IMDISK_QUERY_DEVICE	    ((ULONG) CTL_CODE(FILE_DEVICE_IMDISK, 0x802, METHOD_BUFFERED, 0))
#define IOCTL_IMDISK_QUERY_DRIVER           ((ULONG) CTL_CODE(FILE_DEVICE_IMDISK, 0x803, METHOD_BUFFERED, 0))
#define IOCTL_IMDISK_REFERENCE_HANDLE       ((ULONG) CTL_CODE(FILE_DEVICE_IMDISK, 0x804, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS))
#define IOCTL_IMDISK_SET_DEVICE_FLAGS       ((ULONG) CTL_CODE(FILE_DEVICE_IMDISK, 0x805, METHOD_BUFFERED, 0))
#define IOCTL_IMDISK_REMOVE_DEVICE          ((ULONG) CTL_CODE(FILE_DEVICE_IMDISK, 0x806, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS))
#define IOCTL_IMDISK_IOCTL_PASS_THROUGH	    ((ULONG) CTL_CODE(FILE_DEVICE_IMDISK, 0x807, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS))
#define IOCTL_IMDISK_FSCTL_PASS_THROUGH	    ((ULONG) CTL_CODE(FILE_DEVICE_IMDISK, 0x808, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS))
#define IOCTL_IMDISK_GET_REFERENCED_HANDLE  ((ULONG) CTL_CODE(FILE_DEVICE_IMDISK, 0x809, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS))

///
/// Bit constants for the Flags field in IMDISK_CREATE_DATA
///

/// Read-only device
#define IMDISK_OPTION_RO                0x00000001

/// Check if flags specifies read-only
#define IMDISK_READONLY(x)              ((ULONG)(x) & 0x00000001)

/// Removable, hot-plug, device
#define IMDISK_OPTION_REMOVABLE         0x00000002

/// Check if flags specifies removable
#define IMDISK_REMOVABLE(x)             ((ULONG)(x) & 0x00000002)

/// Specifies that image file is created with sparse attribute.
#define IMDISK_OPTION_SPARSE_FILE       0x00000004

/// Check if flags specifies sparse
#define IMDISK_SPARSE_FILE(x)           ((ULONG)(x) & 0x00000004)

/// Swaps each byte pair in image file.
#define IMDISK_OPTION_BYTE_SWAP         0x00000008

/// Check if flags specifies byte swapping
#define IMDISK_BYTE_SWAP(x)             ((ULONG)(x) & 0x00000008)

/// Device type is virtual harddisk partition
#define IMDISK_DEVICE_TYPE_HD           0x00000010
/// Device type is virtual floppy drive
#define IMDISK_DEVICE_TYPE_FD           0x00000020
/// Device type is virtual CD/DVD-ROM drive
#define IMDISK_DEVICE_TYPE_CD           0x00000030
/// Device type is unknown "raw" (for use with third-party client drivers)
#define IMDISK_DEVICE_TYPE_RAW          0x00000040

/// Extracts the IMDISK_DEVICE_TYPE_xxx from flags
#define IMDISK_DEVICE_TYPE(x)           ((ULONG)(x) & 0x000000F0)

/// Virtual disk is backed by image file
#define IMDISK_TYPE_FILE                0x00000100
/// Virtual disk is backed by virtual memory
#define IMDISK_TYPE_VM                  0x00000200
/// Virtual disk is backed by proxy connection
#define IMDISK_TYPE_PROXY               0x00000300

/// Extracts the IMDISK_TYPE_xxx from flags
#define IMDISK_TYPE(x)                  ((ULONG)(x) & 0x00000F00)

// Types with proxy mode

/// Proxy connection is direct-type
#define IMDISK_PROXY_TYPE_DIRECT        0x00000000
/// Proxy connection is over serial line
#define IMDISK_PROXY_TYPE_COMM          0x00001000
/// Proxy connection is over TCP/IP
#define IMDISK_PROXY_TYPE_TCP           0x00002000
/// Proxy connection uses shared memory
#define IMDISK_PROXY_TYPE_SHM           0x00003000

/// Extracts the IMDISK_PROXY_TYPE_xxx from flags
#define IMDISK_PROXY_TYPE(x)            ((ULONG)(x) & 0x0000F000)

// Types with file mode

/// Serialized I/O to an image file, done in a worker thread
#define IMDISK_FILE_TYPE_QUEUED_IO      0x00000000
/// Direct parallel I/O to AWEAlloc driver (physical RAM), done in request
/// thread
#define IMDISK_FILE_TYPE_AWEALLOC       0x00001000
/// Direct parallel I/O to an image file, done in request thread
#define IMDISK_FILE_TYPE_PARALLEL_IO    0x00002000
/// Buffered I/O to an image file. Disables FILE_NO_INTERMEDIATE_BUFFERING when
/// opening image file.
#define IMDISK_FILE_TYPE_BUFFERED_IO    0x00003000

/// Extracts the IMDISK_FILE_TYPE_xxx from flags
#define IMDISK_FILE_TYPE(x)             ((ULONG)(x) & 0x0000F000)

/// Flag set by write request dispatchers to indicated that virtual disk has
/// been since mounted
#define IMDISK_IMAGE_MODIFIED           0x00010000

// 0x00020000 is reserved. Corresponds to IMSCSI_FAKE_DISK_SIG in
// Arsenal Image Mounter.

/// This flag causes the driver to open image files in shared write mode even
/// if the image is opened for writing. This could be useful in some cases,
/// but could easily corrupt filesystems on image files if used incorrectly.
#define IMDISK_OPTION_SHARED_IMAGE      0x00040000
/// Check if flags indicate shared write mode
#define IMDISK_SHARED_IMAGE(x)          ((ULONG)(x) & 0x00040000)

/// Macro to determine if flags specify either virtual memory (type vm) or
/// physical memory (type file with awealloc) virtual disk drive
#define IMDISK_IS_MEMORY_DRIVE(x) \
  ((IMDISK_TYPE(x) == IMDISK_TYPE_VM) || \
   ((IMDISK_TYPE(x) == IMDISK_TYPE_FILE) && \
    (IMDISK_FILE_TYPE(x) == IMDISK_FILE_TYPE_AWEALLOC)))

/// Specify as device number to automatically select first free.
#define IMDISK_AUTO_DEVICE_NUMBER       ((ULONG)-1)

/**
Structure used by the IOCTL_IMDISK_CREATE_DEVICE and
IOCTL_IMDISK_QUERY_DEVICE calls and by the ImDiskQueryDevice function.
*/
typedef struct _IMDISK_CREATE_DATA
{
    /// On create this can be set to IMDISK_AUTO_DEVICE_NUMBER
    ULONG           DeviceNumber;
    /// Total size in bytes (in the Cylinders field) and virtual geometry.
    DISK_GEOMETRY   DiskGeometry;
    /// The byte offset in the image file where the virtual disk begins.
    LARGE_INTEGER   ImageOffset;
    /// Creation flags. Type of device and type of connection.
    ULONG           Flags;
    /// Drive letter (if used, otherwise zero).
    WCHAR           DriveLetter;
    /// Length in bytes of the FileName member.
    USHORT          FileNameLength;
    /// Dynamically-sized member that specifies the image file name.
    WCHAR           FileName[1];
} IMDISK_CREATE_DATA, *PIMDISK_CREATE_DATA;

typedef struct _IMDISK_SET_DEVICE_FLAGS
{
    ULONG FlagsToChange;
    ULONG FlagValues;
} IMDISK_SET_DEVICE_FLAGS, *PIMDISK_SET_DEVICE_FLAGS;

#define IMDISK_API_NO_BROADCAST_NOTIFY  0x00000001
#define IMDISK_API_FORCE_DISMOUNT       0x00000002


#endif
