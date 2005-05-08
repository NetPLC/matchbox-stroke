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
mb_stroke_mode_new(MBStroke *stroke_app, const char *name)
{
  MBStrokeMode *mode = NULL;

  mode = util_malloc0(sizeof(MBStrokeMode));

  /* extact_match_actions hash */

  mode->exact_match_actions = util_hash_new();
  mode->name                = strdup(name);
  mode->stroke_app          = stroke_app;

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
			       const char     *match_str,
			       MBStrokeAction *action)
{
  DBG("Adding match for mode:%s, '%s'", mode->name, match_str);
  util_hash_insert(mode->exact_match_actions, 
		   strdup(match_str), 
		   (pointer)action);

}

MBStrokeAction*
mb_stroke_mode_find_action(MBStrokeMode   *mode,
			   char           *match_seq)
{
  return util_hash_lookup(mode->exact_match_actions, match_seq);
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
