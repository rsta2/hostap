//
// wpasupplicant.cpp
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
#include "wpasupplicant.h"
#include <fatfs/ff.h>
#include <wifi/p9compat.h>
#include <assert.h>

CWPASupplicant::CWPASupplicant (const char *pConfigFile)
:	m_ConfigFile (pConfigFile),
	m_pDevice (0),
	m_bInitStatus (FALSE)
{
}

CWPASupplicant::~CWPASupplicant (void)
{
}

boolean CWPASupplicant::Initialize (boolean bWaitForActivate)
{
	FIL File;
	FRESULT Result = f_open (&File, m_ConfigFile, FA_READ | FA_OPEN_EXISTING);
	if (Result != FR_OK)
	{
		print ("Cannot open: %s\n", (const char *) m_ConfigFile);

		return FALSE;
	}

	char SSID[50];
	unsigned nBytesRead;
	Result = f_read (&File, SSID, sizeof SSID-1, &nBytesRead);

	f_close (&File);

	if (   Result != FR_OK
	    || nBytesRead == 0)
	{
		print ("Cannot read: %s\n", (const char *) m_ConfigFile);

		return FALSE;
	}

	assert (nBytesRead < sizeof SSID);
	SSID[nBytesRead] = '\0';

	char *pSavePtr;
	m_SSID = strtok_r (SSID, " \t\n", &pSavePtr);
	if (m_SSID.GetLength () == 0)
	{
		print ("SSID expected\n");

		return FALSE;
	}

	m_pDevice = CNetDevice::GetNetDevice (NetDeviceTypeWLAN);
	if (m_pDevice == 0)
	{
		print ("WLAN device not found\n");

		return FALSE;
	}

	m_Event.Clear ();

	kproc ("wpa_supplicant", ProcEntry, this);

	if (!bWaitForActivate)
	{
		return TRUE;
	}

	m_Event.Wait ();

	return m_bInitStatus;
}

void CWPASupplicant::ProcEntry (void *pParam)
{
	CWPASupplicant *pThis = (CWPASupplicant *) pParam;
	assert (pThis != 0);

	CString Command ("join ");
	Command.Append (pThis->m_SSID);
	Command.Append (" 0 off");

	assert (pThis->m_pDevice != 0);
	pThis->m_bInitStatus = pThis->m_pDevice->Control (Command);

	pThis->m_Event.Set ();
}
