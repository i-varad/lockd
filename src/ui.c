
#include <stdio.h>
#include <string.h>
#include <time.h>

#include <X11/Xlib.h>
#include <X11/Xft/Xft.h>
#include <X11/extensions/Xinerama.h>

#include "ui.h"

#define PASSWORD_MASK_MAX 128

static XftFont *clock_font;
static XftFont *date_font;
static XftFont *input_font;

static XftDraw *xft_draw;
static Visual *xft_visual;
static Colormap xft_colormap;
static XftColor text_color;

/*
    Initialize the lock-screen UI and its fonts.

    The fonts and Xft drawing resources are shared between
    monitor windows. ui_draw() changes the target window
    before rendering.
*/
void ui_init(Display *display, Window window, int screen)
{
    XSetWindowBackground(display, window, BlackPixel(display, screen));

    if (clock_font == NULL) {
        clock_font = XftFontOpenName(
            display, screen, "DejaVu Sans:style=Bold:size=72");
    }

    if (date_font == NULL) {
        date_font = XftFontOpenName(
            display, screen, "DejaVu Sans:size=20");
    }

    if (input_font == NULL) {
        input_font = XftFontOpenName(
            display, screen, "DejaVu Sans:size=24");
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
        input_font == NULL || xft_draw == NULL) {
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

/*
    Draw the clock, date, password mask, and optional status.

    password_length determines the number of masking characters.
    The actual password must never be passed to this function.

    status may be NULL or an empty string when no message is needed.
*/
void ui_draw(Display *display, Window window, GC gc, int screen,
             const XineramaScreenInfo *monitor,
             size_t password_length, const char *status)
{
    (void)gc;

    if (monitor == NULL || clock_font == NULL ||
        date_font == NULL || input_font == NULL ||
        xft_draw == NULL) {
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
    XftDrawChange(xft_draw, window);

    /*
        Allocate a drawing color for this render operation.
    */
    XRenderColor white = {
        .red = 0xffff,
        .green = 0xffff,
        .blue = 0xffff,
        .alpha = 0xffff
    };

    XftColor draw_color;

    if (!XftColorAllocValue(
            display,
            DefaultVisual(display, screen),
            DefaultColormap(display, screen),
            &white,
            &draw_color)) {
        return;
    }

    /*
        Measure the clock and date so they can be centered.
    */
    XGlyphInfo clock_extents;
    XGlyphInfo date_extents;

    XftTextExtentsUtf8(
        display,
        clock_font,
        (const FcChar8 *)clock_text,
        (int)strlen(clock_text),
        &clock_extents);

    XftTextExtentsUtf8(
        display,
        date_font,
        (const FcChar8 *)date_text,
        (int)strlen(date_text),
        &date_extents);

    int clock_x =
        (monitor->width - (int)clock_extents.xOff) / 2;

    int date_x =
        (monitor->width - (int)date_extents.xOff) / 2;

    int clock_y = monitor->height / 2;
    int date_y = clock_y + (int)clock_font->height + 12;

    /*
        Draw the clock.
    */
    XftDrawStringUtf8(
        xft_draw,
        &draw_color,
        clock_font,
        clock_x,
        clock_y,
        (const FcChar8 *)clock_text,
        (int)strlen(clock_text));

    /*
        Draw the date.
    */
    XftDrawStringUtf8(
        xft_draw,
        &draw_color,
        date_font,
        date_x,
        date_y,
        (const FcChar8 *)date_text,
        (int)strlen(date_text));

    /*
        Build a masked representation of the password.

        Only the password length is received. The password
        contents are never accessed by the UI module.
    */
    char masked_password[PASSWORD_MASK_MAX + 1];

    size_t mask_length = password_length;

    if (mask_length > PASSWORD_MASK_MAX) {
        mask_length = PASSWORD_MASK_MAX;
    }

    memset(masked_password, '*', mask_length);
    masked_password[mask_length] = '\0';

    /*
        Draw the mask below the date.
        Do not draw an empty mask when no characters are entered.
    */
    if (mask_length > 0) {
        XGlyphInfo password_extents;

        XftTextExtentsUtf8(
            display,
            input_font,
            (const FcChar8 *)masked_password,
            (int)mask_length,
            &password_extents);

        int password_x =
            (monitor->width - (int)password_extents.xOff) / 2;

        int password_y =
            date_y + (int)date_font->height + 32;

        XftDrawStringUtf8(
            xft_draw,
            &draw_color,
            input_font,
            password_x,
            password_y,
            (const FcChar8 *)masked_password,
            (int)mask_length);
    }

    /*
        Draw an optional authentication status message.
        Examples: "Authentication failed" or "Authenticating...".
    */
    if (status != NULL && status[0] != '\0') {
        XGlyphInfo status_extents;
        size_t status_length = strlen(status);

        XftTextExtentsUtf8(
            display,
            input_font,
            (const FcChar8 *)status,
            (int)status_length,
            &status_extents);

        int status_x =
            (monitor->width - (int)status_extents.xOff) / 2;

        int status_y =
            date_y + (int)date_font->height + 72;

        XftDrawStringUtf8(
            xft_draw,
            &draw_color,
            input_font,
            status_x,
            status_y,
            (const FcChar8 *)status,
            (int)status_length);
    }

    /*
        Release the temporary drawing color.
    */
    XftColorFree(
        display,
        DefaultVisual(display, screen),
        DefaultColormap(display, screen),
        &draw_color);
}

/*
    Release all resources owned by the UI module.
*/
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

    if (input_font != NULL) {
        XftFontClose(display, input_font);
        input_font = NULL;
    }

    if (text_color.pixel != 0) {
        XftColorFree(
            display, xft_visual, xft_colormap, &text_color);
        text_color.pixel = 0;
    }

    xft_visual = NULL;
    xft_colormap = 0;
}
