#include "matchbox-stroke.h"

void*
util_malloc0(int size)
{
  void *p;

  p = malloc(size);
  memset(p, 0, size);

  return p;
}

void
util_fatal_error(char *msg)
{
  fprintf(stderr, "matchbox-stroke: *Error*  %s", msg);
  exit(1);
}



#define UTF8_COMPUTE(Char, Mask, Len)                                         \
  if (Char < 128)                                                             \
    {                                                                         \
      Len = 1;                                                                \
      Mask = 0x7f;                                                            \
    }                                                                         \
  else if ((Char & 0xe0) == 0xc0)                                             \
    {                                                                         \
      Len = 2;                                                                \
      Mask = 0x1f;                                                            \
    }                                                                         \
  else if ((Char & 0xf0) == 0xe0)                                             \
    {                                                                         \
      Len = 3;                                                                \
      Mask = 0x0f;                                                            \
    }                                                                         \
  else if ((Char & 0xf8) == 0xf0)                                             \
    {                                                                         \
      Len = 4;                                                                \
      Mask = 0x07;                                                            \
    }                                                                         \
  else if ((Char & 0xfc) == 0xf8)                                             \
    {                                                                         \
      Len = 5;                                                                \
      Mask = 0x03;                                                            \
    }                                                                         \
  else if ((Char & 0xfe) == 0xfc)                                             \
    {                                                                         \
      Len = 6;                                                                \
      Mask = 0x01;                                                            \
    }                                                                         \
  else                                                                        \
    Len = -1;


int
util_utf8_char_cnt(const unsigned char *str)
{
  const unsigned char *p = str;
  int                      mask, len, result = 0;

  /* XXX Should validate too */

  while (*p != '\0')
    {
      UTF8_COMPUTE(*p, mask, len);
      p += len;
      result++;
    }

  return result;
}

boolean 
util_file_readable(char *path)
{
  struct stat st;

  if (stat(path, &st)) 
    return False;
 
 return True;
}

void 	 /* skip_first_last */
util_bresenham_line(int                x1, 
		    int                y1, 
		    int                x2, 
		    int                y2,
		    UtilPlotFunc       plot_func, 
		    void              *userdata)
{
    int dy = y2 - y1;
    int dx = x2 - x1;
    int stepx, stepy;
    
    if (dy < 0) 
      {
	dy = -dy;
	stepy = -1;
      } 
    else stepy = 1;

    if (dx < 0) 
      {
	dx = -dx;
	stepx = -1;
      } 
    else stepx = 1;
    
    if (dx == 0 && dy == 0)
	return;

    dy <<= 1;
    dx <<= 1;

    if (dx > dy) 
      {
	int fraction = dy - (dx >> 1);

	while (1) 
	  {
	    if (fraction >= 0) 
	      {
		y1 += stepy;
		fraction -= dx;
	      }

	    x1       += stepx;
	    fraction += dy;

	    if (x1 == x2)
		break;

	    (plot_func)(x1, y1, userdata);
	}
      } 
    else 
      {
	int fraction = dx - (dy >> 1);
	while (1) 
	  {
	    if (fraction >= 0) 
	      {
		x1 += stepx;
		fraction -= dy;
	      }

	    y1       += stepy;
	    fraction += dx;

	    if (y1 == y2)
		break;

	    (plot_func)(x1, y1, userdata);
	}
    }
}
