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

#ifndef _COMMAND_DB_H__
#define _COMMAND_DB_H__

#pragma warning(disable: 4786) 
#pragma warning(disable: 4503) 

#include <windows.h>
#include <map>
#include <vector>
#include <string>

using std::string;


class CommandDB {
public:
	typedef enum { 
		COMMAND_STATE_ALLOWED, 
		COMMAND_STATE_PWD_PROTECTED, 
		COMMAND_STATE_DISALLOWED 
	} COMMAND_STATE;
/*
	typedef enum {
		COMMAND_PRIORITY_NORMAL = 0x00,
		COMMAND_PRIO
	
	} COMMAND_PRIORITY;
*/
private:
	struct CommandInfo
	{
		COMMAND_STATE eCommandState;
		int nNumHandlers;
		HANDLE hHookableEvent;
	};

	typedef std::map<string, CommandInfo> CommandMap;

	struct HandleToCommandMapping
	{
		HANDLE hHookHandle;
		CommandMap::iterator itCommand;
	};

	typedef std::vector<HandleToCommandMapping> MappingVector;

public:
	static CommandDB& Instance();
	
	typedef CommandMap::const_iterator Iterator;

	Iterator Begin() { return commandMap.begin(); }
	Iterator End() { return commandMap.end(); }

	HANDLE AddCommandHandler(const string& strCommandName, 
							 const HANDLE hHookableEvent, 
							 const HANDLE hHookHandle,
							 const COMMAND_STATE eState);
	HANDLE RemoveCommandHandler(const HANDLE hHandler);
	int SetCommandState(const string& strCommandName, COMMAND_STATE eNewState);
	int GetCommandInfo(Iterator it, string& strCommandName, COMMAND_STATE& eState, HANDLE& hHookableEvent) const;
	int GetCommandInfo(const string& strCommandName, COMMAND_STATE& eState, HANDLE& hHookableEvent) const;
	bool CommandExists(const string& strCommandName) const;

private:
	CommandDB(const CommandDB&);
	CommandDB& operator=(const CommandDB&);

private:
	CommandDB() {}

private:
	CommandMap commandMap;
	MappingVector mappingVector;
};

#endif
