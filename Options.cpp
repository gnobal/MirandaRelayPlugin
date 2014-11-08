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

#include <sstream>
#include "Common.h"
#include "Options.h"
#include "CommandDB.h"
#include "ProtocolDB.h"
#include "random/contacts/m_contacts.h"
#include "core/m_system.h"
#include "protocols/protocols/m_protocols.h"
#include "protocols/protocols/m_protosvc.h"

string GetContactUniqueString(HANDLE hContact)
{
	if (hContact == NULL)
		return "";

	CONTACTINFO ci = {0};
	std::stringstream strs;

	
	ci.cbSize = sizeof(CONTACTINFO);
	ci.hContact = hContact;
	ci.dwFlag = CNF_UNIQUEID;

	if (CallService(MS_CONTACT_GETCONTACTINFO,0,reinterpret_cast<LPARAM>(&ci))) 
		return "";

	strs << ci.szProto;

	switch (ci.type) {

	case CNFT_BYTE:
		strs << ci.bVal; 
		break;
	
	case CNFT_WORD:
		strs << ci.wVal;
		break;
	
	case CNFT_DWORD:
		strs << ci.dVal;
		break;
	
	case CNFT_ASCIIZ:
		{
			struct MM_INTERFACE mm = {0};
			
			strs << ci.pszVal;
			mm.cbSize = sizeof(struct MM_INTERFACE);
			CallService(MS_SYSTEM_GET_MMI,0,(LPARAM)&mm);
			mm.mmi_free(ci.pszVal);
		}
		break;
	
	default:
		break;
	}

	return strs.str();
}

HANDLE GetRelayToContact()
{
	HANDLE hSelectedContact = NULL;

	DBVARIANT dbvSelectedContact = {0};
	const int nRetSelectedContact = DBGetContactSetting(NULL, MODULE_NAME, OPT_RELAY_TO_CONTACT, &dbvSelectedContact);
	if (nRetSelectedContact == 0)
	{
		string strSelectedContact = dbvSelectedContact.pszVal;
		HANDLE hCurrContact = reinterpret_cast<HANDLE>(CallService(MS_DB_CONTACT_FINDFIRST, 0, 0));
		while (hCurrContact != NULL)
		{
			if (GetContactUniqueString(hCurrContact) == strSelectedContact)
			{
				hSelectedContact = hCurrContact;
				break;
			}
			hCurrContact = reinterpret_cast<HANDLE>(CallService(MS_DB_CONTACT_FINDNEXT, reinterpret_cast<WPARAM>(hCurrContact), 0));
		}
	}
	DBFreeVariant(&dbvSelectedContact);

	return hSelectedContact;
}

HANDLE GetComboItemData(HWND hCombo, LONG lIndex)
{
	LRESULT lRet = SendMessage(hCombo, CB_GETITEMDATA, lIndex, 0);
	if (lRet == CB_ERR)
		return NULL;

	return reinterpret_cast<HANDLE>(lRet);
}

HANDLE GetComboCurItemData(HWND hCombo)
{
	const LRESULT lCurSel = SendMessage(hCombo, CB_GETCURSEL, 0, 0);
	if (lCurSel == CB_ERR)
		return NULL;
	
	return GetComboItemData(hCombo, lCurSel);
}

int FindComboItemDataIndex(HWND hCombo, HANDLE hContactToFind)
{
	if (hContactToFind == NULL)
		return 0;

	const LRESULT lCount = SendMessage(hCombo, CB_GETCOUNT, 0, 0);
	if (lCount == CB_ERR)
		return 0;
	
	for (LONG lCurr = 0; lCurr < lCount; ++lCurr)
	{
		if (hContactToFind == GetComboItemData(hCombo, lCurr))
			return (int) lCurr;
	}

	return 0;
}

void ListBoxToStringAndDB(HWND hLB, string& strCommands, CommandDB::COMMAND_STATE eNewState)
{
	const LRESULT lNumItems = SendMessage(hLB, LB_GETCOUNT, 0, 0);
	if (lNumItems == LB_ERR || lNumItems == 0)
		return;
	
	for (LRESULT lCurrItem = 0; lCurrItem < lNumItems; ++lCurrItem)
	{
		const LRESULT lTextLen = SendMessage(hLB, LB_GETTEXTLEN, lCurrItem, 0);
		TCHAR* szCurrItem = new TCHAR[lTextLen + 1];
		SendMessage(hLB, LB_GETTEXT, lCurrItem, reinterpret_cast<LPARAM>(szCurrItem));
		CommandDB::Instance().SetCommandState(szCurrItem, eNewState);
		strCommands += string(szCurrItem) + " ";
		delete[] szCurrItem;
	}
}

