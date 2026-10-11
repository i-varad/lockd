#ifndef LOCKD_UI_H
#define LOCKD_UI_H

#include <X11/Xlib.h>
#include <X11/extensions/Xinerama.h>

/*
    Draw the lock-screen contents into the supplied window.
    The caller owns the window and graphics context.
*/
void ui_draw(Display *display, Window window, GC gc, int screen,
             const XineramaScreenInfo *monitor);

/*
    Initialize the lock-screen UI and its fonts.
*/
void ui_init(Display *display, Window window, int screen);

/*
    Release UI resources.
*/
void ui_cleanup(Display *display);

#endif