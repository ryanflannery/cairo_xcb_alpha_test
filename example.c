/*
 * NOTE: This is a little long because I'm including all XCB setup I have
 * (including handling tiling window managers). It's below the main cairo logic
 * for ease, but included for posterity. This is a subset of a larger program
 * I'm working on.
 *
 * WHAT THIS IS:
 *    A sample program illustrating a problem I'm having with cairo/xcb, where I
 *    want to have a window that supports transparency (alpha channel) for the
 *    background color, and other components, with a regular re-draw cycle
 *    (every 1-second), but the alpha-channel portion of the render appears
 *    additive / cumulative. That is: every render increases the alpha value,
 *    thus making it more opaque / less transparent every second, and doesn't
 *    clear-out the previously rendered content.
 *
 *    I've read and tried a number of things with the cairo API (and xcb) but
 *    cannot seem to figure it out.
 *
 * WHAT THIS PROGRAM DOES:
 *    It renders a 200 pixel by 200 pixel square, with a 1-pixel black border,
 *    at the (200,200) (x,y) coordinate of an X11 display.
 *    It starts with a semi-transparent red background and a small seim-
 *    transparent green square drawn in the upper-left corner.
 *    Every second, it re-renders the display, re-drawing the backgrond and
 *    re-drawing the square slightly more to the lower-right corner (advancing
 *    along the diagonal).
 *    The square is green pure green (r=0, g=1, b=0) with alpha = 0.5.
 *    The initial background color is r=0.8, g=0, b=0, alpha = 0.1.
 *
 * DESIRED END RESULT:
 *    1. The window ALWAYS has background r=0.8, g=0, b=0, a=0.1
 *    2. The green square is ERASED with each loop and only the most recently
 *       drawn square is shown, with color r=0, g=1, b=0, a=0.5
 *
 * WHAT I OBSERVE:
 *    1. The first render is perfect
 *    2. Each subsequent render "adds" the alpha channel to the previous one
 *       - Thus the transparency is eliminated in a few iterations
 *    3. I have a good handle on *why* this is happening, but don't see how
 *       to correct it.
 *
 * THINGS I'VE TRIED:
 *    1. cairo_set_operator(cairo, CAIRO_OPERATOR_SOURCE);
 *       Called before the loop -and- the cairo_paint() after the
 *       cairo_pop_group_to_source() call (and reset it to *_OVER afterwards)
 *    2. Moving the cairo_clear_background() call below to before (and after)
 *       the cairo_push_group() call.
 *    3. In the #2 variant, wrapping that with calls to cairo_set_operator()
 *       with CAIRO_OPERATOR_SOURCE (and subsequent calls to reset to _OVER).
 *    4. I've looked at other operators, but they don't seem applicable (and
 *       have tested most in many configurations).
 *    5. Also looked at using cairo_mask() / cairo_mask_surface() per
 *       https://stackoverflow.com/questions/34831744/draw-icon-with-transparency-in-xlib-and-cairo
 *    6. So much google'ing/duckduckgo'ing
 *
 * TO BUILD/RUN:
 *    compile:
 *       $(CC) cairo_example.c -c `pkg-config --cflags cairo` -o cairo_example.o
 *    link:
 *       $(CC) cairo_example.o `pkg-config --libs cairo` -o cairo_example
 */

#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <xcb/xcb.h>
#include <xcb/xcb_icccm.h>
#include <cairo/cairo.h>
#include <cairo/cairo-xcb.h>

/*
 * XCB info. Full setup is included for posterity, but below main() for ease
 * of reading
 */
int               default_screen;
xcb_connection_t *xcon;
xcb_screen_t     *xscreen;
xcb_drawable_t   xwindow;
xcb_visualtype_t *xvisual;

void setup_xcb(); /* what actually does the xcb setup */

/* starting location & dimensions of window */
const int x = 200;
const int y = 200;
const int w = 200;
const int h = 200;

/* relevant cairo stuff */
cairo_t          *cairo;
cairo_surface_t  *surface;

/* effectively what i do to clear the background */
void
cairo_clear_background()
{
   cairo_set_source_rgba(cairo, 0.8, 0, 0, 0.1);
   cairo_paint(cairo);
}

