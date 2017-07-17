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
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <dirent.h>
#include <malloc.h>
#include <errno.h>
#define MAX_CONTEXT_SIZE 255
#define DEFAULT_LABEL "system_u:object_r:unlabeled_t"

struct file_queue {
  char *path;
  struct file_queue *next;
};
typedef struct file_queue FQ;


FQ *create_queue(char *path) {
  FQ *ret = malloc(sizeof(FQ));
  ret->path = malloc(strlen(path)+1);
  ret->next = 0;
  strncpy(ret->path, path, strlen(path));
  *(ret->path+strlen(path)) = 0;
  return ret;
}

FQ *append_queue(FQ *q, char *path) {
  FQ *ret = create_queue(path);
  q->next = ret;
  return ret;
}

void free_queue(FQ *root) {
  if(!root) {
    return;
  }
  FQ *q = root;
  do {
    FQ *n = q->next;
    if(q->path) {
      free(q->path);
    }
    free(q);
    if(n) {
      q = n;
    } else {
      q = 0;
    }
  } while(q);
}

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
  int mode;
  if((mode = restorecon(path, verbose)) > 0) {
    if(S_ISLNK(mode) || !S_ISDIR(mode)) { // skip if it is a link
      return 0;
    }
    struct dirent *dir;
    DIR *dp;
    if(!(dp = opendir(path))) {
      return -1;
    }
    FQ *root = 0, *q = 0;
    while((dir = readdir(dp))) {
      if(strcmp(dir->d_name, ".") == 0 || strcmp(dir->d_name, "..") == 0) {
        continue;
      }
      char *path_next = (char *)malloc(strlen(path)+strlen(dir->d_name)+2);
      int fl = (path[strlen(path)] == '/');
      sprintf(path_next, (fl)?"%s%s":"%s/%s", path, dir->d_name);
      if(!root) {
        root = create_queue(path_next);
        q = root;
      } else {
        q = append_queue(q, path_next);
      }
      free(path_next);
    }
    closedir(dp);
    q = root;
    while(q) {
      recursecon(q->path, verbose);
      q = q->next;
    }
    free_queue(root);
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
  printf("  -i - set ionice to idle/nice to 20 to prevent cpu load\n");
  printf(" -h/-? - see this help\n");
  return 0;
}

enum {
  IOPRIO_CLASS_NONE,
  IOPRIO_CLASS_RT,
  IOPRIO_CLASS_BE,
  IOPRIO_CLASS_IDLE,
};

enum {
  IOPRIO_WHO_PROCESS = 1,
  IOPRIO_WHO_PGRP,
  IOPRIO_WHO_USER,
};

#define IOPRIO_CLASS_SHIFT  13

void set_idle() {
  if(syscall(SYS_ioprio_set, IOPRIO_WHO_PROCESS, 0, IOPRIO_CLASS_IDLE|IOPRIO_CLASS_BE<<IOPRIO_CLASS_SHIFT) < 0) {
    fprintf(stderr, "ioprio_set(): %s\n", strerror(errno));
    exit(-1);
  }
  // nice
  if(setpriority(PRIO_PROCESS, 0, 20) < 0) {
    fprintf(stderr, "setpriority(): %s\n", strerror(errno));
  }

}

int main(int argc, char *argv[]) {
  int o;
  int verbose = 0;
  int recurse = 0;
  int idle = 0;

  while((o = getopt(argc, argv, "Rvhi")) > 0) {
    if(o == 'h') {
      return print_help();
    } else if(o == 'R') {
      recurse = 1;
    } else if(o == 'v') {
      verbose = 1;
    } else if(o == 'i') {
      idle = 1;
    } else {
      return print_help();
    }
  }
  if ( idle ) {
    set_idle();
    if (verbose) {
      printf("Setting iopriority to IDLE priority\n");
      printf("Setting priority to 20 priority\n");
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

