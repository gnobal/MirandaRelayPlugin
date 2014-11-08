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

#include "CommandDB.h"
#include <utility>

using std::make_pair;

CommandDB& CommandDB::Instance()
{
	static CommandDB theDB;
	
	return theDB;
}

HANDLE CommandDB::AddCommandHandler(const string& strCommandName, 
									const HANDLE hHookableEvent, 
									const HANDLE hHookHandle,
									const COMMAND_STATE eState)
{
	CommandInfo info;
	info.eCommandState = eState;
	info.nNumHandlers = 1;
	info.hHookableEvent = hHookableEvent;
	
	std::pair<CommandMap::iterator, bool> insertPair = commandMap.insert(make_pair(strCommandName, info));
	
	HandleToCommandMapping mapping;
	mapping.hHookHandle = hHookHandle;
	mapping.itCommand = insertPair.first;
	
	mappingVector.push_back(mapping);

	const HANDLE hRet = reinterpret_cast<HANDLE>(mappingVector.size()-1);

	if (insertPair.second) // New command
		return hRet;

	insertPair.first->second.nNumHandlers++;
	return hRet;
}

HANDLE CommandDB::RemoveCommandHandler(const HANDLE hHandler)
{
	MappingVector::size_type nMappingId = reinterpret_cast<MappingVector::size_type>(hHandler);

	if (nMappingId < 0 || nMappingId >= mappingVector.size())
		return NULL;

	CommandMap::iterator it = mappingVector[nMappingId].itCommand;
	if (--it->second.nNumHandlers == 0)
		commandMap.erase(it);

	return mappingVector[nMappingId].hHookHandle;
}

int CommandDB::SetCommandState(const string& strCommandName, COMMAND_STATE eNewState)
{
	CommandMap::iterator it = commandMap.find(strCommandName);
	if (it == commandMap.end())
		return -1;

	it->second.eCommandState = eNewState;
	return 0;
}

bool CommandDB::CommandExists(const string& strCommandName) const
{
	Iterator it = commandMap.find(strCommandName);
	if (it == commandMap.end())
		return false;

	return true;
}

int CommandDB::GetCommandInfo(const string& strCommandName, COMMAND_STATE& eState, HANDLE& hHookableEvent) const
{
	Iterator it = commandMap.find(strCommandName);
	if (it == commandMap.end())
		return -1;

	eState = it->second.eCommandState;
	hHookableEvent = it->second.hHookableEvent;

	return 0;
}

int CommandDB::GetCommandInfo(Iterator it, string& strCommandName, COMMAND_STATE& eState, HANDLE& hHookableEvent) const
{
	eState = it->second.eCommandState;
	hHookableEvent = it->second.hHookableEvent;
	strCommandName = it->first;

	return 0;
}