void TransferListBoxItems(HWND hFrom, HWND hTo)
{
	LRESULT lNumSelected = SendMessage(hFrom, LB_GETSELCOUNT, 0, 0);
	if (lNumSelected == LB_ERR || lNumSelected == 0)
		return;
	
	int* arrSelectedItems = new int[lNumSelected];
	SendMessage(hFrom, LB_GETSELITEMS, lNumSelected, reinterpret_cast<LPARAM>(arrSelectedItems));
	for (int i = 0; i < lNumSelected; ++i)
	{
		const int nRealItemIndex = arrSelectedItems[i]-i; // Because we remove one each time
		LRESULT lTextLen = SendMessage(hFrom, LB_GETTEXTLEN, nRealItemIndex, 0);
		TCHAR* szCurrItem = new TCHAR[lTextLen + 1];
		SendMessage(hFrom, LB_GETTEXT, nRealItemIndex, reinterpret_cast<LPARAM>(szCurrItem));
		const LRESULT lNewIndex = SendMessage(hTo, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(szCurrItem));
		SendMessage(hTo, LB_SETSEL, TRUE, lNewIndex);
		SendMessage(hFrom, LB_DELETESTRING, nRealItemIndex, 0);
		delete[] szCurrItem;
	}
	delete[] arrSelectedItems;
}

string strCreateRandomPassword()
{
	string strPassword;
	strPassword.resize(10);

	srand(time(NULL));
	for (int i=0; i<strPassword.size(); ++i)
	{
		strPassword[i] = rand() % 255 + 1;
	}
	return strPassword;
}

void ReadOptions(HWND hDlg)
{
	// NOTES: 
	// 1. CommandDB is initialized when modules register for commands

	pluginRuntimeData.hRelayToContact = GetRelayToContact();

	HWND hWndRelayToCombo = GetDlgItem(hDlg, IDC_RELAY_TO_CONTACT);

	HANDLE hCurrContact = reinterpret_cast<HANDLE>(CallService(MS_DB_CONTACT_FINDFIRST, 0, 0));
	while (hCurrContact != NULL)
	{
		const char* szContactName = 
			reinterpret_cast<char*>(CallService(MS_CLIST_GETCONTACTDISPLAYNAME, reinterpret_cast<WPARAM>(hCurrContact), 0));
		const LRESULT lItemPos = SendMessage(hWndRelayToCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(szContactName));
		if (lItemPos == CB_ERR)
			break;

		const LRESULT lRet = SendMessage(hWndRelayToCombo, CB_SETITEMDATA, lItemPos, reinterpret_cast<LPARAM>(hCurrContact));
		if (lRet == CB_ERR)
			break;

		hCurrContact = reinterpret_cast<HANDLE>(CallService(MS_DB_CONTACT_FINDNEXT, reinterpret_cast<WPARAM>(hCurrContact), 0));
	}

	SendDlgItemMessage(hDlg, IDC_RELAY_TO_CONTACT, CB_SETCURSEL, FindComboItemDataIndex(hWndRelayToCombo, pluginRuntimeData.hRelayToContact), 0);
	
	CheckDlgButton(hDlg, IDC_ENABLE_RELAY, IsPluginEnabled());

	CheckDlgButton(hDlg, IDC_CHECK_MIMIC, 
		(BOOL)DBGetContactSettingByte(NULL, MODULE_NAME, OPT_MIMIC, FALSE));
	
	CheckDlgButton(hDlg, IDC_CHECK_ALERT_ONLINE, 
		(BOOL)DBGetContactSettingByte(NULL, MODULE_NAME, OPT_ALERT_ONLINE, FALSE));
	
	CheckDlgButton(hDlg, IDC_CHECK_ALERT_OFFLINE, 
		(BOOL)DBGetContactSettingByte(NULL, MODULE_NAME, OPT_ALERT_OFFLINE, FALSE));

	CheckDlgButton(hDlg, IDC_CHECK_ALERT_CHANGE_FROM_OFFLINE, 
		(BOOL)DBGetContactSettingByte(NULL, MODULE_NAME, OPT_ALERT_CHANGE_FROM_OFFLINE, FALSE));

	CheckDlgButton(hDlg, IDC_TIMESTAMPS, 
		(BOOL)DBGetContactSettingByte(NULL, MODULE_NAME, OPT_TIMESTAMPS, FALSE));

	CheckDlgButton(hDlg, IDC_CREATE_HISTORY, 
		(BOOL)DBGetContactSettingByte(NULL, MODULE_NAME, OPT_CREATE_HISTORY_EVENTS, TRUE));

	const char* szProto = reinterpret_cast<char*>(CallService(MS_PROTO_GETCONTACTBASEPROTO,(WPARAM)pluginRuntimeData.hRelayToContact,0));
	int nNumProtos = 0;
	PROTOCOLDESCRIPTOR **protos;
	CallService(MS_PROTO_ENUMPROTOCOLS,(WPARAM) &nNumProtos,(LPARAM) &protos);
	HWND hIgnoreProtocols = GetDlgItem(hDlg, IDC_IGNORE_PROTOCOLS);
	for(int i = 0; i < nNumProtos; ++i) 
	{
		if(protos[i]->type != PROTOTYPE_PROTOCOL)
		{
			continue;
		}
		
		const LRESULT lNewIndex = SendMessage(hIgnoreProtocols, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(protos[i]->szName));
		if (ProtocolDB::Instance().GetProtocolState(protos[i]->szName) == ProtocolDB::PROTOCOL_STATE_IGNORED)
		{
			SendMessage(hIgnoreProtocols, LB_SETSEL, TRUE, lNewIndex);
		}
	}
	
}

