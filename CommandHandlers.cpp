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

#include "Common.h"
#include "m_relay.h"
#include "ContactDB.h"
#include "CommandDB.h"
#include "Options.h"

#include "protocols/protocols/m_protocols.h"
#include "protocols/protocols/m_protosvc.h"

#include <string>
#include <sstream>

using std::string;

string GetOnlineListAsString()
{
	ContactDB& db = ContactDB::Instance();
	string list = "\r\n";

	for (ContactDB::SizeType nContactId = 0; nContactId < db.Size(); ++nContactId)
	{
		const HANDLE hContact = db[nContactId];
		const WPARAM wStatus = GetContactStatus(hContact);
		if (wStatus == ID_STATUS_OFFLINE)
			continue;

		const char* szStatus = 
			reinterpret_cast<const char*>(CallService(MS_CLIST_GETSTATUSMODEDESCRIPTION, wStatus, 0));
		
		const char* szName = 
			reinterpret_cast<const char*>(CallService(MS_CLIST_GETCONTACTDISPLAYNAME, reinterpret_cast<WPARAM>(hContact), 0));
		
		std::stringstream strs;
		strs << nContactId;
		string strContactId;
		strs >> strContactId;
		list += strContactId + "  " + szName + "  " + szStatus + "\r\n";
	}

	return list;
}


int CommandWhoHandler(WPARAM wParam, LPARAM )
{
	if (wParam == RELAYCOMMANDEVENT_SENDHELP)
	{
		const char* szHelp = "\r\nwho"
			"\r\nSends you the list of online contacts, including their ID (to be used in other commands) and their status";
		RELAYMESSAGEINFO rmi = {0};
		rmi.cbSize = sizeof(RELAYMESSAGEINFO);
		rmi.szMessage =  szHelp;

		CallService(MS_RELAY_RELAY_MESSAGE, 0, reinterpret_cast<LPARAM>(&rmi));
	}
	else if (wParam == RELAYCOMMANDEVENT_HANDLEEVENT)
	{
		const string strOnlineList = GetOnlineListAsString();
		
		RELAYMESSAGEINFO rmi = {0};
		rmi.cbSize = sizeof(RELAYMESSAGEINFO);
		rmi.szMessage =  strOnlineList.c_str();

		CallService(MS_RELAY_RELAY_MESSAGE, 0, reinterpret_cast<LPARAM>(&rmi));
	}
	
	return 0;
}

void AddContactHistoryEvent(HANDLE hContact, const string& strMessage, long lEventFlags)
{
	DBEVENTINFO dbei={0};

	dbei.cbSize = sizeof(dbei);
	dbei.eventType = EVENTTYPE_MESSAGE;
	dbei.flags = lEventFlags;
	dbei.szModule = (char*) CallService(MS_PROTO_GETCONTACTBASEPROTO,(WPARAM)hContact,0);
	dbei.timestamp = time(NULL);
	dbei.cbBlob = strMessage.length() + 1;
	dbei.pBlob = (PBYTE) strMessage.c_str();
	CallService(MS_DB_EVENT_ADD,(WPARAM)hContact,(LPARAM)&dbei);
}

