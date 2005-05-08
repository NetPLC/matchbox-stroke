#include "matchbox-stroke.h"

MBStrokeStroke*
mb_stroke_current_stroke(MBStroke *stroke)
{
  return stroke->current_stroke;
}


MBStrokeMode*
mb_stroke_lookup_mode(MBStroke     *stroke, 
		      char         *name)
{
  MBStrokeMode *mode_item = stroke->modes;

  while (mode_item != NULL)
    {
      if (streq( mb_stroke_mode_name(mode_item), name))
	return mode_item;
      mode_item = mb_stroke_mode_next(mode_item);
    }

  return NULL;
}

MBStrokeMode*
mb_stroke_current_mode(MBStroke     *stroke)
{
  return stroke->current_mode;
}

MBStrokeMode*
mb_stroke_global_mode(MBStroke     *stroke)
{
  return stroke->global_mode;
}

MBStroke*
mb_stroke_app_new(int *argc, char ***argv)
{
  MBStroke *stroke = NULL;

  stroke = util_malloc0(sizeof(MBStroke));

  stroke->global_mode = mb_stroke_mode_new(stroke, 
					   "global");

  mb_stroke_add_mode(stroke, stroke->global_mode);

  if (!mb_stroke_config_load(stroke))
    return NULL;

  if (!matchbox_stroke_ui_init(stroke))
    return NULL;

  if (!matchbox_stroke_ui_realize(stroke->ui))
    return NULL;

  stroke->current_mode = stroke->global_mode;

  return stroke;
}

void
mb_stroke_run(MBStroke *stroke)
{
  mb_stroke_ui_event_loop(stroke->ui);
}

int
main(int argc, char **argv)
{
  MBStroke *stroke = NULL;

  stroke = mb_stroke_app_new(&argc, &argv);

  if (stroke)
    mb_stroke_run(stroke);

  return 1;
}