void WriteOptions(HWND hDlg)
{
	// NOTE: The order of the following lines matters!
	DBWriteContactSettingByte(NULL, MODULE_NAME, OPT_ENABLE, 
		(BYTE) IsDlgButtonChecked(hDlg, IDC_ENABLE_RELAY));

	if (!IsPluginEnabled())
	{
		pluginRuntimeData.hRelayToContact = NULL;
	}
	else
	{
		HWND hWndRelayToCombo = GetDlgItem(hDlg, IDC_RELAY_TO_CONTACT);
		pluginRuntimeData.hRelayToContact = GetComboCurItemData(hWndRelayToCombo);
	}

	const string strUnique = GetContactUniqueString(pluginRuntimeData.hRelayToContact);
	DBWriteContactSettingString(NULL, MODULE_NAME, OPT_RELAY_TO_CONTACT, strUnique.c_str());

	DBWriteContactSettingByte(NULL, MODULE_NAME, OPT_MIMIC, 
		(BYTE) IsDlgButtonChecked(hDlg, IDC_CHECK_MIMIC));

	DBWriteContactSettingByte(NULL, MODULE_NAME, OPT_ALERT_ONLINE, 
		(BYTE) IsDlgButtonChecked(hDlg, IDC_CHECK_ALERT_ONLINE));

	DBWriteContactSettingByte(NULL, MODULE_NAME, OPT_ALERT_OFFLINE, 
		(BYTE) IsDlgButtonChecked(hDlg, IDC_CHECK_ALERT_OFFLINE));

	DBWriteContactSettingByte(NULL, MODULE_NAME, OPT_ALERT_CHANGE_FROM_OFFLINE, 
		(BYTE) IsDlgButtonChecked(hDlg, IDC_CHECK_ALERT_CHANGE_FROM_OFFLINE));

	DBWriteContactSettingByte(NULL, MODULE_NAME, OPT_TIMESTAMPS, 
		(BYTE) IsDlgButtonChecked(hDlg, IDC_TIMESTAMPS));

	DBWriteContactSettingByte(NULL, MODULE_NAME, OPT_CREATE_HISTORY_EVENTS, 
		(BYTE) IsDlgButtonChecked(hDlg, IDC_CREATE_HISTORY));

	HWND hIgnoreProtocols = GetDlgItem(hDlg, IDC_IGNORE_PROTOCOLS);
	LRESULT lItemCount = SendMessage(hIgnoreProtocols, LB_GETCOUNT, 0, 0);
	for (int i = 0; i < lItemCount; ++i)
	{
		LRESULT lTextLen = SendMessage(hIgnoreProtocols, LB_GETTEXTLEN, i, 0);
		TCHAR* szCurrItem = new TCHAR[lTextLen + 1];
		SendMessage(hIgnoreProtocols, LB_GETTEXT, i, reinterpret_cast<LPARAM>(szCurrItem));
		
		ProtocolDB::PROTOCOL_STATE eProtocolState = ProtocolDB::PROTOCOL_STATE_ALLOWED;
		if (SendMessage(hIgnoreProtocols, LB_GETSEL, i, 0) != 0)
		{
			eProtocolState = ProtocolDB::PROTOCOL_STATE_IGNORED;
		}
		ProtocolDB::Instance().SetProtocolState(szCurrItem, eProtocolState);
		delete[] szCurrItem;
	}
}

