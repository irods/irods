/* For copyright information please refer to files in the COPYRIGHT directory
 */

#include "datetime.h"
static char* defaultformat = "%b %d %Y %H:%M:%S";
/*
 * returns 0 if failed
 *         1 if succeeded
 */
int strttime(char* timestr, char* timeformat, rodsLong_t* t) {
	if(*timeformat==0) {
		timeformat = defaultformat;
	}
	struct tm tm;
	if (strptime(timestr, timeformat, &tm) == 0) {
		return 0;
	}
/*	printf("year: %d; month: %d; day: %d;\n",
        tm.tm_year, tm.tm_mon, tm.tm_mday);
	printf("hour: %d; minute: %d; second: %d\n",
			tm.tm_hour, tm.tm_min, tm.tm_sec);
	printf("week day: %d; year day: %d\n", tm.tm_wday, tm.tm_yday);*/
	tm.tm_isdst = -1;
	*t = (rodsLong_t) mktime(&tm);
	if (*t == -1) {
		return 0;
	}
	/*printf("seconds since the Epoch: %ld\n", (long) t); */
	return 1;
}

/*
 * returns 0 if failed
 *         1 if succeeded
 */
int ttimestr(char* timestr, int n, char* timeformat, rodsLong_t* t) {
	if(*timeformat==0) {
		timeformat = defaultformat;
	}
	time_t t2 = (time_t) *t;
	struct tm tm = *localtime(&t2);
	if (strftime(timestr, n, timeformat, &tm) == 0) {
		return 0;
	}
	return 1;
}

