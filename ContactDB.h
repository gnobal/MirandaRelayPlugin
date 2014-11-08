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

#ifndef _CONTACT_DB_H__
#define _CONTACT_DB_H__

#pragma warning(disable: 4786) 

#include <windows.h>
#include <map>
#include <vector>

class ContactDB {
private:
	typedef std::map<HANDLE, int> ContactMap;
	typedef std::vector<HANDLE> ContactVector;

public:
	static ContactDB& Instance();

	typedef ContactVector::size_type SizeType;
	SizeType Size() const { return contactVector.size(); }
	HANDLE operator[] (SizeType n) { return contactVector[n]; }

	int AddContact(HANDLE hContact);
	int GetContactId(HANDLE hContact) const;
	HANDLE GetContactHandle(int nContactId) const;

private:
	ContactDB(const ContactDB&);
	ContactDB& operator=(const ContactDB&);

private:
	ContactDB() {}

private:
	ContactMap contactMap;
	ContactVector contactVector;
};

#endif
