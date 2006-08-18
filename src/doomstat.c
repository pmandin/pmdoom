// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: doomstat.c,v 1.1 2006/08/18 19:01:29 patrice Exp $
//
// Copyright (C) 1993-1996 by id Software, Inc.
//
// This source is available for distribution and/or modification
// only under the terms of the DOOM Source Code License as
// published by id Software. All rights reserved.
//
// The source is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// FITNESS FOR A PARTICULAR PURPOSE. See the DOOM Source Code License
// for more details.
//
// $Log: doomstat.c,v $
// Revision 1.1  2006/08/18 19:01:29  patrice
// *** empty log message ***
//
//
// DESCRIPTION:
//	Put all global tate variables here.
//
//-----------------------------------------------------------------------------

#include <stdio.h>

#include "doomstat.h"


// Game Mode - identify IWAD as shareware, retail etc.
GameMode_t gamemode = indetermined;
GameMission_t	gamemission = doom;

// Language.
Language_t   language = english;

// Set if homebrew PWAD stuff has been added.
boolean	modifiedgame;