void EnableRelayOptionsDlgItems(HWND hDlg, const BOOL bEnable)
{
	EnableWindow(GetDlgItem(hDlg, IDC_CHECK_MIMIC), bEnable);
	EnableWindow(GetDlgItem(hDlg, IDC_CHECK_ALERT_ONLINE), bEnable);
	EnableWindow(GetDlgItem(hDlg, IDC_CHECK_ALERT_OFFLINE), bEnable);
	EnableWindow(GetDlgItem(hDlg, IDC_CHECK_ALERT_CHANGE_FROM_OFFLINE), bEnable);
	EnableWindow(GetDlgItem(hDlg, IDC_RELAY_TO_CONTACT), bEnable);
//	EnableWindow(GetDlgItem(hDlg, IDC_LIST_ALLOWED_COMMANDS), bEnable);
//	EnableWindow(GetDlgItem(hDlg, IDC_LIST_DISALLOWED_COMMANDS), bEnable);
//	EnableWindow(GetDlgItem(hDlg, IDC_LIST_PASSWORD_COMMANDS), bEnable);
//	EnableWindow(GetDlgItem(hDlg, IDC_BUTTON_ALLOW2PASSWD), bEnable);
//	EnableWindow(GetDlgItem(hDlg, IDC_BUTTON_PASSWD2ALLOW), bEnable);
//	EnableWindow(GetDlgItem(hDlg, IDC_BUTTON_PASSWD2DISALLOW), bEnable);
//	EnableWindow(GetDlgItem(hDlg, IDC_BUTTON_DISALLOW2PASSWD), bEnable);
	EnableWindow(GetDlgItem(hDlg, IDC_BUTTON_COMMAND_SECURITY), bEnable);
	
//	EnableWindow(GetDlgItem(hDlg, IDC_PASSWORD), bEnable);
//	EnableWindow(GetDlgItem(hDlg, IDC_AUTO_LOGOFF_TIME), bEnable);
//	EnableWindow(GetDlgItem(hDlg, IDC_AUTOLOCK_RETRIES), bEnable);
	EnableWindow(GetDlgItem(hDlg, IDC_TIMESTAMPS), bEnable);
	EnableWindow(GetDlgItem(hDlg, IDC_CREATE_HISTORY), bEnable);
}

void ReadCommandSecurityOptions(HWND hDlg)
{
	HWND hlbAllowedCommands = GetDlgItem(hDlg, IDC_LIST_ALLOWED_COMMANDS);
	HWND hlbDisallowedCommands = GetDlgItem(hDlg, IDC_LIST_DISALLOWED_COMMANDS);
	HWND hlbPwdProtectedCommands = GetDlgItem(hDlg, IDC_LIST_PASSWORD_COMMANDS);
	CommandDB& db = CommandDB::Instance();
	for (CommandDB::Iterator it = db.Begin(); it != db.End(); ++it)
	{
		string strCommandName;
		CommandDB::COMMAND_STATE eState = CommandDB::COMMAND_STATE_DISALLOWED;
		HANDLE hHookableEvent = NULL;
		db.GetCommandInfo(it, strCommandName, eState, hHookableEvent);

		HWND hSendToList;
		if (eState == CommandDB::COMMAND_STATE_ALLOWED)
		{
			hSendToList = hlbAllowedCommands;
		}
		else if (eState == CommandDB::COMMAND_STATE_DISALLOWED)
		{
			hSendToList = hlbDisallowedCommands;
		}
		else if (eState == CommandDB::COMMAND_STATE_PWD_PROTECTED)
		{
			hSendToList = hlbPwdProtectedCommands;
		}

		SendMessage(hSendToList, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(strCommandName.c_str()));
	}

	DBVARIANT dbvPassword = {0};
	string strPassword;
	const int nRetPassword = DBGetContactSetting(NULL, MODULE_NAME, OPT_PASSWORD, &dbvPassword);
	if (nRetPassword != 0)
	{
		string strRandomPassword = strCreateRandomPassword();
		DBWriteContactSettingString(NULL, MODULE_NAME, OPT_PASSWORD, strRandomPassword.c_str());
		strPassword = strRandomPassword;
	}
	else
	{
		strPassword = dbvPassword.pszVal;
	}
	
	DBFreeVariant(&dbvPassword);

	SetDlgItemText(hDlg, IDC_PASSWORD, strPassword.c_str());

	SetDlgItemInt(hDlg, 
		IDC_AUTO_LOGOFF_TIME, 
		DBGetContactSettingDword(NULL, MODULE_NAME, OPT_AUTO_LOGOFF_TIME, OPT_DEFAULT_AUTO_LOGOFF_TIME), 
		FALSE);

	SetDlgItemInt(hDlg, 
		IDC_AUTOLOCK_RETRIES, 
		DBGetContactSettingDword(NULL, MODULE_NAME, OPT_AUTO_LOCK_FAILED_ATTEMPTS, OPT_DEFAULT_AUTO_LOCK_FAILED_ATTEMPTS), 
		FALSE);
}

