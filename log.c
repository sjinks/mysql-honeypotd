#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <unistd.h>
#include "log.h"
#include "globals.h"

void my_log(int priority, const char *format, ...)
{
    va_list ap;
    time_t now;
    struct tm timeinfo;
    char timestring[32];
    va_start(ap, format);

#ifndef MINIMALISTIC_BUILD
    if (globals.no_syslog) {
#endif
        time(&now);
        localtime_r(&now, &timeinfo);
        strftime(timestring, sizeof(timestring), "%Y-%m-%d %H:%M:%S", &timeinfo);
        fprintf(
            stderr, "%s %s[%d]: ",
            timestring,
#ifndef MINIMALISTIC_BUILD
            globals.daemon_name,
#else
            "mysql-honeypotd",
#endif
            getpid()
        );
        vfprintf(stderr, format, ap);
        fprintf(stderr, "\n");
#ifndef MINIMALISTIC_BUILD
    }
    else {
        vsyslog(priority, format, ap);
    }
#endif

    va_end(ap);
}
