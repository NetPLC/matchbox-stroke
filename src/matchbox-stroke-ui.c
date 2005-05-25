#include "matchbox-stroke.h"

#include <X11/Xft/Xft.h>

struct MBStrokeUI
{
  Display           *xdpy;
  int                xscreen;
  Window             xwin_root, xwin;

  int                dpy_width;
  int                dpy_height;
  int                xwin_width;
  int                xwin_height;

  Pixmap             xwin_pixmap;
  Picture            xwin_pict;
  XRenderPictFormat *xwin_format;

  Pixmap             root_pixmap_orig;

  Pixmap             pentip_pixmap;
  Picture            pentip_pict;
  XRenderPictFormat *pentip_format;

  FakeKey           *fakekey;
  MBStroke          *stroke;

  XftFont           *font;
  XftColor           font_col; 

  MBStrokeUIMode     mode;

  int                fade_cnt;
  boolean            have_grab;

  GC                 xgc;
};

#define UI_WANT_FULLSCREEN(u) ((u)->mode == MBStrokeUIModeFullscreen)

static struct {
  unsigned int   width;
  unsigned int   height;
  unsigned int   bytes_per_pixel; /* 3:RGB, 4:RGBA */
  unsigned char  pixel_data[8 * 8 * 4 + 1];
} PentipImage = {
  8, 8, 4,
  "\0\0\0\0\345\15\15\5\345\15\15\12\345\15\15\17\345\15\15\17\345\15\15\12"
  "\345\15\15\5\0\0\0\0\345\15\15\5\345\15\15\17\345\15\15\35\345\15\15%\345"
  "\15\15%\345\15\15\35\345\15\15\17\345\15\15\5\345\15\15\12\345\15\15\35\345"
  "\15\15;\345\15\15N\345\15\15N\345\15\15;\345\15\15\35\345\15\15\12\345\15"
  "\15\24\345\15\15/\345\15\15Y\345\15\15v\345\15\15v\345\15\15Y\345\15\15/"
  "\345\15\15\24\345\15\15\24\345\15\15""6\345\15\15`\345\15\15\200\345\15\15"
  "\200\345\15\15`\345\15\15""6\345\15\15\24\345\15\15\17\345\15\15*\345\15"
  "\15N\345\15\15j\345\15\15j\345\15\15N\345\15\15*\345\15\15\17\345\15\15\12"
  "\345\15\15\31\345\15\15/\345\15\15?\345\15\15?\345\15\15/\345\15\15\31\345"
  "\15\15\12\0\0\0\0\345\15\15\12\345\15\15\24\345\15\15\31\345\15\15\31\345"
  "\15\15\24\345\15\15\12\0\0\0\0",
};

#define PENTIP_WIDTH  (PentipImage.width)
#define PENTIP_HEIGHT (PentipImage.height)


static void
mb_stroke_ui_clear_recogniser(MBStrokeUI *ui, int alpha);

static void
mb_stroke_ui_fullscreen_init(MBStrokeUI *ui);

static void
mb_stroke_ui_fullscreen_stroke_finish(MBStrokeUI *ui);


/* Fix byte order as it comes from the GIMP, and pre-multiply
   alpha.
   Thanks to Keith Packard.
*/
static void 
fix_image(unsigned char *image, int npixels)
{

#define FbIntMult(a,b,t) ((t) = (a) * (b) + 0x80, ( ( ( (t)>>8 ) + (t) )>>8 ))

    int	i;
    for (i = 0; i < npixels; i++)
    {
	unsigned char	a, r, g, b;
	unsigned short	t;
	
	b = image[i*4 + 0];
	g = image[i*4 + 1];
	r = image[i*4 + 2];
	a = image[i*4 + 3];

	r = FbIntMult(a, r, t);
	g = FbIntMult(a, g, t);
	b = FbIntMult(a, b, t);
	image[i*4 + 0] = r;
	image[i*4 + 1] = g;
	image[i*4 + 2] = b;
	image[i*4 + 3] = a;
    }
}

static int 
handle_xerror(Display *dpy, XErrorEvent *e)
{
  char msg[255];
  XGetErrorText(dpy, e->error_code, msg, sizeof msg);
  fprintf(stderr, "matchbox-stroke: X error warning (%#lx): %s (opcode: %i)\n",
		e->resourceid, msg, e->request_code);
  return 0;
}

