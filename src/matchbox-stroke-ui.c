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

  Pixmap             pentip_pixmap;
  Picture            pentip_pict;
  XRenderPictFormat *pentip_format;


  FakeKey      *fakekey;
  MBStroke     *stroke;

  XftFont      *font;
  XftColor      font_col; 

  /* unused ? */

  XftDraw      *xft_backbuffer;  
  Pixmap        backbuffer;
  GC            xgc;
};

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


static void
mb_stroke_ui_clear_recogniser(MBStrokeUI *ui);


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
    atom_NET_WM_STATE_SKIP_TASKBAR,
    atom_NET_WM_STATE,
    atom_MOTIF_WM_HINTS;
  

  /*
  PropMotifWmHints    *mwm_hints;
  XSizeHints           size_hints;
  XRenderColor         coltmp;
  XWMHints            *wm_hints;
  */

  XSetWindowAttributes win_attr;



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

  atom_NET_WM_STATE =
    XInternAtom(ui->xdpy, "_NET_WM_STATE", False);


  win_attr.override_redirect = False; /* Set to true for extreme case */
  win_attr.event_mask 
    = ButtonPressMask|ButtonReleaseMask|Button1MotionMask|
    StructureNotifyMask|ExposureMask;

  ui->xwin = XCreateWindow(ui->xdpy,
			   ui->xwin_root,
			   0, 0,
			   ui->xwin_width, ui->xwin_height,
			   0,
			   CopyFromParent, CopyFromParent, CopyFromParent,
			   CWOverrideRedirect|CWEventMask,
			   &win_attr);

#if 0
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

#endif

    ui->xwin_format 
      = XRenderFindVisualFormat(ui->xdpy,DefaultVisual(ui->xdpy, ui->xscreen));

    ui->xwin_pixmap = XCreatePixmap(ui->xdpy, ui->xwin,
				    ui->xwin_width,
				    ui->xwin_height,
				    DefaultDepth(ui->xdpy, ui->xscreen));

    /* attr.subwindow_mode = IncludeInferiors; */

    ui->xwin_pict = XRenderCreatePicture(ui->xdpy, ui->xwin_pixmap, 
					 ui->xwin_format,
					 0 /* CPSubwindowMode */, NULL);

    init_pentip_image(ui);  

    ui->xgc = XCreateGC(ui->xdpy, ui->xwin, 0, NULL);

    XSetBackground(ui->xdpy, ui->xgc, BlackPixel(ui->xdpy, ui->xscreen ));
    XSetForeground(ui->xdpy, ui->xgc, WhitePixel(ui->xdpy, ui->xscreen ));

    mb_stroke_ui_clear_recogniser(ui);

    
    return 1;
}


static void
mb_stroke_ui_clear_recogniser(MBStrokeUI *ui)
{
  XRenderColor color;

  color.red = color.green = color.blue = color.alpha = 0xffff;

  XRenderFillRectangle(ui->xdpy,
		       PictOpSrc, 
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
		   7, 7);
  
  XCopyArea(ui->xdpy, 
	    ui->xwin_pixmap,
	    ui->xwin,
	    ui->xgc,
	    0, 0, ui->xwin_width, ui->xwin_height, 0, 0);

  XFlush(ui->xdpy);
}



void
mb_stroke_ui_pentip_draw_line_to(MBStrokeUI *ui, int x2, int y2)
{
  int x1, y1;

  if (mb_stroke_stroke_get_last_point(mb_stroke_current_stroke(ui->stroke),
				      &x1, &y1))
    {
      DBG("ENTER\n");
      util_bresenham_line(x1, y1, x2, y2,  
			  &mb_stroke_ui_pentip_draw_point,
			  (void *)ui);
      DBG("LEAVE\n");
    }
}

void
mb_stroke_ui_stroke_start(MBStrokeUI *ui, int x, int y)
{
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
      printf("got '%s'\n", recog);
    }
  else
    {
      printf("recog failed\n");
    }

  mb_stroke_ui_clear_recogniser(ui);
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
	    ; /* Nothing yet */
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

  if ((ui->fakekey = fakekey_init(ui->xdpy)) == NULL)
    return 0;

  ui->stroke      = stroke;
  ui->xscreen     = DefaultScreen(ui->xdpy);
  ui->xwin_root   = RootWindow(ui->xdpy, ui->xscreen);   
  ui->xwin_width  = 320;
  ui->xwin_height = 320;

  return 1;
}
