/*
 * CTimer.h
 *
 * Created on: 2009-7-13
 *      Author: DEAN
 */

// Call SetTimer() to set the timer_interval. Call StartTimer()
// to enable it and call StopTimer() to stop it.
// The work you want to do should be written on OnTimer
// function.
//////////////////////////////////////////////////////////////////////////

#ifndef CTIMER_H_
#define CTIMER_H_

#include <pthread.h>
#include <sys/time.h>

#include  "../../../CqUsbCam/CqUsbCam.h"

class CTimer
{
	private:

		pthread_t thread_timer;

		long m_second, m_microsecond;

		static void *OnTimer_stub(void *p)
		{
			(static_cast<CTimer*>(p))->thread_proc();
		}

		void thread_proc();
		void OnTimer();

	public:
		CCqUsbCam*  m_pcam;
		CTimer();
		CTimer(long second, long microsecond, CCqUsbCam * cam);
		virtual ~CTimer();
		void SetTimer(long second,long microsecond);
		void StartTimer();
		void StopTimer();
};




#endif /* CTIMER_H_ */




