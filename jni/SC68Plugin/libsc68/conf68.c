/*
 * @file    conf68.c
 * @brief   sc68 config file
 * @author  http://sourceforge.net/users/benjihan
 *
 * Copyright (C) 1998-2013 Benjamin Gerard
 *
 * Time-stamp: <2013-08-16 05:05:28 ben>
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.
 *
 * If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_IO68_CONFIG_OPTION68_H
# include "io68/config_option68.h"
#else
# include "io68/default_option68.h"
#endif

#include "conf68.h"

/* file68 headers */
#include <sc68/file68.h>
#include <sc68/file68_err.h>
#include <sc68/file68_uri.h>
#include <sc68/file68_str.h>
#include <sc68/file68_msg.h>
#include <sc68/file68_opt.h>
#include <sc68/file68_reg.h>

/* standard headers */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifndef DEBUG_CONFIG68_O
# define DEBUG_CONFIG68_O 0
#endif

static int        config68_cat = msg68_DEFAULT;
static int        config68_use_registry = -1;
static char     * config68_def_name = "sc68";
static const char cuk_fmt[] = "CUK:Software/sashipa/sc68-%s/";
static const char lmk_str[] = "LMK:Software/sashipa/sc68/config/";

/* exported */
int          config68_opt_count;
option68_t * config68_options;


typedef union {
  int i;          /**< Used with CONFIG68_INT fields. */
  const char * s; /**< Used with CONFIG68_STR fields. */
} config68_value_t;

typedef struct _config68_field_s config68_field_t;

typedef struct
{
  int              exported; /**< exported as cli option.    */
  const char      *name;     /**< name of the entry.         */
  config68_type_t  type;     /**< Type of this config entry. */
  const char      *comment;  /**< Comment.                   */
  config68_value_t min;      /**< Minimum value allowed.     */
  config68_value_t max;      /**< Maximum value allowed.     */
  config68_value_t def;      /**< Default value the entry.   */
  config68_value_t val;      /**< Value for the entry.       */
} config68_entry_t;

struct _config68_s {
  char * name;  /**< Application name.              */
  int saved;    /**< True if config has been saved. */
  int size;     /**< Number of entries allocated.   */
  int n;        /**< Number of entries in used.     */

  /**
   * Config entry table.
   * @warning Must be at the end of the struct.
   */
  config68_entry_t entries[1];
};


/* Defines for the config default values. */
#define AMIGA_BLEND     0x5000      /* Amiga default blending factor. */
#define DEFAULT_TIME    (3*60)      /* Track default time in second.  */
#define FORCE_TRACK     0           /* 0:no forced track.             */
#define FORCE_LOOP      0           /* 0:no forced loop.              */
#define SKIP_TIME       0           /* Skip music time in sec.        */
#define MAX_TIME        (24*60*60)  /* 1 day should be enought.       */
#define DEFAULT_SEEKSPD 0x0F00      /* << 8 */
#define MAX_SEEKSPD     0x1F00

static const config68_entry_t conftab[] = {
  { 0,
    "version", CONFIG68_INT,
    "major*100+minor",
    {0}, {10000}, {PACKAGE_VERNUM}
  },
  { 1,                          /* controled by application */
    "sampling-rate", CONFIG68_INT,
    "sampling rate in Hz",
    {SAMPLING_RATE_MIN},{SAMPLING_RATE_MAX},{SAMPLING_RATE_DEF}
  },
  { 1,
    "asid", CONFIG68_INT,
    "aSIDfier settings {0:off 1:safe 2:force}",
    {0},{2},{0}
  },
  /* { 1, */
  /*   "amiga-blend", CONFIG68_INT, */
  /*   "Amiga left/right voices blending factor {32768:center}", */
  /*   {0},{65535},{AMIGA_BLEND} */
  /* }, */
  { 0,
    "force-track", CONFIG68_INT,
    "override default track {0:off}",
    {0}, {SC68_MAX_TRACK}, {FORCE_TRACK}
  },
  { 0,
    "force-loop", CONFIG68_INT,
    "override default loop {0:off -1:inf}",
    {-1}, {100}, {FORCE_LOOP}
  },
  { 0,
    "skip-time", CONFIG68_INT,
    "prevent short track from being played (in sec) {0:off}",
    {0}, {MAX_TIME}, {SKIP_TIME}
  },
  { 1,
    "default-time", CONFIG68_INT,
    "default track time (in second)",
    {0}, {MAX_TIME}, {DEFAULT_TIME}
  },
  { 0,                          /* already exported by file68 */
    "music-path", CONFIG68_STR,
    "local sc68 music path",
    {0}, {0}, {0}
  },
  { 0,                          /* currently unsupported */
    "allow-remote", CONFIG68_INT,
    "enable remote access (using curl) (disable is not upported)",
    {0}, {1}, {1}
  },
  { 0,                          /* already exported by file68 */
    "remote-music-path", CONFIG68_STR,
    "remote sc68 music path",
    {0}, {0}, {0}
  }
};

