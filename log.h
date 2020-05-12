#ifndef LOG_H
#define LOG_H

/* Make LOG_XXX constants available */
#include <syslog.h>

void my_log(int priority, const char *format, ...);

#endif
