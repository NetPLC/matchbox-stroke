#include "matchbox-stroke.h"

MBStroke*
matchbox_stroke_new(int *argc, char ***argv)
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
matchbox_stroke_run(MBStroke *stroke)
{
  mb_stroke_ui_event_loop(stroke->ui);
}

int
main(int argc, char **argv)
{
  MBStroke *stroke = NULL;

  stroke = matchbox_stroke_new(&argc, &argv);



  if (stroke)
    matchbox_stroke_run(stroke);

  return 1;
}
