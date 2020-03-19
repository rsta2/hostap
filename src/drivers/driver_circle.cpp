//
// driver_circle.cpp
//
// Driver interface for Circle network driver
// by R. Stange <rsta2@o2online.de>
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.
//
// Alternatively, this software may be distributed under the terms of BSD
// license.
//

/*
 * The "struct brcmf_*_le" definitions in this file are:
 *
 * Copyright (c) 2012 Broadcom Corporation
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

extern "C" {

#include "includes.h"
#include "common.h"
#include "driver.h"
#include "eloop.h"

}

#include <circle/netdevice.h>
#include <wifi/bcm4343.h>
#include <assert.h>

typedef u16 __le16;
typedef u32 __le32;

struct brcmf_bss_info_le
{
	__le32 version;		/* version field */
#define	BRCMF_BSS_INFO_VERSION	109 /* curr ver of brcmf_bss_info_le struct */
	__le32 length;		/* byte length of data in this record,
				 * starting at version and including IEs
				 */
	u8 BSSID[ETH_ALEN];
	__le16 beacon_period;	/* units are Kusec */
	__le16 capability;	/* Capability information */
	u8 SSID_len;
	u8 SSID[32];
	struct {
		__le32 count;	/* # rates in this set */
		u8 rates[16];	/* rates in 500kbps units w/hi bit set if basic */
	} rateset;		/* supported rates */
	__le16 chanspec;	/* chanspec for bss */
	__le16 atim_window;	/* units are Kusec */
	u8 dtim_period;		/* DTIM period */
	__le16 RSSI;		/* receive signal strength (in dBm) */
	s8 phy_noise;		/* noise (in dBm) */

	u8 n_cap;		/* BSS is 802.11N Capable */
	/* 802.11N BSS Capabilities (based on HT_CAP_*): */
	__le32 nbss_cap;
	u8 ctl_ch;		/* 802.11N BSS control channel number */
	__le32 reserved32[1];	/* Reserved for expansion of BSS properties */
	u8 flags;		/* flags */
	u8 reserved[3];		/* Reserved for expansion of BSS properties */
#define BRCMF_MCSSET_LEN		16
	u8 basic_mcs[BRCMF_MCSSET_LEN];	/* 802.11N BSS required MCS set */

	__le16 ie_offset;	/* offset at which IEs start, from beginning */
	__le32 ie_length;	/* byte length of Information Elements */
	__le16 SNR;		/* average SNR of during frame reception */
	/* Add new fields here */
	/* variable length Information Elements */
};

struct brcmf_escan_result_le
{
	__le32 buflen;
	__le32 version;
	__le16 sync_id;
	__le16 bss_count;
	struct brcmf_bss_info_le bss_info_le;
};

struct wpa_driver_circle_data
{
	void *ctx;
	CBcm4343Device *netdev;
};

static void *wpa_driver_circle_init (void *ctx, const char *ifname)
{
	CNetDevice *netdev = CNetDevice::GetNetDevice (NetDeviceTypeWLAN);
	if (netdev == 0)
	{
		return 0;
	}

	wpa_driver_circle_data *drv = (wpa_driver_circle_data *) os_zalloc (sizeof *drv);
	if (drv == 0)
	{
		return 0;
	}

	drv->ctx = ctx;
	drv->netdev = (CBcm4343Device *) netdev;	// netdev can only be of this type

	return drv;
}

static void wpa_driver_circle_deinit (void *priv)
{
	wpa_driver_circle_data *drv = (wpa_driver_circle_data *) priv;
	assert (drv != 0);

	os_free (drv);
}

#define SCAN_DURATION_SECS	3

static void wpa_driver_circle_scan_timeout (void *eloop_ctx, void *timeout_ctx)
{
	wpa_driver_circle_data *drv = (wpa_driver_circle_data *) eloop_ctx;
	assert (drv != 0);

	assert (drv->netdev != 0);
	drv->netdev->Control ("escan 0");	// stop scan

	wpa_supplicant_event (timeout_ctx, EVENT_SCAN_RESULTS, 0);
}

