
/**
 * Offline ( when SELinux is disabled ) restorecon
 * Uses both matchpathcon & setfilecon ( xattr ) to set contexts
 * It's usefull to do a restorecon BEFORE reboot to avoid .autorelabel
 * (C) Ivan Agarkov, 2017
 **/

#define _GNU_SOURCE
#include <stdlib.h>
#include <string.h>
#include "queue.h"

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