static const int nconfig = sizeof(conftab) / sizeof(conftab[0]);

static const char config_header[] =
  "# -*- conf-mode -*-\n"
  "#\n"
  "# sc68 config file generated by " PACKAGE_STRING "\n"
  "#\n"
  "# " PACKAGE_URL "\n"
  "#\n"
  "# You can edit this file. If you remove it, sc68 will generate\n"
  "# a new one at start-up with default values, but you will lost your\n"
  "# total playing time. To avoid it, you should either save its value\n"
  "# or delete all lines you want to be resetted.\n"
  "# - *int* : integer values; \"C\" number format (e.g.0xFE0120).\n"
  "# - *str* : String values; quoted with (\"); must not contain (\").\n"
  "#\n";

static int is_symbol_char(int c)
{
  return
    (c>='0' && c<='9')
    || (c>='a' && c<='z')
    || (c>='A' && c<='Z')
    || c=='_'
    || c=='.';
}

static int digit(int c, unsigned int base)
{
  int n = -1;
  if (c <= '9') {
    n = c - '0';
  } else if (c <= 'Z') {
    n = c - 'A' + 10;
  } else if (c <= 'z'){
    n = c - 'a' + 10;
  }
  if ((unsigned int)n < base) {
    return n;
  }
  return -1;
}

static int config_set_int(config68_t * conf, config68_entry_t *e, int v)
{
  int m,M;

  if (e->type != CONFIG68_INT)
    return -1;
  m = e->min.i;
  M = e->max.i;
  if (m != M) {
    if (m==0 && M == 1) {
      v = !!v;                          /* boolean */
    } else if (v < m) {
      v = m;
    } else if (v > M) {
      v = M;
    }
  }

  if (v != e->val.i) {
    conf->saved = 0;
    e->val.i = v;
  }
  return 0;
}

static int config_set_str(config68_t * conf, config68_entry_t *e,
                          const char * s)
{
  int err = 0;
  int m,M;

  if (e->type != CONFIG68_STR)
    return -1;
  m = e->min.i;
  M = e->max.i;
  if (m != M) {
    int v = s ? strlen(s) : 0;
    if (v < m || v > M) {
      s = 0;
      err = -1;
    }
  }
  if (!s) {
    if (e->val.s) {
      free((void*)e->val.s);
      e->val.s = 0;
      conf->saved = 0;
    }
  } else if (!e->val.s || strcmp(s,e->val.s)) {
    free((void*)e->val.s);
    e->val.s = strdup68(s);
    err = -!e->val.s;
    conf->saved = 0;
  }
  return err;
}


/* Check config values and correct invalid ones */
int config68_valid(config68_t * conf)
{
  int err = 0;
  int i;

  if (!conf)
    return -1;

  for (i=0; i<conf->n; i++) {
    config68_entry_t *e = conf->entries+i;
    switch (e->type) {
    case CONFIG68_INT:
      err |= config_set_int(conf, e, e->val.i);
      break;
    case CONFIG68_STR:
      err |= config_set_str(conf, e, e->val.s);
      break;
    default:
      err = -1;
    }
  }

  return -!!err;
}