int
main()
{
   const size_t fsize = 100;
   char file1[fsize];
   char file2[fsize];

   /* xcb setup (all below main, for ease, but also posterity) */
   setup_xcb();

   /* cairo setup */
   surface = cairo_xcb_surface_create(
         xcon,
         xwindow,
         xvisual,
         w, h);

   cairo = cairo_create(surface);

   /* map window & first draw */
   xcb_map_window(xcon, xwindow);
   cairo_clear_background();
   xcb_flush(xcon);

   /* begin main draw loop */
   for (int i = 0; i < 11; i++) {
      /*
      snprintf(file1, fsize, "cairo_example_root_%02d.png", i);
      cairo_surface_write_to_png(surface, file1);
      snprintf(file2, fsize, "cairo_example_target_%02d.png", i);
      cairo_surface_write_to_png(cairo_get_target(cairo), file2);
      */

      /* START: create new group/buffer, set it's background  */
      cairo_push_group(cairo);

         xcb_flush(xcon);
         snprintf(file1, fsize, "cimg_%02d_before.png", i);
         cairo_surface_write_to_png(cairo_get_target(cairo), file1);
      cairo_set_operator(cairo, CAIRO_OPERATOR_SOURCE);
      cairo_set_source_rgba(cairo, 0.8, 0, 0, 0.1);
      cairo_paint(cairo);
      cairo_set_operator(cairo, CAIRO_OPERATOR_OVER);
         xcb_flush(xcon);
         snprintf(file2, fsize, "cimg_%02d_later.png", i);
         cairo_surface_write_to_png(cairo_get_target(cairo), file2);

      /* now do all my drawing... */
      cairo_set_source_rgba(cairo, 0, 1, 0, 0.5);
      cairo_rectangle(cairo,
            i * 10 + 10,   /* x */
            i * 10 + 10,   /* y */
            50, 50);       /* w, h */
      cairo_fill(cairo);


      /* END: pop group/buffer (exposing it) and render */
      cairo_pop_group_to_source(cairo);
      /*cairo_set_operator(cairo, CAIRO_OPERATOR_SOURCE); XXX Makes the background black (?) */
      cairo_paint(cairo);
      /*cairo_set_operator(cairo, CAIRO_OPERATOR_OVER); only done if th eprevious cairo_set_operator() is done */
      xcb_flush(xcon);
      sleep(1);
   }

   /* cleanup */
   cairo_surface_destroy(surface);
   cairo_destroy(cairo);
   xcb_disconnect(xcon);

   return 0;
}

/* XCB Setup Stuff */

xcb_screen_t*
get_xscreen(xcb_connection_t *c, int screen)
{
   xcb_screen_iterator_t i = xcb_setup_roots_iterator(xcb_get_setup(c));;
   for (; i.rem; --screen, xcb_screen_next(&i)) {
      if (0 == screen)
         return i.data;
   }
   return NULL;
}

xcb_visualtype_t*
get_xvisual(xcb_screen_t *screen)
{
   xcb_depth_iterator_t i = xcb_screen_allowed_depths_iterator(screen);
   for (; i.rem; xcb_depth_next(&i)) {
      xcb_visualtype_iterator_t vi;
      vi = xcb_depth_visuals_iterator(i.data);
      for (; vi.rem; xcb_visualtype_next(&vi)) {
         if (screen->root_visual == vi.data->visual_id) {
            return vi.data;
         }
      }
   }

   return NULL;
}

/*
 * XCB setup to handle tiling window managers - this can (probably) be safely
 * ignored. I'm only incuding it for completeness' sake.
 */
