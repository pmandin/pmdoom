// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: m_bbox.c,v 1.1 2006/08/18 19:01:29 patrice Exp $
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
// $Log: m_bbox.c,v $
// Revision 1.1  2006/08/18 19:01:29  patrice
// *** empty log message ***
//
//
// DESCRIPTION:
//	Main loop menu stuff.
//	Random number LUT.
//	Default Config File.
//	PCX Screenshots.
//
//-----------------------------------------------------------------------------

#include "doomtype.h"
#include "m_bbox.h"

void M_ClearBox (fixed_t *box)
{
	box[BOXTOP] = box[BOXRIGHT] = MININT;
	box[BOXBOTTOM] = box[BOXLEFT] = MAXINT;
}

void
M_AddToBox
( fixed_t*	box,
  fixed_t	x,
  fixed_t	y )
{
	if (x<box[BOXLEFT])
		box[BOXLEFT] = x;
	else if (x>box[BOXRIGHT])
		box[BOXRIGHT] = x;
	if (y<box[BOXBOTTOM])
		box[BOXBOTTOM] = y;
	else if (y>box[BOXTOP])
		box[BOXTOP] = y;
}
