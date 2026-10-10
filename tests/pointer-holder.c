#include <stdio.h>
#include <X11/Xlib.h>

int main(void){
    
    Display *display = XOpenDisplay(NULL);

    if(display == NULL){
        fprintf(stderr, "pointer-holder: cannot open display\n");
        return 1;
    }


    int screen = DefaultScreen(display);
    Window root = RootWindow(display, screen);

    Window window = XCreateSimpleWindow(
        display,
        root,
        100,
        100,
        300,
        200,
        0,
        0,
        0
    );

    XSelectInput(display, window, StructureNotifyMask);
    XMapWindow(display, window);
    XFlush(display);


    for(;;){
        
        XEvent event;
        XNextEvent(display, &event);

        if(event.type == MapNotify){
            break;
        }
    }



    int status = XGrabPointer(
        display,
        window,
        False,
        ButtonPressMask |
        ButtonReleaseMask |
        PointerMotionMask,
        GrabModeAsync,
        GrabModeAsync,
        None,
        None,
        CurrentTime
    );



    printf("pointer-holder: grab status =%d\n", status);
    fflush(stdout);

    if(status != GrabSuccess){
        XDestroyWindow(display, window);
        XCloseDisplay(display);

        return 1;
    }

    for(;;){
        XEvent event;
        XNextEvent(display, &event);
    }

}