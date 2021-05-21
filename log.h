#ifndef LOG_H
#define LOG_H

/* Make LOG_XXX constants available */
#include <syslog.h>

#if defined(__clang__) || defined(__GNUC__)
__attribute__((format(printf, 2, 3)))
#endif
void my_log(int priority, const char *format, ...);

#endif
