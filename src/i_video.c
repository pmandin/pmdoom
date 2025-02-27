// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
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
// DESCRIPTION:
//	DOOM graphics stuff
//
//-----------------------------------------------------------------------------

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>

#include <SDL.h>

#include "doomstat.h"
#include "i_system.h"
#include "i_video.h"
#include "v_video.h"
#include "m_argv.h"
#include "d_main.h"
#include "r_draw.h"

#include "doomdef.h"

#include "st_stuff.h"
#include "r_main.h"
#include "r_plane.h"
#include "am_map.h"
#include "r_things.h"

#include "i_rgb2yuv.h"

/*--- Defines ---*/

#define FRAME_DURATION (1000 / (TICRATE * 2))	/* How much to pause if we are a fast machine */

/*--- Local variables ---*/

static SDL_Surface *screen, *shadow=NULL;
static SDL_Color colors[256];
static SDL_Rect update_area;
static Uint32 scr_flags;
static int fps=0, last_fps=0, frame_tick=0;
static int new_width=0,new_height=0;

static SDL_Overlay *overlay;
static int overlay_format=SDL_YUY2_OVERLAY;

static SDL_GrabMode mouse_grab=SDL_GRAB_OFF;
static int mouseb=0;

static SDL_Joystick *joystick = NULL;
static int joyx=0,joyy=0,joyb=0;
int num_joystick;	/* from doomrc file */

sysvideo_t sysvideo =
{
	SCREENWIDTH, SCREENHEIGHT, 8, SCREENWIDTH,
	false, false, true, false, true
};

/*--- Local functions ---*/

static void InitSdlMode(int width, int height, int bpp);
static int xlatekey(SDLKey keysym);

//
//  Translates the key currently in X_event
//

static int xlatekey(SDLKey keysym)
{
	int rc=0;

    switch(keysym)
	{
		case SDLK_UP:		rc=KEY_UPARROW;		break;
		case SDLK_DOWN:		rc=KEY_DOWNARROW;	break;
		case SDLK_RIGHT:	rc=KEY_RIGHTARROW;	break;
		case SDLK_LEFT:		rc=KEY_LEFTARROW;	break;

		case SDLK_ESCAPE:	rc=KEY_ESCAPE;		break;
		case SDLK_RETURN:	rc=KEY_ENTER;		break;
		case SDLK_KP_ENTER:	rc=KEY_ENTER;		break;
		case SDLK_TAB:		rc=KEY_TAB;			break;
		case SDLK_SPACE:	rc=' ';				break;
		case SDLK_PAUSE:	rc=KEY_PAUSE;		break;
		case SDLK_BACKSPACE:rc=KEY_BACKSPACE;	break;

		case SDLK_MINUS:	rc=KEY_MINUS;		break;
		case SDLK_KP_MINUS:	rc=KEY_MINUS;		break;
		case SDLK_EQUALS:	rc=KEY_EQUALS;		break;
		case SDLK_KP_PLUS:	rc=KEY_EQUALS;		break;

		case SDLK_KP_DIVIDE:	rc='/';			break;
		case SDLK_KP_MULTIPLY:	rc='*';			break;

		case SDLK_RSHIFT:
		case SDLK_LSHIFT:	rc=KEY_RSHIFT;		break;
		case SDLK_RCTRL:
		case SDLK_LCTRL:	rc=KEY_RCTRL;		break;
		case SDLK_RALT:
		case SDLK_LALT:		rc=KEY_RALT;		break;

		case SDLK_HELP:		rc=KEY_F11;			break;
		case SDLK_UNDO:		rc=KEY_F12;			break;

		default:
			if ((keysym>=SDLK_F1) && (keysym<=SDLK_F10))
				rc=keysym-SDLK_F1+KEY_F1;
			if ((keysym>=SDLK_F11) && (keysym<=SDLK_F12))
				rc=keysym-SDLK_F11+KEY_F11;
			if ((keysym>=SDLK_0) && (keysym<=SDLK_9))
				rc=keysym-SDLK_0+'0';
			if ((keysym>=SDLK_KP0) && (keysym<=SDLK_KP9))
				rc=keysym-SDLK_KP0+'0';
			if ((keysym>=SDLK_a) && (keysym<=SDLK_z))
				rc=keysym-SDLK_a+'a';
			break;
	}

	return rc;
}

