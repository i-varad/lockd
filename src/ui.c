#include <stdio.h>
#include <string.h>
#include <time.h>

#include <X11/Xlib.h>
#include <X11/Xft/Xft.h>
#include <X11/extensions/Xinerama.h>

#include "ui.h"

static XftFont *clock_font;
static XftFont *date_font;
static XftDraw *xft_draw;
static Visual *xft_visual;
static Colormap xft_colormap;
static XftColor text_color;

void ui_init(Display *display, Window window, int screen)
{
    XSetWindowBackground(display, window, BlackPixel(display, screen));

    /*
        Fonts and drawing resources are shared across monitor windows.
        Initialize them only once.
    */
    if (clock_font == NULL) {
        clock_font = XftFontOpenName(
            display, screen, "DejaVu Sans:style=Bold:size=72");
    }

    if (date_font == NULL) {
        date_font = XftFontOpenName(
            display, screen, "DejaVu Sans:size=20");
    }

    if (xft_visual == NULL) {
        xft_visual = DefaultVisual(display, screen);
        xft_colormap = DefaultColormap(display, screen);
    }

    if (xft_draw == NULL) {
        xft_draw = XftDrawCreate(
            display, window, xft_visual, xft_colormap);
    }

    if (clock_font == NULL || date_font == NULL ||
        xft_draw == NULL) {
        fprintf(stderr, "lockd: failed to initialize Xft UI resources\n");
        return;
    }

    if (text_color.pixel == 0) {
        if (!XftColorAllocName(
                display, xft_visual, xft_colormap,
                "white", &text_color)) {
            fprintf(stderr, "lockd: failed to allocate text color\n");
        }
    }
}

void ui_draw(
    Display *display,
    Window window,
    GC gc,
    int screen,
    const XineramaScreenInfo *monitor)
{
    (void)gc;

    if (monitor == NULL || clock_font == NULL ||
        date_font == NULL || xft_draw == NULL) {
        return;
    }

    time_t now = time(NULL);
    struct tm *local_time = localtime(&now);

    if (local_time == NULL) {
        return;
    }

    char clock_text[16];
    char date_text[64];

    if (strftime(clock_text, sizeof(clock_text),
                 "%H:%M", local_time) == 0 ||
        strftime(date_text, sizeof(date_text),
                 "%A %B %d", local_time) == 0) {
        return;
    }

    XClearWindow(display, window);

    /*
        XftDraw is associated with a specific drawable.
        Recreate it when drawing into a different monitor window.
    */
    XftDrawChange(xft_draw, window);

    XRenderColor white = {
        .red = 0xffff,
        .green = 0xffff,
        .blue = 0xffff,
        .alpha = 0xffff
    };

    XftColor draw_color;
    if (!XftColorAllocValue(
            display, DefaultVisual(display, screen),
            DefaultColormap(display, screen),
            &white, &draw_color)) {
        return;
    }

    XGlyphInfo clock_extents;
    XGlyphInfo date_extents;

    XftTextExtentsUtf8(
        display, clock_font,
        (const FcChar8 *)clock_text,
        (int)strlen(clock_text), &clock_extents);

    XftTextExtentsUtf8(
        display, date_font,
        (const FcChar8 *)date_text,
        (int)strlen(date_text), &date_extents);

    int clock_x = (monitor->width - (int)clock_extents.xOff) / 2;
    int date_x = (monitor->width - (int)date_extents.xOff) / 2;

    /*
        Xft draws text using a baseline, not its top edge.
        These offsets place the clock and date around the center.
    */
    int clock_y = monitor->height / 2;
    int date_y = clock_y + (int)clock_font->height + 12;

    XftDrawStringUtf8(
        xft_draw, &draw_color, clock_font,
        clock_x, clock_y,
        (const FcChar8 *)clock_text,
        (int)strlen(clock_text));

    XftDrawStringUtf8(
        xft_draw, &draw_color, date_font,
        date_x, date_y,
        (const FcChar8 *)date_text,
        (int)strlen(date_text));

    XftColorFree(
        display, DefaultVisual(display, screen),
        DefaultColormap(display, screen), &draw_color);
}

void ui_cleanup(Display *display)
{
    if (xft_draw != NULL) {
        XftDrawDestroy(xft_draw);
        xft_draw = NULL;
    }

    if (clock_font != NULL) {
        XftFontClose(display, clock_font);
        clock_font = NULL;
    }

    if (date_font != NULL) {
        XftFontClose(display, date_font);
        date_font = NULL;
    }

    if (text_color.pixel != 0) {
        XftColorFree(
            display, xft_visual, xft_colormap, &text_color);
        text_color.pixel = 0;
    }

    xft_visual = NULL;
}