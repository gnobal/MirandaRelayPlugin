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

#pragma warning(disable: 4786) 

#include "Common.h"
#include "m_relay.h"
#include "Options.h"
#include "ContactDB.h"
#include "CommandDB.h"
#include "ProtocolDB.h"
#include "CommandHandlers.h"

#include <string>
#include <cstring>
#include <sstream>
#include <algorithm>

#include "protocols/protocols/m_protocols.h"
#include "protocols/protocols/m_protosvc.h"

using std::string;

HINSTANCE hInst;
PLUGINLINK *pluginLink;

// Hooked events
HANDLE hEventDbEventAdded;
HANDLE hEventContactSettingChanged;
HANDLE hEventOptions;

// Supplied services
HANDLE hServiceRelayMessage;
HANDLE hServiceRegisterCommandHandler;
HANDLE hServiceUnregisterCommandHandler;
HANDLE hServiceGetContactHandleFromId;
//HANDLE hServiceGetRelayContactHandle;

// Command handlers
HANDLE hWhoCommandHandler;
HANDLE hSendCommandHandler;
HANDLE hHelpCommandHandler;
HANDLE hChstatusCommandHandler;
HANDLE hMimicCommandHandler;
HANDLE hLogonCommandHandler;
HANDLE hLogoffCommandHandler;
HANDLE hTimesCommandHandler;
HANDLE hAlertsCommandHandler;
/*
HANDLE hPauseCommandHandler;
HANDLE hResumeCommandHandler;
*/
RelayPluginRuntimeData pluginRuntimeData;

PLUGININFO pluginInfo={
	sizeof(PLUGININFO),
	"Relay",
	PLUGIN_MAKE_VERSION(0,1,9,1),
	"Relays messages to and from a contact by using a mediating computer",
	"Amit Schreiber",
	"gnobal@sourceforge.net",
	"© 2004 Amit Schreiber",
	"http://miranda-icq.sourceforge.net/",
	0,		//not transient
	0		//doesn't replace anything built-in
};



BOOL WINAPI DllMain(HINSTANCE hinstDLL,DWORD fdwReason,LPVOID lpvReserved)
{
	hInst = hinstDLL;
	return TRUE;
}

extern "C" __declspec(dllexport) PLUGININFO* MirandaPluginInfo(DWORD mirandaVersion)
{
	return &pluginInfo;
}

BOOL IsContactProtocolIgnored(HANDLE hContact)
{
	const char* szProto = reinterpret_cast<char*>(CallService(MS_PROTO_GETCONTACTBASEPROTO,(WPARAM)hContact,0));
	if(szProto == NULL) 
		return FALSE;

	if (ProtocolDB::Instance().GetProtocolState(szProto) == ProtocolDB::PROTOCOL_STATE_IGNORED)
		return TRUE;

	return FALSE;
}

void HandleIncomingMessage(HANDLE hSender, const string& message)
{
	const int nContactId = ContactDB::Instance().GetContactId(hSender);
	if (nContactId == -1)
	{
		const string strInternalError = "Could not send message because of a relay error. Sorry.";
		CallContactService(hSender, PSS_MESSAGE, 0, reinterpret_cast<LPARAM>(strInternalError.c_str()));
		return;
	}

	if (IsContactProtocolIgnored(hSender))
		return;

	std::stringstream strs;
	strs << nContactId;
	string strSenderId;
	strs >> strSenderId;
	const string strSenderName = 
		reinterpret_cast<const char*>(CallService(MS_CLIST_GETCONTACTDISPLAYNAME, reinterpret_cast<WPARAM>(hSender), 0));
	
	const string msgToSend = strSenderName + " (" + strSenderId + "): " + message;
	
	RELAYMESSAGEINFO rmi = {0};
	rmi.cbSize = sizeof(RELAYMESSAGEINFO);
	rmi.szMessage = msgToSend.c_str();

	CallService(MS_RELAY_RELAY_MESSAGE, 0, reinterpret_cast<LPARAM>(&rmi));
}

