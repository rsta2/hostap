//
// l2_packet_circle.cpp
//
// Layer2 packet handling interface for Circle
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
#include "eloop.h"
#include "l2_packet.h"

}

#include <circle/net/netsubsystem.h>
#include <circle/net/netdevlayer.h>
#include <assert.h>

#define SOCK_FD		1

struct l2_packet_data
{
	void (*rx_callback) (void *ctx, const u8 *src_addr, const u8 *buf, size_t len);
	void *rx_callback_ctx;
	u8 own_addr[ETH_ALEN];
};

static void l2_packet_receive (int sock, void *eloop_ctx, void *sock_ctx);

l2_packet_data * l2_packet_init (const char *ifname, const u8 *own_addr, unsigned short protocol,
				 void (*rx_callback) (void *ctx, const u8 *src_addr,
						      const u8 *buf, size_t len),
				 void *rx_callback_ctx, int l2_hdr)
{
	assert (own_addr == 0);
	assert (protocol == 0x888E);
	assert (l2_hdr == 0);

	l2_packet_data *l2 = (l2_packet_data *) os_zalloc (sizeof *l2);
	if (l2 == 0)
	{
		return 0;
	}

	l2->rx_callback = rx_callback;
	l2->rx_callback_ctx = rx_callback_ctx;

	const CMACAddress *mac = CNetSubSystem::Get ()->GetNetDeviceLayer ()->GetMACAddress ();
	assert (mac != 0);
	mac->CopyTo (l2->own_addr);

	eloop_register_read_sock (SOCK_FD, l2_packet_receive, l2, 0);

	return l2;
}

void l2_packet_deinit (l2_packet_data *l2)
{
	if (l2 == 0)
	{
		return;
	}

	eloop_unregister_read_sock (SOCK_FD);

	os_free (l2);
}

int l2_packet_get_own_addr (l2_packet_data *l2, u8 *addr)
{
	assert (l2 != 0);
	assert (addr != 0);
	os_memcpy (addr, l2->own_addr, ETH_ALEN);

	return 0;
}

int l2_packet_send (l2_packet_data *l2, const u8 *dst_addr, u16 proto, const u8 *buf, size_t len)
{
	if (l2 == 0)
	{
		return -1;
	}

	// TODO: send packet

	return 0;
}

static void l2_packet_receive (int sock, void *eloop_ctx, void *sock_ctx)
{
	assert (sock == SOCK_FD);
	l2_packet_data *l2 = (l2_packet_data *) eloop_ctx;
	assert (l2 != 0);

	// TODO: call l2->rx_callback
}

void l2_packet_notify_auth_start (l2_packet_data *l2)
{
}

int l2_packet_get_ip_addr (l2_packet_data *l2, char *buf, size_t len)
{
	return -1;
}
