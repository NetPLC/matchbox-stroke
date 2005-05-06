#include "matchbox-stroke.h"

struct MBStrokeAction
{
  MBStrokeActionType type;
  MBStroke          *stroke_app;

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
      char *name;
    } 
    mode; 

    struct 
    {
      char *data;
    } 
    exec; 

  } u;
};

MBStrokeAction*
mb_stroke_action_new(MBStroke *stroke_app)
{
  MBStrokeAction *action = NULL;

  action = util_malloc0(sizeof(MBStrokeAction));

  action->stroke_app = stroke_app;
  action->type       = MBStrokeActionNone;

  return action;
}

MBStrokeActionType
mb_stroke_action_type(MBStrokeAction *action)
{
  return action->type;
}

void
mb_stroke_action_set_as_utf8char(MBStrokeAction *action,
				 unsigned char  *utf8char)
{
  action->type             = MBStrokeActionChar;
  action->u.utf8char.data = utf8char; 
}

unsigned char*
mb_stroke_action_get_utf8char(MBStrokeAction *action)
{
  if (action->type == MBStrokeActionChar)
    return action->u.utf8char.data;

  return NULL;
}

void
mb_stroke_action_set_as_keysym(MBStrokeAction *action ,
			       KeySym          keysym)
{
  ;
}

void
mb_stroke_action_set_as_mode_switch(MBStrokeAction *action,
				    char           *mode_name)
{
  ;
}