void I_ShutdownGraphics(void)
{
	if (joystick!=NULL) {
		if (SDL_JoystickOpened(SDL_JoystickIndex(joystick))) {
			SDL_JoystickClose(joystick);
		}
	}

	if (overlay) {
		SDL_FreeYUVOverlay(overlay);
		overlay = NULL;
	}

	if (shadow) {
		SDL_FreeSurface(shadow);
		shadow=NULL;
	}
}

void I_StartTic(void)
{
	SDL_Event	event;
	event_t		doom_event;

	while (SDL_PollEvent(&event)) {
		switch(event.type) {
			case SDL_KEYDOWN:
				if (event.key.keysym.mod & KMOD_ALT) {
					if (event.key.keysym.sym == SDLK_RETURN) {
						/* Toggle fullscreen */
						scr_flags ^= SDL_FULLSCREEN;
						new_width = sysvideo.width;
						new_height = sysvideo.height;
						break;
					} else if (event.key.keysym.sym == SDLK_g) {
						/* Toggle mouse grab in windowed mode only */
						if (screen->flags & SDL_FULLSCREEN) {
							break;
						}

						mouse_grab = SDL_WM_GrabInput(SDL_GRAB_QUERY);
						if (mouse_grab == SDL_GRAB_ON) {
							mouse_grab = SDL_WM_GrabInput(SDL_GRAB_OFF);
						} else {
							mouse_grab = SDL_WM_GrabInput(SDL_GRAB_ON);
						}
						if (mouse_grab == SDL_GRAB_OFF) {
							SDL_ShowCursor(SDL_ENABLE);
						} else {
							SDL_ShowCursor(SDL_DISABLE);
						}
						break;
					} else if (event.key.keysym.sym == SDLK_p) {
						/* Simulate Pause key press */
						doom_event.type = ev_keydown;
						doom_event.data1 = xlatekey(SDLK_PAUSE);
						D_PostEvent(&doom_event);
						break;
					}
				}
				doom_event.type = ev_keydown;
				doom_event.data1 = xlatekey(event.key.keysym.sym);
				D_PostEvent(&doom_event);
				break;
			case SDL_KEYUP:
				doom_event.type = ev_keyup;
				doom_event.data1 = xlatekey(event.key.keysym.sym);
				D_PostEvent(&doom_event);
				break;
			case SDL_MOUSEMOTION:
				if (mouse_grab == SDL_GRAB_ON) {
					doom_event.type = ev_mouse;
					doom_event.data1 = mouseb = event.motion.state;
					doom_event.data2 = event.motion.xrel*10;
					doom_event.data3 = -event.motion.yrel*10;
					D_PostEvent(&doom_event);
				}
				break;
			case SDL_MOUSEBUTTONDOWN:
			case SDL_MOUSEBUTTONUP:
				if (mouse_grab == SDL_GRAB_ON) {
					switch(event.button.state) {
						case SDL_RELEASED:
							mouseb &= ~(1<<(event.button.button-1));
							break;
						case SDL_PRESSED:
							mouseb |= (1<<(event.button.button-1));
							break;
					}
					doom_event.type = ev_mouse;
					doom_event.data1 = mouseb;
					doom_event.data2 = doom_event.data3 = 0;
					D_PostEvent(&doom_event);
				}
				break;
			case SDL_JOYHATMOTION:
				joyx = joyy = 0;
				if (event.jhat.value & SDL_HAT_UP) {
					joyy = -32768;
				} else if (event.jhat.value & SDL_HAT_DOWN) {
					joyy = 32767;
				}
				if (event.jhat.value & SDL_HAT_LEFT) {
					joyx = -32768;
				} else if (event.jhat.value & SDL_HAT_RIGHT) {
					joyx = 32767;
				}
				doom_event.type = ev_joystick;
				doom_event.data1 = joyb;
				doom_event.data2 = joyx>>8;
				doom_event.data3 = joyy>>8;
				D_PostEvent(&doom_event);
				break;
			case SDL_JOYAXISMOTION:
				switch(event.jaxis.axis & 1)  {
					case 0:
						joyx = event.jaxis.value;
						break;
					case 1:
						joyy = event.jaxis.value;
						break;
				}
				doom_event.type = ev_joystick;
				doom_event.data1 = joyb;
				doom_event.data2 = joyx>>8;
				doom_event.data3 = joyy>>8;
				D_PostEvent(&doom_event);
				break;
			case SDL_JOYBUTTONDOWN:
			case SDL_JOYBUTTONUP:
				switch(event.jbutton.state) {
					case SDL_RELEASED:
						joyb &= ~(1<<event.jbutton.button);
						break;
					case SDL_PRESSED:
						joyb |= (1<<event.jbutton.button);
						break;
				}
				doom_event.type = ev_joystick;
				doom_event.data1 = joyb;
				doom_event.data2 = joyx>>8;
				doom_event.data3 = joyy>>8;
				D_PostEvent(&doom_event);
				break;
			case SDL_ACTIVEEVENT:
				if (event.active.gain == 0) {
					if (event.active.state & SDL_APPACTIVE) {
						/* We were iconified, simulate Pause keypress */
						if (!paused) {
							doom_event.type = ev_keydown;
							doom_event.data1 = xlatekey(SDLK_PAUSE);
							D_PostEvent(&doom_event);
						}
					}
				}
				break;
			case SDL_VIDEORESIZE:
				new_width = event.resize.w;
				new_height = event.resize.h;
				break;
			case SDL_QUIT:
				I_Quit();
				break;
		}
	}
}

