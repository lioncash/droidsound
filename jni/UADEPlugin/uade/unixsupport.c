/* UNIX support tools for uadecore.

   Copyright 2000 - 2005 (C) Heikki Orsila <heikki.orsila@iki.fi>
   
   This module is licensed under the GNU GPL.
*/

#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <libgen.h>
#include <sys/socket.h>
#include <unistd.h>
#include <limits.h>
#include <ctype.h>
#include <android/log.h>

/*#include <uadectl.h>*/
#include <uade/unixatomic.h>


static int url_to_fd(const char *url, int flags, mode_t mode)
{
  int fd;
  if (strncmp(url, "fd://", 5) == 0) {
    char *endptr;
    if (url[5] == 0)
      return -1;
    fd = strtol(&url[5], &endptr, 10);
    if (*endptr != 0)
      return -1;
  } else {
    if (flags & O_WRONLY) {
      fd = open(url, flags, mode);
    } else {
      fd = open(url, flags);
    }
  }
  if (fd < 0)
    fd = -1;
  return fd;
}


/* This must read the full size_t count if it can, and therefore we use
   atomic_read() */
ssize_t uade_ipc_read(void *f, const void *buf, size_t count)
{
  int fd = (intptr_t) f;
  return atomic_read(fd, buf, count);
}


/* This must write the full size_t count if it can, and therefore we use
   atomic_write() */
ssize_t uade_ipc_write(void *f, const void *buf, size_t count)
{
  int fd = (intptr_t) f;
  return atomic_write(fd, buf, count);
}


void *uade_ipc_set_input(const char *input)
{
  int fd;
  if ((fd = url_to_fd(input, O_RDONLY, 0)) < 0) {
    __android_log_print(ANDROID_LOG_VERBOSE, "UADE", "Cannot open input file %s: %s\n", input, strerror(errno));
    exit(1);
  }
  return (void *) ((intptr_t) fd);
}


void *uade_ipc_set_output(const char *output)
{
  int fd;
  if ((fd = url_to_fd(output, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)) < 0) {
    __android_log_print(ANDROID_LOG_VERBOSE, "UADE", "Cannot open output file %s: %s\n", output, strerror(errno));
    exit(1);
  }
  return (void *) ((intptr_t) fd);
}


static int uade_amiga_scandir(char *real, char *dirname, char *fake, int ml)
{
  DIR *dir;
  struct dirent *direntry;
  if (!(dir = opendir(dirname))) {
    __android_log_print(ANDROID_LOG_VERBOSE, "UADE", "UADE: Can't open dir (%s) (amiga scandir)\n", dirname);
    return 0;
  }
  while ((direntry = readdir(dir))) {
    if (!strcmp(fake, direntry->d_name)) {
      if (((int) strlcpy(real, direntry->d_name, ml)) >= ml) {
    __android_log_print(ANDROID_LOG_VERBOSE, "UADE", "UADE: %s does not fit real", direntry->d_name);
    closedir(dir);
    return 0;
      }
      break;
    }
  }
  if (direntry) {
    closedir(dir);
    return 1;
  }
  rewinddir(dir);
  while ((direntry = readdir(dir))) {
    if (!strcasecmp(fake, direntry->d_name)) {
      if (((int) strlcpy(real, direntry->d_name, ml)) >= ml) {
    __android_log_print(ANDROID_LOG_VERBOSE, "UADE", "UADE: %s does not fit real", direntry->d_name);
    closedir(dir);
    return 0;
      }
      break;
    }
  }
  closedir(dir);
  return direntry ? 1 : 0;
}


char *uade_dirname(char *dst, char *src, size_t maxlen)
{
  char *srctemp = strdup(src);
  if (srctemp == NULL)
    return NULL;
  strlcpy(dst, dirname(srctemp), maxlen);
  free(srctemp);
  return dst;
}