int CommandSendHandler(WPARAM wParam, LPARAM lParam)
{
	if (wParam == RELAYCOMMANDEVENT_SENDHELP)
	{
		static const char* szHelp = "\r\nsend contactID message" 
			"\r\nSends a message to a contact"
			"\r\ncontactId is a valid ID either retrieved by the who command or from an incoming relayed message.";
		RELAYMESSAGEINFO rmi = {0};
		rmi.cbSize = sizeof(RELAYMESSAGEINFO);
		rmi.szMessage =  szHelp;
		
		CallService(MS_RELAY_RELAY_MESSAGE, 0, reinterpret_cast<LPARAM>(&rmi));
	}
	else if (wParam == RELAYCOMMANDEVENT_HANDLEEVENT)
	{
		RELAYCOMMANDEVENTINFO* rcei = reinterpret_cast<RELAYCOMMANDEVENTINFO*>(lParam);

		const string strCommandLine = rcei->szCommandLine;
		const string::size_type posArgContactId = strCommandLine.find_first_not_of(szWhiteSpaces, rcei->nCommandEndPos+1);
		if (posArgContactId == string::npos)
		{
			RELAYMESSAGEINFO rmi = {0};
			rmi.cbSize = sizeof(RELAYMESSAGEINFO);
			rmi.szMessage =  "send: not enough arguments (no contact ID)";
			
			CallService(MS_RELAY_RELAY_MESSAGE, 0, reinterpret_cast<LPARAM>(&rmi));
			return 0;
		}

		const string::size_type posArgMessage = strCommandLine.find_first_of(szWhiteSpaces, posArgContactId);
		if (posArgMessage == string::npos || posArgMessage+1 == strCommandLine.size() )
		{
			RELAYMESSAGEINFO rmi = {0};
			rmi.cbSize = sizeof(RELAYMESSAGEINFO);
			rmi.szMessage =  "send: not enough arguments (no message to send)";
			
			CallService(MS_RELAY_RELAY_MESSAGE, 0, reinterpret_cast<LPARAM>(&rmi));
			return 0;
		}

		const string strContactId = strCommandLine.substr(posArgContactId, posArgMessage-posArgContactId);
		const string strMessage = strCommandLine.substr(posArgMessage+1);
		
		std::stringstream strs;
		strs << strContactId;
		int nContactId;
		strs >> nContactId;
		
		HANDLE hSendTo = ContactDB::Instance().GetContactHandle(nContactId);
		if (hSendTo == NULL || !strs)
		{
			RELAYMESSAGEINFO rmi = {0};
			rmi.cbSize = sizeof(RELAYMESSAGEINFO);
			rmi.szMessage =  "send: invalid contact ID";
			
			CallService(MS_RELAY_RELAY_MESSAGE, 0, reinterpret_cast<LPARAM>(&rmi));
			return 0;
		}

		if ((BOOL) DBGetContactSettingByte(NULL, MODULE_NAME, OPT_CREATE_HISTORY_EVENTS, TRUE))
		{
			AddContactHistoryEvent(hSendTo, strMessage, DBEF_SENT);
		}


		CallContactService(hSendTo, PSS_MESSAGE, 0, reinterpret_cast<LPARAM>(strMessage.c_str()));
	}

	return 0;
}

int CommandHelpHandler(WPARAM wParam,LPARAM lParam)
{
	if (wParam == RELAYCOMMANDEVENT_SENDHELP)
	{
		static const char* szHelp = "\r\nhelp [command]" 
			"\r\nIf [command] is specidfied sends help about the command"
			"\r\nOtherwise shows the list of available commands";
		RELAYMESSAGEINFO rmi = {0};
		rmi.cbSize = sizeof(RELAYMESSAGEINFO);
		rmi.szMessage =  szHelp;
		
		CallService(MS_RELAY_RELAY_MESSAGE, 0, reinterpret_cast<LPARAM>(&rmi));
	}
	else if (wParam == RELAYCOMMANDEVENT_HANDLEEVENT)
	{
		RELAYCOMMANDEVENTINFO* rcei = reinterpret_cast<RELAYCOMMANDEVENTINFO*>(lParam);

		const string strCommandLine = string(rcei->szCommandLine) + " "; // Add a whitespace intentionally
		const string::size_type posArgCommand = strCommandLine.find_first_not_of(szWhiteSpaces, rcei->nCommandEndPos+1);
		if (posArgCommand == string::npos)
		{
			string strCommands = "\r\nAvailable commands:";
			CommandDB& db = CommandDB::Instance();
			for (CommandDB::Iterator it = db.Begin(); it != db.End(); ++it)
			{
				CommandDB::COMMAND_STATE eState = CommandDB::COMMAND_STATE_DISALLOWED;
				HANDLE hHookableEvent = NULL;
				string strCommandName;
				db.GetCommandInfo(it, strCommandName, eState, hHookableEvent);
				strCommands += string("\r\n") + strCommandName;
			}
			strCommands += "\r\nEnter \"help help\" for more information about this command";

			RELAYMESSAGEINFO rmi = {0};
			rmi.cbSize = sizeof(RELAYMESSAGEINFO);
			rmi.szMessage = strCommands.c_str();
			CallService(MS_RELAY_RELAY_MESSAGE, 0, reinterpret_cast<LPARAM>(&rmi));

			return 0;
		}

		const string::size_type posArgCommandEnd = strCommandLine.find_first_of(szWhiteSpaces, posArgCommand);
		const string strCommand = strCommandLine.substr(posArgCommand, posArgCommandEnd-posArgCommand);

		CommandDB::COMMAND_STATE eState = CommandDB::COMMAND_STATE_DISALLOWED;
		HANDLE hHookableEvent = NULL;
		if (CommandDB::Instance().GetCommandInfo(strCommand, eState, hHookableEvent) == -1)
		{
			const string strMsgToSend = string("help: unknown command: ") + strCommand;

			RELAYMESSAGEINFO rmi = {0};
			rmi.cbSize = sizeof(RELAYMESSAGEINFO);
			rmi.szMessage = strMsgToSend.c_str();
			CallService(MS_RELAY_RELAY_MESSAGE, 0, reinterpret_cast<LPARAM>(&rmi));
			return 0;
		}

		RELAYCOMMANDEVENTINFO rceiHelpRequest = {0};
		rceiHelpRequest.cbSize = sizeof(RELAYCOMMANDEVENTINFO);
		rceiHelpRequest.szCommand = strCommand.c_str();
		const int nNotifyRet = NotifyEventHooks(hHookableEvent, 
			RELAYCOMMANDEVENT_SENDHELP, reinterpret_cast<LPARAM>(&rceiHelpRequest));
	}

	return 0;
}

