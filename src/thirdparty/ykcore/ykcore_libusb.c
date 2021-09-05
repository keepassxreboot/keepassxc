/* -*- mode:C; c-file-style: "bsd" -*- */
/*
 * Copyright (c) 2008-2014 Yubico AB
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <usb.h>
#include <stdio.h>
#include <string.h>

#include "ykcore.h"
#include "ykdef.h"
#include "ykcore_backend.h"

#define HID_GET_REPORT			0x01
#define HID_SET_REPORT			0x09

/*************************************************************************
 **  function _ykusb_write						**
 **  Set HID report							**
 **									**
 **  int _ykusb_write(YUBIKEY *yk, int report_type, int report_number,	**
 **			char *buffer, int size)				**
 **									**
 **  Where:								**
 **  "yk" is handle to open Yubikey					**
 **  "report_type" is HID report type (in, out or feature)		**
 **  "report_number" is report identifier				**
 **  "buffer" is pointer to in buffer					**
 **  "size" is size of the buffer					**
 **									**
 **  Returns: Nonzero if successful, zero otherwise			**
 **									**
 *************************************************************************/

int _ykusb_write(void *dev, int report_type, int report_number,
		 char *buffer, int size)
{
	int rc = usb_claim_interface((usb_dev_handle *)dev, 0);

	if (rc >= 0) {
		int rc2;
		rc = usb_control_msg((usb_dev_handle *)dev,
				     USB_TYPE_CLASS | USB_RECIP_INTERFACE | USB_ENDPOINT_OUT,
				     HID_SET_REPORT,
				     report_type << 8 | report_number, 0,
				     buffer, size,
				     1000);
		/* preserve a control message error over an interface
		   release one */
		rc2 = usb_release_interface((usb_dev_handle *)dev, 0);
		if (rc >= 0 && rc2 < 0)
			rc = rc2;
	}
	if (rc >= 0)
		return 1;
	yk_errno = YK_EUSBERR;
	return 0;
}

/*************************************************************************
**  function _ykusb_read						**
**  Get HID report							**
**                                                                      **
**  int _ykusb_read(YUBIKEY *dev, int report_type, int report_number,	**
**		       char *buffer, int size)				**
**                                                                      **
**  Where:                                                              **
**  "dev" is handle to open Yubikey					**
**  "report_type" is HID report type (in, out or feature)		**
**  "report_number" is report identifier					**
**  "buffer" is pointer to in buffer					**
**  "size" is size of the buffer					**
**									**
**  Returns: Number of bytes read. Zero if failure			**
**                                                                      **
*************************************************************************/

int _ykusb_read(void *dev, int report_type, int report_number,
		char *buffer, int size)
{
	int rc = usb_claim_interface((usb_dev_handle *)dev, 0);

	if (rc >= 0) {
		int rc2;
		rc = usb_control_msg((usb_dev_handle *)dev,
				     USB_TYPE_CLASS | USB_RECIP_INTERFACE | USB_ENDPOINT_IN,
				     HID_GET_REPORT,
				     report_type << 8 | report_number, 0,
				     buffer, size,
				     1000);
		/* preserve a control message error over an interface
		   release one */
		rc2 = usb_release_interface((usb_dev_handle *)dev, 0);
		if (rc >= 0 && rc2 < 0)
			rc = rc2;
	}
	if (rc >= 0)
		return rc;
	if(rc == 0)
		yk_errno = YK_ENODATA;
	else
		yk_errno = YK_EUSBERR;
	return 0;
}

int _ykusb_start(void)
{
	int rc;
	usb_init();

	rc = usb_find_busses();
	if (rc >= 0)
		rc = usb_find_devices();

	if (rc >= 0)
		return 1;
	yk_errno = YK_EUSBERR;
	return 0;
}

extern int _ykusb_stop(void)
{
	return 1;
}

void *_ykusb_open_device(int vendor_id, const int *product_ids, size_t pids_len, int index)
{
	struct usb_bus *bus;
	struct usb_device *yk_device = NULL;
	struct usb_dev_handle *h = NULL;
	int rc = YK_EUSBERR;
	int found = 0;

	for (bus = usb_get_busses(); bus; bus = bus->next) {
		struct usb_device *dev;
		rc = YK_ENOKEY;
		for (dev = bus->devices; dev; dev = dev->next) {
			if (dev->descriptor.idVendor == vendor_id) {
				size_t j;
				for (j = 0; j < pids_len; j++) {
					if (dev->descriptor.idProduct == product_ids[j]) {
                                                found++;
                                                if (found-1 == index) {
                                                        yk_device = dev;
                                                        break;
                                                }
					}
				}
			}
		}
	}
	if(yk_device != NULL) {
		rc = YK_EUSBERR;
		h = usb_open(yk_device);
#ifdef LIBUSB_HAS_DETACH_KERNEL_DRIVER_NP
		if (h != NULL)
			usb_detach_kernel_driver_np(h, 0);
#endif
		/* This is needed for yubikey-personalization to work inside virtualbox virtualization. */
		if (h != NULL)
			usb_set_configuration(h, 1);
		goto done;
	}
 done:
	if (h == NULL)
		yk_errno = rc;
	return h;
}

int _ykusb_close_device(void *yk)
{
	int rc = usb_close((usb_dev_handle *) yk);

	if (rc >= 0)
		return 1;
	yk_errno = YK_EUSBERR;
	return 0;
}

int _ykusb_get_vid_pid(void *yk, int *vid, int *pid) {
	struct usb_dev_handle *h = yk;
	struct usb_device *dev = usb_device(h);
	*vid = dev->descriptor.idVendor;
	*pid = dev->descriptor.idProduct;
	return 1;
}

const char *_ykusb_strerror(void)
{
	return usb_strerror();
}