static int keycmp(const char * k1, const char * k2)
{
  int c1,c2;

  if (k1 == k2) return 0;
  if (!k1) return -1;
  if (!k2) return  1;
  do {
    c1 = *k1++; if (c1 == '_') c1 = '-';
    c2 = *k2++; if (c2 == '_') c2 = '-';
  } while (c1 == c2 && c1);
  return c1 - c2;
}


int config68_get_idx(const config68_t * conf, const char * name)
{
  if (!conf)
    return -1;
  if (name) {
    int i;
    for (i=0; i<conf->n; i++) {
      if (!keycmp(name, conf->entries[i].name)) {
        return i;
      }
    }
  }
  return -1;
}

config68_type_t config68_range(const config68_t * conf, int idx,
                               int * min, int * max, int * def)
{
  config68_type_t type = CONFIG68_ERR;
  int vmin = 0 , vmax = 0, vdef = 0;

  if (conf && idx >= 0 && idx < conf->n) {
    type = conf->entries[idx].type;
    vmin = conf->entries[idx].min.i;
    vmax = conf->entries[idx].max.i;
    vdef = conf->entries[idx].def.i;
  }
  if (min) *min = vmin;
  if (max) *max = vmax;
  if (def) *def = vdef;
  return type;
}

config68_type_t config68_get(const config68_t * conf,
                             int * v,
                             const char ** name)
{
  int idx;
  config68_type_t type = CONFIG68_ERR;

  if (conf) {
    idx = v ? *v : -1;
    if (idx == -1 && name) {
      idx = config68_get_idx(conf, *name);
    }
    if (idx >= 0 && idx < conf->n) {
      switch (type = conf->entries[idx].type) {
      case CONFIG68_INT:
        if (v) *v = conf->entries[idx].val.i;
        break;

      case CONFIG68_STR:
        if (name) *name = conf->entries[idx].val.s
                    ? conf->entries[idx].val.s
                    : conf->entries[idx].def.s;
        break;

      default:
        type = CONFIG68_ERR;
        break;
      }
    }
  }
  return type;
}

config68_type_t config68_set(config68_t * conf, int idx, const char * name,
                             int v, const char * s)
{
  config68_type_t type = CONFIG68_ERR;
  if (conf) {
    if (name) {
      idx = config68_get_idx(conf, name);
    }
    if (idx >= 0 && idx < conf->n) {
      switch (type = conf->entries[idx].type) {
      case CONFIG68_INT:
        config_set_int(conf, conf->entries+idx, v);
        break;

      case CONFIG68_STR:
        if (!config_set_str(conf, conf->entries+idx, s)) {
          break;
        }
      default:
        type = CONFIG68_ERR;
        break;
      }
    }
  }
  return type;
}