void WriteCommandSecurityOptions(HWND hDlg)
{
	string strAllowedCommands;
	string strDisallowedCommands;
	string strPwdProtectedCommands;

	ListBoxToStringAndDB(GetDlgItem(hDlg, IDC_LIST_PASSWORD_COMMANDS), 
		strPwdProtectedCommands, CommandDB::COMMAND_STATE_PWD_PROTECTED);

	ListBoxToStringAndDB(GetDlgItem(hDlg, IDC_LIST_ALLOWED_COMMANDS), 
		strAllowedCommands, CommandDB::COMMAND_STATE_ALLOWED);

	ListBoxToStringAndDB(GetDlgItem(hDlg, IDC_LIST_DISALLOWED_COMMANDS), 
		strDisallowedCommands, CommandDB::COMMAND_STATE_DISALLOWED);

	DBWriteContactSettingString(NULL, MODULE_NAME, OPT_ALLOWED_COMMANDS, strAllowedCommands.c_str());
	DBWriteContactSettingString(NULL, MODULE_NAME, OPT_DISALLOWED_COMMANDS, strDisallowedCommands.c_str());
	DBWriteContactSettingString(NULL, MODULE_NAME, OPT_PWD_PROTECTED_COMMANDS, strPwdProtectedCommands.c_str()); 

	int nPasswordLength = GetWindowTextLength(GetDlgItem(hDlg, IDC_PASSWORD));
	char* szPassword = new char[nPasswordLength + 1];
	GetDlgItemText(hDlg, IDC_PASSWORD, szPassword, nPasswordLength + 1);
	DBWriteContactSettingString(NULL, MODULE_NAME, OPT_PASSWORD, szPassword);
	delete[] szPassword;

	DBWriteContactSettingDword(NULL, MODULE_NAME, OPT_AUTO_LOGOFF_TIME,
		GetDlgItemInt(hDlg, IDC_AUTO_LOGOFF_TIME, NULL, FALSE));

	DBWriteContactSettingDword(NULL, MODULE_NAME, OPT_AUTO_LOCK_FAILED_ATTEMPTS,
		GetDlgItemInt(hDlg, IDC_AUTOLOCK_RETRIES, NULL, FALSE));
}