int CommandChstatusHandler(WPARAM wParam,LPARAM lParam)
{
	if (wParam == RELAYCOMMANDEVENT_SENDHELP)
	{
		static const char* szHelp = "\r\nchstatus <NewStatus>" 
			"\r\nSets your remote status. NewStatus can be one of the following:"
			"\r\nonline, offline, away, dnd, na, busy, freechat, invisible, onthephone, outtolunch";

		RELAYMESSAGEINFO rmi = {0};
		rmi.cbSize = sizeof(RELAYMESSAGEINFO);
		rmi.szMessage =  szHelp;
		
		CallService(MS_RELAY_RELAY_MESSAGE, 0, reinterpret_cast<LPARAM>(&rmi));
	}
	else if (wParam == RELAYCOMMANDEVENT_HANDLEEVENT)
	{
		RELAYCOMMANDEVENTINFO* rcei = reinterpret_cast<RELAYCOMMANDEVENTINFO*>(lParam);
		
		const string strCommandLine = string(rcei->szCommandLine) + " "; // Add a whitespace intentionally
		const string::size_type posArgStatus = strCommandLine.find_first_not_of(szWhiteSpaces, rcei->nCommandEndPos+1);
		
		if (posArgStatus == string::npos)
		{
			RELAYMESSAGEINFO rmi = {0};
			rmi.cbSize = sizeof(RELAYMESSAGEINFO);
			rmi.szMessage =  "chstatus: not enough arguments (no new status)";
			
			CallService(MS_RELAY_RELAY_MESSAGE, 0, reinterpret_cast<LPARAM>(&rmi));
			return 0;
		}

		const string::size_type posArgStatusEnd = strCommandLine.find_first_of(szWhiteSpaces, posArgStatus);
		const string strStatus = strCommandLine.substr(posArgStatus, posArgStatusEnd-posArgStatus);

		int nStatus;
		if (strStatus == "offline")
		{
			nStatus = ID_STATUS_OFFLINE;
		}
		else if (strStatus == "online")
		{
			nStatus = ID_STATUS_ONLINE;
		}
		else if (strStatus == "away")
		{
			nStatus = ID_STATUS_AWAY;
		}
		else if (strStatus == "dnd")
		{
			nStatus = ID_STATUS_DND;
		}
		else if (strStatus == "na")
		{
			nStatus = ID_STATUS_NA;
		}
		else if (strStatus == "busy")
		{
			nStatus = ID_STATUS_OCCUPIED;
		}
		else if (strStatus == "freechat")
		{
			nStatus = ID_STATUS_FREECHAT;
		}
		else if (strStatus == "invisible")
		{
			nStatus = ID_STATUS_INVISIBLE;
		}
		else if (strStatus == "onthephone")
		{
			nStatus = ID_STATUS_ONTHEPHONE;
		}
		else if (strStatus == "outtolunch")
		{
			nStatus = ID_STATUS_OUTTOLUNCH;
		}
		else
		{
			RELAYMESSAGEINFO rmi = {0};
			rmi.cbSize = sizeof(RELAYMESSAGEINFO);
			rmi.szMessage =  "chstatus: unknown status";
			
			CallService(MS_RELAY_RELAY_MESSAGE, 0, reinterpret_cast<LPARAM>(&rmi));
			return 0;
		}

		DBWriteContactSettingByte(NULL, MODULE_NAME, OPT_MIMIC, (BYTE) FALSE);
		ChangeStatus(nStatus);
	}

	return 0;
}