void
wm_hints()
{
   enum {
      NET_WM_XINFO_TYPE,
      NET_WM_XINFO_TYPE_DOCK,
      NET_WM_DESKTOP,
      NET_WM_STRUT_PARTIAL,
      NET_WM_STRUT,
      NET_WM_STATE,
      NET_WM_STATE_STICKY,
      NET_WM_STATE_ABOVE
   };

   static const char *atoms[] = {
      "_NET_WM_XINFO_TYPE",
      "_NET_WM_XINFO_TYPE_DOCK",
      "_NET_WM_DESKTOP",
      "_NET_WM_STRUT_PARTIAL",
      "_NET_WM_STRUT",
      "_NET_WM_STATE",
      "_NET_WM_STATE_STICKY",
      "_NET_WM_STATE_ABOVE"
   };
   const size_t natoms = sizeof(atoms)/sizeof(char*);

   xcb_intern_atom_cookie_t xcookies[natoms];
   xcb_atom_t               xatoms[natoms];
   xcb_intern_atom_reply_t *xatom_reply;
   size_t i;

   for (i = 0; i < natoms; i++)
      xcookies[i] = xcb_intern_atom(xcon, 0, strlen(atoms[i]), atoms[i]);

   for (i = 0; i < natoms; i++) {
      xatom_reply = xcb_intern_atom_reply(xcon, xcookies[i], NULL);
      if (!xatom_reply)
         errx(1, "%s: xcb atom reply failed for %s", __FUNCTION__, atoms[i]);

      xatoms[i] = xatom_reply->atom;
      free(xatom_reply);
   }

   enum {
      left,           right,
      top,            bottom,
      left_start_y,   left_end_y,
      right_start_y,  right_end_y,
      top_start_x,    top_end_x,
      bottom_start_x, bottom_end_x
   };
   unsigned long struts[12] = { 0 };

   struts[top] = y + h;
   struts[top_start_x] = x;
   struts[top_end_x] = x + w;

	xcb_change_property(xcon, XCB_PROP_MODE_REPLACE, xwindow,
         xatoms[NET_WM_XINFO_TYPE], XCB_ATOM_ATOM, 32, 1,
         &xatoms[NET_WM_XINFO_TYPE_DOCK]);
	xcb_change_property(xcon, XCB_PROP_MODE_APPEND, xwindow,
         xatoms[NET_WM_STATE], XCB_ATOM_ATOM, 32, 2,
         &xatoms[NET_WM_STATE_STICKY]);
	xcb_change_property(xcon, XCB_PROP_MODE_REPLACE, xwindow,
         xatoms[NET_WM_DESKTOP], XCB_ATOM_CARDINAL, 32, 1,
         (const uint32_t []){ -1 } );
	xcb_change_property(xcon, XCB_PROP_MODE_REPLACE, xwindow,
         xatoms[NET_WM_STRUT_PARTIAL], XCB_ATOM_CARDINAL, 32, 12, struts);
	xcb_change_property(xcon, XCB_PROP_MODE_REPLACE, xwindow,
         xatoms[NET_WM_STRUT], XCB_ATOM_CARDINAL, 32, 4, struts);

   /* remove window from window manager tabbing */
   const uint32_t val[] = { 1 };
   xcb_change_window_attributes(xcon, xwindow,
         XCB_CW_OVERRIDE_REDIRECT, val);
}

void
setup_xcb()
{
   xcon = xcb_connect(NULL, &default_screen);
   if (xcb_connection_has_error(xcon)) {
      xcb_disconnect(xcon);
      errx(1, "Failed to establish connection to X");
   }

   if (NULL == (xscreen = get_xscreen(xcon, default_screen)))
      errx(1, "Failed to retrieve X screen");

   if (NULL == (xvisual = get_xvisual(xscreen)))
      errx(1, "Failed to retrieve X visual context");

   static uint32_t valwin[2] = {
      XCB_NONE,
      XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_BUTTON_PRESS
   };

   xwindow = xcb_generate_id(xcon);
   xcb_create_window(
         xcon,
         XCB_COPY_FROM_PARENT,
         xwindow,
         xscreen->root,
         x, y,
         y, h,
         1,    /* border width */
         XCB_WINDOW_CLASS_INPUT_OUTPUT,
         xscreen->root_visual,
         XCB_CW_EVENT_MASK | XCB_CW_BACK_PIXMAP,
         valwin);

   xcb_get_geometry_cookie_t gcookie = xcb_get_geometry(xcon, xwindow);
   xcb_get_geometry_reply_t *geo = xcb_get_geometry_reply(xcon, gcookie, NULL);
   printf("depth = %d\n", geo->depth);
   free(geo);

   wm_hints();
}
