#include "matchbox-stroke.h"

MBStrokeStroke*
mb_stroke_current_stroke(MBStroke *stroke)
{
  return stroke->current_stroke;
}

MBStroke*
mb_stroke_app_new(int *argc, char ***argv)
{
  MBStroke *stroke = NULL;

  stroke = util_malloc0(sizeof(MBStroke));

  if (!matchbox_stroke_ui_init(stroke))
    return NULL;

  if (!matchbox_stroke_ui_realize(stroke->ui))
    return NULL;

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