/* opens file in amiga namespace */
FILE *uade_open_amiga_file(char *aname, const char *playerdir)
{
  char *separator;
  char *ptr;
  char copy[PATH_MAX];
  char dirname[PATH_MAX];
  char fake[PATH_MAX];
  char real[PATH_MAX];
  int len;
  DIR *dir;
  FILE *file;

  if (strlcpy(copy, aname, sizeof(copy)) >= sizeof(copy)) {
    __android_log_print(ANDROID_LOG_VERBOSE, "UADE", "UADE: error: Amiga tried to open a very long filename\nplease REPORT THIS!\n");
    return NULL;
  }
  ptr = copy;
  if ((separator = strchr(ptr, (int) ':'))) {
    len = (int) (separator - ptr);
    memcpy(dirname, ptr, len);
    dirname[len] = 0;
    if (!strcasecmp(dirname, "ENV")) {
      snprintf(dirname, sizeof(dirname), "%s/ENV/", playerdir);
    } else if (!strcasecmp(dirname, "S")) {
      snprintf(dirname, sizeof(dirname), "%s/S/", playerdir);
    } else {
      __android_log_print(ANDROID_LOG_VERBOSE, "UADE", "UADE: open_amiga_file: unknown amiga volume (%s)\n", aname);
      return NULL;
    }
    if (!(dir = opendir(dirname))) {
      __android_log_print(ANDROID_LOG_VERBOSE, "UADE", "UADE: Can't open dir (%s) (volume parsing)\n", dirname);
      return NULL;
    }
    closedir(dir);
    ptr = separator + 1;
  } else {
    if (*ptr == '/') {
      /* absolute path */
      strlcpy(dirname, "/", sizeof(dirname));
      ptr++;
    } else {
      /* relative path */
      strlcpy(dirname, "./", sizeof(dirname));
    }
  }

  while ((separator = strchr(ptr, (int) '/'))) {
    len = (int) (separator - ptr);
    if (!len) {
      ptr++;
      continue;
    }
    memcpy(fake, ptr, len);
    fake[len] = 0;
    if (uade_amiga_scandir(real, dirname, fake, sizeof(real))) {
      /* found matching entry */
      if (strlcat(dirname, real, sizeof(dirname)) >= sizeof(dirname)) {
    __android_log_print(ANDROID_LOG_VERBOSE, "UADE", "UADE: Too long dir path (%s + %s)\n", dirname, real);
    return NULL;
      }
      if (strlcat(dirname, "/", sizeof(dirname)) >= sizeof(dirname)) {
    __android_log_print(ANDROID_LOG_VERBOSE, "UADE", "UADE: Too long dir path (%s + %s)\n", dirname, "/");
    return NULL;
      }
    } else {
      /* didn't find entry */
      return NULL;
    }
    ptr = separator + 1;
  }

  if (!(dir = opendir(dirname))) {
    __android_log_print(ANDROID_LOG_VERBOSE, "UADE", "Can't open dir (%s) (after dir scanning)\n", dirname);
    return NULL;
  }
  closedir(dir);

  if (uade_amiga_scandir(real, dirname, ptr, sizeof(real))) {
    /* found matching entry */
    if (strlcat(dirname, real, sizeof(dirname)) >= sizeof(dirname)) {
      __android_log_print(ANDROID_LOG_VERBOSE, "UADE", "UADE: Too long dir path (%s + %s)\n", dirname, real);
      return NULL;
    }
  } else {
    /* didn't find entry */
    return NULL;
  }
  if (!(file = fopen(dirname, "r"))) {
    __android_log_print(ANDROID_LOG_VERBOSE, "UADE", "UADE: Couldn't open file (%s) induced by (%s)\n", dirname, aname);
  }
  return file;
}


void uade_portable_initializations(void)
{
  int signals[] = {SIGINT, -1};
  int *signum = signals;
  struct sigaction act;
  memset(&act, 0, sizeof act);
  act.sa_handler = SIG_IGN;

  while (*signum != -1) {
    while (1) {
      if ((sigaction(*signum, &act, NULL)) < 0) {
    if (errno == EINTR)
      continue;
    __android_log_print(ANDROID_LOG_VERBOSE, "UADE", "Cannot ignore signal %d: %s\n", *signum, strerror(errno));
    exit(1);
      }
      break;
    }
    signum++;
  }
}


void uade_arch_spawn(struct uade_ipc *ipc, pid_t *uadepid,
             const char *uadename)
{
  int fds[2];
  char input[32], output[32];

  if (socketpair(AF_UNIX, SOCK_STREAM, 0, fds)) {
    __android_log_print(ANDROID_LOG_VERBOSE, "UADE", "Cannot create socketpair: %s\n", strerror(errno));
    abort();
  }

  *uadepid = fork();
  if (*uadepid < 0) {
    __android_log_print(ANDROID_LOG_VERBOSE, "UADE", "Fork failed: %s\n", strerror(errno));
    abort();
  }

  /* The child (*uadepid == 0) will execute uadecore */
  if (*uadepid == 0) {
    int fd;
    int maxfds;

    if ((maxfds = sysconf(_SC_OPEN_MAX)) < 0) {
      maxfds = 1024;
      __android_log_print(ANDROID_LOG_VERBOSE, "UADE", "Getting max fds failed. Using %d.\n", maxfds);
    }

    /* close everything else but stdin, stdout, stderr, and in/out fds */
    for (fd = 3; fd < maxfds; fd++) {
      if (fd != fds[1])
    atomic_close(fd);
    }

    /* give in/out fds as command line parameters to the uade process */
    snprintf(input, sizeof(input), "fd://%d", fds[1]);
    snprintf(output, sizeof(output), "fd://%d", fds[1]);

    execlp(uadename, uadename, "-i", input, "-o", output, (char *) NULL);
    __android_log_print(ANDROID_LOG_VERBOSE, "UADE", "uade execlp (%s) failed: %s\n", uadename, strerror(errno));
    abort();
  }

  /* Close fds that the uadecore uses */
  if (atomic_close(fds[1]) < 0) {
    __android_log_print(ANDROID_LOG_VERBOSE, "UADE", "Could not close uadecore fds: %s\n", strerror(errno));
    kill (*uadepid, SIGTERM);
    abort();
  }

  do {
    snprintf(output, sizeof output, "fd://%d", fds[0]);
    snprintf(input, sizeof input, "fd://%d", fds[0]);
    uade_set_peer(ipc, 1, input, output);
  } while (0);
}

/*
 * A hack that converts X:\something style windows names into cygwin style name
 * /cygdrive/X/something. All '\\' characters are converted into '/'
 * characters.
 */
char *windows_to_cygwin_path(const char *path)
{
  size_t i;
  char *s;
  size_t len = strlen(path);

  if (len == 0)
    return calloc(1, 1);

  if (len >= 2 && isalpha(path[0]) && path[1] == ':') {
    /* uses windows drive names */
    size_t newlen = len + 32;
    s = malloc(newlen);
    if (s != NULL)
      snprintf(s, newlen, "/cygdrive/%c/%s", path[0], &path[2]);
  } else {
    s = strdup(path);
  }
  if (s == NULL)
    return NULL;

  for (i = 0; s[i] != 0; i++) {
    if (s[i] == '\\')
      s[i] = '/';
  }

  return s;
}
