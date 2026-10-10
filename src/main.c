#include <stdio.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>



/* keyboard and pointer aquisition handler */
static int acquire_input(Display *display, Window window){
        int status;

        status = XGrabKeyboard(
            display,
            window,
            False,
            GrabModeAsync,
            GrabModeAsync,
            CurrentTime
        );

        if(status != GrabSuccess){
            fprintf(
                stderr, "lockd: keyboard grab failed, status=%d\n",
                status
            );

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
            CurrentTime
        );

        if(status != GrabSuccess){
            fprintf(
                stderr,
                "lockd: pointer grab failed, status=%d\n",
                status
            );

            XUngrabKeyboard(display, CurrentTime);
            XFlush(display);

            printf("lockd: keyboard grab rolled back\n");
            return 0;
        }


        printf("lockd: pointer grab successful\n");
        return 1;
}



/* release inputs */
static void release_input(Display *display){
    XUngrabPointer(display, CurrentTime);
    XUngrabKeyboard(display, CurrentTime);

    XFlush(display);


    printf("lockd: input grabs released\n");
}


int main()
{

    /*
    Display *display;
    is an opaque Xlib data structure representing connection to X server.
    */
    Display *display;

    display = XOpenDisplay(NULL);

    // confirming if connection to X server is established or not.
    if (display == NULL)
    {
        fprintf(stderr, "lockd: failed to open X display\n");
        return 1;
    }

    printf("lockd: connected to X server\n");

    /* Name of the X11 display we are connected to. */
    printf("  display: %s\n", DisplayString(display));

    /* Vendor/implementation of the X11 server. */
    printf("  vendor: %s\n", ServerVendor(display));

    /* Major and revision number of the X11 protocol. */
    printf("  protocol: %d.%d\n",
           ProtocolVersion(display),
           ProtocolRevision(display));

    int screen = DefaultScreen(display);
    Window root = RootWindow(display, screen);

    printf("  screen: %d\n", screen);
    printf("  root window: 0x%lx\n", root);

    int width = DisplayWidth(display, screen);
    int height = DisplayHeight(display, screen);

    printf("  width: %d\n", width);
    printf("  height: %d\n", height);

    /* Creating first Window */

    Window window = XCreateSimpleWindow(
        display,
        root,
        0,
        0,
        width,
        height,
        0,
        0,
        0);

    XSelectInput(
        display,
        window,
        ButtonPressMask | KeyPressMask | KeyReleaseMask | StructureNotifyMask);

    /* Calling X server to map the window */
    XMapWindow(display, window);

    /* flushing the requests from the buffer */
    XFlush(display);

    // XSetInputFocus(
    //     display,
    //     window,
    //     RevertToParent,
    //     CurrentTime
    // );

    // XFlush(display);

    /* selecting the events which should be delivered to this window */

    /* Testing window we just created */


    /* State Variable for keyboard and mouse inputs */
    int input_grabbed = 0;

    /*Event Loop*/
    for (;;)
    {
        XEvent event;

        XNextEvent(display, &event);

        if (event.type == MapNotify)
        {
            printf("lockd: window mapped\n");

            XSetInputFocus(
                display,
                window,
                RevertToParent,
                CurrentTime);

            XFlush(display);
        }

        if(event.type == MotionNotify){
            printf(
                "lockd: MotionNotify x=%d y=%d\n",
                event.xmotion.x,
                event.xmotion.y 
            );
        }
        
        if (event.type == ButtonPress)
        {
            printf(
                "lockd: ButtonPress button=%u x=%d y=%d\n",
                event.xbutton.button,
                event.xbutton.x,
                event.xbutton.y);
        }

        if (event.type == KeyPress)
        {
            
            KeySym keysym;
            char buffer[32];
            int length;

            length = XLookupString(
                &event.xkey,
                buffer,
                sizeof(buffer) - 1,
                &keysym,
                NULL
            );
 
            buffer[length] = '\0';

            printf(
                "lockd: KeyPress keycode=%u\n keysym=%s bytes=%d text=\"%s\"\n",
                event.xkey.keycode, 
                XKeysymToString(keysym) != NULL ? XKeysymToString(keysym) : "Unknown",
                length,
                buffer
            );  


            /* inputs aquisition handlers implementation */

            if(keysym == XK_g && !input_grabbed){
                input_grabbed = acquire_input(display, window);
            }

            if(keysym == XK_u && input_grabbed){
                release_input(display);
                input_grabbed = 0;
            }

            /* inputs aquisition handlers implementation end*/


        }

        if (event.type == KeyRelease)
        {
            printf(
                "lockd: KeyRelease keycode=%u\n",
                event.xkey.keycode);
        }
    }

    // XCloseDisplay(display);

    return 0;
}