static boolean
get_xevent_timed(Display        *dpy,
		 XEvent         *event_return, 
		 struct timeval *tv)
{
  if (tv->tv_usec == 0 && tv->tv_sec == 0)
    {
      XNextEvent(dpy, event_return);
      return True;
    }

  XFlush(dpy);

  if (XPending(dpy) == 0) 
    {
      int fd = ConnectionNumber(dpy);

      fd_set readset;
      FD_ZERO(&readset);
      FD_SET(fd, &readset);

      if (select(fd+1, &readset, NULL, NULL, tv) == 0) 
	return False;
      else 
	{
	  XNextEvent(dpy, event_return);
	  return True;
	}
    } else {
      XNextEvent(dpy, event_return);
      return True;
    }
}

static boolean
get_desktop_area(MBStrokeUI *ui, int *x, int *y, int *width, int *height)
{
  Atom           atom_area, type;
  int            result, format;
  long           nitems, bytes_after;
  int           *geometry = NULL;

  atom_area = XInternAtom (ui->xdpy, "_NET_WORKAREA", False);

  result = XGetWindowProperty (ui->xdpy, 
			       RootWindow(ui->xdpy, ui->xscreen),
			       atom_area,
			       0, 16L, False, XA_CARDINAL, &type, &format,
			       &nitems, &bytes_after, 
			       (unsigned char **)&geometry);

  if (result != Success || nitems < 4 || geometry == NULL)
    {
      if (geometry) XFree(geometry); 
      return False;
    }

  if (x) *x           = geometry[0];
  /* 
   * XXX !Big hack!. We add 20 here to account for the titlebar area.  
   * This of course is very wrong. figure out better way    
  */
  if (y) *y           = geometry[1] + 20;
  if (width)  *width  = geometry[2];
  if (height) *height = geometry[3];
  
  XFree(geometry);

  return True;
}

static boolean
init_pentip_image(MBStrokeUI *ui)
{
  XImage *ximage;
  GC      gc;
  XRenderPictFormat pict_format;

  pict_format.type             = PictTypeDirect;
  pict_format.depth            = 32;
  pict_format.direct.alpha     = 24;
  pict_format.direct.alphaMask = 0xff;
  pict_format.direct.red       = 16;
  pict_format.direct.redMask   = 0xff;
  pict_format.direct.green     = 8;
  pict_format.direct.greenMask = 0xff;
  pict_format.direct.blue      = 0;
  pict_format.direct.blueMask  = 0xff;
  
  ui->pentip_format = XRenderFindFormat (ui->xdpy,
					 PictFormatType|
					 PictFormatDepth|
					 PictFormatAlpha|
					 PictFormatAlphaMask|
					 PictFormatRed|
					 PictFormatRedMask|
					 PictFormatGreen|
					 PictFormatGreenMask|
					 PictFormatBlue|
					 PictFormatBlueMask,
					 &pict_format, 0);
    
  ui->pentip_pixmap = XCreatePixmap(ui->xdpy, ui->xwin,
				    PentipImage.width, 
				    PentipImage.height, 32);

  ui->pentip_pict = XRenderCreatePicture(ui->xdpy,
					 ui->pentip_pixmap,
					 ui->pentip_format,
					 0, 0);

  gc = XCreateGC(ui->xdpy, ui->pentip_pixmap, 0, 0);


  fix_image((unsigned char*)PentipImage.pixel_data, 
	    PentipImage.width * PentipImage.height);

  ximage = XCreateImage(ui->xdpy, 
			DefaultVisual(ui->xdpy, ui->xscreen), 
			32, 
			ZPixmap,
			0, (char *) PentipImage.pixel_data,
			PentipImage.width, 
			PentipImage.height,
			32, 
			PentipImage.width * 4);

  XPutImage(ui->xdpy, 
	    ui->pentip_pixmap, 
	    gc, 
	    ximage,
	    0, 0, 0, 0, 
	    PentipImage.width, PentipImage.height);

  /* XXX Destroy the image ? */

  XFreeGC(ui->xdpy, gc);

  return 1;
}

static void
mb_stroke_ui_show(MBStrokeUI  *ui)
{
  XMapWindow(ui->xdpy, ui->xwin);
}
			  
