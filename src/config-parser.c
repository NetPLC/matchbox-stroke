#include "matchbox-stroke.h"

/* 
    <strokes>

    <options>

    </options>

    <stroke .../> 	

    <mode id="abc">
    <stroke exact="12345" 
            sloppy="21[12]323" 
            action="utf8char|lookup"
                    modifier:
		    exec: />


    </mode>
    <mode id="ABC">

    </mode>

    </strokes>
*/

struct _keysymlookup
{
  KeySym keysym;   char *name;  
} 
MBStrokeKeysymLookup[] =
{
 { XK_BackSpace,   "backspace" },
 { XK_Tab,	   "tab"       },
 { XK_Linefeed,    "linefeed"  },
 { XK_Clear,       "clear"     },	
 { XK_Return,      "return"    },
 { XK_Pause,       "pause" },	
 { XK_Scroll_Lock, "scrolllock" },	
 { XK_Sys_Req,     "sysreq" },
 { XK_Escape,      "escape" },	
 { XK_Delete,      "delete" },	
 { XK_Home,        "home" },
 { XK_Left,        "left" },
 { XK_Up,          "up"   },
 { XK_Right,       "right" },
 { XK_Down,        "down"  },
 { XK_Prior,       "prior" },		
 { XK_Page_Up,     "pageup" },	
 { XK_Next,        "next"   },
 { XK_Page_Down,   "pagedown" },
 { XK_End,         "end" },
 { XK_Begin,	   "begin" },
 { XK_space,        "space" },
 { XK_F1,          "f1" },
 { XK_F2,          "f2" },
 { XK_F3,          "f3" },
 { XK_F4,          "f4" },
 { XK_F5,          "f5" },
 { XK_F6,          "f6" },
 { XK_F7,          "f7" },
 { XK_F8,          "f8" },
 { XK_F9,          "f9" },
 { XK_F10,         "f10" },
 { XK_F11,         "f11" },
 { XK_F12,         "f12" }
};

#if 0

struct _modlookup
{
  char *name; MBKeyboardKeyModType type;
}
ModLookup[] =
{
  { "shift",   MBKeyboardKeyModShift },
  { "alt",     MBKeyboardKeyModAlt },
  { "ctrl",    MBKeyboardKeyModControl },
  { "control", MBKeyboardKeyModControl },
  { "mod1",    MBKeyboardKeyModMod1 },
  { "mod2",    MBKeyboardKeyModMod2 },
  { "mod3",    MBKeyboardKeyModMod3 },
  { "caps",    MBKeyboardKeyModCaps }
};

#endif

typedef struct MBStrokeConfigState
{
  MBStroke         *stroke;
  MBStrokeMode     *current_mode;
  Bool              error;
  char             *error_msg;
}
MBStrokeConfigState;

KeySym
config_str_to_keysym(const char* str)
{
  int i;

  DBG("checking %s", str);

  for (i=0; i<sizeof(MBStrokeKeysymLookup)/sizeof(struct _keysymlookup); i++)
    if (streq(str, MBStrokeKeysymLookup[i].name))
      return MBStrokeKeysymLookup[i].keysym;

  DBG("didnt find it %s", str);

  return 0;
}


/*
MBStrokeKeyModType
config_str_to_modtype(const char* str)
{
  int i;

  for (i=0; i<sizeof(ModLookup)/sizeof(struct _modlookup); i++)
    {
      DBG("checking '%s' vs '%s'", str, ModLookup[i].name);
      if (streq(str, ModLookup[i].name))
	return ModLookup[i].type;
    }

  return 0;
}
*/

static unsigned char* 
config_load_file(MBStroke *stroke)
{
  struct stat    stat_info;
  FILE*          fp;
  unsigned char* result;

  char          *country  = NULL;  
  char          *lang     = NULL;
  int            n = 0, i = 0;
  char           path[1024]; 	/* XXX MAXPATHLEN */

  /* strokes[-country].xml */

  /* This is an overide mainly for people developing keyboard layouts  */

  if (getenv("MB_STROKE_CONFIG"))
    {
      snprintf(path, 1024, "%s", getenv("MB_STROKE_CONFIG"));

      DBG("checking %s\n", path);

      if (util_file_readable(path))
	goto load;

      return NULL;
    }

  lang = getenv("MB_STROKE_LANG");

  if (lang == NULL)
    lang = getenv("LANG");

  if (lang)
    {
      n = strlen(lang) + 2;

      country = alloca(n);

      snprintf(country, n, "-%s", lang);

      /* strip anything after first '.' */
      while(country[i] != '\0')
	if (country[i] == '.')
	  country[i] = '\0';
	else
	  i++;
    }


  if (getenv("HOME"))
    {
      snprintf(path, 1024, "%s/matchbox/strokes.xml", getenv("HOME"));

      DBG("checking %s\n", path);

      if (util_file_readable(path))
	goto load;
    }

  snprintf(path, 1024, PKGDATADIR "/strokes%s.xml",
	   country == NULL ? "" : country);

  DBG("checking %s\n", path);

  if (util_file_readable(path))
    goto load;

  snprintf(path, 1024, PKGDATADIR "/strokes.xml");
  
  DBG("checking %s\n", path);

  if (!util_file_readable(path))
    return NULL;

 load:

  if (stat(path, &stat_info)) 
    return NULL;

  if ((fp = fopen(path, "rb")) == NULL) 
    return NULL;

  DBG("loading %s\n", path);

  result = malloc(stat_info.st_size + 1);

  n = fread(result, 1, stat_info.st_size, fp);

  if (n >= 0) result[n] = '\0';
  
  fclose(fp);

  return result;
}

