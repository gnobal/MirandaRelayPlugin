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

#include "ProtocolDB.h"
#include "Common.h"
#include "Options.h"

const string ProtocolDB::strProtocolOptionPrefix = "ProtocolState_";


ProtocolDB::PROTOCOL_STATE ProtocolDB::GetProtocolState(const string& strProtocolName)
{
	/*
	ProtocolStateMap::iterator it = stateMap.find(strProtocolName);
	if (it == stateMap.end())
		return PROTOCOL_STATE_ALLOWED;

	return it->second;
	*/
	return (PROTOCOL_STATE) DBGetContactSettingByte(NULL, MODULE_NAME, 
		ConstructProtocolOptionName(strProtocolName).c_str(), PROTOCOL_STATE_ALLOWED);
}

void ProtocolDB::SetProtocolState(const string& strProtocolName, 
								  const ProtocolDB::PROTOCOL_STATE eState)
{
	// stateMap[strProtocolName] = eState;
	DBWriteContactSettingByte(NULL, MODULE_NAME, 
		ConstructProtocolOptionName(strProtocolName).c_str(), 
		(BYTE) eState);
}

ProtocolDB& ProtocolDB::Instance()
{
	static ProtocolDB theDB;
	
	return theDB;
}

string ProtocolDB::ConstructProtocolOptionName(const string& strProtocolName)
{
	return strProtocolOptionPrefix + strProtocolName;
}
