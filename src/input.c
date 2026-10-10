#include <stdio.h>
#include <X11/Xlib.h>

#include "input.h"

/*
    Acquire keyboard and pointer input for the specified window.
 
    If the pointer grab fails after the keyboard grab succeeds,
    release the keyboard grab to avoid a partially acquired state.
 */
int acquire_input(Display *display, Window window)
{
    int status;

    status = XGrabKeyboard(
        display,
        window,
        False,
        GrabModeAsync,
        GrabModeAsync,
        CurrentTime);

    if (status != GrabSuccess)
    {
        fprintf(
            stderr,
            "lockd: keyboard grab failed, status=%d\n",
            status);

        return 0;
    }

    printf("lockd: keyboard grab successful\n");

    status = XGrabPointer(
        display,
        window,
        False,
        ButtonPressMask | ButtonReleaseMask | PointerMotionMask,
        GrabModeAsync,
        GrabModeAsync,
        None,
        None,
        CurrentTime);

    if (status != GrabSuccess)
    {
        fprintf(
            stderr,
            "lockd: pointer grab failed, status=%d\n",
            status);

        /* Roll back the successful keyboard grab. */
        XUngrabKeyboard(display, CurrentTime);
        XFlush(display);

        printf("lockd: keyboard grab rolled back\n");
        return 0;
    }

    printf("lockd: pointer grab successful\n");
    return 1;
}

/*
    Release keyboard and pointer grabs, then flush the requests
    so they are sent to the X server.
 */


void release_input(Display *display)
{
    XUngrabPointer(display, CurrentTime);
    XUngrabKeyboard(display, CurrentTime);

    XFlush(display);

    printf("lockd: input grabs released\n");
}