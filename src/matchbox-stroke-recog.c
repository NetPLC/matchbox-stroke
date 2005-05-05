#include "matchbox-stroke.h"

/* 
 * Code here very much based on that in libstroke. 
 */

#include <math.h> 		/* for fabs() :/ */

/* largest number of points allowed to be sampled */
#define STROKE_MAX_POINTS 10000

/* number of sample points required to have a valid stroke */
#define STROKE_MIN_POINTS 50

/* maximum number of numbers in stroke */
#define STROKE_MAX_SEQUENCE 20

/* threshold of size of smaller axis needed for it to define its own
   bin size */
#define STROKE_SCALE_RATIO 4

/* minimum percentage of points in bin needed to add to sequence */
#define STROKE_BIN_COUNT_PERCENT 0.07

struct MBStrokeStroke
{
  int                  min_x, min_y, max_x, max_y;
  MBStrokeStrokePoint *first_point, *last_point;
  int                  n_points;
  int                  last_x, last_y;
  MBStroke            *stroke_app;
};


struct MBStrokeStrokePoint
{
  MBStrokeStrokePoint *next;
  int x;
  int y;
};

MBStrokeStrokePoint*
mb_stroke_stroke_point_new(int x, int y)
{
  MBStrokeStrokePoint *pt = NULL;

  pt = util_malloc0(sizeof(MBStrokeStrokePoint));

  pt->x    = x;
  pt->y    = y;
  pt->next = NULL;

  return pt;
}

MBStrokeStroke*
mb_stroke_stroke_new(MBStroke *stroke_app)
{
  MBStrokeStroke *stroke = NULL;

  stroke = util_malloc0(sizeof(MBStrokeStroke));

  stroke->min_x = 10000;
  stroke->min_y = 10000;
  stroke->max_x = -1;
  stroke->max_y = -1;
  stroke->first_point        = NULL;
  stroke->stroke_app         = stroke_app;

  stroke_app->current_stroke = stroke;

  return stroke;
}

MBStrokeStrokePoint*
mb_stroke_stroke_get_last_point(MBStrokeStroke *stroke, int *x, int *y)
{
  MBStrokeStrokePoint *pt;

  if (stroke->first_point == stroke->last_point)
    return NULL;

  for (pt = stroke->first_point; 
       pt->next != stroke->last_point;
       pt = pt->next);

  *x = pt->x;
  *y = pt->y;

  return pt;
}

void
mb_stroke_stroke_append_point(MBStrokeStroke *stroke, int x, int y)
{
  MBStrokeStrokePoint *new_pt = NULL;
  int                  delx, dely;
  float                ix, iy; 

  /*  XXX TODO: 
   *   Does it make more sense to preallocate stroke points ( array )
   *   and avoid mass of mallocs etc ?
  */

  if (stroke->n_points < STROKE_MAX_POINTS) 
    {
      new_pt = mb_stroke_stroke_point_new(x, y);

      if (stroke->first_point == NULL)
	{
	  stroke->first_point = stroke->last_point = new_pt;
	}
      else
	{
	  /* XXX FIX THIS
           *  - get rid of float code !
           *  - Im sure interp can be done nicer and smaller
	  */

	  /* interpolate between last and current point */
	  delx = (x - stroke->last_point->x);
	  dely = (y - stroke->last_point->y);

	  /* step by the greatest delta direction */
	  if (abs(delx) > abs(dely)) 
	    {
	      iy = stroke->last_point->y;

	      /* go from the last point to the current, whatever direction it
		 may be */
	      for (ix = stroke->last_point->x;
		   (delx > 0) ? (ix < x) : (ix > x);
		   ix += (delx > 0) ? 1 : -1) 
		{

		  /* step the other axis by the correct increment */

		  iy += fabs(((float) dely
			      / (float) delx)) * (float) ((dely < 0) ? -1.0 : 1.0);

		  /* add the interpolated point */
		  stroke->last_point->next = new_pt;
		  stroke->last_point       = new_pt;
		  new_pt->x = (int)ix;
		  new_pt->y = (int)iy;

		  /* update metrics */
		  if (((int) ix) < stroke->min_x) stroke->min_x = (int) ix;
		  if (((int) ix) > stroke->max_x) stroke->max_x = (int) ix;
		  if (((int) iy) < stroke->min_y) stroke->min_y = (int) iy;
		  if (((int) iy) > stroke->max_y) stroke->max_y = (int) iy;
		  stroke->n_points++;

		  new_pt = mb_stroke_stroke_point_new(0, 0);
		}
	    } 
	  else   /* same thing, but for dely larger than delx case... */
	    {
	      ix = stroke->last_point->x;

	      /* go from the last point to the current, whatever direction
		 it may be */
	      for (iy = stroke->last_point->y; 
		   (dely > 0) ? (iy < y) : (iy > y);
		   iy += (dely > 0) ? 1 : -1) 
		{

		  /* step the other axis by the correct increment */
		  ix += fabs(((float) delx / (float) dely))
		    * (float) ((delx < 0) ? -1.0 : 1.0);

		  /* add the interpolated point */
		  stroke->last_point->next = new_pt;
		  stroke->last_point = new_pt;
		  new_pt->x = (int)ix;
		  new_pt->y = (int)iy;

		  /* update metrics */
		  if (((int) ix) < stroke->min_x) stroke->min_x = (int) ix;
		  if (((int) ix) > stroke->max_x) stroke->max_x = (int) ix;
		  if (((int) iy) < stroke->min_y) stroke->min_y = (int) iy;
		  if (((int) iy) > stroke->max_y) stroke->max_y = (int) iy;
		  stroke->n_points++;

		  new_pt = mb_stroke_stroke_point_new(0, 0);
		}
	    }
	}

      new_pt->x = x;
      new_pt->y = y;
      new_pt->next = NULL;

      
    }
}

