#ifndef CASH_H
#define CASH_H

#include "include.h"

/*Environment stuff*/
typedef struct{
  char *home,
    *logname,
    *display,
    *path,
    *term;
  char cur_dir[PATH_MAX];
}ENVIRONMENT;

#endif