int CommandMimicHandler(WPARAM wParam,LPARAM lParam)
{
	if (wParam == RELAYCOMMANDEVENT_SENDHELP)
	{
		static const char* szHelp = "\r\nmimic on|off" 
			"\r\nSets the \"mimic the relay-to status\" option on or off"
			"\r\nThis is useful when using the chstatus command which automatically sets mimic to off";

		RELAYMESSAGEINFO rmi = {0};
		rmi.cbSize = sizeof(RELAYMESSAGEINFO);
		rmi.szMessage =  szHelp;
		
		CallService(MS_RELAY_RELAY_MESSAGE, 0, reinterpret_cast<LPARAM>(&rmi));
	}
	else if (wParam == RELAYCOMMANDEVENT_HANDLEEVENT)
	{
		RELAYCOMMANDEVENTINFO* rcei = reinterpret_cast<RELAYCOMMANDEVENTINFO*>(lParam);

		const string strCommandLine = string(rcei->szCommandLine) + " "; // Add a whitespace intentionally
		const string::size_type posArgState = strCommandLine.find_first_not_of(szWhiteSpaces, rcei->nCommandEndPos+1);
		
		if (posArgState == string::npos)
		{
			RELAYMESSAGEINFO rmi = {0};
			rmi.cbSize = sizeof(RELAYMESSAGEINFO);
			rmi.szMessage =  "mimic: not enough arguments (no new state)";
			
			CallService(MS_RELAY_RELAY_MESSAGE, 0, reinterpret_cast<LPARAM>(&rmi));
			return 0;
		}

		const string::size_type posArgStateEnd = strCommandLine.find_first_of(szWhiteSpaces, posArgState);
		const string strState = strCommandLine.substr(posArgState, posArgStateEnd-posArgState);

		BYTE bNewState;
		if (strState == "on")
		{
			bNewState = (BYTE) TRUE;
		}
		else if (strState == "off")
		{
			bNewState = (BYTE) FALSE;
		}
		else
		{
			RELAYMESSAGEINFO rmi = {0};
			rmi.cbSize = sizeof(RELAYMESSAGEINFO);
			rmi.szMessage =  "mimic: unknown state";
			
			CallService(MS_RELAY_RELAY_MESSAGE, 0, reinterpret_cast<LPARAM>(&rmi));
			return 0;
		}

		DBWriteContactSettingByte(NULL, MODULE_NAME, OPT_MIMIC, bNewState);
	}

	return 0;
}