//
// I_FinishUpdate
//
void I_FinishUpdate (void)
{
	static int frame_start_tick = 0, frame_end_tick = 0;
	int cur_ticks, cur_frame_duration;

	// draws little dots on the bottom of the screen
	if (devparm)
		ST_DrawFps(last_fps);

	if (sysvideo.overlay) {
		SDL_Rect ov_rect;
		int dstw = (screen->h * SCREENWIDTH) / SCREENHEIGHT;
		int dsth = (screen->w * SCREENHEIGHT) / SCREENWIDTH;

		if (dstw > screen->w) {
			dstw = screen->w;
		} else if (dsth > screen->h) {
			dsth = screen->h;
		}

		if (screen->w > dstw) {
			ov_rect.x = (screen->w - dstw)>>1;
			ov_rect.y = 0;
			ov_rect.w = dstw;
			ov_rect.h = screen->h;
		} else {
			ov_rect.x = 0;
			ov_rect.y = (screen->h - dsth)>>1;
			ov_rect.w = screen->w;
			ov_rect.h = dsth;
		}

                switch (overlay_format)
                {
                    case SDL_YUY2_OVERLAY:
                         I_RGB8toYUY2(shadow, overlay);
                         break;
                    case SDL_YV12_OVERLAY:
                         I_RGB8toYV12(shadow, overlay);
                         break;
                    case SDL_UYVY_OVERLAY:
                         I_RGB8toUYVY(shadow, overlay);
                         break;
                    case SDL_YVYU_OVERLAY:
                         I_RGB8toYVYU(shadow, overlay);
                         break;
                    case SDL_IYUV_OVERLAY:
                         I_RGB8toIYUV(shadow, overlay);
                         break;
                }

                SDL_DisplayYUVOverlay(overlay, &ov_rect);
	} else {
		if (SDL_MUSTLOCK(screen)) {
			SDL_UnlockSurface(screen);
		}

		if (shadow) {
			SDL_BlitSurface(shadow, NULL, screen, &update_area);
		}
		SDL_Flip(screen);

		if (SDL_MUSTLOCK(screen)) {
			SDL_LockSurface(screen);
		}

		if (screen->flags & SDL_DOUBLEBUF) {
			if (!shadow) {
				screens[0] = screen->pixels;
				screens[0] += update_area.y * screen->pitch;
				screens[0] += update_area.x;

				R_InitBuffer (scaledviewwidth, viewheight);
				AM_SetViewSize();
			}
		}
	}

	fps++;
	frame_end_tick = cur_ticks = SDL_GetTicks();
	if (cur_ticks-frame_tick>1000) {
		frame_tick=cur_ticks;
		last_fps=fps;
		fps=0;
	}

	cur_frame_duration = frame_end_tick - frame_start_tick;
	if (cur_frame_duration < FRAME_DURATION) {
		int wait_duration = FRAME_DURATION - cur_frame_duration - 1;
		if (wait_duration>0) {
			SDL_Delay(wait_duration);
		}
	}

	if (new_width && new_height) {
		if (!sysvideo.overlay && SDL_MUSTLOCK(screen)) {
			SDL_UnlockSurface(screen);
		}

		InitSdlMode(new_width, new_height, sysvideo.bpp);
		new_width = new_height = 0;
	}

	frame_start_tick = frame_end_tick;
}