static int save_config_entry(vfs68_t * os, const config68_entry_t * e)
{
  int i,err = 0;
  char tmp[64];

  /* Save comment on this entry (description and range) */
  err |= vfs68_puts(os, "\n# ") < 0;
  err |= vfs68_puts(os, e->comment) < 0;
  switch (e->type) {
  case CONFIG68_INT:
    sprintf(tmp, "; *int* [%d..%d]", e->min.i, e->max.i);
    err |= vfs68_puts(os, tmp) < 0;
    sprintf(tmp, " (%d)\n", e->def.i);
    err |= vfs68_puts(os, tmp) < 0;
    break;

  case CONFIG68_STR:
    err |= vfs68_puts(os, "; *str* (\"") < 0;
    err |= vfs68_puts(os, e->def.s) < 0;
    err |= vfs68_puts(os, "\")\n") < 0;
    break;

  default:
    vfs68_puts(os, e->name);
    vfs68_puts(os, ": invalid type\n");
    return -1;
  }

  /* transform name */
  for (i=0; e->name[i]; ++i) {
    int c = e->name[i];
    tmp[i] = (c == '-') ? '_' : c;
  }
  tmp[i] = 0;


  switch (e->type) {
  case CONFIG68_INT:
    err |= vfs68_puts(os, tmp) < 0;
    err |= vfs68_putc(os, '=') < 0;
    sprintf(tmp, "%d", e->val.i);
    err |= vfs68_puts(os, tmp) < 0;
    TRACE68(config68_cat,"conf68: save name='%s'=%d\n",e->name,e->val.i);
    break;

  case CONFIG68_STR: {
    const char * s = e->val.s ? e->val.s : e->def.s;
    if (!s) {
      s = "";
      err |= vfs68_putc(os, '#') < 0;
    }
    err |= vfs68_puts(os, tmp) < 0;
    err |= vfs68_putc(os, '=') < 0;
    err |= vfs68_putc(os, '"') < 0;
    err |= vfs68_puts(os, s) < 0;
    err |= vfs68_putc(os, '"') < 0;
    TRACE68(config68_cat,"conf68: save name='%s'=\"%s\"\n",e->name,s);
  } break;

  default:
    break;
  }
  err |= vfs68_putc(os, '\n') < 0;

  return err;
}

int config68_save(config68_t * conf)
{
  int i, err = 0;
  char tmp[128];

  if (!conf)
    return error68(0,"conf68: null pointer");

  if (!config68_use_registry) {
    /* Save into file */
    vfs68_t * os=0;
    const int sizeof_config_hd = sizeof(config_header)-1;

    strncpy(tmp, "sc68://config/", sizeof(tmp));
    strncat(tmp, conf->name, sizeof(tmp));
    os = uri68_vfs(tmp, 2, 0);
    err = vfs68_open(os);
    if (!err) {
      TRACE68(config68_cat,"conf68: save into \"%s\"\n",
              vfs68_filename(os));
      err =
        - (vfs68_write(os, config_header, sizeof_config_hd)
           != sizeof_config_hd);
    }
    for (i=0; !err && i < conf->n; ++i) {
      err = save_config_entry(os, conf->entries+i);
    }
    vfs68_close(os);
    vfs68_destroy(os);
  } else {
    /* Save into registry */
    int l = snprintf(tmp, sizeof(tmp), cuk_fmt, conf->name);
    char * s = tmp + l;
    l = sizeof(tmp) - l;

    for (i=0; i<conf->n; ++i) {
      config68_entry_t * e = conf->entries+i;
      strncpy(s,e->name,l);
      switch (e->type) {
      case CONFIG68_INT:
        TRACE68(config68_cat,
                "conf68: save '%s' <- %d\n", tmp, e->val.i);
        err |= registry68_puti(0, tmp, e->val.i);
        break;
      case CONFIG68_STR:
        if (e->val.s) {
          TRACE68(config68_cat,
                  "conf68: save '%s' <- '%s'\n", tmp, e->val.s);
          err |= registry68_puts(0, tmp, e->val.s);
        }
      default:
        break;
      }
    }

  }

  return err;
}

/* Load config from registry */
static int load_from_registry(config68_t * conf)
{
  int err = 0, i, j;
  char paths[2][64];

  j = 0;
  snprintf(paths[j], sizeof(paths[j]), cuk_fmt, conf->name);
  ++j;
  strncpy(paths[j], lmk_str, sizeof(paths[j]));
  ++j;

  for (i=0; i<conf->n; ++i) {
    config68_entry_t * e = conf->entries+i;
    char path[128], str[512];
    int  k, val;

    for (k=0; k<j; ++k) {
      strncpy(path, paths[k], sizeof(path));
      strncat(path, e->name, sizeof(path));

      TRACE68(config68_cat, "conf68: trying -- '%s'\n", path);
      if (e->type == CONFIG68_STR)
        err = registry68_gets(0, path, str, sizeof(str));
      else
        err = registry68_geti(0, path, &val);

      if (!err) {
        if (e->type == CONFIG68_STR) {
          config_set_str(conf, conf->entries+i, str);
          TRACE68(config68_cat,
                  "conf68: load '%s' <- '%s'\n", path, e->val.s);
        } else {
          config_set_int(conf, conf->entries+i, val);
          TRACE68(config68_cat,
                  "conf68: load '%s' <- %d\n", path, e->val.i);
        }
        break;
      }
    }
  }

  return 0;
}