int CommandTimesHandler(WPARAM wParam,LPARAM lParam)
{
	if (wParam == RELAYCOMMANDEVENT_SENDHELP)
	{
		static const char* szHelp = "\r\ntimes on|off" 
			"\r\nSets the \"show timestamps in relayed messages\" option on or off";

		RELAYMESSAGEINFO rmi = {0};
		rmi.cbSize = sizeof(RELAYMESSAGEINFO);
		rmi.szMessage =  szHelp;
		
		CallService(MS_RELAY_RELAY_MESSAGE, 0, reinterpret_cast<LPARAM>(&rmi));
	}
	else if (wParam == RELAYCOMMANDEVENT_HANDLEEVENT)
	{
		RELAYCOMMANDEVENTINFO* rcei = reinterpret_cast<RELAYCOMMANDEVENTINFO*>(lParam);

		const string strCommandLine = string(rcei->szCommandLine) + " "; // Add a whitespace intentionally
		const string::size_type posArgState = strCommandLine.find_first_not_of(szWhiteSpaces, rcei->nCommandEndPos+1);
		
		if (posArgState == string::npos)
		{
			RELAYMESSAGEINFO rmi = {0};
			rmi.cbSize = sizeof(RELAYMESSAGEINFO);
			rmi.szMessage =  "times: not enough arguments (no new state)";
			
			CallService(MS_RELAY_RELAY_MESSAGE, 0, reinterpret_cast<LPARAM>(&rmi));
			return 0;
		}

		const string::size_type posArgStateEnd = strCommandLine.find_first_of(szWhiteSpaces, posArgState);
		const string strState = strCommandLine.substr(posArgState, posArgStateEnd-posArgState);

		BYTE bNewState;
		if (strState == "on")
		{
			bNewState = (BYTE) TRUE;
		}
		else if (strState == "off")
		{
			bNewState = (BYTE) FALSE;
		}
		else
		{
			RELAYMESSAGEINFO rmi = {0};
			rmi.cbSize = sizeof(RELAYMESSAGEINFO);
			rmi.szMessage =  "times: unknown state";
			
			CallService(MS_RELAY_RELAY_MESSAGE, 0, reinterpret_cast<LPARAM>(&rmi));
			return 0;
		}

		DBWriteContactSettingByte(NULL, MODULE_NAME, OPT_TIMESTAMPS, bNewState);
	}

	return 0;
}

int CommandLogonHandler(WPARAM wParam,LPARAM lParam)
{
	if (wParam == RELAYCOMMANDEVENT_SENDHELP)
	{
		static const char* szHelp = "\r\nlogon <password>" 
			"\r\nLogs you on to allow using password protected commands";

		RELAYMESSAGEINFO rmi = {0};
		rmi.cbSize = sizeof(RELAYMESSAGEINFO);
		rmi.szMessage =  szHelp;
		
		CallService(MS_RELAY_RELAY_MESSAGE, 0, reinterpret_cast<LPARAM>(&rmi));
	}
	else if (wParam == RELAYCOMMANDEVENT_HANDLEEVENT)
	{
		RELAYCOMMANDEVENTINFO* rcei = reinterpret_cast<RELAYCOMMANDEVENTINFO*>(lParam);

		if (pluginRuntimeData.nNumFailedLogonAttempts == 
			DBGetContactSettingDword(NULL, MODULE_NAME, OPT_AUTO_LOCK_FAILED_ATTEMPTS, OPT_DEFAULT_AUTO_LOCK_FAILED_ATTEMPTS))
		{
			RELAYMESSAGEINFO rmi = {0};
			rmi.cbSize = sizeof(RELAYMESSAGEINFO);
			rmi.szMessage =  "logon: you have been locked out because of too many failed logon attempts; restart Miranda to reset the lock";
			
			CallService(MS_RELAY_RELAY_MESSAGE, 0, reinterpret_cast<LPARAM>(&rmi));
			return 0;
		}

		const string strCommandLine = string(rcei->szCommandLine); // Add a whitespace intentionally
		const string::size_type posArgPassword = strCommandLine.find_first_of(szWhiteSpaces, rcei->nCommandEndPos);
		if (posArgPassword == string::npos || posArgPassword+1 == strCommandLine.size() )
		{
			RELAYMESSAGEINFO rmi = {0};
			rmi.cbSize = sizeof(RELAYMESSAGEINFO);
			rmi.szMessage =  "logon: not enough arguments (no password)";
			
			CallService(MS_RELAY_RELAY_MESSAGE, 0, reinterpret_cast<LPARAM>(&rmi));
			return 0;
		}

		const string strIncomingPassword = strCommandLine.substr(posArgPassword+1);
		DBVARIANT dbvPassword = {0};
		const int nRetPassword = DBGetContactSetting(NULL, MODULE_NAME, OPT_PASSWORD, &dbvPassword);
		if (nRetPassword != 0)
		{
			RELAYMESSAGEINFO rmi = {0};
			rmi.cbSize = sizeof(RELAYMESSAGEINFO);
			rmi.szMessage =  "logon: no password defined; cannot logon";
			
			CallService(MS_RELAY_RELAY_MESSAGE, 0, reinterpret_cast<LPARAM>(&rmi));
			return 0;
		}

		const string strRealPassword = dbvPassword.pszVal;
		DBFreeVariant(&dbvPassword);

		if (strIncomingPassword != strRealPassword)
		{
			pluginRuntimeData.nNumFailedLogonAttempts++;


			RELAYMESSAGEINFO rmi = {0};
			rmi.cbSize = sizeof(RELAYMESSAGEINFO);
			rmi.szMessage =  "logon: incorrect password";
			
			CallService(MS_RELAY_RELAY_MESSAGE, 0, reinterpret_cast<LPARAM>(&rmi));
			return 0;
		}

		pluginRuntimeData.nNumFailedLogonAttempts = 0;
		pluginRuntimeData.bUserLoggedOn = TRUE;
	}
	return 0;
}