//
// I_ReadScreen
//
void I_ReadScreen (byte* scr)
{
	Uint8 *src, *dest;
	int y;

	src = (Uint8 *)screens[0];
	dest = (Uint8 *)scr;

	for (y=0;y<sysvideo.height;y++) {
		memcpy (dest, src, sysvideo.width);
		src += sysvideo.pitch;
		dest += sysvideo.width;
	}
}

/* Mouse stuff, when pausing/unpausing game */

void I_GrabMouse(void)
{
	if (screen->flags & SDL_FULLSCREEN)
		return;

	SDL_WM_GrabInput(mouse_grab);
	if (mouse_grab == SDL_GRAB_ON) {
		SDL_ShowCursor(SDL_DISABLE);
	}
}

void I_UngrabMouse(void)
{
	if (screen->flags & SDL_FULLSCREEN)
		return;

	SDL_WM_GrabInput(SDL_GRAB_OFF);
	SDL_ShowCursor(SDL_ENABLE);
}

//
// I_SetPalette
//
void I_SetPalette (byte* palette)
{
	int i;

	if (palette) {
		for (i=0; i<256; i++) {
			colors[i].r = gammatable[usegamma][*palette++];
			colors[i].g = gammatable[usegamma][*palette++];
			colors[i].b = gammatable[usegamma][*palette++];
		}
	}

	if (shadow)
		SDL_SetColors(shadow, colors, 0,256);
	if (sysvideo.overlay) {
		I_Pal2Yuv(colors);
	}
	SDL_SetPalette(screen, SDL_LOGPAL|SDL_PHYSPAL, colors, 0, 256);
}