static int 
stroke_bin (MBStrokeStrokePoint *pt, 
	    int                  bound_x_1, 
	    int                  bound_x_2,
            int                  bound_y_1, 
	    int                  bound_y_2)
{
  int bin_num = 1;
  if (pt->x > bound_x_1) bin_num += 1;
  if (pt->x > bound_x_2) bin_num += 1;
  if (pt->y > bound_y_1) bin_num += 3;
  if (pt->y > bound_y_2) bin_num += 3;

  return bin_num;
}


int 
mb_stroke_stroke_trans (MBStrokeStroke *stroke, char *sequence)
{
  /* number of bins recorded in the stroke */
  int sequence_count = 0;

  /* points-->sequence translation scratch variables */
  int prev_bin    = 0;
  int current_bin = 0;
  int bin_count   = 0;

  /* flag indicating the start of a stroke - always count it in the sequence */
  int first_bin = 1;

  /* bin boundary and size variables */
  int delta_x, delta_y;
  int bound_x_1, bound_x_2;
  int bound_y_1, bound_y_2;
  int pt_cnt = 0;

  MBStrokeStrokePoint *tmp_pt = NULL;

  /* determine size of grid */
  delta_x = stroke->max_x - stroke->min_x;
  delta_y = stroke->max_y - stroke->min_y;

  /* calculate bin boundary positions */
  bound_x_1 = stroke->min_x + (delta_x / 3);
  bound_x_2 = stroke->min_x + 2 * (delta_x / 3);

  bound_y_1 = stroke->min_y + (delta_y / 3);
  bound_y_2 = stroke->min_y + 2 * (delta_y / 3);

  if (delta_x > STROKE_SCALE_RATIO * delta_y) 
    {
      bound_y_1 = (stroke->max_y + stroke->min_y - delta_x) / 2 + (delta_x / 3);
      bound_y_2 = (stroke->max_y + stroke->min_y - delta_x) / 2 + 2 * (delta_x / 3);
    } 
  else if (delta_y > STROKE_SCALE_RATIO * delta_x) 
    {
      bound_x_1 = (stroke->max_x + stroke->min_x - delta_y) / 2 + (delta_y / 3);
      bound_x_2 = (stroke->max_x + stroke->min_x - delta_y) / 2 + 2 * (delta_y / 3);
    }

  DBG(" point count: %d\n",stroke->n_points);
  DBG(" min_x: %d\n",stroke->min_x);
  DBG(" max_x: %d\n",stroke->max_x);
  DBG(" min_y: %d\n",stroke->min_y);
  DBG(" max_y: %d\n",stroke->max_y);
  DBG(" delta_x: %d\n",delta_x);
  DBG(" delta_y: %d\n",delta_y);
  DBG(" bound_x_1: %d\n",bound_x_1);
  DBG(" bound_x_2: %d\n",bound_x_2);
  DBG(" bound_y_1: %d\n",bound_y_1);
  DBG(" bound_y_2: %d\n",bound_y_2);
  

  /*
    build string by placing points in bins, collapsing bins and
    discarding those with too few points...
  */

  while (stroke->first_point != NULL) 
    {


      /* figure out which bin the point falls in */
      current_bin = stroke_bin(stroke->first_point,
			       bound_x_1, bound_x_2,
			       bound_y_1, bound_y_2);

    /* if this is the first point, consider it the previous bin,
       too. */

      prev_bin = (prev_bin == 0) ? current_bin : prev_bin;

      if (prev_bin == current_bin)
	bin_count++;
      else 
	{  /* we are moving to a new bin -- consider adding to the
               sequence */
	  if ((bin_count > (stroke->n_points * STROKE_BIN_COUNT_PERCENT))
	      || (first_bin == 1)) {
	    first_bin = 0;
	    sequence[sequence_count++] = '0' + prev_bin;
	    /*        printf ("DEBUG:: adding sequence: %d\n",prev_bin);
	     */
	  }

	  /* restart counting points in the new bin */
	  bin_count=0;
	  prev_bin = current_bin;
	}

      /* move to next point, freeing current point from list */

      tmp_pt = stroke->first_point;
      stroke->first_point = stroke->first_point->next;

      if (tmp_pt)
	  free (tmp_pt);

      pt_cnt++;
    }

  stroke->last_point = NULL;

  /* add the last run of points to the sequence */
  sequence[sequence_count++] = '0' + current_bin;
  
  DBG("adding final sequence: %d\n",current_bin); 

  /* bail out on error cases */
  if ((stroke->n_points < STROKE_MIN_POINTS)
      || (sequence_count > STROKE_MAX_SEQUENCE)) 
    {
      stroke->n_points = 0;
      strcpy (sequence,"0");
      return 0;
    }

  /* add null termination and leave */
  stroke->n_points = 0;
  sequence[sequence_count] = '\0';

  return 1;
}



