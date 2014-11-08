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
#include "Options.h"
#include "ProtocolDB.h"
#include "protocols/protocols/m_protocols.h"
#include "protocols/protocols/m_protosvc.h"

const char* szWhiteSpaces = " \t\n\r";

BOOL IsPluginEnabled()
{
	return (BOOL) DBGetContactSettingByte(NULL, MODULE_NAME, OPT_ENABLE, FALSE);
}


int GetContactStatus(HANDLE hContact)
{
	char* szProto = reinterpret_cast<char*>(CallService(MS_PROTO_GETCONTACTBASEPROTO,(WPARAM)hContact,0));
	if(szProto == NULL) 
		return ID_STATUS_OFFLINE;
	
	return DBGetContactSettingWord(hContact,szProto,"Status",ID_STATUS_OFFLINE);
}

void ChangeStatus(int nStatus)
{
	const char* szProto = reinterpret_cast<char*>(CallService(MS_PROTO_GETCONTACTBASEPROTO,(WPARAM)pluginRuntimeData.hRelayToContact,0));

	int nNumProtos = 0;
	PROTOCOLDESCRIPTOR **protos;
	CallService(MS_PROTO_ENUMPROTOCOLS,(WPARAM) &nNumProtos,(LPARAM) &protos);

	for(int i = 0; i < nNumProtos; ++i) 
	{
		if(protos[i]->type != PROTOTYPE_PROTOCOL || 
		   CallProtoService(protos[i]->szName, PS_GETCAPS, PFLAGNUM_2, 0) == 0 ||
		   strcmp(szProto, protos[i]->szName) == 0 ||
		   ProtocolDB::Instance().GetProtocolState(protos[i]->szName) == ProtocolDB::PROTOCOL_STATE_IGNORED) 
		{
			continue;
		}
		
		CallProtoService(protos[i]->szName,PS_SETSTATUS,nStatus, 0); 
	}
}
