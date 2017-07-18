
/**
 * Offline ( when SELinux is disabled ) restorecon
 * Uses both matchpathcon & setfilecon ( xattr ) to set contexts
 * It's usefull to do a restorecon BEFORE reboot to avoid .autorelabel
 * (C) Ivan Agarkov, 2017
 **/

#ifndef _QUEUE_H
#define _QUEUE_H
struct file_queue {
  char *path;
  struct file_queue *next;
};
typedef struct file_queue FQ;

FQ *create_queue(char *);
FQ *append_queue(FQ *, char *);
void free_queue(FQ *);
#endif
