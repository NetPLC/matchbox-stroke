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

typedef struct UtilHash            UtilHash;
typedef struct UtilHashNode        UtilHashNode;

typedef struct MBStroke            MBStroke;
typedef struct MBStrokeUI          MBStrokeUI;
typedef struct MBStrokeMode        MBStrokeMode;
typedef struct MBStrokeAction      MBStrokeAction;
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


struct MBStrokeMode
{
  char        *name;
  UtilHash    *exact_match_actions; 

  /*
  UtilRegexps *fuzzy_matches;
  */

  MBStrokeMode *next;
};

struct MBStrokeAction
{
  MBStrokeActionType type;

  union 
  {
    struct 
    {
      uchar *data;
    } 
    utf8char; 

    struct 
    {
      KeySym data;
    } 
    keysym; 

    struct 
    {
      int data; /* ??? */
    } 
    modifier; 

    struct 
    {
      MBStrokeMode data;
    } 
    mode; 

    struct 
    {
      char *data;
    } 
    exec; 

  } u;
};

/* stroke */


/* Recogniser */

MBStrokeStrokePoint*
mb_stroke_stroke_point_new(int x, int y);

MBStrokeStroke*
mb_stroke_stroke_new(void);

void
mb_stroke_stroke_append_point(MBStrokeStroke *stroke, int x, int y);

int 				/* temp */
mb_stroke_stroke_trans (MBStrokeStroke *stroke, char *sequence);


/* UI */

void
mb_stroke_ui_event_loop(MBStrokeUI *ui);

int
matchbox_stroke_ui_realize(MBStrokeUI *ui);

int
matchbox_stroke_ui_init(MBStroke *stroke);

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
