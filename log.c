#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <unistd.h>
#include "log.h"
#include "globals.h"

void my_log(int priority, const char *format, ...)
{
    va_list ap;
    va_start(ap, format);

    if (globals.no_syslog) {
        time_t now;
        struct tm* timeinfo;
        char timestring[32];

        time(&now);
        timeinfo = localtime(&now);
        strftime(timestring, sizeof(timestring), "%Y-%m-%d %H:%M:%S", timeinfo);
        fprintf(stderr, "%s %s[%d]: ", timestring, globals.daemon_name, getpid());
        vfprintf(stderr, format, ap);
        fprintf(stderr, "\n");
    }
    else {
        vsyslog(priority, format, ap);
    }

    va_end(ap);
}
