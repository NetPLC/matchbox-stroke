#include "matchbox-stroke.h"

struct MBStrokeMode
{
  char        *name;
  UtilHash    *exact_match_actions; 

  /*
    UtilRegexps *fuzzy_matches;
  */

  MBStroke     *stroke_app;
  MBStrokeMode *next;
};

MBStrokeMode*
mb_stroke_mode_new(MBStroke *stroke_app, char *name)
{
  MBStrokeMode *mode = NULL;

  mode = util_malloc0(sizeof(MBStrokeMode));

  /* extact_match_actions hash */

  mode->name       = strdup(name);
  mode->stroke_app = stroke_app;

  return mode;
}

void
mb_stroke_add_mode(MBStroke     *stroke, 
		   MBStrokeMode *mode)
{
  MBStrokeMode *mode_item = NULL;

  if (stroke->modes == NULL)
    {
      stroke->modes = mode;
      return;
    }

  mode_item = stroke->modes;

  while (mb_stroke_mode_next(mode_item) != NULL) 
    mode_item = mb_stroke_mode_next(mode_item);
 
 
  mode_item->next = mode;
}


void
mb_stroke_mode_add_exact_match(MBStrokeMode   *mode,
			       char           *match_str,
			       MBStrokeAction *action)
{
  ;
}


const char*
mb_stroke_mode_name(MBStrokeMode   *mode)
{
  return mode->name;
}


MBStrokeMode*
mb_stroke_mode_next(MBStrokeMode   *mode)
{
  return mode->next;
}
