/* See LICENSE file for license details. */
#define _POSIX_C_SOURCE 200809L
#include <stdarg.h>
#include <unistd.h>
#include <xcb/xcb.h>
#include "config.h"

static int ret = 0;

static size_t strlen_c(char * str) {
    size_t n = 0;
    char * p = str;
    while ((* (p++)) != 0) {
        ++n;
    }
    return n;
}

static int strcmp_c(char * str1, char * str2) {
    char * c1 = str1;
    char * c2 = str2;
    while ((* c1) && ((* c1) == (* c2))) {
        ++c1;
        ++c2;
    }
    int n = (* c1) - (* c2);
    return n;
}

static int die(char * errstr) {
    size_t n = 0;
    char * p = errstr;
    while ((* (p++)) != 0) {
        ++n;
    }
    ssize_t o = write(STDERR_FILENO, errstr, n);
    int ret = 1;
    if (o < 0) {
        ret = -1;
    }
    return ret;
}

static void testCookie(xcb_void_cookie_t   cookie,
                       xcb_connection_t  * connection,
                       char              * errorMessage) {
    xcb_generic_error_t *error = xcb_request_check(connection, cookie);
    if (error) {
        ret = die(errorMessage);
        xcb_disconnect(connection);
        ret = 1;
    }
}

static xcb_gc_t getFontGC(xcb_connection_t * connection,
                          xcb_window_t       window,
                          char             * font_name) {
    xcb_font_t font = xcb_generate_id (connection);
    xcb_void_cookie_t fontCookie = xcb_open_font_checked(connection,
                                                         font,
                                                         strlen_c(font_name),
                                                         font_name );

    testCookie(fontCookie, connection, "can't open font");
    xcb_gcontext_t gc = xcb_generate_id(connection);
    uint32_t mask = XCB_GC_FOREGROUND
        | XCB_GC_BACKGROUND
        | XCB_GC_FONT
        | XCB_GC_GRAPHICS_EXPOSURES;
    uint32_t value_list[4] = { FOREGROUND, BACKGROUND, font, 0 };
    xcb_void_cookie_t gcCookie = xcb_create_gc_checked(connection,
                                                       gc,
                                                       window,
                                                       mask,
                                                       value_list );

    testCookie(gcCookie, connection, "can't create gc");
    fontCookie = xcb_close_font_checked (connection, font);
    testCookie(fontCookie, connection, "can't close font");
    return gc;
}

static void drawText(xcb_connection_t * connection,
                     xcb_window_t       window,
                     int16_t            x1,
                     int16_t            y1,
                     char             * label,
                     char             * font_name) {
    xcb_gcontext_t gc = getFontGC (connection, window, font_name);
    xcb_void_cookie_t textCookie = xcb_image_text_8_checked (connection,
                                                             strlen_c(label),
                                                             window,
                                                             gc,
                                                             x1, y1,
                                                             label );

    testCookie(textCookie, connection, "can't paste text");
    xcb_void_cookie_t gcCookie = xcb_free_gc (connection, gc);
    testCookie(gcCookie, connection, "can't free gc");
    xcb_flush(connection);
}

int main (int argc, char *argv[]) {
    char * str = NULL;

    if ((argc == 2) && (strcmp_c("-v", argv[1]) == 0)) {
        ret = die("xdate-0.0.1, 2020 Michael Czigler, see LICENSE for details\n");
    }

    if ((ret == 0) && (argc != 2)) {
        ret = die("usage: xdate [str] [-v]\n");
    }

    if (ret == 0) {
        str = argv[1];

    }

    xcb_connection_t * dpy = xcb_connect(NULL, NULL);
    if (!dpy) {
        ret = die("ERROR: can't connect to an X server\n");
    }

    xcb_screen_t * scre = xcb_setup_roots_iterator(xcb_get_setup(dpy)).data;
    if (!scre) {
        ret = die("ERROR: can't get the current screen\n");
        xcb_disconnect(dpy);
    }

    if (scre) {
        drawText(dpy, scre->root, XPOS, YPOS, str, FONT);
    }
    return ret;
}