static const char *
attr_get_val (char *key, const char **attr)
{
  int i = 0;
  
  while (attr[i] != NULL)
    {
      if (!strcmp(attr[i], key))
	return attr[i+1];
      i += 2;
    }
  
  return NULL;
}

static void
config_handle_mode_tag(MBStrokeConfigState *state, const char **attr)
{
  const char *val;

  if ((val = attr_get_val("id", attr)) != NULL)
    {
      state->current_mode = mb_stroke_mode_new(state->stroke, val);

      mb_stroke_add_mode(state->stroke, state->current_mode); 
    }
  else
    {
      state->error_msg = "mode tag missing id attribute";
      state->error     = True;
      return;
    }
}

static void
config_handle_stroke_tag(MBStrokeConfigState *state, const char **attr)
{
  MBStrokeAction *action = NULL;
  const char     *val = NULL, *exact = NULL; /* , *sloppy_match = NULL; */
  KeySym          found_keysym;

  /* action */

  val = attr_get_val("action", attr);

  if (!val)
    {
      state->error_msg = "stroke tag missing 'action' attribute";
      state->error     = True;
      return;
    }


  action = mb_stroke_action_new(state->stroke);

  if (!strncmp(val, "modifier:", 9))
    {
      /* TODO
      MBKeyboardKeyModType found_type;
      
      DBG("checking '%s'", &val[9]);
      
      found_type = config_str_to_modtype(&val[9]);
      
      if (found_type)
	{
	  mb_kbd_key_set_modifer_action(state->current_key,
					keystate,
					found_type);
	}
      else
	{
	  state->error = True;
	  return;
	}
      */
    }
  else if (!strncmp(val, "xkeysym:", 8))
    {
      DBG("Checking %s\n", &val[8]);

      /* found_keysym = XStringToKeysym(&val[8]); */

      /*
	    }
	  else 
	    {
	      state->error = True;
	      return;
	    }
      */
    }
  else
    {
      /* must be keysym */
      if (strlen(val) > 1  	/* match backspace, return etc */
	  && ((found_keysym  = config_str_to_keysym(val)) != 0))
	{
	  /*
	      mb_kbd_key_set_keysym_action(state->current_key, 
					   keystate,
					   found_keysym);
	  */

	}
      else
	{
	  mb_stroke_action_set_as_utf8char(action, val);

	  DBG("Addded keysym action for '%s'\n", val);
	}
    }

  /* exact match, sloppy */

  exact = attr_get_val("exact", attr);

  if (!exact)
    {
      state->error_msg = "stroke tag missing 'exact' attribute";
      state->error     = True;
      return;
    }

  
  mb_stroke_mode_add_exact_match(state->current_mode, exact, action);


  /*  */
}


static void 
config_xml_start_cb(void *data, const char *tag, const char **attr)
{
  MBStrokeConfigState *state = (MBStrokeConfigState *)data;

  if (streq(tag, "mode"))
    {
      config_handle_mode_tag(state, attr);
    }
  else if (streq(tag, "stroke"))
    {
      config_handle_stroke_tag(state, attr);
    }

  if (state->error)
    {
      util_fatal_error("Error parsing\n");
    }
}

int
mb_stroke_config_load(MBStroke *stroke)
{
  unsigned char         *data;
  XML_Parser             p;
  MBStrokeConfigState *state;

  if ((data = config_load_file(stroke)) == NULL)
    util_fatal_error("Couldn't find a keyboard config file\n");

  p = XML_ParserCreate(NULL);

  if (!p) 
    util_fatal_error("Couldn't allocate memory for XML parser\n");

  state = util_malloc0(sizeof(MBStrokeConfigState));

  state->stroke       = stroke;
  state->current_mode = mb_stroke_global_mode(stroke);

  XML_SetElementHandler(p, config_xml_start_cb, NULL);

  /* XML_SetCharacterDataHandler(p, chars); */

  XML_SetUserData(p, (void *)state);

  if (! XML_Parse(p, data, strlen(data), 1)) {
    fprintf(stderr, 
	    "matchbox-keyboard: XML Parse error at line %d:\n%s\n",
	    XML_GetCurrentLineNumber(p),
	    XML_ErrorString(XML_GetErrorCode(p)));
    util_fatal_error("XML Parse failed.\n");
  }

  return 1;
}
