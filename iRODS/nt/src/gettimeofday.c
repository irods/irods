/*
 * Defines gettimeofday
 *
 * Based on timeval.h by Wu Yongwei
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <windows.h>
#include "Unix2Nt.h"



#if defined(_MSC_VER) || defined(_MSC_EXTENSIONS)
  #define EPOCHFILETIME 11644473600000000Ui64
#else
  #define EPOCHFILETIME 11644473600000000ULL
#endif

int gettimeofday(struct timeval *tv, struct timezone *tz)
{
    FILETIME        ft;
    LARGE_INTEGER   li;
    __int64         t;
    static int      tzflag;

    if (tv)
    {
        GetSystemTimeAsFileTime(&ft);
        li.LowPart  = ft.dwLowDateTime;
        li.HighPart = ft.dwHighDateTime;
        t  = li.QuadPart;       /* In 100-nanosecond intervals */
        t -= EPOCHFILETIME;     /* Offset to the Epoch time */
        t /= 10;                /* In microseconds */
        tv->tv_sec  = (long)(t / 1000000);
        tv->tv_usec = (long)(t % 1000000);
    }

    if (tz)
    {
		int t_seconds;
		int t_hours;
        if (!tzflag)
        {
            _tzset();
            tzflag++;
        }
		_get_timezone(&t_seconds);
        tz->tz_minuteswest =  t_seconds/ 60;
		_get_daylight(&t_hours);
        tz->tz_dsttime = t_hours;
    }

    return 0;
}