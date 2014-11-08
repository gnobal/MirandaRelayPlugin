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

#ifndef _COMMAND_HANDLERS_H__
#define _COMMAND_HANDLERS_H__

#include <windows.h>

int CommandWhoHandler(WPARAM wParam,LPARAM lParam);
int CommandSendHandler(WPARAM wParam,LPARAM lParam);
int CommandHelpHandler(WPARAM wParam,LPARAM lParam);
int CommandChstatusHandler(WPARAM wParam,LPARAM lParam);
int CommandMimicHandler(WPARAM wParam,LPARAM lParam);
int CommandLogonHandler(WPARAM wParam,LPARAM lParam);
int CommandLogoffHandler(WPARAM wParam,LPARAM lParam);
int CommandTimesHandler(WPARAM wParam,LPARAM lParam);
int CommandAlertsHandler(WPARAM wParam, LPARAM lParam);
/*
int CommandPauseHandler(WPARAM wParam, LPARAM lParam);
int CommandResumeHandler(WPARAM wParam, LPARAM lParam);
*/
#endif