void HandleCommandFromRelay(string& strCommandLine)
{
	const string::size_type posCommand = strCommandLine.find_first_not_of(szWhiteSpaces);
	if (posCommand == string::npos)
	{
		return;
	}

	const string::size_type posArgs = strCommandLine.find_first_of(szWhiteSpaces, posCommand+1);
	string strCommand = strCommandLine.substr(posCommand, posArgs);

	CommandDB::COMMAND_STATE eCommandState = CommandDB::COMMAND_STATE_DISALLOWED;
	HANDLE hHookableEvent = NULL;
	if (CommandDB::Instance().GetCommandInfo(strCommand, eCommandState, hHookableEvent) == -1)
		return;

	const time_t tLastActivity = pluginRuntimeData.tLastActivity; // Used late in password protected commands
	time(&pluginRuntimeData.tLastActivity);
	
	if (eCommandState == CommandDB::COMMAND_STATE_DISALLOWED)
	{
		const string strMsgToSend = string("Command disallowed: ") + strCommand;
		
		RELAYMESSAGEINFO rmi = {0};
		rmi.cbSize = sizeof(RELAYMESSAGEINFO);
		rmi.szMessage = strMsgToSend.c_str();

		CallService(MS_RELAY_RELAY_MESSAGE, 0, reinterpret_cast<LPARAM>(&rmi));
		return;
	}
	else if (eCommandState == CommandDB::COMMAND_STATE_PWD_PROTECTED)
	{
		if (pluginRuntimeData.nNumFailedLogonAttempts ==
			DBGetContactSettingDword(NULL, MODULE_NAME, OPT_AUTO_LOCK_FAILED_ATTEMPTS, OPT_DEFAULT_AUTO_LOCK_FAILED_ATTEMPTS))
		{
			RELAYMESSAGEINFO rmi = {0};
			rmi.cbSize = sizeof(RELAYMESSAGEINFO);
			rmi.szMessage =  "Error: you are locked out and cannot execute any password protected commands; restart Miranda to reset the lock";
			
			CallService(MS_RELAY_RELAY_MESSAGE, 0, reinterpret_cast<LPARAM>(&rmi));
			return;
		}

		time_t tNow;
		time(&tNow);

		const int nNumSecondsPassed = (int) difftime(tNow, tLastActivity);
		if (pluginRuntimeData.bUserLoggedOn == FALSE || 
			nNumSecondsPassed > DBGetContactSettingDword(NULL, MODULE_NAME, OPT_AUTO_LOGOFF_TIME, OPT_DEFAULT_AUTO_LOGOFF_TIME) * 60)
		{
			pluginRuntimeData.bUserLoggedOn = FALSE;

			string strMsgToSend = strCommand + ": command is password protected; you must be logged on to use it";

			RELAYMESSAGEINFO rmi = {0};
			rmi.cbSize = sizeof(RELAYMESSAGEINFO);
			rmi.szMessage =  strMsgToSend.c_str();
			
			CallService(MS_RELAY_RELAY_MESSAGE, 0, reinterpret_cast<LPARAM>(&rmi));
			return;
		}
	}

	RELAYCOMMANDEVENTINFO rcei = {0};
	rcei.cbSize = sizeof(RELAYCOMMANDEVENTINFO);
	rcei.szCommand = strCommand.c_str();
	rcei.szCommandLine = strCommandLine.c_str();
	rcei.nCommandStartPos = posCommand;
	rcei.nCommandEndPos = posCommand + strCommand.size()-1;
	
	const int nNotifyRet = NotifyEventHooks(hHookableEvent, RELAYCOMMANDEVENT_HANDLEEVENT, 
		reinterpret_cast<LPARAM>(&rcei));

	return;
}

