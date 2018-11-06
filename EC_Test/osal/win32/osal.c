/*
 * Licensed under the GNU General Public License version 2 with exceptions. See
 * LICENSE file in the project root for full license information
 */

#include <winsock2.h>
#include <osal.h>
#include "osal_win32.h"

static int64_t sysfrequency;
static double qpc2usec;

#define USECS_PER_SEC     1000000	//微秒与秒之间的转换

int osal_gettimeofday (struct timeval *tv, struct timezone *tz)
{
   int64_t wintime, usecs;
   if(!sysfrequency)
   {
      timeBeginPeriod(1);//设置时间精度为1ms,实际精度应该大于1ms
      QueryPerformanceFrequency((LARGE_INTEGER *)&sysfrequency);//或得硬件所支持的最高精度计时器频率
      qpc2usec = 1000000.0 / sysfrequency;//1个微秒时间所需要的计数个数
   }
   QueryPerformanceCounter((LARGE_INTEGER *)&wintime);//获得当前系统计数个数
   usecs = (int64_t)((double)wintime * qpc2usec);//算出当前时间微秒数
   tv->tv_sec = (long)(usecs / 1000000);//得到当前秒数
   tv->tv_usec = (long)(usecs - (tv->tv_sec * 1000000));//得到当前微秒数

   return 1;
}

ec_timet osal_current_time (void)
{
   struct timeval current_time; //
   ec_timet return_value;

   osal_gettimeofday (&current_time, 0);//获取当前时间
   return_value.sec = current_time.tv_sec;//当前秒
   return_value.usec = current_time.tv_usec;//当前微秒
   return return_value;
}

//计算时间差
void osal_time_diff(ec_timet *start, ec_timet *end, ec_timet *diff)
{
   if (end->usec < start->usec) {//结束的时间微秒数小于开始的时间微秒数
      diff->sec = end->sec - start->sec - 1;//
      diff->usec = end->usec + 1000000 - start->usec;
   }
   else {//否则如下运算
      diff->sec = end->sec - start->sec;
      diff->usec = end->usec - start->usec;
   }
}

void osal_timer_start (osal_timert *self, uint32 timeout_usec)
{
   struct timeval start_time;
   struct timeval timeout;
   struct timeval stop_time;

   osal_gettimeofday (&start_time, 0);//获取当前时间
   timeout.tv_sec = timeout_usec / USECS_PER_SEC;//超时微秒数
   timeout.tv_usec = timeout_usec % USECS_PER_SEC;//超时秒数
   timeradd (&start_time, &timeout, &stop_time);//计算出超时的终点时间

   self->stop_time.sec = stop_time.tv_sec;
   self->stop_time.usec = stop_time.tv_usec;
}

//时间是否到期
boolean osal_timer_is_expired (osal_timert *self)
{
   struct timeval current_time;
   struct timeval stop_time;
   int is_not_yet_expired;

   osal_gettimeofday (&current_time, 0);//获取当前时间
   stop_time.tv_sec = self->stop_time.sec;//停止时间的秒数
   stop_time.tv_usec = self->stop_time.usec;//停止时间的微秒数
   //条件不成立时 is_not_yet_expired 返回true；
   is_not_yet_expired = timercmp (&current_time, &stop_time, <);

   return is_not_yet_expired == FALSE;
}

int osal_usleep(uint32 usec)
{
   osal_timert qtime;
   osal_timer_start(&qtime, usec);
   if(usec >= 1000)
   {
      SleepEx(usec / 1000, FALSE);
   }
   while(!osal_timer_is_expired(&qtime));
   return 1;
}

void *osal_malloc(size_t size)
{
   return malloc(size);
}

void osal_free(void *ptr)
{
   free(ptr);
}

int osal_thread_create(void **thandle, int stacksize, void *func, void *param)
{
   *thandle = CreateThread(NULL, stacksize, func, param, 0, NULL);
   if(!thandle)
   {
      return 0;
   }
   return 1;
}

int osal_thread_create_rt(void **thandle, int stacksize, void *func, void *param)
{
   int ret;
   ret = osal_thread_create(thandle, stacksize, func, param);
   if (ret)
   {
      ret = SetThreadPriority(*thandle, THREAD_PRIORITY_TIME_CRITICAL);
   }
   return ret;
}
