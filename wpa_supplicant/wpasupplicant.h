//
// wpasupplicant.h
//
// WPA Supplicant wrapper class for Circle
// by R. Stange <rsta2@o2online.de>
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.
//
// Alternatively, this software may be distributed under the terms of BSD
// license.
//
#ifndef _wpasupplicant_h
#define _wpasupplicant_h

#include <circle/netdevice.h>
#include <circle/sched/synchronizationevent.h>
#include <circle/string.h>
#include <circle/types.h>

class CWPASupplicant
{
public:
	CWPASupplicant (const char *pConfigFile);
	~CWPASupplicant (void);

	boolean Initialize (boolean bWaitForActivate = TRUE);

private:
	static void ProcEntry (void *pParam);

private:
	CString m_ConfigFile;
	CString m_SSID;

	CNetDevice *m_pDevice;

	CSynchronizationEvent m_Event;
	volatile boolean m_bInitStatus;
};

#endif
