#include <stdio.h>
#include "threadqueue.h"
#include <time.h>
#include <sys/timeb.h>
#include "iostream"
#include "fstream"
#include "sstream"
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/timeb.h>
#include <cstring>
int writeLog(std::string s);
void initlog();
void * LogThread(void * arg);
void stoplog();