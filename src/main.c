
#include <stdio.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <X11/extensions/Xinerama.h>

#include "input.h"
#include "ui.h"

enum lock_state
{
    READY,
    GRABBED
};

struct monitor_window
{
    Window window;
    GC gc;
    XineramaScreenInfo monitor;
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

static int find_window(
    const struct monitor_window *windows,
    int count,
    Window window)
{
    for (int i = 0; i < count; i++)
    {
        if (windows[i].window == window)
        {
            return i;
        }
    }

    return -1;
}

int main(void)
{
    Display *display = XOpenDisplay(NULL);

    if (display == NULL)
    {
        fprintf(stderr, "lockd: failed to open X display\n");
        return 1;
    }

    int screen = DefaultScreen(display);
    Window root = RootWindow(display, screen);

    printf("lockd: connected to X server %s\n",
           DisplayString(display));

    printf("lockd: desktop dimensions %dx%d\n",
           DisplayWidth(display, screen),
           DisplayHeight(display, screen));

    XineramaScreenInfo fallback_monitor;
    XineramaScreenInfo *detected_monitors = NULL;
    int monitor_count = 0;

    if (XineramaIsActive(display))
    {
        detected_monitors = XineramaQueryScreens(
            display, &monitor_count);
    }

    if (detected_monitors == NULL || monitor_count == 0)
    {
        fallback_monitor.screen_number = screen;
        fallback_monitor.x_org = 0;
        fallback_monitor.y_org = 0;
        fallback_monitor.width = DisplayWidth(display, screen);
        fallback_monitor.height = DisplayHeight(display, screen);

        monitor_count = 1;
        detected_monitors = &fallback_monitor;

        printf("lockd: using single-screen fallback\n");
    }

    printf("lockd: detected %d monitor(s)\n", monitor_count);

    struct monitor_window *windows = calloc(
        (size_t)monitor_count, sizeof(*windows));

    if (windows == NULL)
    {
        fprintf(stderr, "lockd: failed to allocate monitor windows\n");

        if (detected_monitors != &fallback_monitor)
        {
            XFree(detected_monitors);
        }

        XCloseDisplay(display);
        return 1;
    }

    XSetWindowAttributes attributes = {0};

    attributes.override_redirect = True;
    attributes.background_pixel = BlackPixel(display, screen);
    attributes.event_mask =
        ButtonPressMask |
        KeyPressMask |
        KeyReleaseMask |
        StructureNotifyMask |
        ExposureMask;

    int created = 0;

    for (int i = 0; i < monitor_count; i++)
    {
        windows[i].monitor = detected_monitors[i];

        XineramaScreenInfo *monitor = &windows[i].monitor;

        printf(
            "lockd: monitor %d: x=%d y=%d width=%d height=%d\n",
            i,
            monitor->x_org,
            monitor->y_org,
            monitor->width,
            monitor->height);

        windows[i].window = XCreateWindow(
            display,
            root,
            monitor->x_org,
            monitor->y_org,
            (unsigned int)monitor->width,
            (unsigned int)monitor->height,
            0,
            CopyFromParent,
            InputOutput,
            CopyFromParent,
            CWOverrideRedirect | CWBackPixel | CWEventMask,
            &attributes);

        if (windows[i].window == None)
        {
            fprintf(stderr,
                    "lockd: failed to create window for monitor %d\n",
                    i);
            break;
        }

        windows[i].gc = XCreateGC(
            display, windows[i].window, 0, NULL);

        if (windows[i].gc == NULL)
        {
            fprintf(stderr,
                    "lockd: failed to create graphics context\n");

            XDestroyWindow(display, windows[i].window);
            windows[i].window = None;
            break;
        }

        ui_init(display, windows[i].window, screen);

        XMapRaised(display, windows[i].window);
        created++;
    }

    if (detected_monitors != &fallback_monitor)
    {
        XFree(detected_monitors);
    }

    if (created != monitor_count)
    {
        for (int i = 0; i < created; i++)
        {
            XFreeGC(display, windows[i].gc);
            XDestroyWindow(display, windows[i].window);
        }

        if (created > 0)
        {
            ui_cleanup(display);
        }

        free(windows);
        XCloseDisplay(display);
        return 1;
    }

    XFlush(display);

    enum lock_state state = READY;
    int running = 1;

    /*
     * Focus one window for development keyboard controls.
     * Input grabs will be revisited for the multi-window design.
     */
    XSetInputFocus(
        display,
        windows[0].window,
        RevertToParent,
        CurrentTime);

    while (running)
    {
        XEvent event;
        XNextEvent(display, &event);

        int index = find_window(
            windows, monitor_count, event.xany.window);

        if (index < 0)
        {
            continue;
        }

        Window window = windows[index].window;
        GC gc = windows[index].gc;
        XineramaScreenInfo *monitor = &windows[index].monitor;

        if (event.type == MapNotify)
        {
            printf("lockd: monitor %d window mapped: 0x%lx\n",
                   index, window);

            ui_draw(display, window, gc, screen, monitor);
            XFlush(display);
        }
        else if (event.type == Expose && event.xexpose.count == 0)
        {
            ui_draw(display, window, gc, screen, monitor);
            XFlush(display);
        }
        else if (event.type == ButtonPress)
        {
            printf("lockd: monitor %d ButtonPress button=%u\n",
                   index, event.xbutton.button);
        }
        else if (event.type == KeyPress)
        {
            KeySym keysym;
            char buffer[32];

            int length = XLookupString(
                &event.xkey,
                buffer,
                sizeof(buffer) - 1,
                &keysym,
                NULL);

            buffer[length] = '\0';

            const char *name = XKeysymToString(keysym);

            printf("lockd: KeyPress keysym=%s text=\"%s\"\n",
                   name != NULL ? name : "Unknown",
                   buffer);

            if (keysym == XK_q)
            {
                printf("lockd: shutdown requested\n");
                running = 0;
            }
            else if (keysym == XK_g && state == READY)
            {
                if (acquire_input(display, window))
                {
                    state = GRABBED;
                }

                printf("lockd: state=%s\n",
                       lock_state_name(state));
            }
            else if (keysym == XK_u && state == GRABBED)
            {
                release_input(display);
                state = READY;

                printf("lockd: state=%s\n",
                       lock_state_name(state));
            }
        }
        else if (event.type == KeyRelease)
        {
            printf("lockd: KeyRelease keycode=%u\n",
                   event.xkey.keycode);
        }
        else if (event.type == UnmapNotify)
        {
            printf("lockd: monitor %d window unmapped\n", index);
        }
        else if (event.type == DestroyNotify)
        {
            printf("lockd: monitor %d window destroyed\n", index);
            running = 0;
        }
    }

    if (state == GRABBED)
    {
        release_input(display);
        state = READY;

        printf("lockd: state=%s\n",
               lock_state_name(state));
    }

    for (int i = 0; i < monitor_count; i++)
    {
        XFreeGC(display, windows[i].gc);
        XDestroyWindow(display, windows[i].window);
    }

    ui_cleanup(display);

    XCloseDisplay(display);
    free(windows);

    printf("lockd: shutdown complete\n");
    return 0;
}
