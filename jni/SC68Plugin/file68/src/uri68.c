/*
 * @file    uri68.c
 * @brief   uri parser and dispatcher
 * @author  http://sourceforge.net/users/benjihan
 *
 * Copyright (C) 2001-2013 Benjamin Gerard
 *
 * Time-stamp: <2013-08-26 11:14:35 ben>
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
#include "file68_uri.h"
#include "file68_rsc.h"
#include "file68_str.h"
#include "file68_msg.h"
#include "file68_vfs_null.h"
#include "file68_vfs_file.h"
#include "file68_vfs_fd.h"
#include "file68_vfs_curl.h"
#include "file68_vfs_ao.h"

#include <string.h>
#include <ctype.h>
#include <assert.h>

#define MYHD "uri68  : "

static scheme68_t * schemes;
static int uri_cat = msg68_DEFAULT;

/**
 * @retval -1  on error
 * @retval >0  length of scheme string
 */
static
int parse_scheme(const char * uri)
{
  int i = 0;

  /* First char must be alpha */
  if ( ! isalpha((int)uri[i]))
    return 0;

  /* Others must be alpha, digit, dot `.', plus `+' or hyphen `-' */
  for (i=1;
       isalnum((int)uri[i]) || uri[i] == '+' || uri[i] == '.' || uri[i] == '-';
       ++i);

  /* Must end by a colon `:' */
  return (uri[i] == ':') ? i+1 : 0;
}

int uri68_get_scheme(char * scheme, int max, const char *uri)
{
  int len = -1;

  if (uri) {
    len = parse_scheme(uri);
    if (scheme) {
      if (len == 0 )
        scheme[0] = 0;
      else if (len > 0) {
        if (len >= max)
          return -1;
        len = max-1;
        memcpy(scheme, uri, len);
        scheme[len] = 0;
      }
    }
  }
  return len;
}

void uri68_unregister(scheme68_t * scheme)
{
  if (scheme) {
    TRACE68(uri_cat, MYHD "unregister scheme -- %s\n", scheme->name);
    if (scheme == schemes)
      schemes = scheme->next;
    else if (schemes) {
      scheme68_t * sch;
      for (sch = schemes; sch->next; sch = sch->next)
        if (sch->next == scheme) {
          sch->next = scheme->next;
          break;
        }
    }
    scheme->next = 0;
  }
}

int uri68_register(scheme68_t * scheme)
{
  if (!scheme)
    return -1;

  assert(!scheme->next);
  scheme->next = schemes;
  schemes = scheme;
  TRACE68(uri_cat, MYHD "registered scheme -- %s\n", scheme->name);

  return 0;
}

vfs68_t * uri68_vfs_va(const char * uri, int mode, int argc, va_list list)
{
  vfs68_t * vfs = 0;
  scheme68_t * scheme;

  for (scheme = schemes; scheme; scheme = scheme->next) {
    int res = scheme->ismine(uri);
    if (!res)
      continue;
    if ( (mode & res & 3) == ( mode & 3 ) )
      break;
  }

  if (scheme)
    vfs = scheme->create(uri, mode, argc, list);

  TRACE68(uri_cat, MYHD
          "create url='%s' %c%c => [%s,'%s']\n",
          strnevernull68(uri),
          (mode&1) ? 'R' : '.',
          (mode&2) ? 'W' : '.',
          strok68(!vfs),
          vfs68_filename(vfs));

  return vfs;
}

vfs68_t * uri68_vfs(const char * uri, int mode, int argc, ...)
{
  vfs68_t * vfs;
  va_list list;

  va_start(list, argc);
  vfs = uri68_vfs_va(uri, mode, argc, list);
  va_end(list);

  return vfs;
}