static int
mb_stroke_ui_resources_create(MBStrokeUI  *ui)
{

#define PROP_MOTIF_WM_HINTS_ELEMENTS    5
#define MWM_HINTS_DECORATIONS          (1L << 1)
#define MWM_DECOR_BORDER               (1L << 1)

  typedef struct
  {
    unsigned long       flags;
    unsigned long       functions;
    unsigned long       decorations;
    long                inputMode;
    unsigned long       status;
  } PropMotifWmHints ;

  Atom /* atom_wm_protocols[3], */ 
    atom_NET_WM_WINDOW_TYPE, 
    atom_NET_WM_WINDOW_TYPE_TOOLBAR,
    atom_NET_WM_WINDOW_TYPE_DOCK,
    atom_NET_WM_STRUT_PARTIAL,
    atom_NET_WM_STATE_SKIP_PAGER,
    atom_NET_WM_STATE_SKIP_TASKBAR,
    atom_NET_WM_STATE,
    atom_MOTIF_WM_HINTS;
  
  PropMotifWmHints    *mwm_hints;
  XSizeHints           size_hints;
  XWMHints            *wm_hints;

  XSetWindowAttributes win_attr;
  int                  desk_width = 0, desk_height = 0;

  XRenderColor         coltmp;

  /*
  atom_wm_protocols = { 
    XInternAtom(ui->xdpy, "WM_DELETE_WINDOW",False),
    XInternAtom(ui->xdpy, "WM_PROTOCOLS",False),
    XInternAtom(ui->xdpy, "WM_NORMAL_HINTS", False),
  };
  */
  atom_NET_WM_WINDOW_TYPE =
    XInternAtom(ui->xdpy, "_NET_WM_WINDOW_TYPE" , False);

  atom_NET_WM_WINDOW_TYPE_TOOLBAR =
    XInternAtom(ui->xdpy, "_NET_WM_WINDOW_TYPE_TOOLBAR", False);

  atom_NET_WM_WINDOW_TYPE_DOCK = 
    XInternAtom(ui->xdpy, "_NET_WM_WINDOW_TYPE_DOCK", False);

  atom_MOTIF_WM_HINTS =
    XInternAtom(ui->xdpy, "_MOTIF_WM_HINTS", False);

  atom_NET_WM_STRUT_PARTIAL = 
    XInternAtom(ui->xdpy, "_NET_WM_STRUT_PARTIAL", False);

  atom_NET_WM_STATE_SKIP_TASKBAR =
    XInternAtom(ui->xdpy, "_NET_WM_STATE_SKIP_TASKBAR", False);

  atom_NET_WM_STATE_SKIP_PAGER = 
    XInternAtom(ui->xdpy, "_NET_WM_STATE_SKIP_PAGER", False);

  atom_NET_WM_STATE =
    XInternAtom(ui->xdpy, "_NET_WM_STATE", False);


  win_attr.override_redirect = False; /* Set to true for extreme case */
  win_attr.event_mask 
    = ButtonPressMask|ButtonReleaseMask|Button1MotionMask|
    StructureNotifyMask|ExposureMask;

  if (UI_WANT_FULLSCREEN(ui))
    {
      /* fullscreen, just re-use the rootwin */

      ui->xwin_width  = mb_stroke_ui_display_width(ui);
      ui->xwin_height = mb_stroke_ui_display_height(ui);
      ui->xwin        = ui->xwin_root;


    }
  else
    {
      ui->xwin_width  = mb_stroke_ui_display_width(ui);
      ui->xwin_height = mb_stroke_ui_display_height(ui) / 3;

      /* below assumes non fullscreen */

      if (get_desktop_area(ui, NULL, NULL, &desk_width, &desk_height))
	{
	  ui->xwin_width = desk_width;
	  ui->xwin_height = desk_height / 3;
	}

      ui->xwin = XCreateWindow(ui->xdpy,
			       ui->xwin_root,
			       0, 0,
			       ui->xwin_width, ui->xwin_height,
			       0,
			       CopyFromParent, CopyFromParent, CopyFromParent,
			       CWOverrideRedirect|CWEventMask,
			       &win_attr);

      /* again assumes mb and non fullscreen */

      XChangeProperty(ui->xdpy, ui->xwin, 
		      atom_NET_WM_WINDOW_TYPE, XA_ATOM, 32, 
		      PropModeReplace, 
		      (unsigned char *) &atom_NET_WM_WINDOW_TYPE_TOOLBAR, 1);

      
      wm_hints = XAllocWMHints();
      
      if (wm_hints)
	{
	  DBG("setting no focus hint");
	  wm_hints->input = False;
	  wm_hints->flags = InputHint;
	  XSetWMHints(ui->xdpy, ui->xwin, wm_hints );
	  XFree(wm_hints);
	}
      
      size_hints.flags = PPosition | PSize | PMinSize;
      size_hints.x = 0;
      size_hints.y = 0;
      size_hints.width      =  ui->xwin_width; 
      size_hints.height     =  ui->xwin_height;
      size_hints.min_width  =  ui->xwin_width;
      size_hints.min_height =  ui->xwin_height;
      
      XSetStandardProperties(ui->xdpy, ui->xwin, "Keyboard", 
			     NULL, 0, NULL, 0, &size_hints);
      
      mwm_hints = util_malloc0(sizeof(PropMotifWmHints));
  
      if (mwm_hints)
	{
	  mwm_hints->flags = MWM_HINTS_DECORATIONS;
	  mwm_hints->decorations = 0;
	  
	  XChangeProperty(ui->xdpy, ui->xwin, atom_MOTIF_WM_HINTS, 
			  XA_ATOM, 32, PropModeReplace, 
			  (unsigned char *)mwm_hints, 
			  PROP_MOTIF_WM_HINTS_ELEMENTS);
	  
	  free(mwm_hints);
	}
      
      {
	Atom states[] = { atom_NET_WM_STATE_SKIP_TASKBAR, 
			  atom_NET_WM_STATE_SKIP_PAGER };
	
	XChangeProperty(ui->xdpy, ui->xwin, 
			atom_NET_WM_STATE, XA_ATOM, 32, 
			PropModeReplace, 
			(unsigned char *)states, 2);
      }
    }

  /* drawables etc  */

  ui->xwin_format 
    = XRenderFindVisualFormat(ui->xdpy,DefaultVisual(ui->xdpy, ui->xscreen));

  ui->xwin_pixmap = XCreatePixmap(ui->xdpy, ui->xwin,
				  ui->xwin_width,
				  ui->xwin_height,
				  DefaultDepth(ui->xdpy, ui->xscreen));

  

  if (UI_WANT_FULLSCREEN(ui))
    {
      XRenderPictureAttributes attr;

      attr.subwindow_mode = IncludeInferiors; 

      ui->xwin_pict = XRenderCreatePicture(ui->xdpy, ui->xwin_pixmap, 
					   ui->xwin_format,
					   CPSubwindowMode, &attr);
    }
  else
    ui->xwin_pict = XRenderCreatePicture(ui->xdpy, ui->xwin_pixmap, 
					 ui->xwin_format,
					 0, NULL);
  
  init_pentip_image(ui);  

  if (UI_WANT_FULLSCREEN(ui))
    {
      unsigned long gcm;
      XGCValues     gcv;

      gcm                = GCSubwindowMode; 
      gcv.subwindow_mode = IncludeInferiors;

      ui->xgc = XCreateGC(ui->xdpy, ui->xwin, gcm, &gcv);

      /* XXX backup to root window - need to this before every stroke */

      ui->root_pixmap_orig = XCreatePixmap(ui->xdpy, ui->xwin,
					   ui->xwin_width,
					   ui->xwin_height,
					   DefaultDepth(ui->xdpy,ui->xscreen));

      mb_stroke_ui_fullscreen_init(ui);

    }
  else
    {
      ui->xgc = XCreateGC(ui->xdpy, ui->xwin, 0, NULL);
  
      XSetBackground(ui->xdpy, ui->xgc, WhitePixel(ui->xdpy, ui->xscreen ));
      XSetForeground(ui->xdpy, ui->xgc, BlackPixel(ui->xdpy, ui->xscreen ));
  
      mb_stroke_ui_clear_recogniser(ui, 0xffff);
    }

  coltmp.red = 0xbbbb; 
  coltmp.green = coltmp.blue  = 0x0000; 
  coltmp.alpha = 0xdddd;

  XftColorAllocValue(ui->xdpy,
		     DefaultVisual(ui->xdpy, ui->xscreen), 
		     DefaultColormap(ui->xdpy, ui->xscreen),
		     &coltmp,
		     &ui->font_col);

  ui->font = XftFontOpenName(ui->xdpy, ui->xscreen, "Sans-24:bold");
  
  return 1;
}

