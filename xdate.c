#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <xcb/xcb.h>

#define WIDTH 300
#define HEIGHT 100

static xcb_gc_t getFontGC(xcb_connection_t * c,
                          xcb_screen_t     * screen,
                          xcb_window_t       window,
                          const char       * font_name);

static void drawText(xcb_connection_t * c,
                     xcb_screen_t     * screen,
                     xcb_window_t       window,
                     int16_t            x1,
                     int16_t            y1,
                     const char       * label);

static void testCookie(xcb_void_cookie_t   cookie,
                       xcb_connection_t  * connection,
                       char              * errMessage ) {
    xcb_generic_error_t *error = xcb_request_check (connection, cookie);
    if (error) {
            fprintf (stderr, "ERROR: %s : %"PRIu8"\n", errMessage , error->error_code);
            xcb_disconnect (connection);
            exit (-1);
    }
}

static void drawText (xcb_connection_t  *connection,
                      xcb_screen_t     *screen,
                      xcb_window_t      window,
                      int16_t           x1,
                      int16_t           y1,
                      const char       *label ) {
    /* get graphics context */
    xcb_gcontext_t gc = getFontGC (connection, screen, window, "fixed");
    /* draw the text */
    xcb_void_cookie_t textCookie = xcb_image_text_8_checked (connection,
                                                             strlen (label),
                                                             window,
                                                             gc,
                                                             x1, y1,
                                                             label );

    testCookie(textCookie, connection, "can't paste text");
    /* free the gc */
    xcb_void_cookie_t gcCookie = xcb_free_gc (connection, gc);

    testCookie(gcCookie, connection, "can't free gc");
}

static xcb_gc_t getFontGC (xcb_connection_t * connection,
                           xcb_screen_t     * screen,
                           xcb_window_t        window,
                           const char       * font_name ) {
    /* get font */
    xcb_font_t font = xcb_generate_id (connection);
    xcb_void_cookie_t fontCookie = xcb_open_font_checked (connection,
                                                          font,
                                                          strlen (font_name),
                                                          font_name );

    testCookie(fontCookie, connection, "can't open font");
    /* create graphics context */
    xcb_gcontext_t    gc            = xcb_generate_id (connection);
    uint32_t          mask          = XCB_GC_FOREGROUND | XCB_GC_BACKGROUND | XCB_GC_FONT;
    uint32_t          value_list[3] = { screen->black_pixel, screen->white_pixel, font };
    xcb_void_cookie_t gcCookie      = xcb_create_gc_checked (connection,
                                                             gc,
                                                             window,
                                                             mask,
                                                             value_list );

    testCookie(gcCookie, connection, "can't create gc");
    /* close font */
    fontCookie = xcb_close_font_checked (connection, font);
    testCookie(fontCookie, connection, "can't close font");
    return gc;
}

int main (void) {
    /* get the connection */
    int screenNum;
    xcb_connection_t *connection = xcb_connect (NULL, &screenNum);
    if (!connection) {
        fprintf (stderr, "ERROR: can't connect to an X server\n");
        return -1;
    }
    /* get the current screen */
    xcb_screen_iterator_t iter = xcb_setup_roots_iterator (xcb_get_setup (connection));
    // we want the screen at index screenNum of the iterator
    for (int i = 0; i < screenNum; ++i) {
        xcb_screen_next (&iter);
    }

    xcb_screen_t *screen = iter.data;
    if (!screen) {
        fprintf (stderr, "ERROR: can't get the current screen\n");
        xcb_disconnect (connection);
        return -1;
    }

    /* create the window */
    xcb_window_t window = xcb_generate_id (connection);
    uint32_t mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
    uint32_t values[2];
    values[0] = screen->white_pixel;
    values[1] = XCB_EVENT_MASK_KEY_RELEASE |
                XCB_EVENT_MASK_BUTTON_PRESS |
                XCB_EVENT_MASK_EXPOSURE |
                XCB_EVENT_MASK_POINTER_MOTION;

    xcb_void_cookie_t windowCookie = xcb_create_window_checked (connection,
                                                                screen->root_depth,
                                                                window, screen->root,
                                                                20, 200, 
                                                                WIDTH, HEIGHT,
                                                                0, XCB_WINDOW_CLASS_INPUT_OUTPUT,
                                                                screen->root_visual,
                                                                mask, values);

    testCookie(windowCookie, connection, "can't create window");
    xcb_void_cookie_t mapCookie = xcb_map_window_checked (connection, window);
    testCookie(mapCookie, connection, "can't map window");
    xcb_flush(connection);  // make sure window is drawn

    xcb_generic_event_t  *event;
    for(;;) { ;
        if ( (event = xcb_poll_for_event(connection)) ) {
            switch (event->response_type & ~0x80) {
                case XCB_EXPOSE: {
                    drawText (connection, 
                              screen,
                              window,
                              10, HEIGHT - 10,
                              "Press ESC key to exit..." );
                    break;
                }
                case XCB_KEY_RELEASE: {
                    xcb_key_release_event_t *kr = (xcb_key_release_event_t *)event;
                    switch (kr->detail) {
                        /* ESC */
                        case 9: {
                            free (event);
                            xcb_disconnect (connection);
                            return 0;
                        }
                    }
                    free (event);
                }
            }
        }
    }
    return 0;
}