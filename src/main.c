#include <stdio.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>


enum lock_state {
    READY,
    GRABBED,
    ERROR
};


static const char *lock_state_name(enum lock_state state){
    
    switch (state)
    {
    case READY :
        return "READY";
        
    case GRABBED:
        return "GRABBED";
    
    default:
        return "UNKNOWN";
    }

}


/* keyboard and pointer aquisition handler */
static int acquire_input(Display *display, Window window)
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
            stderr, "lockd: keyboard grab failed, status=%d\n",
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

        XUngrabKeyboard(display, CurrentTime);
        XFlush(display);

        printf("lockd: keyboard grab rolled back\n");
        return 0;
    }

    printf("lockd: pointer grab successful\n");
    return 1;
}

/* release inputs */
static void release_input(Display *display)
{
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
    enum lock_state state = READY;

    // added while testing
    int running = 1;

    /*Event Loop*/
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

            XSetInputFocus(
                display,
                window,
                RevertToParent,
                CurrentTime);

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

            printf(
                "lockd: KeyPress keycode=%u\n keysym=%s bytes=%d text=\"%s\"\n",
                event.xkey.keycode,
                XKeysymToString(keysym) != NULL ? XKeysymToString(keysym) : "Unknown",
                length,
                buffer);

            /* testing */

            if (keysym == XK_q)
            {
                printf("lockd: shutdown requested\n");
                running = 0;
            }

            /* inputs aquisition / grab handlers implementation */

            if (keysym == XK_g && state == READY)
            {
                if(acquire_input(display, window)){
                    state = GRABBED;
                    printf("lockd: state=%s\n", lock_state_name(state));
                } else {
                    state = READY;
                    printf("lockd: state=%s\n", lock_state_name(state));

                }
            }

            if (keysym == XK_u && state == GRABBED)
            {
                release_input(display);
                state = READY;
                printf("lockd: state=%s\n", lock_state_name(state));

            }

            /* inputs aquisition handlers implementation end*/
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
    }

    if (state == GRABBED)
    {
        release_input(display);
        state = READY;
        printf("lockd: state=%s\n", lock_state_name(state));

    }

    XDestroyWindow(display, window);
    XCloseDisplay(display);

    printf("lockd: shutdown complete\n");

    // XCloseDisplay(display);

    return 0;
}