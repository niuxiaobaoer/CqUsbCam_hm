#ifndef IMGFRAME_H
#define IMGFRAME_H

#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <list>


#include "Types.h"

#define _SYSTIME
#ifdef _SYSTIME
#include <time.h>
#include <sys/timeb.h>
#include "sstream"
#endif
class CImgFrame
{
public:
    cq_int32_t m_width;
    cq_int32_t m_height;
    cq_int32_t isSoftTrig;
    cq_int32_t m_camNum;

    cq_int64_t m_timeStamp;
    cq_int64_t m_framecnt;
    cq_uint8_t* m_imgBuf;
#ifdef _SYSTIME
    std::string systime;
#endif
    CImgFrame(const cq_int32_t width, const cq_int32_t height, const cq_int32_t camNum):m_width(width),m_height(height),m_camNum(camNum)
    {
        m_imgBuf=new cq_uint8_t[height*width];
#ifdef _SYSTIME
        timeb mt;
    	tm *vtm;
    	long tt;
	    time(&tt);
	    vtm=localtime(&tt);
	    ftime(&mt);
	    std::stringstream ss;
	    ss<<"T: "<<vtm->tm_hour<<":"<<vtm->tm_min<<":"<<vtm->tm_sec<<"."<<mt.millitm<<"|||";
        systime=ss.str();
#endif
    }
    ~CImgFrame(void)
    {
        if (m_imgBuf!=NULL)
            delete m_imgBuf;
    }
    /*
	void clone(CImgFrame * frame)
    {
         
        m_width=frame->m_width;
        m_height=frame->m_height;
        m_imgBuf=new cq_uint8_t[frame->m_height*frame->m_width];
        memcpy(m_imgBuf, frame->m_imgBuf, m_width*m_height);
    }
	*/
};
#endif // IMGFRAME_H