static int wpa_driver_circle_scan2 (void *priv, wpa_driver_scan_params *params)
{
	wpa_driver_circle_data *drv = (wpa_driver_circle_data *) priv;
	assert (drv != 0);

	assert (params != 0);

	// TODO: allow scan params
	//assert (params->num_ssids == 0);
	assert (params->extra_ies == 0);
	assert (params->extra_ies_len == 0);
	assert (params->freqs == 0);

	assert (drv->netdev != 0);
	// increase scan duration here to be sure, scan is not started again
	if (!drv->netdev->Control ("escan %u", SCAN_DURATION_SECS+2))
	{
		return -1;
	}

	eloop_cancel_timeout (wpa_driver_circle_scan_timeout, drv, drv->ctx);
	eloop_register_timeout (SCAN_DURATION_SECS, 0, wpa_driver_circle_scan_timeout,
				drv, drv->ctx);

	return 0;
}

static int chanspec2freq (u16 chanspec)		// TODO
{
	u8 chan = chanspec & 0xFF;

	if (1 <= chan && chan <= 14)
	{
		static const int low_freqs[] =
		{
			2412, 2417, 2422, 2427, 2432, 2437, 2442,
			2447, 2452, 2457, 2462, 2467, 2472, 2484
		};

		return low_freqs[chan-1];
	}

	if (36 <= chan && chan <= 140)
	{
		if (65 <= chan && chan <= 99)
		{
			return -1;
		}

		return 5180 + (chan-36) * 5;
	}

	return -1;
}

#define MAX_SCAN_RESULTS	128

static wpa_scan_results *wpa_driver_circle_get_scan_results2 (void *priv)
{
	wpa_driver_circle_data *drv = (wpa_driver_circle_data *) priv;
	assert (drv != 0);

	wpa_scan_res **res_vector =
		(wpa_scan_res **) os_zalloc (MAX_SCAN_RESULTS * sizeof (wpa_scan_res *));
	if (res_vector == 0)
	{
		return 0;
	}

	wpa_scan_results *results = (wpa_scan_results *) os_zalloc (sizeof (wpa_scan_results));
	if (results == 0)
	{
		os_free (res_vector);

		return 0;
	}
	results->res = res_vector;
	results->num = 0;

	unsigned len;
	u8 buf[FRAME_BUFFER_SIZE];
	assert (drv->netdev != 0);
	while (drv->netdev->ReceiveScanResult (buf, &len))
	{
		// remove remaining scan messages, if vector is full
		if (results->num == MAX_SCAN_RESULTS)
		{
			continue;
		}

		// TODO: validate escan result data
		brcmf_escan_result_le *scan_res = (brcmf_escan_result_le *) buf;
		assert (scan_res->version == BRCMF_BSS_INFO_VERSION);

		brcmf_bss_info_le *bss = &scan_res->bss_info_le;
		for (unsigned i = 0; i < scan_res->bss_count; i++)
		{
			int freq = chanspec2freq (bss->chanspec);
			if (freq <= 0)
			{
				continue;
			}

			wpa_scan_res *res =
				(wpa_scan_res *) os_zalloc (sizeof (wpa_scan_res) + bss->ie_length);
			if (res == 0)
			{
				break;
			}

			os_memset (res, 0, sizeof *res);

			res->flags = WPA_SCAN_LEVEL_DBM | WPA_SCAN_QUAL_INVALID;
			os_memcpy (res->bssid, bss->BSSID, ETH_ALEN);
			res->freq = freq;
			res->beacon_int = bss->beacon_period;
			res->caps = bss->capability;
			res->noise = bss->phy_noise;
			res->level = bss->RSSI;
			// TODO: set res->tsf
			// TODO: set res->age

			// append IEs
			res->ie_len = bss->ie_length;
			os_memcpy ((u8 *) res + sizeof *res, (u8 *) bss + bss->ie_offset,
				   bss->ie_length);

			*res_vector++ = res;
			results->num++;

			bss = (brcmf_bss_info_le *) ((u8 *) bss + bss->length);
		}
	}

	return results;
}

extern "C" const struct wpa_driver_ops wpa_driver_circle_ops =
{
	.name = "circle",
	.desc = "Circle network driver",
	.init = wpa_driver_circle_init,
	.deinit = wpa_driver_circle_deinit,
	.get_scan_results2 = wpa_driver_circle_get_scan_results2,
	.scan2 = wpa_driver_circle_scan2,
};