int EventDbEventAdded(WPARAM wParam,LPARAM lParam)
{
	if (!IsPluginEnabled())
		return 0;

	if (pluginRuntimeData.hRelayToContact == NULL)
	{
		pluginRuntimeData.hRelayToContact = GetRelayToContact();
	}

	const HANDLE hContact = reinterpret_cast<HANDLE>(wParam);
	const HANDLE hEvent = reinterpret_cast<HANDLE>(lParam);

	DBEVENTINFO dbei = {0};
	dbei.cbSize = sizeof(dbei);
	dbei.cbBlob = CallService(MS_DB_EVENT_GETBLOBSIZE, reinterpret_cast<LPARAM>(hEvent), 0);
	if (dbei.cbBlob == -1)
		return 0;

	dbei.pBlob = new BYTE[dbei.cbBlob];
	if (CallService(MS_DB_EVENT_GET, lParam, reinterpret_cast<LPARAM>(&dbei)) != 0)
	{
		delete[] dbei.pBlob;
		return 0;
	}
	string blob = (char*) dbei.pBlob;
	delete[] dbei.pBlob;
	
	switch (dbei.eventType)
	{
	case EVENTTYPE_MESSAGE:
		{
			// FIX: Incorrect check of flag. Should check for !DBEF_READ
			// if (!(dbei.flags & DBEF_READ)) // Message was sent, no need to handle
			// if (dbei.flags == DBEF_SENT) // Message was sent, no need to handle
			if (dbei.flags & DBEF_SENT) // Message was sent, no need to handle
				break;

			if (hContact == pluginRuntimeData.hRelayToContact)
			{
				HandleCommandFromRelay(blob);
			}
			else
			{
				HandleIncomingMessage(hContact, blob);
			}
		}
		break;
		
		
	default:
		break;
	}

	return 0;
}

void RelayContactPresence(HANDLE hContact, const int nStatus)
{
	const int nContactId = ContactDB::Instance().GetContactId(hContact);

	if (IsContactProtocolIgnored(hContact))
		return;
	
	std::stringstream strs;
	strs << nContactId;
	string strContactId;
	strs >> strContactId;

	const string strContactName = 
		reinterpret_cast<const char*>(CallService(MS_CLIST_GETCONTACTDISPLAYNAME, reinterpret_cast<WPARAM>(hContact), 0));
	
	const char* szStatus = 
		reinterpret_cast<const char*>(CallService(MS_CLIST_GETSTATUSMODEDESCRIPTION, (WPARAM) nStatus, 0));

	const string msgToSend = strContactName + " (" + strContactId + ") " + "is now " + szStatus;
		// (bOnline ? "online" : "offline");

	RELAYMESSAGEINFO rmi = {0};
	rmi.cbSize = sizeof(RELAYMESSAGEINFO);
	rmi.szMessage = msgToSend.c_str();

	CallService(MS_RELAY_RELAY_MESSAGE, 0, reinterpret_cast<LPARAM>(&rmi));
}

int EventContactSettingChanged(WPARAM wParam, LPARAM lParam)
{
	const HANDLE hContact = reinterpret_cast<HANDLE>(wParam);
	const DBCONTACTWRITESETTING* cws = (DBCONTACTWRITESETTING*) lParam;

	if (hContact == NULL || strcmp(cws->szSetting,"Status") != 0)
		return 0;

	if (!IsPluginEnabled())
		return 0;
/*
	if (pluginRuntimeData.bPaused)
		return 0;
*/	
	if (pluginRuntimeData.hRelayToContact == NULL)
	{
		pluginRuntimeData.hRelayToContact = GetRelayToContact();
	}

	if (hContact == pluginRuntimeData.hRelayToContact &&
		((BOOL)DBGetContactSettingByte(NULL, MODULE_NAME, OPT_MIMIC, FALSE)))
	{
		ChangeStatus(GetContactStatus(pluginRuntimeData.hRelayToContact));;
		return 0;
	}
	
	const int nStatus = GetContactStatus(hContact);
	if (nStatus == ID_STATUS_OFFLINE)
	{
		if ((BOOL)DBGetContactSettingByte(NULL, MODULE_NAME, OPT_ALERT_OFFLINE, FALSE))
		{
			RelayContactPresence(hContact, nStatus);
		}
		return 0;
	}

	// May be the first time we see this contact, so we must add to the contact db
	ContactDB::Instance().AddContact(hContact);

	const int nOldStatus = DBGetContactSettingWord((HANDLE)wParam,"UserOnline","OldStatus",ID_STATUS_OFFLINE);
	if (nOldStatus == ID_STATUS_OFFLINE &&
		(BOOL)DBGetContactSettingByte(NULL, MODULE_NAME, OPT_ALERT_CHANGE_FROM_OFFLINE, FALSE))
	{
		RelayContactPresence(hContact, nStatus);
		return 0;
	}

	if (nStatus == ID_STATUS_ONLINE &&
		(BOOL)DBGetContactSettingByte(NULL, MODULE_NAME, OPT_ALERT_ONLINE, FALSE))
	{
		RelayContactPresence(hContact, nStatus);
		return 0;
	}

	return 0;
}

