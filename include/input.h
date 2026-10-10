#ifndef LOCKD_INPUT_H
#define LOCKD_INPUT_H

#include <X11/Xlib.h>

/*
    Acquire keyboard and pointer input as one logical operation.

    X11 acquires these separately.
    If keyboard acquisition succeeded and pointer acquisition failed,
    In that case, release the keyboard grab before returning the failuer
    to avoid a partially grabbed acquired state.

    Returns 1 if both grabs succeed, or 0 otherwise.
*/

int acquire_input(Display *display, Window window);



/*
    Release both input grabs and flush the request to X server.
    Used when the user unlocks the prototype or during shutdown.
*/
void release_input(Display *display);


#endif