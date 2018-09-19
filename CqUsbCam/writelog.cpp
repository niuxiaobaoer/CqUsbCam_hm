#include "writelog.h"
pthread_t logtid;
threadqueue logqueue;
int logrunning=1;
using namespace std;
int writeLog(std::string s)
{
	
	//testvoid(static_cast<void*>(&s));
	//threadmsg logmsg;
	//logmsg.data=(void*)s.c_str();
	//logmsg.msgtype=s.length()+1;
	//current time
	timeb mt;
	tm *vtm;
	long tt;
	time(&tt);
	vtm=localtime(&tt);
	ftime(&mt);

	std::stringstream ss;
	ss<<"T: "<<vtm->tm_hour<<":"<<vtm->tm_min<<":"<<vtm->tm_sec<<"."<<mt.millitm<<"|||";
	
	std::string temp;
	temp=ss.str();
	temp+=s;
	char* data=new char[temp.length()+1];
	sprintf(data,"%s",temp.c_str());
	thread_queue_add(&logqueue,(void*)data,temp.length()+1);

}


void initlog()
{
	
	logrunning=1;
	thread_queue_init(&logqueue);
	pthread_create(&logtid,NULL,LogThread,NULL);
}
void * LogThread(void * arg)
{
	threadmsg logmsg;
	std::ofstream logfile;
	logfile.open("logfile.txt",ios::out);
	
	timespec timeoutqueue;
	timeoutqueue.tv_sec=1;
	logfile<<"start log"<<endl;
	while(logrunning)
	{
		int ret=thread_queue_get(&logqueue,&timeoutqueue,&logmsg);
		if (ret==0)
		{
			
			int len=logmsg.msgtype;
			char* writedata=new char[len];
			memcpy(writedata,(char*) logmsg.data,len);
			std::string temp2((char*) logmsg.data);
			if(logfile.is_open())
			{
				logfile<<temp2;
				logfile.flush();
				cout<<temp2;
			}
			else
			{
			cout<<"file error"<<endl;
			}
			//cout<<logfile.write(temp2.c_str(),temp2.length());
			//cout<<temp2;
			
			//logfile<<temp2;
			delete writedata;
		}
	}
	logfile.close();
}
void stoplog()
{
	logrunning=0;
}