static int RelayMessage(WPARAM, LPARAM lParam)
{
	if (!IsPluginEnabled())
		return 0;
/*
	if (pluginRuntimeData.bPaused)
		return 0;
*/
	if (pluginRuntimeData.hRelayToContact == NULL)
	{
		pluginRuntimeData.hRelayToContact = GetRelayToContact();
	}

	const RELAYMESSAGEINFO* rmi = reinterpret_cast<RELAYMESSAGEINFO*>(lParam);
	if (rmi->cbSize != sizeof(RELAYMESSAGEINFO))
		return 1;

	if (!(BOOL)DBGetContactSettingByte(NULL, MODULE_NAME, OPT_TIMESTAMPS, FALSE))
	{
		CallContactService(pluginRuntimeData.hRelayToContact, PSS_MESSAGE, 0, reinterpret_cast<LPARAM>(rmi->szMessage));
		return 0;
	}

	string strMessage = rmi->szMessage;
	char szTimeString[200];
	const size_t nStringSize = sizeof(szTimeString) / sizeof(szTimeString[0]);
	time_t tTime;
	time(&tTime);
	struct tm* now = localtime(&tTime);

	strftime(szTimeString, nStringSize,"%H:%M:%S", now);
	string strTimePrefix = string("(") + szTimeString + string(") ");
	strMessage = strTimePrefix + strMessage;

	CallContactService(pluginRuntimeData.hRelayToContact, PSS_MESSAGE, 0, reinterpret_cast<LPARAM>(strMessage.c_str()));

	return 0;
}

static int RegisterCommandHandler(WPARAM, LPARAM lParam)
{
	CommandDB& db = CommandDB::Instance();
	const RELAYREGISTERINFO* rri = reinterpret_cast<RELAYREGISTERINFO*>(lParam);
	if (rri->cbSize != sizeof(RELAYREGISTERINFO) ||
		strpbrk(rri->szCommand, " \t\n\r") != NULL)
		return static_cast<int>(NULL);

	const string strCommandName = rri->szCommand;
	const string strEventName = string(MS_RELAY_INTERNAL_COMMAND_EVENT) + strCommandName;
	
	HANDLE hHookableEventHandle = NULL;
	if (!db.CommandExists(strCommandName) &&
		(hHookableEventHandle = CreateHookableEvent(strEventName.c_str())) == NULL )
	{
		return NULL;
	}

	const HANDLE hHookHandle = HookEvent(strEventName.c_str(), rri->handlerProc);
	if (hHookHandle == NULL)
		return NULL;

	DBVARIANT dbvAllow = {0};
	const int nRetAllow = DBGetContactSetting(NULL, MODULE_NAME, OPT_ALLOWED_COMMANDS, &dbvAllow);
	DBVARIANT dbvDisallow = {0};
	const int nRetDisallow = DBGetContactSetting(NULL, MODULE_NAME, OPT_DISALLOWED_COMMANDS, &dbvDisallow);
	DBVARIANT dbvPwdProtect = {0};
	const int nRetPwdProtect = DBGetContactSetting(NULL, MODULE_NAME, OPT_PWD_PROTECTED_COMMANDS, &dbvPwdProtect);

	CommandDB::COMMAND_STATE eCommandState;
	switch (rri->nInitialState)
	{
	case COMMAND_INITIAL_STATE_ALLOWED:
		eCommandState = CommandDB::COMMAND_STATE_ALLOWED;
		break;

	case COMMAND_INITIAL_STATE_PWD_PROTECTED:
		eCommandState = CommandDB::COMMAND_STATE_PWD_PROTECTED;
		break;

	case COMMAND_INITIAL_STATE_DISALLOWED:
	default:
		eCommandState = CommandDB::COMMAND_STATE_DISALLOWED;
		break;
	}

	if (nRetAllow == 0 || nRetDisallow == 0 || nRetPwdProtect == 0)
	{
		const string strAllowedCommands = dbvAllow.pszVal == NULL ? "" : dbvAllow.pszVal;
		const string strDisallowedCommands = dbvDisallow.pszVal == NULL ? "" : dbvDisallow.pszVal;
		const string strPwdProtectedCommands = dbvPwdProtect.pszVal == NULL ? "" : dbvPwdProtect.pszVal;

		if (strAllowedCommands.find(strCommandName + " ") != string::npos)
		{
			// Override default: user allows
			eCommandState = CommandDB::COMMAND_STATE_ALLOWED;
		}
		else if (strDisallowedCommands.find(strCommandName + " ") != string::npos) 
		{
			// Override default: user disallows
			eCommandState = CommandDB::COMMAND_STATE_DISALLOWED;
		}
		else if (strPwdProtectedCommands.find(strCommandName + " ") != string::npos)
		{
			// Override default: user password protects
			eCommandState = CommandDB::COMMAND_STATE_PWD_PROTECTED;
		}
	}
	
	DBFreeVariant(&dbvAllow);
	DBFreeVariant(&dbvDisallow);
	DBFreeVariant(&dbvPwdProtect);
	
	const HANDLE hDbHandle = db.AddCommandHandler(strCommandName, hHookableEventHandle, hHookHandle, eCommandState);

	return reinterpret_cast<int>(hDbHandle);
}

