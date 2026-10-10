
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <X11/Xlib.h>
#include <X11/extensions/Xinerama.h>

#include "ui.h"

static XFontStruct *clock_font;
static XFontStruct *date_font;

void ui_init(Display *display, Window window, int screen)
{
    XSetWindowBackground(
        display,
        window,
        BlackPixel(display, screen));

    clock_font = XLoadQueryFont(display, "9x15");
    date_font = XLoadQueryFont(display, "6x10");

    if (clock_font == NULL){
        clock_font = XLoadQueryFont(display, "9x15");
    }

    if (date_font == NULL){
        date_font = XLoadQueryFont(display, "6x10");
    }

    if (clock_font == NULL || date_font == NULL){
        fprintf(stderr, "lockd: failed to load UI fonts\n");
    }


}

void ui_draw(
    Display *display,
    Window window,
    GC gc,
    int screen,
    const XineramaScreenInfo *monitor)
{
    if (monitor == NULL || clock_font == NULL || date_font == NULL)
    {
        return;
    }

    time_t now = time(NULL);
    struct tm *local_time = localtime(&now);

    if (local_time == NULL)
    {
        return;
    }

    char clock_text[16];
    char date_text[64];

    if (strftime(clock_text, sizeof(clock_text),
                 "%H:%M", local_time) == 0 ||
        strftime(date_text, sizeof(date_text),
                 "%A %B %d", local_time) == 0)
    {
        return;
    }

    XClearWindow(display, window);

    XSetForeground(
        display,
        gc,
        WhitePixel(display, screen));

    int clock_width = XTextWidth(
        clock_font,
        clock_text,
        (int)strlen(clock_text));

    int date_width = XTextWidth(
        date_font,
        date_text,
        (int)strlen(date_text));

    /* Center the clock within this monitor's window. */
    XSetFont(display, gc, clock_font->fid);

    XDrawString(
        display,
        window,
        gc,
        (monitor->width - clock_width) / 2,
        monitor->height / 2,
        clock_text,
        (int)strlen(clock_text));

    /* Center the date below the clock. */
    XSetFont(display, gc, date_font->fid);

    XDrawString(
        display,
        window,
        gc,
        (monitor->width - date_width) / 2,
        monitor->height / 2 + 35,
        date_text,
        (int)strlen(date_text));
}

void ui_cleanup(Display *display)
{
    if (clock_font != NULL)
    {
        XFreeFont(display, clock_font);
        clock_font = NULL;
    }

    if (date_font != NULL)
    {
        XFreeFont(display, date_font);
        date_font = NULL;
    }
}
