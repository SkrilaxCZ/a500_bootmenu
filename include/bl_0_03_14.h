/*
 * This file maps functions and variables that can be found in Acer 0.03.14-ICS bootloader,
 * and the patched version.
 *
 * Copyright (C) 2012 Skrilax_CZ
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/*
 * Please note: Entry point of the application occurs when the bootloader
 * is about to check for bootmode (MSC commands).
 *
 * Initial key state is already stored. Application is entered every time,
 * and can override boot mode.
 *
 * This file contains only functions that are linked to the bootloader binary.
 */

#ifndef BL_0_03_14_H
#define BL_0_03_14_H

#include <stddef.h>
#include <stdarg.h>

#ifdef __thumb__

#include "bootimg.h"

/* ===========================================================================
 * Thumb Mode functions
 * ===========================================================================
 */

/* Update bootloader if we have the blob (partially related to msc)
 * This function implements FOTA check, that's why recovery & bootloader update is merged.
 */
int check_bootloader_update(void* global_handle);

/*
 * GPIO
 */

/* GPIO state */
int get_gpio(int row, int column);

/*
 * Display functions
 */

/* Print on the display */
void println_display(const char* fmt, ...);

/* Print error on the display (red color) */
void println_display_error(const char* fmt, ...);

/* Print bootlogo */
void print_bootlogo();

/* Clear screen */
void clear_screen();

/* Unknown framebuffer call (used on screen refresh) */
void framebuffer_unknown_call();

/*
 * Partitions
 */

/* Partition open type */
#define PARTITION_OPEN_READ         1
#define PARTITION_OPEN_WRITE        2

/* Partition set position origin */
#define PARTITION_SETPOS_ABSOLUTE   0
#define PARTITION_SETPOS_RELATIVE   1

/* use LNX, SOS, CAC as partition ID */

/* Open partition */
int open_partition(const char* partition, int open_type, int* partition_handle);

/* Get partition position */
int get_partition_position(int partition_handle, uint64_t* postition);

/* Set partition position */
int set_partition_position(int partition_handle, int64_t offset, int origin);

/* Read partition */
int read_partition(int partition_handle, void* buffer, uint32_t buffer_length, uint32_t* processed_bytes);

/* Write partiiton */
int write_partition(int partition_handle, void* buffer, uint32_t data_size, uint32_t* processed_bytes);

/* Close partiiton */
int close_partition(int partition_handle);

/* Format partition */
int format_partition(const char* partition);

/* Get partition info */
int get_partition_size(const char* partition, uint64_t* partition_size);

/*
 * Miscellaneuos
 */

/* Check for wifi only tablet (00 - wifi only, 01 - 3G modem */
int is_wifi_only();

/* For some reason, strtol is not coming with std lib functions */
long int strtol(const char* str, char** endptr, int base);

/* Get serial info */
void get_serial_no(uint32_t* serial_no);

/* Toggle vibrator */
void toggle_vibrator(int state);

/* Reboot */
void reboot(void* global_handle);

/*
 * Booting
 */

/* Loads Android image, returns NULL for failure, actual return value wasn't used otherwise */
int android_load_image(struct boot_img_hdr** bootimg_ptr, uint32_t* bootimg_size, const char* partition);

/* Boots Android image, returns in case of error */
void android_boot_image(struct boot_img_hdr* bootimg, uint32_t bootimg_size, uint32_t ram_base);

/* Adds an ATAG */
int add_atag(uint32_t atag, uint32_t size, void* data);

