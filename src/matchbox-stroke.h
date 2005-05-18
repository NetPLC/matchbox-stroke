#ifndef HAVE_MB_STROKE_H
#define HAVE_MB_STROKE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>

#include <locale.h>

#include <expat.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/Xresource.h>
#include <X11/keysym.h>

#include <fakekey/fakekey.h>

#if HAVE_CONFIG_H
#include "config.h"
#endif

#if (WANT_DEBUG)
#define DBG(x, a...) \
 fprintf (stderr,  __FILE__ ":%d,%s() " x "\n", __LINE__, __func__, ##a)
#else
#define DBG(x, a...) do {} while (0)
#endif

#define MARK() DBG("mark")

typedef void*         pointer;
typedef const void*   constpointer;
typedef unsigned char uchar ;
typedef Bool          boolean ;

typedef void (*UtilPlotFunc) (int x, int y, void *data);

typedef struct UtilHash            UtilHash;
typedef struct UtilHashNode        UtilHashNode;

typedef struct MBStroke            MBStroke;
typedef struct MBStrokeUI          MBStrokeUI;
typedef struct MBStrokeMode        MBStrokeMode;
typedef struct MBStrokeAction      MBStrokeAction;

typedef struct MBStrokeRegex       MBStrokeRegex;
typedef struct MBStrokeStroke      MBStrokeStroke;
typedef struct MBStrokeStrokePoint MBStrokeStrokePoint;

typedef enum 
{
  MBStrokeActionNone  = 0,
  MBStrokeActionChar,
  MBStrokeActionXKeySym,
  MBStrokeActionModifier,
  MBStrokeActionModeSwitch,
  MBStrokeActionExec,

} MBStrokeActionType;


struct MBStroke
{
  MBStrokeUI     *ui;
  MBStrokeMode   *modes, *global_mode, *current_mode;
  MBStrokeStroke *current_stroke;
};

/* stroke */

void
mb_stroke_add_mode(MBStroke     *stroke, 
		   MBStrokeMode *mode);

MBStrokeMode*
mb_stroke_lookup_mode(MBStroke     *stroke, 
		      char         *name);

MBStrokeMode*
mb_stroke_current_mode(MBStroke     *stroke);

MBStrokeMode*
mb_stroke_global_mode(MBStroke     *stroke);

MBStrokeStroke*
mb_stroke_current_stroke(MBStroke *stroke);

/* config */

int
mb_stroke_config_load(MBStroke *stroke);

/* mode */

MBStrokeMode*
mb_stroke_mode_new(MBStroke *stroke_app, const char *name);

void
mb_stroke_mode_add_exact_match(MBStrokeMode   *mode,
			       const char     *match_str,
			       MBStrokeAction *action);

boolean
mb_stroke_mode_add_fuzzy_match(MBStrokeMode   *mode,
			       const char     *match_str,
			       MBStrokeAction *action);

MBStrokeAction*
mb_stroke_mode_match_seq(MBStrokeMode   *mode,
			 char           *match_seq);

const char*
mb_stroke_mode_name(MBStrokeMode   *mode);

MBStrokeMode*
mb_stroke_mode_next(MBStrokeMode   *mode);


/* actions */

MBStrokeAction*
mb_stroke_action_new(MBStroke *stroke_app);

MBStrokeActionType
mb_stroke_action_type(MBStrokeAction *action);

void
mb_stroke_action_set_as_utf8char(MBStrokeAction      *action,
				 const unsigned char *utf8char);

void
mb_stroke_action_set_as_keysym(MBStrokeAction *action ,
			       KeySym          keysym);

void
mb_stroke_action_set_as_mode_switch(MBStrokeAction *action,
				    char           *mode_name);

void
mb_stroke_action_execute(MBStrokeAction *action);

/* Recogniser */

MBStrokeStrokePoint*
mb_stroke_stroke_point_new(int x, int y);

MBStrokeStroke*
mb_stroke_stroke_new(MBStroke *stroke_app);

void
mb_stroke_stroke_append_point(MBStrokeStroke *stroke, int x, int y);

int 				/* temp */
mb_stroke_stroke_trans (MBStrokeStroke *stroke, char *sequence);

MBStrokeStrokePoint*
mb_stroke_stroke_get_last_point(MBStrokeStroke *stroke, int *x, int *y);

MBStrokeRegex*
mb_stroke_regex_new(char *regex_str, char **err_msg);

boolean
mb_stroke_regex_match(MBStrokeRegex *match, char *seq);

MBStrokeRegex*
mb_stroke_regex_next(MBStrokeRegex *regex);

void
mb_stroke_regex_set_next(MBStrokeRegex *regex, MBStrokeRegex *next);

void
mb_stroke_regex_set_action(MBStrokeRegex *regex, MBStrokeAction *action);

MBStrokeAction*
mb_stroke_regex_get_action(MBStrokeRegex *regex);


/* UI */

void
mb_stroke_ui_key_press_release(MBStrokeUI          *ui,
			       const unsigned char *utf8_char_in,
			       int                  modifiers);
void
mb_kbd_ui_keysym_press_release(MBStrokeUI  *ui,
			       KeySym       ks,
			       int          modifiers);
void
mb_stroke_ui_event_loop(MBStrokeUI *ui);

int
matchbox_stroke_ui_realize(MBStrokeUI *ui);

int
matchbox_stroke_ui_init(MBStroke *stroke);

int
mb_stroke_ui_display_width(MBStrokeUI *ui);

int
mb_stroke_ui_display_height(MBStrokeUI *ui);

void
mb_stroke_ui_debug_grid(MBStrokeUI *ui, 
			int         minx,
			int         miny,
			int         maxx,
			int         maxy,
			int         bound_x1,
			int         bound_x2,
			int         bound_y1,
			int         bound_y2);

/* Util */

#define streq(a,b)      (strcmp(a,b) == 0)
#define strcaseeq(a,b)  (strcasecmp(a,b) == 0)
#define unless(x)       if (!(x))
#define util_abs(x)     ((x) > 0) ? (x) : -1*(x)

void*
util_malloc0(int size);

void
util_fatal_error(char *msg);

int
util_utf8_char_cnt(const unsigned char *str);

boolean 
util_file_readable(char *path);

void 	 /* skip_first_last */
util_bresenham_line(int                x1, 
		    int                y1, 
		    int                x2, 
		    int                y2,
		    UtilPlotFunc       plot_func, 
		    void               *userdata);

/* Util Hash */

UtilHash*
util_hash_new(void);

pointer
util_hash_lookup(UtilHash *hash, char *key);

void
util_hash_insert(UtilHash *hash, char *key, pointer val);

void 
util_hash_destroy(UtilHash *hash);

#endif
