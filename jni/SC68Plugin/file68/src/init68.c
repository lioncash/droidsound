/*
 * @file    init68.c
 * @brief   library initialization
 * @author  http://sourceforge.net/users/benjihan
 *
 * Copyright (C) 2001-2013 Benjamin Gerard
 *
 * Time-stamp: <2013-08-16 01:15:54 ben>
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
# include "config.h"
#endif

#include "file68_api.h"
#include "file68.h"
#include "file68_opt.h"
#include "file68_msg.h"
#include "file68_err.h"
#include "file68_reg.h"
#include "file68_vfs_ao.h"
#include "file68_vfs_curl.h"
#include "file68_vfs_fd.h"
#include "file68_vfs_file.h"
#include "file68_vfs_mem.h"
#include "file68_vfs_null.h"
#include "file68_vfs_z.h"
#include "file68_rsc.h"
#include "file68_str.h"

#include <stdlib.h>
#include <string.h>

static volatile int init;

int  option68_init(void);              /* defined in option68.c   */
void option68_shutdown(void);          /* defined in option68.c   */
int  file68_loader_init(void);         /* defined in file68.c     */
void file68_loader_shutdown(void);     /* defined in file68.c     */
static char * mygetenv(const char *name)
{
#ifndef HAVE_GETENV
  return 0;
#else
  return getenv(name);
#endif
}

/* Get path from registry, converts '\' to '/' and remove trailing '/'.
 *
 * @return pointer to the end of string
 * @retval 0 error
 */
static char * get_reg_path(registry68_key_t key, char * kname,
                           char * buffer, int buflen)
{
  char * e = 0;

  if (registry68_support() &&
      !registry68_gets(key,kname,buffer,buflen)) {
    for (e=buffer; *e; ++e) {
      if (*e == '\\') *e = '/';
    }
    if (e > buffer && e[-1] != '/')
      --e;
    *e = 0;
  }
  if (!e) buffer[0] = 0;
  return e;
}

static const char dbgcat[] = "debug";
static const char rsccat[] = "files";
static const char prefix[] = "sc68-";

/* list of options / envvars
 *
 * "debug" is a string to be evaluted as an integer later in the
 * process (once debug features have been defined).
 *
 */
static option68_t opts[] = {
  {0,option68_BOL,prefix,"no-debug",dbgcat,"disable all debug output"      },
  {0,option68_STR,prefix,"debug"   ,dbgcat,"set debug features"            },
  {0,option68_STR,prefix,"data"    ,rsccat,"shared (system) resource path" },
  {0,option68_STR,prefix,"home"    ,rsccat,"private (user) resource path"  },
  {0,option68_STR,prefix,"music"   ,rsccat,"music database path"           },
  {0,option68_STR,prefix,"rmusic"  ,rsccat,"online music base URI"         }
};

static char * convert_backslash(char * s) {
  int i;
  for (i=0; s[i]; ++i)
    if (s[i] == '\\')
      s[i] = '/';
  return s;
}