/*
 * Fastboot
 * ----------------------
 *
 * The fastboot protocol is a mechanism for communicating with bootloaders
 * over USB.  It is designed to be very straightforward to implement, to
 * allow it to be used across a wide range of devices and from hosts running
 * Linux, Windows, or OSX.
 *
 *
 * Basic Requirements
 * ------------------
 *
 * Two bulk endpoints (in, out) are required
 * Max packet size must be 64 bytes for full-speed and 512 bytes for
 *  high-speed USB
 * The protocol is entirely host-driven and synchronous (unlike the
 *  multi-channel, bi-directional, asynchronous ADB protocol)
 *
 *
 * Transport and Framing
 * ---------------------
 *
 * 1. Host sends a command, which is an ascii string in a single
 *   packet no greater than 64 bytes.
 *
 * 2. Client response with a single packet no greater than 64 bytes.
 *   The first four bytes of the response are "OKAY", "FAIL", "DATA",
 *   or "INFO".  Additional bytes may contain an (ascii) informative
 *   message.
 *
 *   a. INFO -> the remaining 60 bytes are an informative message
 *      (providing progress or diagnostic messages).  They should
 *      be displayed and then step #2 repeats
 *
 *   b. FAIL -> the requested command failed.  The remaining 60 bytes
 *      of the response (if present) provide a textual failure message
 *      to present to the user.  Stop.
 *
 *   c. OKAY -> the requested command completed successfully.  Go to #5
 *
 *   d. DATA -> the requested command is ready for the data phase.
 *      A DATA response packet will be 12 bytes long, in the form of
 *      DATA00000000 where the 8 digit hexidecimal number represents
 *      the total data size to transfer.
 *
 * 3. Data phase.  Depending on the command, the host or client will
 *   send the indicated amount of data.  Short packets are always
 *   acceptable and zero-length packets are ignored.  This phase continues
 *   until the client has sent or received the number of bytes indicated
 *   in the "DATA" response above.
 *
 * 4. Client responds with a single packet no greater than 64 bytes.
 *   The first four bytes of the response are "OKAY", "FAIL", or "INFO".
 *   Similar to #2:
 *
 *   a. INFO -> display the remaining 60 bytes and return to #4
 *
 *   b. FAIL -> display the remaining 60 bytes (if present) as a failure
 *      reason and consider the command failed.  Stop.
 *
 *   c. OKAY -> success.  Go to #5
 *
 * 5. Success.  Stop.
 *
 *
 * Example Session
 * ---------------
 *
 * Host:    "getvar:version"        request version variable
 *
 * Client:  "OKAY0.4"               return version "0.4"
 *
 * Host:    "getvar:nonexistant"    request some undefined variable
 *
 * Client:  "OKAY"                  return value ""
 *
 * Host:    "download:00001234"     request to send 0x1234 bytes of data
 *
 * Client:  "DATA00001234"          ready to accept data
 *
 * Host:    < 0x1234 bytes >        send data
 *
 * Client:  "OKAY"                  success
 *
 * Host:    "flash:bootloader"      request to flash the data to the bootloader
 *
 * Client:  "INFOerasing flash"     indicate status / progress
 *         "INFOwriting flash"
 *         "OKAY"                  indicate success
 *
 * Host:    "powerdown"             send a command
 *
 * Client:  "FAILunknown command"   indicate failure
 *
 *
 * Command Reference
 * -----------------
 *
 * Command parameters are indicated by printf-style escape sequences.
 *
 * Commands are ascii strings and sent without the quotes (which are
 *  for illustration only here) and without a trailing 0 byte.
 *
 * Commands that begin with a lowercase letter are reserved for this
 *  specification.  OEM-specific commands should not begin with a
 *  lowercase letter, to prevent incompatibilities with future specs.
 *
 * "getvar:%s"           Read a config/version variable from the bootloader.
 *                       The variable contents will be returned after the
 *                       OKAY response.
 *
 * "download:%08x"       Write data to memory which will be later used
 *                       by "boot", "ramdisk", "flash", etc.  The client
 *                       will reply with "DATA%08x" if it has enough
 *                       space in RAM or "FAIL" if not.  The size of
 *                       the download is remembered.
 *
 *  "verify:%08x"        Send a digital signature to verify the downloaded
 *                       data.  Required if the bootloader is "secure"
 *                       otherwise "flash" and "boot" will be ignored.
 *
 *  "flash:%s"           Write the previously downloaded image to the
 *                       named partition (if possible).
 *
 *  "erase:%s"           Erase the indicated partition (clear to 0xFFs)
 *
 *  "boot"               The previously downloaded data is a boot.img
 *                       and should be booted according to the normal
 *                       procedure for a boot.img
 *
 *  "continue"           Continue booting as normal (if possible)
 *
 *  "reboot"             Reboot the device.
 *
 *  "reboot-bootloader"  Reboot back into the bootloader.
 *                       Useful for upgrade processes that require upgrading
 *                       the bootloader and then upgrading other partitions
 *                       using the new bootloader.
 *
 *  "powerdown"          Power off the device.
 *
 *
 *
 * Client Variables
 * ----------------
 *
 * The "getvar:%s" command is used to read client variables which
 * represent various information about the device and the software
 * on it.
 *
 * The various currently defined names are:
 *
 *  version             Version of FastBoot protocol supported.
 *                      It should be "0.4" for this document.
 *
 *  version-bootloader  Version string for the Bootloader.
 *
 *  version-baseband    Version string of the Baseband Software
 *
 *  product             Name of the product
 *
 *  serialno            Product serial number
 *
 *  secure              If the value is "yes", this is a secure
 *                      bootloader requiring a signature before
 *                      it will install or boot images.
 *
 * Names starting with a lowercase character are reserved by this
 * specification.  OEM-specific names should not start with lowercase
 * characters.
 */

/*
 * Several RE notes:
 * send / receive get some enum as return value -> bootloader passes on retval 5 or 0 (and stores smth with retval 5)
 */

/*
 * Unknown initialization functions
 */
void fastboot_init_unk0(void* global_handle);
void fastboot_init_unk1();

/*
 * Load fastboot handle
 * Returns 0 for OK, 1 for ERROR
 */
int fastboot_load_handle(int* fastboot_handle);
void fastboot_unload_handle(int fastboot_handle);

/*
 * Send to host
 */
int fastboot_send(int fastboot_handle, const char *command, uint32_t command_length);

/*
 * Receive from host
 */
int fastboot_recv0(int fastboot_handle, char* cmd_buffer, uint32_t buffer_length, uint32_t* cmd_length);
int fastboot_recv5(int fastboot_handle, char* cmd_buffer, uint32_t buffer_length, uint32_t* cmd_length);

#endif

/* ===========================================================================
 * ARM Mode functions
 * ===========================================================================
 */

/*
 * Standard library:
 *
 * You can use your own of course, but these are found in the bootloader
 */

int strcmp(const char *str1, const char *str2);
int strncmp(const char *str1, const char *str2, int n);
char* strncpy(char *destination, const char *source, int num);
int strlen(const char *str);
int snprintf(char *str, int size, const char *format, ...);
int vsnprintf(char *str, int size, const char *format, va_list ap);

void* malloc(int size);
int memcmp(const void *ptr1, const void *ptr2, int num);
void* memcpy(void *destination, const void *source, int num);
void* memset(void *ptr, int value, int num);
void free(void* ptr);

void printf(const char *format, ...);

void sleep(int ms);

/* ===========================================================================
 * Variables
 * ===========================================================================
 */

/* Bootloader version */
extern const char* bootloader_version;

/* Framebuffer */
extern uint8_t** framebuffer_ptr;

/* Framebuffer size */
extern uint32_t* framebuffer_size_ptr;

/* Fastboot unknown */
extern int* fastboot_unk_handle_var;

#endif //!BL_0_03_14_H
