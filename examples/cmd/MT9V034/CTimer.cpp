/*
 * CTimer.cpp
 *
 * Created on: 2009-7-13
 *      Author: DEAN
 */
#include "CTimer.h"
#include <iostream>
#include <sys/select.h>
#include <time.h>
#include <pthread.h>
#include  "../../../CqUsbCam/CqUsbCam.h"
#include <string>
using namespace std;


static string sensor;

CTimer::CTimer():
	m_second(0), m_microsecond(0)
{
	
}

CTimer::CTimer(long second, long microsecond, CCqUsbCam* cam) :
	m_second(second), m_microsecond(microsecond), m_pcam(cam)
{

}
CTimer::~CTimer()
{

}

void CTimer::SetTimer(long second, long microsecond)

{

	m_second = second;
	m_microsecond = microsecond;

}

void CTimer::StartTimer()
{

	pthread_create(&thread_timer, NULL, OnTimer_stub, this);

}

void CTimer::StopTimer()

{
	pthread_cancel(thread_timer);
	pthread_join(thread_timer, NULL); //wait the thread stopped

}


void CTimer::thread_proc()
{

	while (true)
	{
		OnTimer();
		pthread_testcancel();

		struct timeval tempval;
		tempval.tv_sec = m_second;
		tempval.tv_usec = m_microsecond;
		select(0, NULL, NULL, NULL, &tempval);
	}
}
void CTimer::setCallBack(void(* cb)())
{
	callfunc=cb;
}
void CTimer::OnTimer()
{
	callfunc();
	//m_pcam->SoftTrig();
	//trigcnt++;
	//cout<<trigcnt<<endl;
	
	// static int cnt = 0;
	// cnt = cnt % 250;
	// if(cnt < 50)
	// {
	// 	m_pcam->SoftTrig();
	// 	 cout<< cnt <<endl;
	// }
	 

	// cnt ++;
}





