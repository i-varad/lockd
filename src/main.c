#include <stdio.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>

#include "input.h"
#include "ui.h"

/*
    READY : no input grabs are held by lockd.
    GRABBED : both keyboard and pointer grabs succeeded.
*/
enum lock_state
{
    READY,
    GRABBED
};



static const char *lock_state_name(enum lock_state state)
{

    switch (state)
    {
    case READY:
        return "READY";

    case GRABBED:
        return "GRABBED";

    default:
        return "UNKNOWN";
    }
}



int main()
{

    /*
        Display *display;
        is an opaque Xlib data structure representing connection to X server.
    */
    Display *display;

    /*
        Opens a connection to the X server.
        Passing NULL makes xlib use the display from the environment
    */
    display = XOpenDisplay(NULL);

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

    /*
        Obtain the default screen and its root window.
        The root window is the top-level parent for ordinary X11 windows on this screen
    */
    int screen = DefaultScreen(display);
    Window root = RootWindow(display, screen);

    printf("  screen: %d\n", screen);
    printf("  root window: 0x%lx\n", root);

    int width = DisplayWidth(display, screen);
    int height = DisplayHeight(display, screen);

    printf("  width: %d\n", width);
    printf("  height: %d\n", height);

    /*
        Create a window covering the screen dimentions
        This is currently a basic test window; lock-screen rendering
        and window-manager integration will be added later.
    */
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



    /* Create the graphics context used by the UI for drawing */
    GC gc = XCreateGC(display, window, 0, NULL);


    /*
        Subscribe to the events this window needs to observe
        StructureNotifyMask allows lockd to recieve notification about
        changes to the window's structure, including mapping.
    */
    XSelectInput(
        display,
        window,
        ButtonPressMask | 
        KeyPressMask | 
        KeyReleaseMask | 
        StructureNotifyMask |
        ExposureMask    
    );

    /* Requesting for X server to make this window visible */
    XMapWindow(display, window);

    /* Send buffered Xlib requests to the server without waiting for a reply */
    XFlush(display);

    /* enum state */
    enum lock_state state = READY;

    int running = 1;

    /*
        Process X11 events until shutdown is requested.
        The current development controls are:
            g - attempt to acquire keyboard and pointer grabs
            u - release grabs
            q - exit the program
    */
    while (running)
    {

        XEvent event;
        XNextEvent(display, &event);

        if (event.type == MapNotify)
        {
            printf("lockd: MapNotify event=0x%lx window=0x%lx override_redirect=%d\n",
                   event.xmap.event,
                   event.xmap.window,
                   event.xmap.override_redirect);

            /*
                wait until the window has been mapped before requesting input focus
                requesting focus too early caused BadMatch during earlier tests.
            */
            XSetInputFocus(
                display,
                window,
                RevertToParent,
                CurrentTime);

            ui_draw(display, window, gc, screen);

            XFlush(display);
        }


        if (event.type == UnmapNotify)
        {
            printf(
                "lockd: UnmapNotify event=0x%lx window=0x%lx from_configure=%d\n",
                event.xunmap.event,
                event.xunmap.window,
                event.xunmap.from_configure);
        }


        if (event.type == ReparentNotify)
        {
            printf(
                "lockd: ReparentNotify event=0x%lx window=0x%lx parent=0x%lx\n",
                event.xreparent.event,
                event.xreparent.window,
                event.xreparent.parent);
        }


        if (event.type == MotionNotify)
        {
            printf(
                "lockd: MotionNotify x=%d y=%d\n",
                event.xmotion.x,
                event.xmotion.y);
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
                NULL);

            buffer[length] = '\0';

            const char *keysym_name = XKeysymToString(keysym);
           
            printf(
                "lockd: KeyPress keycode=%u\n keysym=%s bytes=%d text=\"%s\"\n",
                event.xkey.keycode,
                keysym_name != NULL ? keysym_name : "Unknown",
                length,
                buffer);


            if (keysym == XK_q)
            {
                printf("lockd: shutdown requested\n");
                running = 0;
            }



            /*
                acquire input only when lockd is not already holding the grabs
            */
            if (keysym == XK_g && state == READY)
            {
                if (acquire_input(display, window))
                {
                    state = GRABBED;
                    printf("lockd: state=%s\n", lock_state_name(state));
                }
                else
                {
                    state = READY;
                    printf("lockd: state=%s\n", lock_state_name(state));
                }
            }



            /* release input only when both grabs were acquired successfully */
            if (keysym == XK_u && state == GRABBED)
            {
                release_input(display);
                state = READY;
                printf("lockd: state=%s\n", lock_state_name(state));
            }

        }



        if (event.type == KeyRelease)
        {
            printf(
                "lockd: KeyRelease keycode=%u\n",
                event.xkey.keycode);
        }



        if (event.type == DestroyNotify)
        {
            printf(
                "lockd: DestroyNotify window=0x%lx\n",
                event.xdestroywindow.window);

            running = 0;
        }



        /* 
            Redraw the UI when the X server reports that part of 
            the window needs to be repainted.
        */
       if(event.type == Expose && event.xexpose.count == 0){
            ui_draw(display, window, gc, screen);
            XFlush(display);
       }

    }




    /*
        Ensure no input grabs remain active when the program exits.
        destroy the test window and close the connection to X server.
    */
    if (state == GRABBED)
    {
        release_input(display);
        state = READY;
        printf("lockd: state=%s\n", lock_state_name(state));
    }


    /* releasing the graphics context */
    XFreeGC(display, gc);

    XDestroyWindow(display, window);
    XCloseDisplay(display);


    printf("lockd: shutdown complete\n");
    return 0;
}