static void
mb_stroke_ui_fullscreen_init(MBStrokeUI *ui)
{
  Cursor cursor;

  if (!UI_WANT_FULLSCREEN(ui))
    return;

  cursor = XCreateFontCursor(ui->xdpy, XC_left_ptr);

  /*
  if (XGrabButton(ui->xdpy, ui->xwin_root, False,
		  ButtonPressMask|ButtonReleaseMask|Button1MotionMask,
		  GrabModeAsync,
		  GrabModeAsync, 
		  None, cursor, CurrentTime) != GrabSuccess)
  */

  if (XGrabButton(ui->xdpy, 
		  Button1, 
		  AnyModifier, 
		  ui->xwin_root, 
		  True, 
		  ButtonPressMask|ButtonReleaseMask|ButtonMotionMask,
		  GrabModeSync, 
		  GrabModeAsync, None, None) != GrabSuccess)
    {
      /* fprintf(stderr, "matchbox-stroke: mouse grab failed"); */
    }

}


static void
mb_stroke_ui_fullscreen_stroke_finish(MBStrokeUI *ui)
{
  if (!UI_WANT_FULLSCREEN(ui))
    return;

  /* copy our backup of the root */
  XCopyArea(ui->xdpy, 
	    ui->root_pixmap_orig,
	    ui->xwin,
	    ui->xgc,
	    0, 0, ui->xwin_width, ui->xwin_height, 0, 0);

  // XUngrabServer(ui->xdpy);

  XSync(ui->xdpy, False);
}

