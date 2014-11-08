/*
 *	Copyright (C) 2003, 2004 Amit Schreiber <gnobal@yahoo.com>
 *
 *	This file is part of Relay plugin for Miranda IM.
 *
 *	Relay plugin for Miranda IM is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	Relay plugin for Miranda IM is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with Relay plugin for Miranda IM ; if not, write to the Free Software
 *	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __RELAY_COMMON_H__
#define __RELAY_COMMON_H__

#pragma warning(disable: 4786) 

#include <windows.h>
#include "resource.h"
#include <string>
#include <ctime>

#include "random/plugins/newpluginapi.h"
#include "database/m_database.h"
#include "ui/options/m_options.h"
#include "random/langpack/m_langpack.h"
#include "ui/contactlist/m_clist.h"

#define MS_RELAY_INTERNAL_COMMAND_EVENT "Relay/InternalCommandEvent/"

extern HINSTANCE hInst;

struct RelayPluginRuntimeData
{
	RelayPluginRuntimeData() 
		: hRelayToContact(NULL), nNumFailedLogonAttempts(0), 
		tLastActivity(0), bUserLoggedOn(FALSE) /*, bPaused(FALSE)*/ {}
	HANDLE hRelayToContact;
	int nNumFailedLogonAttempts;
	time_t tLastActivity;
	BOOL bUserLoggedOn;
//	BOOL bPaused;
};

extern RelayPluginRuntimeData pluginRuntimeData;
extern const char* szWhiteSpaces;

BOOL IsPluginEnabled();
int GetContactStatus(HANDLE hContact);
void ChangeStatus(int nStatus);

#endif