int CommandLogoffHandler(WPARAM wParam,LPARAM lParam)
{
	if (wParam == RELAYCOMMANDEVENT_SENDHELP)
	{
		static const char* szHelp = "\r\nlogoff" 
			"\r\nLogs you off to disallow using password protected commands";

		RELAYMESSAGEINFO rmi = {0};
		rmi.cbSize = sizeof(RELAYMESSAGEINFO);
		rmi.szMessage =  szHelp;
		
		CallService(MS_RELAY_RELAY_MESSAGE, 0, reinterpret_cast<LPARAM>(&rmi));
	}
	else if (wParam == RELAYCOMMANDEVENT_HANDLEEVENT)
	{
		pluginRuntimeData.bUserLoggedOn = FALSE;
	}
		
	return 0;
}

int CommandAlertsHandler(WPARAM wParam, LPARAM lParam)
{
	if (wParam == RELAYCOMMANDEVENT_SENDHELP)
	{
		static const char* szHelp = "\r\nalerts online|offline|fromoffline on|off" 
			"\r\nTurns on or off online/offline/change from offline alerts."
			"\r\nExamples: alerts online off"
			"\r\n          alerts fromoffline on";

		RELAYMESSAGEINFO rmi = {0};
		rmi.cbSize = sizeof(RELAYMESSAGEINFO);
		rmi.szMessage =  szHelp;
		
		CallService(MS_RELAY_RELAY_MESSAGE, 0, reinterpret_cast<LPARAM>(&rmi));
	}
	else if (wParam == RELAYCOMMANDEVENT_HANDLEEVENT)
	{
		RELAYCOMMANDEVENTINFO* rcei = reinterpret_cast<RELAYCOMMANDEVENTINFO*>(lParam);

		const string strCommandLine = rcei->szCommandLine;
		const string::size_type posArgStatus = strCommandLine.find_first_not_of(szWhiteSpaces, rcei->nCommandEndPos+1);
		if (posArgStatus == string::npos)
		{
			RELAYMESSAGEINFO rmi = {0};
			rmi.cbSize = sizeof(RELAYMESSAGEINFO);
			rmi.szMessage =  "alerts: not enough arguments (no status)";
			
			CallService(MS_RELAY_RELAY_MESSAGE, 0, reinterpret_cast<LPARAM>(&rmi));
			return 0;
		}

		const string::size_type posArgStatusEnd = strCommandLine.find_first_of(szWhiteSpaces, posArgStatus+1);
		if (posArgStatus == string::npos)
		{
			RELAYMESSAGEINFO rmi = {0};
			rmi.cbSize = sizeof(RELAYMESSAGEINFO);
			rmi.szMessage =  "alerts: not enough arguments (no new state)";
			
			CallService(MS_RELAY_RELAY_MESSAGE, 0, reinterpret_cast<LPARAM>(&rmi));
			return 0;
		}
	
		const string strStatus = strCommandLine.substr(posArgStatus, posArgStatusEnd-posArgStatus);
		const char* szStatusOption;
		if (strStatus == "online")
		{
			szStatusOption = OPT_ALERT_ONLINE;
		}
		else if (strStatus == "offline")
		{
			szStatusOption = OPT_ALERT_OFFLINE;
		}
		else if (strStatus == "fromoffline")
		{
			szStatusOption = OPT_ALERT_CHANGE_FROM_OFFLINE;
		}
		else
		{
			RELAYMESSAGEINFO rmi = {0};
			rmi.cbSize = sizeof(RELAYMESSAGEINFO);
			rmi.szMessage =  "alerts: unknown status";
			
			CallService(MS_RELAY_RELAY_MESSAGE, 0, reinterpret_cast<LPARAM>(&rmi));
			return 0;
		}

		const string::size_type posArgNewState = strCommandLine.find_first_not_of(szWhiteSpaces, posArgStatusEnd+1);
		if (posArgNewState == string::npos || posArgNewState+1 == strCommandLine.size() )
		{
			RELAYMESSAGEINFO rmi = {0};
			rmi.cbSize = sizeof(RELAYMESSAGEINFO);
			rmi.szMessage =  "alerts: not enough arguments (no new state)";
			
			CallService(MS_RELAY_RELAY_MESSAGE, 0, reinterpret_cast<LPARAM>(&rmi));
			return 0;
		}

		const string::size_type posArgNewStateEnd = strCommandLine.find_first_of(szWhiteSpaces, posArgNewState);
		const string strState = strCommandLine.substr(posArgNewState, posArgNewStateEnd-posArgNewState);

		BYTE bNewState;
		if (strState == "on")
		{
			bNewState = (BYTE) TRUE;
		}
		else if (strState == "off")
		{
			bNewState = (BYTE) FALSE;
		}
		else
		{
			RELAYMESSAGEINFO rmi = {0};
			rmi.cbSize = sizeof(RELAYMESSAGEINFO);
			rmi.szMessage =  "alerts: unknown new state";
			
			CallService(MS_RELAY_RELAY_MESSAGE, 0, reinterpret_cast<LPARAM>(&rmi));
			return 0;
		}

		DBWriteContactSettingByte(NULL, MODULE_NAME, szStatusOption, bNewState);
  }

	return 0;
}
/*
int CommandPauseHandler(WPARAM wParam, LPARAM lParam)
{
	if (wParam == RELAYCOMMANDEVENT_SENDHELP)
	{
		const char* szHelp = "\r\npause"
			"\r\nPauses the relay from working."
			"\r\nTo resume the relay, use the resume command.";
		RELAYMESSAGEINFO rmi = {0};
		rmi.cbSize = sizeof(RELAYMESSAGEINFO);
		rmi.szMessage =  szHelp;

		CallService(MS_RELAY_RELAY_MESSAGE, 0, reinterpret_cast<LPARAM>(&rmi));
	}
	else if (wParam == RELAYCOMMANDEVENT_HANDLEEVENT)
	{
		pluginRuntimeData.bPaused = TRUE;
		
		RELAYMESSAGEINFO rmi = {0};
		rmi.cbSize = sizeof(RELAYMESSAGEINFO);
		rmi.szMessage = "Relay is paused. Use the resume command to resume the relay";

		CallService(MS_RELAY_RELAY_MESSAGE, 0, reinterpret_cast<LPARAM>(&rmi));
	}
	
	return 0;
}

int CommandResumeHandler(WPARAM wParam, LPARAM lParam)
{
	if (wParam == RELAYCOMMANDEVENT_SENDHELP)
	{
		const char* szHelp = "\r\nresume"
			"\r\nResumes the relay from a pause.";
		RELAYMESSAGEINFO rmi = {0};
		rmi.cbSize = sizeof(RELAYMESSAGEINFO);
		rmi.szMessage =  szHelp;

		CallService(MS_RELAY_RELAY_MESSAGE, 0, reinterpret_cast<LPARAM>(&rmi));
	}
	else if (wParam == RELAYCOMMANDEVENT_HANDLEEVENT)
	{
		pluginRuntimeData.bPaused = FALSE;
	}
	
	return 0;
}
*/