static boolean
mb_stroke_ui_fullscreen_stroke_start(MBStrokeUI *ui, int x, int y)
{
  XftDraw *xftdraw;
  int      wa_x, wa_y, wa_width, wa_height;

  if (!UI_WANT_FULLSCREEN(ui))
    return True;

  // XGrabServer(ui->xdpy);

  if (get_desktop_area(ui, &wa_x, &wa_y, &wa_width, &wa_height))
    {
      if (x < wa_x || y < wa_y 
	  || x > wa_x + wa_width 
	  || y > wa_y + wa_height)
	{
	  /* pass this event through our grab */
	  printf("allowing events\n");
	  XAllowEvents(ui->xdpy, ReplayPointer, CurrentTime);
	  return False;
	}
    }

  XAllowEvents(ui->xdpy, AsyncPointer, CurrentTime);

  /* make a temp copy of root win */
  XCopyArea(ui->xdpy, 
	    ui->xwin,
	    ui->root_pixmap_orig,
	    ui->xgc,
	    0, 0, ui->xwin_width, ui->xwin_height, 0, 0);

  /* also copy it to what we'll draw on */
  XCopyArea(ui->xdpy, 
	    ui->xwin,
	    ui->xwin_pixmap,
	    ui->xgc,
	    0, 0, ui->xwin_width, ui->xwin_height, 0, 0);

  /* render our 'mode' */

  xftdraw = XftDrawCreate(ui->xdpy,
			  ui->xwin_pixmap,
			  DefaultVisual(ui->xdpy, ui->xscreen),
			  DefaultColormap(ui->xdpy, ui->xscreen));

  /*
  XftTextRender8 (ui->xdpy,
		  PictOpOver,
		  XftDrawSrcPicture (xftdraw, &ui->font_col),
		  ui->font,
		  ui->xwin_pict,
		  0,0,
		  0,0,
		  "ABC",
		  3);
*/

  XftDrawStringUtf8(xftdraw,
		    &ui->font_col,
		    ui->font,
		    10,
		    10 + ui->font->ascent,
		    "Recognising", 
		    strlen("Recognising"));



  XCopyArea(ui->xdpy, 
	    ui->xwin_pixmap,
	    ui->xwin, 
	    ui->xgc,
	    0, 0, ui->xwin_width, ui->xwin_height, 0, 0);

  /* XXX free xftdraw */

  return True;
}


