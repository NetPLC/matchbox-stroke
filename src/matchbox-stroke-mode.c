#include "matchbox-stroke.h"


struct MBStrokeMode
{
  char           *name;
  UtilHash       *exact_match_actions; 
  MBStrokeRegex  *fuzzy_matches;

  /*
    UtilRegexps *fuzzy_matches;
  */

  MBStroke       *stroke_app;
  MBStrokeMode  *next;
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

boolean
mb_stroke_mode_add_fuzzy_match(MBStrokeMode   *mode,
			       const char     *match_str,
			       MBStrokeAction *action)
{
  MBStrokeRegex  *last_match = mode->fuzzy_matches, *new_regex = NULL;

  DBG("Adding fuzzy match for mode:%s, '%s'", mode->name, match_str);
  
  if ((new_regex = mb_stroke_regex_new(match_str, NULL)) == NULL)
    return False; 		/* XXX return error ? */

  mb_stroke_regex_set_action(new_regex, action);

  if (last_match == NULL)
    {
      mode->fuzzy_matches = new_regex;
      return True;
    }
  
  while (mb_stroke_regex_next(last_match) != NULL)
    last_match = mb_stroke_regex_next(last_match);

  mb_stroke_regex_set_next(last_match, new_regex);

  return True;
}

MBStrokeAction*
mb_stroke_mode_match_seq(MBStrokeMode   *mode,
			 char           *match_seq)
{
  MBStrokeRegex  *reg_item =  mode->fuzzy_matches;
  MBStrokeAction *action   = NULL;

  action = util_hash_lookup(mode->exact_match_actions, match_seq);

  if (action)
    return action;

  /* fun bit */

  while (reg_item != NULL)
    {
      if (mb_stroke_regex_match(reg_item, match_seq))
	{
	  action = mb_stroke_regex_get_action(reg_item);

	  /* cache the seq in the exact hash so we dont need to 
           * hit the regexps again for this.
           *
           * XXX Does it make sense to cache this data on disk or
           *     some such for the user ?
	  */
	  mb_stroke_mode_add_exact_match(mode, match_seq, action);

	  return action;
	}

      reg_item = mb_stroke_regex_next(reg_item);
    }

  return NULL;
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
