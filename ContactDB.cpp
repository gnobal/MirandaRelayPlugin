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

#include "ContactDB.h"
#include <utility>

using std::make_pair;

ContactDB& ContactDB::Instance()
{
	static ContactDB theDB;
	
	return theDB;
}

int ContactDB::AddContact(HANDLE hContact)
{
	std::pair<ContactMap::iterator, bool> insertPair = contactMap.insert(make_pair(hContact, contactVector.size()));
	if (insertPair.second == false)
		return insertPair.first->second;
	
	contactVector.push_back(hContact);
	return insertPair.first->second;
}

int ContactDB::GetContactId(HANDLE hContact) const
{
	ContactMap::const_iterator it = contactMap.find(hContact);
	if (it == contactMap.end())
		return -1;

	return it->second;
}

HANDLE ContactDB::GetContactHandle(int nContactId) const
{
	if (nContactId < 0 || nContactId > contactVector.size()-1)
		return NULL;

	return contactVector[nContactId];
}