static void
mb_stroke_ui_clear_recogniser(MBStrokeUI *ui, int alpha)
{
  XRenderColor color;

  color.red = color.green = color.blue = color.alpha = alpha;

  if (UI_WANT_FULLSCREEN(ui))
    {
      /* nothing as yet */
    }
  else
    {
      XRenderFillRectangle(ui->xdpy,
			   PictOpOver, 
			   ui->xwin_pict, 
			   &color,
			   0, 0, 
			   ui->xwin_width, 
			   ui->xwin_height);

      XCopyArea(ui->xdpy, 
		ui->xwin_pixmap,
		ui->xwin, 
		ui->xgc,
		0, 0, ui->xwin_width, ui->xwin_height, 0, 0);
    }

  XFlush(ui->xdpy);
}

void
mb_stroke_ui_pentip_draw_point(int x, int y, void *cookie)
{

  MBStrokeUI *ui = (MBStrokeUI *)cookie;

  XRenderComposite(ui->xdpy,
		   PictOpOver,
		   ui->pentip_pict, 
		   None, 
		   ui->xwin_pict,
		   0, 0, 
		   0, 0, 
		   x, y,
		   PENTIP_WIDTH, PENTIP_HEIGHT);

  XCopyArea(ui->xdpy, 
	    ui->xwin_pixmap,
	    ui->xwin,
	    ui->xgc,
	    x, y, PENTIP_WIDTH, PENTIP_HEIGHT, x, y);

  XFlush(ui->xdpy);
}



void
mb_stroke_ui_pentip_draw_line_to(MBStrokeUI *ui, int x2, int y2)
{
  int x1, y1;

  if (mb_stroke_stroke_get_last_point(mb_stroke_current_stroke(ui->stroke),
				      &x1, &y1))
    {
      util_bresenham_line(x1, y1, x2, y2,  
			  &mb_stroke_ui_pentip_draw_point,
			  (void *)ui);
    }
}

void
mb_stroke_ui_stroke_start(MBStrokeUI *ui, int x, int y)
{
  /* init for fullscreen mode */
  if (!mb_stroke_ui_fullscreen_stroke_start(ui, x, y))
    return;

  mb_stroke_stroke_new(ui->stroke);

  mb_stroke_stroke_append_point(mb_stroke_current_stroke(ui->stroke), 
				x, y);

  mb_stroke_ui_pentip_draw_point(x, y, ui);
}

void
mb_stroke_ui_stroke_update(MBStrokeUI *ui, int x, int y)
{
  mb_stroke_ui_pentip_draw_line_to(ui, x, y);

  mb_stroke_stroke_append_point(mb_stroke_current_stroke(ui->stroke), x, y); 
}



void
mb_stroke_ui_stroke_finish(MBStrokeUI *ui, int x, int y)
{
  char recog[512];

  mb_stroke_ui_stroke_update(ui, x, y);

  memset(&recog[0], 0, 512);
		
  if (mb_stroke_stroke_trans (mb_stroke_current_stroke(ui->stroke), 
			      &recog[0]))
    {
      MBStrokeAction *action = NULL;

      printf("got '%s'\n", recog);

      if (UI_WANT_FULLSCREEN(ui))
	{
	  mb_stroke_ui_fullscreen_stroke_finish(ui);
	}

      action = mb_stroke_mode_match_seq(mb_stroke_current_mode(ui->stroke), 
					recog);
      if (action)
	mb_stroke_action_execute(action);
    }
  else
    {
      mb_stroke_ui_fullscreen_stroke_finish(ui);
      printf("recog failed\n");
    }

  ui->fade_cnt = 5;


}

void
mb_stroke_ui_debug_grid(MBStrokeUI *ui, 
			int         minx,
			int         miny,
			int         maxx,
			int         maxy,
			int         bound_x1,
			int         bound_x2,
			int         bound_y1,
			int         bound_y2)
{
#if WANT_DEBUG

  XSegment lines[8];

  /* | | | | */

  lines[0].x1 = minx; lines[0].y1 = miny; 
  lines[0].x2 = minx; lines[0].y2 = maxy; 

  lines[1].x1 = bound_x1; lines[1].y1 = miny; 
  lines[1].x2 = bound_x1; lines[1].y2 = maxy; 

  lines[2].x1 = bound_x2; lines[2].y1 = miny; 
  lines[2].x2 = bound_x2; lines[2].y2 = maxy; 
  
  lines[3].x1 = maxx; lines[3].y1 = miny;
  lines[3].x2 = maxx; lines[3].y2 = maxy;

  /* = 
     =  */

  lines[4].x1 = minx; lines[4].y1 = miny;
  lines[4].x2 = maxx; lines[4].y2 = miny;

  lines[5].x1 = minx; lines[5].y1 = bound_y1;
  lines[5].x2 = maxx; lines[5].y2 = bound_y1;

  lines[6].x1 = minx; lines[6].y1 = bound_y2;
  lines[6].x2 = maxx; lines[6].y2 = bound_y2;

  lines[7].x1 = minx; lines[7].y1 = maxy;
  lines[7].x2 = maxx; lines[7].y2 = maxy;

  XDrawSegments(ui->xdpy, ui->xwin, ui->xgc, lines, 8);

  XSync(ui->xdpy, False);
 
#endif 
}

