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

#ifndef _PROTOCOL_DB_H__
#define _PROTOCOL_DB_H__

#pragma warning(disable: 4786) 
#pragma warning(disable: 4503) 

#include <map>
#include <string>

using std::string;


class ProtocolDB {
public:
	typedef enum { 
		PROTOCOL_STATE_ALLOWED,
		PROTOCOL_STATE_IGNORED
	} PROTOCOL_STATE;

public:
	static ProtocolDB& Instance();

	PROTOCOL_STATE GetProtocolState(const string& strProtocolName);
	void SetProtocolState(const string& strProtocolName, const PROTOCOL_STATE eState);

private:
	ProtocolDB(const ProtocolDB&);
	ProtocolDB& operator=(const ProtocolDB&);

private:
	ProtocolDB() {}
	static string ConstructProtocolOptionName(const string& strProtocolName);

private:
	static const string strProtocolOptionPrefix;
	// typedef std::map<string, PROTOCOL_STATE> ProtocolStateMap;
	// ProtocolStateMap stateMap;
};

#endif
