
#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <poll.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <X11/extensions/Xinerama.h>

#include "auth.h"
#include "input.h"
#include "ui.h"

#define PASSWORD_SIZE 256
#define USERNAME_SIZE 256

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

static char password[PASSWORD_SIZE];
static size_t password_length;

static const char *auth_status = "Enter password and press Enter";

/*
    Clear the password buffer using volatile writes so the
    compiler cannot optimize away the clearing operations.
*/
static void clear_password(void)
{
    volatile char *p = password;

    for (size_t i = 0; i < sizeof(password); i++)
    {
        p[i] = '\0';
    }

    password_length = 0;
}

/*
    Find the monitor window associated with an X11 event.
*/
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

/*
    Redraw the lock-screen UI on every monitor.
*/
static void redraw_all(
    Display *display,
    struct monitor_window *windows,
    int monitor_count,
    int screen)
{
    for (int i = 0; i < monitor_count; i++)
    {
        ui_draw(
            display,
            windows[i].window,
            windows[i].gc,
            screen,
            &windows[i].monitor,
            password_length,
            auth_status);
    }

    XFlush(display);
}

int main(void)
{
    int exit_status = 1;
    int created = 0;
    int input_grabbed = 0;
    int ui_initialized = 0;
    int running = 1;

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

    /*
        Determine the current account once at startup.
        Copy the username because getpwuid() returns
        library-managed storage.
    */
    struct passwd *account = getpwuid(getuid());

    if (account == NULL || account->pw_name == NULL)
    {
        fprintf(stderr, "lockd: failed to determine current user\n");
        goto cleanup_display;
    }

    char username[USERNAME_SIZE];

    int username_length = snprintf(
        username,
        sizeof(username),
        "%s",
        account->pw_name);

    if (username_length < 0 ||
        (size_t)username_length >= sizeof(username))
    {
        fprintf(stderr, "lockd: username is too long\n");
        goto cleanup_display;
    }

    /*
        Detect monitors using Xinerama.
        Fall back to the default X screen if unavailable.
    */
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

        goto cleanup_display;
    }

    /*
        Create one black override-redirect window per monitor.
        These windows cover the desktop and receive X11 events.
    */
    XSetWindowAttributes attributes = {0};

    attributes.override_redirect = True;
    attributes.background_pixel = BlackPixel(display, screen);
    attributes.event_mask =
        ButtonPressMask |
        KeyPressMask |
        KeyReleaseMask |
        StructureNotifyMask |
        ExposureMask;

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
            fprintf(
                stderr,
                "lockd: failed to create window for monitor %d\n",
                i);
            break;
        }

        windows[i].gc = XCreateGC(
            display, windows[i].window, 0, NULL);

        if (windows[i].gc == NULL)
        {
            fprintf(
                stderr,
                "lockd: failed to create graphics context\n");

            XDestroyWindow(display, windows[i].window);
            windows[i].window = None;
            break;
        }

        ui_init(display, windows[i].window, screen);
        ui_initialized = 1;

        XMapRaised(display, windows[i].window);
        created++;
    }

    if (detected_monitors != &fallback_monitor)
    {
        XFree(detected_monitors);
        detected_monitors = NULL;
    }

    /*
        Do not continue if even one monitor window failed.
    */
    if (created != monitor_count)
    {
        fprintf(stderr, "lockd: failed to initialize all monitor windows\n");
        goto cleanup_windows;
    }

    XSync(display, False);

    /*
        Focus the first monitor window and acquire both input grabs.

        If either grab fails, do not continue into the password
        event loop. The application cannot safely act as a locker
        without successfully acquiring both.
    */
    XSetInputFocus(
        display,
        windows[0].window,
        RevertToParent,
        CurrentTime);

    XSync(display, False);

    if (!acquire_input(display, windows[0].window))
    {
        fprintf(stderr, "lockd: unable to acquire input grabs\n");
        goto cleanup_windows;
    }

    input_grabbed = 1;

    enum lock_state state = GRABBED;

    printf("lockd: input grabbed; waiting for authentication\n");

    redraw_all(display, windows, monitor_count, screen);

    time_t last_clock_minute = time(NULL) / 60;

    /*
        Main X11 event loop.

        The poll timeout allows the clock to refresh while the
        application is idle.
    */
    while (running)
    {
        struct pollfd x11_fd = {
            .fd = ConnectionNumber(display),
            .events = POLLIN
        };

        int poll_result = poll(&x11_fd, 1, 1000);

        if (poll_result < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }

            perror("lockd: poll");
            break;
        }

        if (poll_result > 0 &&
            (x11_fd.revents & (POLLERR | POLLHUP | POLLNVAL)))
        {
            fprintf(stderr, "lockd: X connection failed\n");
            break;
        }

        if (poll_result > 0 && (x11_fd.revents & POLLIN))
        {
            while (XPending(display) > 0 && running)
            {
                XEvent event;
                XNextEvent(display, &event);

                int index = find_window(
                    windows,
                    monitor_count,
                    event.xany.window);

                if (index < 0)
                {
                    continue;
                }

                Window window = windows[index].window;
                GC gc = windows[index].gc;
                XineramaScreenInfo *monitor = &windows[index].monitor;

                if (event.type == MapNotify)
                {
                    ui_draw(
                        display, window, gc, screen, monitor,
                        password_length, auth_status);
                }
                else if (event.type == Expose &&
                         event.xexpose.count == 0)
                {
                    ui_draw(
                        display, window, gc, screen, monitor,
                        password_length, auth_status);
                }
                else if (event.type == KeyPress && state == GRABBED)
                {
                    KeySym keysym;
                    char buffer[32];

                    int length = XLookupString(
                        &event.xkey,
                        buffer,
                        (int)sizeof(buffer),
                        &keysym,
                        NULL);

                    /*
                        Backspace removes the last password byte.
                    */
                    if (keysym == XK_BackSpace)
                    {
                        if (password_length > 0)
                        {
                            password[--password_length] = '\0';
                        }

                        auth_status = "Enter password and press Enter";
                    }
                    /*
                        Enter submits the password to PAM.
                    */
                    else if (keysym == XK_Return ||
                             keysym == XK_KP_Enter)
                    {
                        if (password_length == 0)
                        {
                            auth_status = "Password cannot be empty";
                        }
                        else
                        {
                            auth_status = "Authenticating...";

                            redraw_all(
                                display, windows, monitor_count, screen);

                            /*
                                PAM authentication is synchronous here.
                                No password contents are logged.
                            */
                            int authenticated = auth_user(
                                username, password);

                            clear_password();

                            if (authenticated)
                            {
                                auth_status = "Authentication successful";

                                printf("lockd: authentication successful\n");

                                /*
                                    This prototype exits after successful
                                    authentication. Input grabs are released
                                    during cleanup.
                                */
                                running = 0;
                                exit_status = 0;
                            }
                            else
                            {
                                auth_status = "Authentication failed";

                                printf("lockd: authentication failed\n");
                            }
                        }
                    }
                    /*
                        Accept printable ASCII for this initial version.
                        Unicode input requires XIM/Xutf8LookupString.
                    */
                    else if (length > 0)
                    {
                        for (int i = 0; i < length; i++)
                        {
                            unsigned char ch = (unsigned char)buffer[i];

                            if (ch >= 0x20 && ch <= 0x7e &&
                                password_length < sizeof(password) - 1)
                            {
                                password[password_length++] = (char)ch;
                            }
                        }

                        password[password_length] = '\0';

                        auth_status = "Enter password and press Enter";
                    }

                    redraw_all(
                        display, windows, monitor_count, screen);
                }
                else if (event.type == ButtonPress)
                {
                    /*
                        Ignore pointer input while locked.
                        Never print password-related data.
                    */
                }
                else if (event.type == UnmapNotify)
                {
                    fprintf(
                        stderr,
                        "lockd: monitor %d window was unmapped\n",
                        index);

                    running = 0;
                }
                else if (event.type == DestroyNotify)
                {
                    fprintf(
                        stderr,
                        "lockd: monitor %d window was destroyed\n",
                        index);

                    running = 0;
                }
            }
        }

        /*
            Redraw the UI when the clock's minute changes.
        */
        time_t current_minute = time(NULL) / 60;

        if (current_minute != last_clock_minute)
        {
            last_clock_minute = current_minute;

            redraw_all(
                display, windows, monitor_count, screen);
        }

        XFlush(display);
    }

    /*
        Cleanup always releases the input grabs if they were acquired.
    */
    if (input_grabbed)
    {
        release_input(display);
        input_grabbed = 0;
        state = READY;
    }

    clear_password();

cleanup_windows:
    if (input_grabbed)
    {
        release_input(display);
        input_grabbed = 0;
    }

    if (windows != NULL)
{
    /*
        Destroy Xft drawing resources before destroying their
        associated windows. XftDrawDestroy() releases an
        XRender Picture associated with its drawable.
    */
    if (ui_initialized)
    {
        ui_cleanup(display);
        ui_initialized = 0;
    }

    for (int i = 0; i < created; i++)
    {
        if (windows[i].gc != NULL)
        {
            XFreeGC(display, windows[i].gc);
        }

        if (windows[i].window != None)
        {
            XDestroyWindow(display, windows[i].window);
        }
    }

    free(windows);
}

    XSync(display, False);

cleanup_display:
    clear_password();
    XCloseDisplay(display);

    printf("lockd: shutdown complete\n");
    return exit_status;
}
