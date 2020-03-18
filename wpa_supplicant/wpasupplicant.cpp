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
#include <wifi/p9compat.h>
#include <assert.h>

extern "C" int wpa_supplicant_main (const char *confname);

CWPASupplicant::CWPASupplicant (const char *pConfigFile)
:	m_ConfigFile (pConfigFile)
{
}

CWPASupplicant::~CWPASupplicant (void)
{
}

boolean CWPASupplicant::Initialize (void)
{
	kproc ("wpa_supplicant", ProcEntry, this);

	return TRUE;
}

void CWPASupplicant::ProcEntry (void *pParam)
{
	CWPASupplicant *pThis = (CWPASupplicant *) pParam;
	assert (pThis != 0);

	wpa_supplicant_main (pThis->m_ConfigFile);
}
