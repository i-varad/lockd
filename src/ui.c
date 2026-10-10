#include <string.h>
#include <X11/Xlib.h>

#include "ui.h"


/*
    Draw the initial lock-screen contents into an existing window.
 
    The caller owns the X11 connection, window, and graphics context.
 */

void ui_draw(Display *display, Window window, GC gc, int screen){

    /* Clear the window using the screen's black pixel. */
    XSetWindowBackground(display, window, BlackPixel(display, screen));
    XClearWindow(display, window);

    /* Draw the initial label in white */
    XSetForeground(display, gc, WhitePixel(display, screen));

    const char *message = "lockd - screen locker development";

    XDrawString(
        display, 
        window, 
        gc, 
        40, 
        60, 
        message, 
        (int)strlen(message));
}