/* Load config from file */
static int load_from_file(config68_t * conf)
{
  vfs68_t * is = 0;
  char s[256], * word;
  int err;
  config68_type_t type;

  if (err = config68_default(conf), err)
    goto error;

  strcpy(s, "sc68://config/");
  strcat(s, conf->name);
  is = uri68_vfs(s, 1, 0);
  err = vfs68_open(is);
  if (err)
    goto error;

  for(;;) {
    char * name;
    int i, len, c = 0, idx;

    len = vfs68_gets(is, s, sizeof(s));
    if (len == -1) {
      err = -1;
      break;
    }
    if (len == 0) {
      break;
    }

    i = 0;

    /* Skip space */
    while (i < len && (c=s[i++], (c == ' ' || c == 9)))
      ;

    if (!is_symbol_char(c)) {
      continue;
    }

    /* Get symbol name. */
    name = s+i-1;
    while (i < len && is_symbol_char(c = s[i++]))
      if (c == '_') s[i-1] = c = '-';
    s[i-1] = 0;

    /* TRACE68(config68_cat,"conf68: load get key name='%s\n", name); */

    /* Skip space */
    while (i < len && (c == ' ' || c == 9)) {
      c=s[i++];
    }

    /* Must have '=' */
    if (c != '=') {
      continue;
    }
    c=s[i++];
    /* Skip space */
    while (i < len && (c == ' ' || c == 9)) {
      c=s[i++];
    }

    if (c == '"') {
      type = CONFIG68_STR;
      word = s + i;
      /*       TRACE68(config68_cat, */
      /*               "conf68: load name='%s' looks like a string(%d)\n", */
      /*               name, type); */
    } else if (c == '-' || digit(c, 10) != -1) {
      type = CONFIG68_INT;
      word = s + i - 1;
      /*       TRACE68(config68_cat, */
      /*               "conf68: load name='%s' looks like an int(%d)\n", name, type); */
    } else {
      TRACE68(config68_cat,
              "conf68: load name='%s' looks like nothing valid\n", name);
      continue;
    }
    /*     TRACE68(config68_cat, */
    /*             "conf68: load name='%s' not parsed value='%s'\n", name, word); */

    idx = config68_get_idx(conf, name);
    if (idx < 0) {
      /* Create this config entry */
      TRACE68(config68_cat, "conf68: load name='%s' unknown\n", name);
      continue;
    }
    if (conf->entries[idx].type != type) {
      TRACE68(config68_cat, "conf68: load name='%s' types differ\n", name);
      continue;
    }

    switch (type) {
    case CONFIG68_INT:
      config_set_int(conf, conf->entries+idx, strtoul(word, 0, 0));
      TRACE68(config68_cat, "conf68: load name='%s'=%d\n",
              conf->entries[idx].name, conf->entries[idx].val.i);
      break;
    case CONFIG68_STR:
      while (i < len && (c=s[i++], c && c != '"'))
        ;
      s[i-1] = 0;
      config_set_str(conf, conf->entries+idx, word);
      TRACE68(config68_cat, "conf68: load name='%s'=\"%s\"\n",
              conf->entries[idx].name, conf->entries[idx].val.s);
    default:
      break;
    }
  }

 error:
  vfs68_destroy(is);
  TRACE68(config68_cat, "conf68: loaded => [%s]\n",strok68(err));
  return err;

}


/* Load config */
int config68_load(config68_t * conf)
{
  int err = -1;
  if (conf) {
    err = config68_use_registry
      ? load_from_registry(conf)
      : load_from_file(conf)
      ;
    if (!err)
      err = config68_valid(conf);
  }
  return err;
}

/* Fill config struct with default values.
 */