static void InitSdlMode(int width, int height, int bpp)
{
	SDL_Surface *output_surf;

	if (bpp<8)
		bpp=8;

	/* Check for minimal size */
	if (width<SCREENWIDTH)
		width=SCREENWIDTH;
	if (height<SCREENHEIGHT)
		height=SCREENHEIGHT;

	/* Check for biggest fullscreen size */
	if (scr_flags & SDL_FULLSCREEN) {
		SDL_Rect **modes;

		modes = SDL_ListModes(NULL, SDL_FULLSCREEN);
		if ((modes!=(SDL_Rect **)0) && (modes!=(SDL_Rect **)-1)) {
			int maxx=0, maxy=0, i;

			for (i=0; modes[i]; ++i) {
				if ((modes[i]->w > maxx) && (modes[i]->h > maxy)) {
					maxx = modes[i]->w;
					maxy = modes[i]->h;
				}
			}

			if ((width>maxx) || (height>maxy)) {
				width=maxx;
				height=maxy;
			}
		}
	}

	screen=SDL_SetVideoMode(width, height, bpp, scr_flags);
	if (!screen)
	    I_Error("Could not open display: %s\n", SDL_GetError());

	if (screen->flags & SDL_FULLSCREEN) {
		SDL_ShowCursor(SDL_DISABLE);
		mouse_grab = SDL_GRAB_ON;
	} else {
		SDL_ShowCursor(SDL_ENABLE);
		/* Restore previous grabbing state */
		mouse_grab = SDL_WM_GrabInput(mouse_grab);
	}

	output_surf = screen;

	if (sysvideo.overlay) {
		/* Create/refresh SDL overlay */
		if (!overlay)
			overlay=SDL_CreateYUVOverlay(SCREENWIDTH, SCREENHEIGHT, overlay_format, screen);
		if (!overlay) {
			I_Error("Can not create overlay: %s\n", SDL_GetError());
			sysvideo.overlay = false;
		}

		if (sysvideo.overlay) {
			if (!shadow)
				shadow = SDL_CreateRGBSurface(SDL_SWSURFACE,SCREENWIDTH,SCREENHEIGHT,8,0,0,0,0);
			if (!shadow)
			    I_Error("Can not create shadow surface: %s\n", SDL_GetError());

		 	output_surf = shadow;
		}
	}

	if (!sysvideo.overlay) {
		if (screen->format->BitsPerPixel==8) {
			if (shadow) {
				SDL_FreeSurface(shadow);
				shadow=NULL;
			}
		} else {
			if (!shadow)
				shadow = SDL_CreateRGBSurface(SDL_SWSURFACE,screen->w,screen->h,8,0,0,0,0);
			if (!shadow)
			    I_Error("Can not create shadow surface: %s\n", SDL_GetError());

			output_surf = shadow;
		}

		if (SDL_MUSTLOCK(screen)) {
			SDL_LockSurface(screen);
		}
	}

	/* Preserve aspect ratio */
	{
		int srcw = output_surf->w;
		int srch = output_surf->h;
		int dstw = (srch * SCREENWIDTH) / SCREENHEIGHT;
		int dsth = (srcw * SCREENHEIGHT) / SCREENWIDTH;

		if (dstw > srcw) {
			dstw = srcw;
		} else if (dsth > srch) {
			dsth = srch;
		}

		if (srcw > dstw) {
			update_area.x = (srcw - dstw)>>1;
			update_area.y = 0;
			update_area.w = dstw;
			update_area.h = srch;
		} else {
			update_area.x = 0;
			update_area.y = (srch - dsth)>>1;
			update_area.w = srcw;
			update_area.h = dsth;
		}

	 	screens[0] = output_surf->pixels;
		if (output_surf!=shadow) {
			screens[0] += update_area.y * output_surf->pitch;
			screens[0] += update_area.x;
		}

		sysvideo.width = update_area.w;
		sysvideo.height = update_area.h;
		sysvideo.pitch = output_surf->pitch;
		sysvideo.bpp = output_surf->format->BitsPerPixel;
	}

	I_SetPalette(NULL);

	/* Reset Doom engine */
	V_Init();
	R_InitPlanes();
	R_InitSpritesData();
	ST_SetNumRefresh(screen->flags & SDL_DOUBLEBUF ? 2 : 1);
	ST_Start();
	AM_SetViewSize();
	R_ExecuteSetViewSize();
	R_FillBackScreen();
	R_DrawViewBorder();
}

void I_InitGraphics(void)
{
	static int firsttime=1;
	int i;

	if (!firsttime)
		return;
	firsttime = 0;

	scr_flags = SDL_HWSURFACE|SDL_HWPALETTE;
	if (sysvideo.doublebuf)
		scr_flags |= SDL_DOUBLEBUF;
	if (sysvideo.fullscreen)
		scr_flags |= SDL_FULLSCREEN;
	if (sysvideo.resize)
		scr_flags |= SDL_RESIZABLE;
	if (sysvideo.overlay && (sysvideo.bpp==8))
		sysvideo.bpp = 32;

	InitSdlMode(sysvideo.width, sysvideo.height, sysvideo.bpp);

	SDL_WM_SetCaption(PACKAGE_STRING, PACKAGE_NAME);

	/* Joystick stuff */
	joystick = SDL_JoystickOpen(num_joystick);
	if (joystick==NULL) {
		fprintf(stderr, "Can not open joystick %d\n", num_joystick);
		for (i=0; i<SDL_NumJoysticks(); i++) {
			joystick=SDL_JoystickOpen(i);
			if (joystick) {
				num_joystick = SDL_JoystickIndex(joystick);
				break;
			}
		}
	}

	if (joystick!=NULL) {
		const char *name;

		name = SDL_JoystickName(num_joystick);
		printf("Opened joystick %d: %s\n", num_joystick, name);
 	}

	frame_tick=SDL_GetTicks();
}
