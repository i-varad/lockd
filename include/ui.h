#ifndef LOCKD_UI_H
#define LOCKD_UI_H


#include <X11/Xlib.h>

/*
    Draw the initial lock screen into the supplied window.

    The caller owns the window and graphics context.
*/
void ui_draw(Display *display, Window window, GC gc, int screen);


/*
    Draw the lock-screen contents into the supplied window.
*/

void ui_init(Display *display, Window window, int screen);



#endif