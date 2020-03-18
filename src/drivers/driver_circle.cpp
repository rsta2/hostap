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

extern "C" {

#include "includes.h"
#include "common.h"
#include "driver.h"

}

#include <circle/netdevice.h>
#include <assert.h>

struct wpa_driver_circle_data
{
	void *ctx;
	CNetDevice *netdev;
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
	drv->netdev = netdev;

	return drv;
}

static void wpa_driver_circle_deinit (void *priv)
{
	wpa_driver_circle_data *drv = (wpa_driver_circle_data *) priv;
	assert (drv != 0);

	os_free (drv);
}

extern "C" const struct wpa_driver_ops wpa_driver_circle_ops =
{
	.name = "circle",
	.desc = "Circle network driver",
	.init = wpa_driver_circle_init,
	.deinit = wpa_driver_circle_deinit,
};
