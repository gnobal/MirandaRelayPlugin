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

#ifndef __RELAY_OPTIONS_H__
#define __RELAY_OPTIONS_H__

#define MODULE_NAME "RelayPlugin"

#define OPT_MIMIC "Mimic"
#define OPT_ENABLE "Enable"
#define OPT_ALERT_ONLINE "AlertOnline"
#define OPT_ALERT_OFFLINE "AlertOffline"
#define OPT_ALERT_CHANGE_FROM_OFFLINE "AlertChangeFromOffline"
#define OPT_ALLOWED_COMMANDS "AllowedCommands"
#define OPT_DISALLOWED_COMMANDS "DisallowedCommands"
#define OPT_PWD_PROTECTED_COMMANDS "PwdProtectedCommands"
#define OPT_PASSWORD "Password"
#define OPT_AUTO_LOGOFF_TIME "AutoLogoffTime"
#define OPT_AUTO_LOCK_FAILED_ATTEMPTS "AutoLockAttempts"
#define OPT_TIMESTAMPS "Timestamps"
#define OPT_RELAY_TO_CONTACT "RelayToContact"
#define OPT_CREATE_HISTORY_EVENTS "CreateHistoryEvents"

#define OPT_DEFAULT_AUTO_LOGOFF_TIME 5
#define OPT_DEFAULT_AUTO_LOCK_FAILED_ATTEMPTS 3

int EventOptionsInitalize(WPARAM addInfo, LPARAM lParam);
HANDLE GetRelayToContact();

#endif