static int UnregisterCommandHandler(WPARAM, LPARAM lParam)
{
	const HANDLE hDbHandle = reinterpret_cast<HANDLE>(lParam);
	const HANDLE hHookHandle = CommandDB::Instance().RemoveCommandHandler(hDbHandle);
	
	return UnhookEvent(hHookHandle);
}

static int GetContactHandleFromId(WPARAM, LPARAM lParam)
{
	const int nContactId = static_cast<int>(lParam);
	return reinterpret_cast<int>(ContactDB::Instance().GetContactHandle(nContactId));
}
/*
static int GetRelayContactHandle(WPARAM, LPARAM)
{
	return reinterpret_cast<int>(pluginOptions.hRelayToContact);
}
*/
HANDLE RegisterHandler(const char* szCommand, int nInitialState, MIRANDAHOOK pfnHandler)
{
	RELAYREGISTERINFO rri = {0};
	rri.cbSize = sizeof(RELAYREGISTERINFO);
	rri.szCommand = szCommand;
	rri.nInitialState = nInitialState;
	rri.handlerProc = pfnHandler;
	
	return (HANDLE) CallService(MS_RELAY_REGISTER_COMMAND_HANDLER, 0, reinterpret_cast<LPARAM>(&rri));
}

extern "C" int __declspec(dllexport) Load(PLUGINLINK *link)
{
	pluginLink = link;

	hEventDbEventAdded  = HookEvent(ME_DB_EVENT_ADDED, EventDbEventAdded);
	hEventContactSettingChanged = HookEvent(ME_DB_CONTACT_SETTINGCHANGED, EventContactSettingChanged);
	hEventOptions = HookEvent(ME_OPT_INITIALISE, EventOptionsInitalize);

	hServiceRelayMessage = CreateServiceFunction(MS_RELAY_RELAY_MESSAGE, RelayMessage);
	hServiceRegisterCommandHandler = 
		CreateServiceFunction(MS_RELAY_REGISTER_COMMAND_HANDLER, RegisterCommandHandler);
	hServiceUnregisterCommandHandler = 
		CreateServiceFunction(MS_RELAY_UNREGISTER_COMMAND_HANDLER, UnregisterCommandHandler);
	hServiceGetContactHandleFromId = 
		CreateServiceFunction(MS_RELAY_GET_CONTACT_HANDLE_FROM_ID, GetContactHandleFromId);
	// hServiceGetRelayContactHandle =
	//	CreateServiceFunction(MS_RELAY_GET_RELAY_CONTACT_HANDLE, GetRelayContactHandle);
	
	hWhoCommandHandler = RegisterHandler("who", COMMAND_INITIAL_STATE_ALLOWED, CommandWhoHandler);
	hSendCommandHandler = RegisterHandler("send", COMMAND_INITIAL_STATE_ALLOWED, CommandSendHandler);
	hHelpCommandHandler = RegisterHandler("help", COMMAND_INITIAL_STATE_ALLOWED, CommandHelpHandler);
	hChstatusCommandHandler = RegisterHandler("chstatus", COMMAND_INITIAL_STATE_ALLOWED, CommandChstatusHandler);
	hMimicCommandHandler = RegisterHandler("mimic", COMMAND_INITIAL_STATE_ALLOWED, CommandMimicHandler);
	hLogonCommandHandler = RegisterHandler("logon", COMMAND_INITIAL_STATE_ALLOWED, CommandLogonHandler);
	hLogoffCommandHandler = RegisterHandler("logoff", COMMAND_INITIAL_STATE_ALLOWED, CommandLogoffHandler);
	hTimesCommandHandler = RegisterHandler("times", COMMAND_INITIAL_STATE_ALLOWED, CommandTimesHandler);
	hAlertsCommandHandler = RegisterHandler("alerts", COMMAND_INITIAL_STATE_ALLOWED, CommandAlertsHandler);
/*
	hPauseCommandHandler = RegisterHandler("pause", COMMAND_INITIAL_STATE_ALLOWED, CommandPauseHandler);;
	hResumeCommandHandler = RegisterHandler("resume", COMMAND_INITIAL_STATE_ALLOWED, CommandResumeHandler);;
*/

	return 0;
}

