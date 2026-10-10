#include <string.h>
#include <X11/Xlib.h>

#include "ui.h"

/*
    configure the window's background before the first draw.
*/

void ui_init(Display *display, Window window, int screen)
{

    XSetWindowBackground(
        display,
        window,
        BlackPixel(display, screen));
}

/*
    Draw the lockscreen content onto the supplied window

    This function may be called repeatedly when the window needs to be repainted
*/
void ui_draw(Display *display, Window window, GC gc, int screen)
{

    /* Clear the window using its configured background color. */
    XClearWindow(display, window);

    /* Draw the initial label in white. */
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