void
mb_stroke_ui_key_press_release(MBStrokeUI          *ui,
			       const unsigned char *utf8_char_in,
			       int                  modifiers)
{
  DBG("Sending '%s'", utf8_char_in);
  fakekey_press(ui->fakekey, utf8_char_in, -1, modifiers);
  fakekey_release(ui->fakekey);
}

void
mb_kbd_ui_keysym_press_release(MBStrokeUI  *ui,
			       KeySym       ks,
			       int          modifiers)
{
  fakekey_press_keysym(ui->fakekey, ks, modifiers);
  fakekey_release(ui->fakekey);
}


void
mb_stroke_ui_event_loop(MBStrokeUI *ui)
{

  struct timeval tvt;

  tvt.tv_sec  = 0;
  tvt.tv_usec = 0;

  while (True)
      {
	XEvent xev;

	if (ui->fade_cnt && !UI_WANT_FULLSCREEN(ui))
	  tvt.tv_usec = 50;
	else
	  tvt.tv_usec = 0;

	if (get_xevent_timed(ui->xdpy, &xev, &tvt))
	  {
	    switch (xev.type) 
	      {
	      case ButtonPress:	
		mb_stroke_ui_stroke_start(ui,
					  xev.xbutton.x,
					  xev.xbutton.y);
					  
		break;
	      case ButtonRelease:	
		mb_stroke_ui_stroke_finish(ui,
					   xev.xbutton.x,
					   xev.xbutton.y);
		break;
	      case MotionNotify:
		mb_stroke_ui_stroke_update(ui, 
					   xev.xmotion.x,
					   xev.xmotion.y);
		break;
	      case MappingNotify: 
		fakekey_reload_keysyms(ui->fakekey);
		XRefreshKeyboardMapping(&xev.xmapping);
		break;
	      case Expose:
		XCopyArea(ui->xdpy, 
			  ui->xwin_pixmap,
			  ui->xwin,
			  ui->xgc,
			  0, 0, ui->xwin_width, ui->xwin_height, 0, 0);
		  break;
	      default:
		break;
	      }
	  }
	else
	  {
	    if (ui->fade_cnt)
	      {
		mb_stroke_ui_clear_recogniser(ui, 0x9999);
		ui->fade_cnt--;
	      }
	  }
      }
}

int
mb_stroke_ui_display_width(MBStrokeUI *ui)
{
  return ui->dpy_width;
}

int
mb_stroke_ui_display_height(MBStrokeUI *ui)
{
  return ui->dpy_height;
}


int
matchbox_stroke_ui_realize(MBStrokeUI *ui)
{
  mb_stroke_ui_resources_create(ui);

  mb_stroke_ui_show(ui);

  return 1;
}


int
matchbox_stroke_ui_init(MBStroke *stroke)
{
  MBStrokeUI *ui = NULL;

  stroke->ui = ui = util_malloc0(sizeof(MBStrokeUI));

  if ((ui->xdpy = XOpenDisplay(getenv("DISPLAY"))) == NULL)
    return 0;

   if (getenv("MB_SYNC"))
     {
       XSynchronize (ui->xdpy, True);
       XSetErrorHandler(handle_xerror); 
     }

  if ((ui->fakekey = fakekey_init(ui->xdpy)) == NULL)
    return 0;


  ui->stroke      = stroke;
  ui->xscreen     = DefaultScreen(ui->xdpy);
  ui->xwin_root   = RootWindow(ui->xdpy, ui->xscreen);   

  ui->dpy_width  = DisplayWidth(ui->xdpy, ui->xscreen);
  ui->dpy_height = DisplayHeight(ui->xdpy, ui->xscreen);

  ui->mode        = stroke->ui_mode;

  return 1;
}
