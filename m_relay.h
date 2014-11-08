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

#ifndef _M_RELAY_H__
#define _M_RELAY_H__

// RELAYMESSAGEINFO: The structure used to send a message to the relay-to contact
//
typedef struct
{
	int cbSize;
	const char* szMessage;
} RELAYMESSAGEINFO;

// Send message to the relay-to contact
// wPARAM = (WPARAM)0
// lParam = (LPARAM)(RELAYMESSAGEINFO*) &rmi
// Returns 0 if message was relayed, 1 otherwiae (right now doesn't confirm the ack)
//
#define MS_RELAY_RELAY_MESSAGE "Relay/RelayMessage"

// RELAYREGISTERINFO: The structure used to register a command handler when the command 
// is received from the relay-to contact.
//
#define COMMAND_INITIAL_STATE_DISALLOWED 0
#define COMMAND_INITIAL_STATE_ALLOWED 1
#define COMMAND_INITIAL_STATE_PWD_PROTECTED 2

typedef struct
{
	int cbSize;
	const char* szCommand; // The command sent by the user. Must not contain white spaces
	int nInitialState; // The command's initial state
	MIRANDAHOOK handlerProc;
} RELAYREGISTERINFO;

// Register handler to incoming command
// wParam = (WPARAM)0
// lParam = (LPARAM)(RELAYREGISTERINFO*) &rri
// Returns a handle hHook to be used later when unregistering (see below), otherwise NULL
//
// The plugin actually registers handlerProc as a Miranda hook, so all rules apply. However,
// you CANNOT use the handle received by this service with Miranda functions and services.
// The handle is internal to the Relay plugin and is mapped internally to a Miranda handle.
// The reason for this is to allow the user full control over commands from the 
// Relay plugin without requiring other plugins to implement anything. It is also for security
// issues.
//
#define MS_RELAY_REGISTER_COMMAND_HANDLER "Relay/RegisterCommandHandler"

// MS_RELAY_UNREGISTER_COMMAND_HANDLER: Unregister a previously registered command handler
// wParam = (WPARAM)0
// lParam = (LPARAM)(HANDLE) hHook
//
#define MS_RELAY_UNREGISTER_COMMAND_HANDLER "Relay/UnregisterCommandHandler"

// RELAYCOMMANDEVENTINFO: The structure received upon a registered command.
// wParam = (WPARAM)(int)mode
// lParam = (LPARAM)(RELAYCOMMANDEVENTINFO*)&rcei

// wParam is one of the following definitions:
#define RELAYCOMMANDEVENT_HANDLEEVENT 0 // rcei is valid
#define RELAYCOMMANDEVENT_SENDHELP 1 // rcei->szCommand is the only valid member when 
									 // wParam==RELAYCOMMANDEVENT_SENDHELP. This is to 
									 // allow plugins to register one handler for multiple 
									 // commands.
									 // The function called should use the 
									 // MS_RELAY_RELAY_MESSAGE service to relay the help 
									 // for the command.

// NOTE: DO NOT modify the members of this structure. Make a copy of them. Other 
// plugins may have registered to handle this command.

typedef struct
{
	int cbSize;
	const char* szCommand; // The command which triggered the hook
	const char* szCommandLine; // The entire(!) command line entered by the user unchanged, 
							   // including the command in szCommand (may include whiteapaces
							   // before the command.
	int nCommandStartPos; // The position in szCommandLine of the first char of the command 
	int nCommandEndPos; // The position in szCommandLine of the last char of the command
} RELAYCOMMANDEVENTINFO;


// MS_RELAY_GET_CONTACT_HANDLE_FROM_ID: Get the contact handle from the ID. 
// The relay-to contact gets the (integer) IDs of the contacts using the "who" command.
// If you wish to supply the user with a command that requires a contact, the user will use 
// the ID to identify the contact, and you will need to convert it to a handle using 
// this service.
// And example would be a "block" command. The user types "block 10" and you need the handle
// to the contact whose ID is 10
// wParam = (WPARAM)0
// lParam = (LPARAM)(int)nContactID
// Returns a valid handle or NULL if the ID is out of bounds
#define MS_RELAY_GET_CONTACT_HANDLE_FROM_ID "Relay/GetContactHandleFromId"

/*

THIS SERVICE IS CURRENTLY NOT SUPPLIED BY THE RELAY PLUGIN

// MS_RELAY_GET_RELAY_CONTACT_HANDLE: Gets the relay-to contact handle.
// The relay-to contact is also one of the contacts in the internal contact DB the 
// relay plugin saves, and has an ID just like a remote contact.
// This service is useful to distinguish the behaviour of your plugin when it concerns with 
// the relay-to contact and the other contacts
// wParam = (WPARAM)0
// lParam = (LPARAM)0
// Returns the relay-to contact handle or NULL if the relay is disabled (!)
#define MS_RELAY_GET_RELAY_CONTACT_HANDLE "Relay/GetRelayContactHandle"
*/
#endif