extern "C" int __declspec(dllexport) Unload(void)
{
	CallService(MS_RELAY_UNREGISTER_COMMAND_HANDLER, 0, reinterpret_cast<LPARAM>(hWhoCommandHandler));
	CallService(MS_RELAY_UNREGISTER_COMMAND_HANDLER, 0, reinterpret_cast<LPARAM>(hSendCommandHandler));
	CallService(MS_RELAY_UNREGISTER_COMMAND_HANDLER, 0, reinterpret_cast<LPARAM>(hHelpCommandHandler));
	CallService(MS_RELAY_UNREGISTER_COMMAND_HANDLER, 0, reinterpret_cast<LPARAM>(hChstatusCommandHandler));
	CallService(MS_RELAY_UNREGISTER_COMMAND_HANDLER, 0, reinterpret_cast<LPARAM>(hMimicCommandHandler));
	CallService(MS_RELAY_UNREGISTER_COMMAND_HANDLER, 0, reinterpret_cast<LPARAM>(hLogonCommandHandler));
	CallService(MS_RELAY_UNREGISTER_COMMAND_HANDLER, 0, reinterpret_cast<LPARAM>(hLogoffCommandHandler));
	CallService(MS_RELAY_UNREGISTER_COMMAND_HANDLER, 0, reinterpret_cast<LPARAM>(hTimesCommandHandler));
	CallService(MS_RELAY_UNREGISTER_COMMAND_HANDLER, 0, reinterpret_cast<LPARAM>(hAlertsCommandHandler));
/*
	CallService(MS_RELAY_UNREGISTER_COMMAND_HANDLER, 0, reinterpret_cast<LPARAM>(hPauseCommandHandler)); 
	CallService(MS_RELAY_UNREGISTER_COMMAND_HANDLER, 0, reinterpret_cast<LPARAM>(hResumeCommandHandler));
*/
	UnhookEvent(hEventDbEventAdded);
	UnhookEvent(hEventContactSettingChanged);
	UnhookEvent(hEventOptions);

	DestroyServiceFunction(hServiceRelayMessage);
	DestroyServiceFunction(hServiceRegisterCommandHandler);
	DestroyServiceFunction(hServiceUnregisterCommandHandler);
	DestroyServiceFunction(hServiceGetContactHandleFromId);
	// DestroyServiceFunction(hServiceGetRelayContactHandle);

	return 0;
}

