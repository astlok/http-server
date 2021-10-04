#include <time.h>
#include "utils.h"

int get_date(char **s) {
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    strftime(*s, sizeof(*s) * 64, "%a, %d %b %G %T GMT", tm);
    return 0;
}
