#include "matchbox-stroke.h"

#include <X11/Xft/Xft.h>

struct MBStrokeUI
{
  Display      *xdpy;
  int           xscreen;
  Window        xwin_root, xwin;

  int           dpy_width;
  int           dpy_height;

  XftFont      *font;
  XftColor      font_col; 

  int           xwin_width;
  int           xwin_height;

  XftDraw      *xft_backbuffer;  
  Pixmap        backbuffer;
  GC            xgc;

  FakeKey      *fakekey;

  MBStroke     *stroke;
};

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
  */

  XWMHints            *wm_hints;
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
    = ButtonPressMask|ButtonReleaseMask|Button1MotionMask|StructureNotifyMask;

  ui->xwin = XCreateWindow(ui->xdpy,
			   ui->xwin_root,
			   0, 0,
			   320, 320,
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

  return 1;
}

void
mb_stroke_ui_event_loop(MBStrokeUI *ui)
{
  char recog[512];
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
		ui->stroke->current_stroke = mb_stroke_stroke_new();
		mb_stroke_stroke_append_point(ui->stroke->current_stroke, 
					      xev.xbutton.x,
					      xev.xbutton.y);
		break;
	      case ButtonRelease:	

 		memset(&recog[0], 0, 512);
		
		if (mb_stroke_stroke_trans (ui->stroke->current_stroke, 
					    &recog[0]))
		  {
		    DBG("got '%s'\n", recog);
		  }
		else
		  {
		    DBG("recog failed\n");
		  }

		break;
	      case MotionNotify:
		mb_stroke_stroke_append_point(ui->stroke->current_stroke, 
					      xev.xmotion.x,
					      xev.xmotion.y);
		break;
	      case MappingNotify: 
		fakekey_reload_keysyms(ui->fakekey);
		XRefreshKeyboardMapping(&xev.xmapping);
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

  ui->stroke    = stroke;
  ui->xscreen   = DefaultScreen(ui->xdpy);
  ui->xwin_root = RootWindow(ui->xdpy, ui->xscreen);   

  return 1;
}