int config68_default(config68_t * conf)
{
  int i;

  if(!conf)
    return -1;

  for (i=0; i < conf->n; i++) {
    config68_entry_t * e = conf->entries+i;

    switch (e->type) {
    case CONFIG68_INT:
      e->val.i = e->def.i;
      break;
    case CONFIG68_STR:
      free((void*)e->val.s);
      e->val.s = 0;
    default:
      break;
    }
  }
  conf->saved = 0;

  return config68_valid(conf);
}

config68_t * config68_create(const char * appname, int size)
{
  config68_t * c;
  int i,j;

  if (size < nconfig)
    size = nconfig;

  c = malloc(sizeof(*c)-sizeof(c->entries)+sizeof(*c->entries)*size);
  if (c) {
    if (appname)
      c->name = strdup68(appname);
    if (!c->name)
      c->name = config68_def_name;
    c->size  = size;
    c->saved = 0;
    for (j=i=0; i<nconfig; ++i) {
      c->entries[j] = conftab[i];
      switch(c->entries[j].type) {
      case CONFIG68_INT:
        config_set_int(c, c->entries+j, c->entries[j].def.i);
        ++j;
        break;

      case CONFIG68_STR:
        c->entries[j].val.s = 0;
        c->entries[j].def.s = 0;
        config_set_str(c, c->entries+j, 0);
        ++j;
        break;

      default:
        break;
      }
    }
    c->n = j;
  }

  return c;
}

void config68_destroy(config68_t * c)
{
  if (c) {
    int i;

    if (c->name != config68_def_name)
      free(c->name);

    for (i=0; i<c->n; ++i) {
      if (c->entries[i].type == CONFIG68_STR) {
        free((void*)c->entries[i].val.s);
      }
    }
    free(c);
  }
}

int config68_init(int force_file)
{
  if (config68_cat == msg68_DEFAULT) {
    int f = msg68_cat("conf","config file", DEBUG_CONFIG68_O);
    if (f > 0) config68_cat = f;
  }

  if (config68_use_registry < 0) {
    config68_use_registry = !force_file && registry68_support();
    TRACE68(config68_cat,
            "conf68: will use %s\n",
            config68_use_registry?"registry":"config file");
  }

  if (!config68_options) {
    int i,n;
    option68_t * options = 0;

    /* count exported config key. */
    for (i=n=0; i<nconfig; ++i) {
      n += !!conftab[i].exported;
    }

    if (n > 0) {
      options = malloc(n*sizeof(*options));
      if (!options) {
        msg68_error("conf68: alloc error\n");
      } else {
        int j;
        for (i=j=0; i<nconfig; ++i) {
          if (!conftab[i].exported) continue;
          options[j].has_arg = (conftab[i].type == CONFIG68_INT)
            ? option68_INT : option68_STR;
          options[j].prefix  = "sc68-";
          options[j].name    = conftab[i].name;
          options[j].cat     = "config";
          options[j].desc    = conftab[i].comment;
          options[j].val.num = 0;
          options[j].val.str = 0;
          options[j].next    = 0;
          options[j].name_len =
            options[j].prefix_len = 0;
          TRACE68(config68_cat,"config68: export %s %s%s\n",
                  options[j].cat, options[j].prefix, options[j].name);
          ++j;
        }
      }
    }
    config68_options      = options;
    config68_opt_count = n;
  }
  return 0;
}

void config68_shutdown(void)
{
  /* release options */
  if (config68_options) {
    int i;
    for (i=0; i<config68_opt_count; ++i) {
      if (config68_options[i].next) {
        msg68_critical("config68: option #%d '%s' still attached\n",
                       i, config68_options[i].name);
        break;
      }
    }
    if (i == config68_opt_count)
      free(config68_options);
    config68_options = 0;
    config68_opt_count = 0;
  }

  /* release debug feature. */
  if (config68_cat != msg68_DEFAULT) {
    msg68_cat_free(config68_cat);
    config68_cat = msg68_DEFAULT;
  }
}
