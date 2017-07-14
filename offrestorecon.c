/**
 * Offline ( when SELinux is disabled ) restorecon
 * Uses both matchpathcon & setfilecon ( xattr ) to set contexts
 * It's usefull to do a restorecon BEFORE reboot to avoid .autorelabel
 * (C) Ivan Agarkov, 2017
 **/

#define _GNU_SOURCE
#include <stdio.h>
#include <selinux/selinux.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <malloc.h>
#include <errno.h>
#define MAX_CONTEXT_SIZE 255
#define DEFAULT_LABEL "system_u:object_r:unlabeled_t"

int get_mode(char *path) {
  struct stat fstat;
  if(stat(path, &fstat) == 0) {
    return fstat.st_mode;
  }
  return -1;
}

int restorecon(char *path, int verbose) {
  int mode;
  char **conptr = malloc(sizeof(char *));
  char context[MAX_CONTEXT_SIZE];
  if((mode = get_mode(path)) > 0) {
    if(matchpathcon(path, mode, conptr) != 0) {
      strncpy(context, DEFAULT_LABEL, sizeof(context));
      fprintf(stderr, "Warning: no default context for %s\n", path);
    } else {
      strncpy(context, *conptr, sizeof(context));
      freecon(*conptr);
    }
    if(setfilecon(path, context) == 0) {
      if(verbose) {
        printf("%s: %s\n", path, context);
      }
    } else {
      mode = -1;
    }
  }
  if(mode < 0) {
    fprintf(stderr, "Error: %s for %s\n", strerror(errno), path);
  }
  free(conptr);
  return mode;
}

int recursecon(char *path, int verbose) {
  // 1. restorecon
  int mode;
  if((mode = restorecon(path, verbose)) > 0) {
    if(S_ISDIR(mode)) {
      if(S_ISLNK(mode)) { // skip if it is a link
        return 0;
      }
      struct dirent *dir;
      DIR *dp;
      if(!(dp = opendir(path))) {
        return -1;
      }
      while((dir = readdir(dp))) {
        if(strcmp(dir->d_name, ".") == 0 || strcmp(dir->d_name, "..") == 0) {
          continue;
        }
        char *path_next = (char *)malloc(strlen(path)+strlen(dir->d_name)+2);
        int fl = (path[strlen(path)] == '/');
        sprintf(path_next, (fl)?"%s%s":"%s/%s", path, dir->d_name);
        recursecon(path_next, verbose);
        free(path_next);
      }
      closedir(dp);
    }
    return 0;
  }
  return -1; // nothing to do if we failed here
}

inline int restore(char *path, int recurse, int verbose) {
  return (recurse)?recursecon(path, verbose):restorecon(path, verbose);
}

int print_help() {
  printf("Usage: %s [-Rvh] <path1>...[pathN]\n", program_invocation_name);
  printf("This program restores SELinux file context ( xattr wrapped fields ) without enabling SELinux on the host\n");
  printf("  -R - recursive restore SELinux labels\n");
  printf("  -v - be verbose\n");
  printf(" -h/-? - see this help\n");
  return 0;
}

int main(int argc, char *argv[]) {
  int o;
  int verbose = 0;
  int recurse = 0;

  while((o = getopt(argc, argv, "Rvh")) > 0) {
    if(o == 'h') {
      return print_help();
    } else if(o == 'R') {
      recurse = 1;
    } else if(o == 'v') {
      verbose = 1;
    } else {
      return print_help();
    }
  }
  if(optind >= argc) {
    return print_help();
  }
  do {
    restore(argv[optind++], recurse, verbose);
  } while(optind < argc);
  return 0;
}

