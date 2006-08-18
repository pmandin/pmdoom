// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: i_main.c,v 1.1 2006/08/18 19:01:29 patrice Exp $
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
// $Log: i_main.c,v $
// Revision 1.1  2006/08/18 19:01:29  patrice
// *** empty log message ***
//
//
// DESCRIPTION:
//	Main program, simply calls D_DoomMain high level loop.
//
//-----------------------------------------------------------------------------

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>

#include <SDL.h>

#include "doomdef.h"

#include "m_argv.h"
#include "d_main.h"

int
main
( int		argc,
  char**	argv ) 
{ 
    myargc = argc; 
    myargv = argv; 

	printf(PACKAGE_STRING " compiled on " __DATE__ "\n");

    D_DoomMain (); 

    return 0;
} 
