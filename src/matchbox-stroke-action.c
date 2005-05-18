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
mb_stroke_action_set_as_utf8char(MBStrokeAction      *action,
				 const unsigned char *utf8char)
{
  action->type             = MBStrokeActionChar;
  action->u.utf8char.data = strdup(utf8char); 
}

const unsigned char*
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
  action->type          = MBStrokeActionXKeySym;
  action->u.keysym.data = keysym; 
}

KeySym
mb_stroke_action_get_keysym(MBStrokeAction *action)
{
  if (action->type == MBStrokeActionXKeySym)
    return action->u.keysym.data; 
  
  return NoSymbol;
}


void
mb_stroke_action_set_as_mode_switch(MBStrokeAction *action,
				    char           *mode_name)
{
  action->type          = MBStrokeActionModeSwitch;
  action->u.mode.name   = strdup(mode_name); 
}

const char*
mb_stroke_action_get_mode_switch(MBStrokeAction *action)
{
  if (action->type == MBStrokeActionModeSwitch)
    return action->u.mode.name;

  return NULL;
}

void
mb_stroke_action_execute(MBStrokeAction *action)
{
  MBStroke *stroke_app;

  if (!action) 
    return;

  stroke_app = action->stroke_app;

  switch (action->type)
    {
    case MBStrokeActionChar:
      mb_stroke_ui_key_press_release(stroke_app->ui,
				     mb_stroke_action_get_utf8char(action),
				     0);
      break;
    case MBStrokeActionXKeySym:
      mb_kbd_ui_keysym_press_release(stroke_app->ui,
				     mb_stroke_action_get_keysym(action),
				     0);
      break;
    case MBStrokeActionModeSwitch:
      /* switch mode here */
      break;
    case MBStrokeActionModifier:

      break;
    case MBStrokeActionExec:

      break;
    default:
  break;

    }
}