BOOL CALLBACK CommandSecurityWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch(msg)
	{
	case WM_INITDIALOG:
		{
			ReadCommandSecurityOptions(hWnd);
		}
		break;
		
	case WM_COMMAND:
		switch(LOWORD(wParam)) 
		{
		case IDOK:
			{
				WriteCommandSecurityOptions(hWnd);
			}
			// FALL THROUGH
		case IDCANCEL:
			{
				DestroyWindow(hWnd);
			}
			break;
			
		case IDC_BUTTON_PASSWD2DISALLOW:
			{
				HWND hlbPwdProtected = GetDlgItem(hWnd, IDC_LIST_PASSWORD_COMMANDS);
				HWND hlbDisallowed = GetDlgItem(hWnd, IDC_LIST_DISALLOWED_COMMANDS);
				TransferListBoxItems(hlbPwdProtected, hlbDisallowed);
			}
			break;
			
		case IDC_BUTTON_DISALLOW2PASSWD:
			{
				HWND hlbPwdProtected = GetDlgItem(hWnd, IDC_LIST_PASSWORD_COMMANDS);
				HWND hlbDisallowed = GetDlgItem(hWnd, IDC_LIST_DISALLOWED_COMMANDS);
				TransferListBoxItems(hlbDisallowed, hlbPwdProtected);
			}
			break;
			
		case IDC_BUTTON_ALLOW2PASSWD:
			{
				HWND hlbPwdProtected = GetDlgItem(hWnd, IDC_LIST_PASSWORD_COMMANDS);
				HWND hlbAllowed = GetDlgItem(hWnd, IDC_LIST_ALLOWED_COMMANDS);
				TransferListBoxItems(hlbAllowed, hlbPwdProtected);
			}
			break;
			
		case IDC_BUTTON_PASSWD2ALLOW:
			{
				HWND hlbPwdProtected = GetDlgItem(hWnd, IDC_LIST_PASSWORD_COMMANDS);
				HWND hlbAllowed = GetDlgItem(hWnd, IDC_LIST_ALLOWED_COMMANDS);
				TransferListBoxItems(hlbPwdProtected, hlbAllowed);
			}
			break;
			
		}
		break;
	}
	return FALSE;
}


BOOL CALLBACK RelayOptionsWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:
		{
			TranslateDialogDefault(hWnd);

			ReadOptions(hWnd);
			EnableRelayOptionsDlgItems(hWnd, IsPluginEnabled());
		}
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDC_ENABLE_RELAY:
			{
				const UINT nChecked = IsDlgButtonChecked(hWnd, IDC_ENABLE_RELAY);
				HWND hWndRelayToCombo = GetDlgItem(hWnd, IDC_RELAY_TO_CONTACT);
				if (nChecked != BST_CHECKED)
				{
					EnableRelayOptionsDlgItems(hWnd, FALSE);
				}
				else
				{
					EnableRelayOptionsDlgItems(hWnd, TRUE);
				}
			}
			break;

		case IDC_BUTTON_COMMAND_SECURITY:
			{
				ShowWindow(CreateDialogParam(hInst,MAKEINTRESOURCE(IDD_COMMAND_SECURITY),hWnd,CommandSecurityWndProc, (LPARAM)0), SW_SHOW);
			}
			return FALSE;


		case IDC_PASSWORD:
			{
				switch (HIWORD(wParam))
				{
				case EN_CHANGE:
					if (reinterpret_cast<HWND>(lParam) == GetFocus())
					{
						break;
					}
					else
					{
						return FALSE;
					}

				default:
					return FALSE;
				}
			}
			break;

		case IDC_AUTO_LOGOFF_TIME:
			{
				switch (HIWORD(wParam))
				{
				case EN_CHANGE:
					if (reinterpret_cast<HWND>(lParam) == GetFocus())
					{
						break;
					}
					else
					{
						return FALSE;
					}

				default:
					return FALSE;
				}
			}
			break;

		case IDC_AUTOLOCK_RETRIES:
			{
				switch (HIWORD(wParam))
				{
				case EN_CHANGE:
					if (reinterpret_cast<HWND>(lParam) == GetFocus())
					{
						break;
					}
					else
					{
						return FALSE;
					}

				default:
					return FALSE;
				}
			}
			break;

		default:
			break;
		}
		SendMessage(GetParent(hWnd), PSM_CHANGED, 0, 0);
		break;

	case WM_NOTIFY:
		switch (((LPNMHDR)lParam)->code)
		{
			case PSN_APPLY:
				WriteOptions(hWnd);
				return TRUE;
		}
		break;

	default:
		break;
	}

	return FALSE;
}


int EventOptionsInitalize(WPARAM addInfo, LPARAM lParam)
{
    OPTIONSDIALOGPAGE odp = {0};
    
	odp.cbSize = sizeof(odp);
	odp.hInstance = hInst;
	odp.pszTemplate = MAKEINTRESOURCE(IDD_RELAY_OPTIONS);
	odp.pszTitle = Translate("Relay Plugin");
	odp.pszGroup = Translate("Plugins");
	// odp.flags = ODPF_BOLDGROUPS;
	odp.pfnDlgProc = RelayOptionsWndProc;
	
	CallService(MS_OPT_ADDPAGE, addInfo, (LPARAM)&odp);

    return 0;
}