int file68_init(int argc, char **argv)
{
  char tmp[1024];
  option68_t * opt;

  if (init) {
    const int i = init & 3;
    const char *message[4] = {
      "clean","initialized","shutdowning","initializing"
    };
    error68("file68: init error -- *%s*", message[i]);
    argc = -1;
    goto out_no_init;
  }
  init = 3;

  /* Options */
  option68_init();

  /* Zlib */
  vfs68_z_init();

  /* Curl */
  vfs68_curl_init();

  /* Xiph AO */
  vfs68_ao_init();

  /* Memory */
  vfs68_mem_init();

  /* NUll */
  vfs68_null_init();

  /* File descriptor */
  vfs68_fd_init();

  /* File */
  vfs68_file_init();

  /* Resource locator */
  rsc68_init();

  /* Loader */
  file68_loader_init();

  option68_append(opts,sizeof(opts)/sizeof(*opts));
  argc = option68_parse(argc, argv, 1);

  /* Check for --sc68-no-debug */
  opt = option68_get("no-debug", 1);
  if (opt && opt->val.num) {
    /* Remove all debug messages whatsoever. */
    msg68_set_handler(0);
  }

  /* Check for --sc68-debug= */

  /* postpone: at this point most debug features have not been created
     yet. it is pretty much useless to set the mask right now. It will
     be done after all inits.
  */
#if 0
  opt = option68_get("debug", 1);
  if (op) {
    debugmsg68_mask = opt->val.num;
  }
#endif

  /* Get share path from registry */
  opt = option68_get("data", 0);
  if (opt) {

    /* Get data path from registry */
    if (!option68_isset(opt)) {
      char * e;
      /* const char path[] = "Resources"; */
      e = get_reg_path(0, "LMK:SOFTWARE/sashipa/sc68/Install_Dir",
                       tmp, sizeof(tmp));
      if (e /* && (e+sizeof(path) < tmp+sizeof(tmp)) */) {
        /* memcpy(e, path, sizeof(path)); */
        option68_set(opt,tmp);
      }
    }

    /* Setup new data path */
    if (option68_isset(opt)) {
      rsc68_set_share(opt->val.str);
#if 0 /* not needed anynore since option68 properly alloc strings */
      if (opt->val.str == tmp)
        option68_unset(opt);    /* Must release tmp ! */
#endif
    }

  }

  /* Get user path  */
  opt = option68_get("home", 0);
  if (opt) {

    /* Get user path from HOME (usually not defined for win32 platform) */
    if (!option68_isset(opt)) {
      const char path[] = "/.sc68";
      const char * env = mygetenv("HOME");
      if(env && strlen(env)+sizeof(path) < sizeof(tmp)) {
        strcpy(tmp,env);
        strcat(tmp,path);
        option68_set(opt,convert_backslash(tmp));
      }
    }

    /* Get user path from registry */
    if (!option68_isset(opt)) {
      const char path[] = "/sc68";
      char * env;

      env = get_reg_path(0, "CUK:Volatile Environment/APPDATA",
                         tmp, sizeof(tmp));

      if(env && strlen(env)+sizeof(path) < sizeof(tmp)) {
        strcpy(tmp,env);
        strcat(tmp,path);
        option68_set(opt,convert_backslash(tmp));
      } else {
        /* Get user from win32 env */
        const char * drv = mygetenv("HOMEDRIVE");
        const char * env = mygetenv("HOMEPATH");
        if (drv && env && strlen(drv)+strlen(env)+sizeof(path) < sizeof(tmp)) {
          strcpy(tmp,drv);
          strcat(tmp,env);
          strcat(tmp,path);
          option68_set(opt,convert_backslash(tmp));
        }
      }
    }


    /* Setup new user path */
    if (option68_isset(opt)) {
      rsc68_set_user(opt->val.str);
      if (opt->val.str == tmp)
        option68_unset(opt);    /* Must release tmp ! */
    }

  }

  /* Setup new music path */
  opt = option68_get("music", 1);
  if (opt) {
    rsc68_set_music(opt->val.str);
  }

  /* Setup new remote path */
  opt = option68_get("remote", 1);
  if (opt) {
    rsc68_set_remote_music(opt->val.str);
  }

  init = 1;
out_no_init:
  return argc;
}

void file68_shutdown(void)
{
  if (init == 1) {
    init = 2;

    /* Options */
    option68_shutdown();

    /* Loader */
    file68_loader_shutdown();

    /* Shutdown resource */
    rsc68_shutdown();

    /* Zlib */
    vfs68_z_shutdown();

    /* Curl */
    vfs68_curl_shutdown();

    /* Xiph AO */
    vfs68_ao_shutdown();

    /* Memory */
    vfs68_mem_shutdown();

    /* NUll */
    vfs68_null_shutdown();

    /* File descriptor */
    vfs68_fd_shutdown();

    /* File */
    vfs68_file_shutdown();

    init = 0;
  }
}

int file68_version(void)
{
  return PACKAGE_VERNUM;
}

const char * file68_versionstr()
{
  return PACKAGE_STRING;
}
