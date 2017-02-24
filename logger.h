#ifndef LOGGER_H
#define LOGGER_H

//=== ЛОГГЕР ===

// #define LOGNAME	"Name"			- добавить в макросы для определения имени лога.
// LOGDECL							- вставить в переменные заголовока (или исходника, если не используется внутри класса).
// LOGDECL_PTHRD_INCLASS_ADD		- вставить в переменные заголовока класса для поддержки потокобезопасности с pthread.
// LOGDECL_INIT_INCLASS(ClassName)	- вставить в исходник класса.
// LOGDECL_INIT_PTHRD_ADD			- вставить в функцию для поддержки потокобезопасности с pthread без использования класса.
// LOGDECL_INIT_PTHRD_INCLASS_EXT_ADD(ClassName) - вставить в исходник класса для работы с внешним мьютексом pthread.
// LOGDECL_INIT_PTHRD_INCLASS_OWN_ADD(ClassName) - вставить в исходник класса для работы с собственным мьютексом pthread.
// LOG_CTRL_BIND_EXT_MUTEX(Mutex)	- вставить в функцию класса для привязки внешнего мьютекса pthread.
// LOG_CTRL_INIT					- вставить в функцию класса или основного исходника.
// LOG(Category,Text)				- отправка сообщения.
// LOG_P(Category,Text)				- потокобезопасная отправка сообщения.
// RETVAL_SET(value)				- установка возвращаемого значения для выхода с макросом.
// LOGCLOSE							- вставить для завершения работы с логом.

//== ВКЛЮЧЕНИЯ.
#include <fstream>
#include <iostream>
#ifndef WIN32
#include <sys/time.h>
#else
#include <Windows.h>
#include <stdint.h>
#include <time.h>
#endif

//== МАКРОСЫ.
#define LOGVarname          o_LogFileStream
#define LOGTimeFormat       "%Y-%m-%d %X"
#ifdef WIN32
#define LOGMsFormat         "%03ld"
#else
#define LOGMsFormat         "%06ld"
#endif
#define LOGSpc              " "
#define LOGRETVAL			_iRetval
#ifndef WIN32
#define LOGDECL             static std::fstream LOGVarname; static char _m_chLogBuf[80]; static char _m_chLogMSBuf[8];      \
							static time_t _LogTimeNow; static timeval _LogTimeval; static int _iRetval;
#else
#define LOGDECL             static std::fstream LOGVarname; static char _m_chLogBuf[80]; static char _m_chLogMSBuf[8];      \
                            static time_t _LogTimeNow; static timeval _LogTimeval; static int _iRetval;                     \
							static int gettimeofday(timeval * tp, struct timezone * tzp){                                   \
                                SYSTEMTIME system_time; FILETIME file_time; GetSystemTime(&system_time);                    \
                                SystemTimeToFileTime(&system_time, &file_time);                                             \
                                tp->tv_usec = (long)(system_time.wMilliseconds); tzp = tzp; return 0;}
#endif
#define LOGDECL_PTHRD_INCLASS_ADD		static pthread_mutex_t _ptLogMutex;
#define LOGDECL_INIT_INCLASS(ClassName)	std::fstream ClassName::LOGVarname; char ClassName::_m_chLogBuf[80];				\
										char ClassName::_m_chLogMSBuf[8]; time_t ClassName::_LogTimeNow;					\
										timeval ClassName::_LogTimeval; int ClassName::_iRetval;
#define LOGDECL_INIT_PTHRD_INCLASS_EXT_ADD(ClassName)	pthread_mutex_t ClassName::_ptLogMutex;
#define LOGDECL_INIT_PTHRD_INCLASS_OWN_ADD(ClassName)	pthread_mutex_t ClassName::_ptLogMutex = PTHREAD_MUTEX_INITIALIZER;
#define LOGDECL_INIT_PTHRD_ADD							pthread_mutex_t _ptLogMutex = PTHREAD_MUTEX_INITIALIZER;
#define LOGOPEN(Filename)   LOGVarname.open(Filename, std::ios_base::in|std::ios_base::out|std::ios_base::trunc)
#define LOGCLOSE            LOGVarname.close()
#define LOG(Category,Text)  _LogTimeNow = time(0);                                                                          \
							gettimeofday(&_LogTimeval, NULL);                                                               \
							strftime(_m_chLogBuf, sizeof(_m_chLogBuf), LOGTimeFormat, localtime(&_LogTimeNow));             \
							sprintf(_m_chLogMSBuf, LOGMsFormat, _LogTimeval.tv_usec);                                       \
							LOGVarname << _m_chLogBuf << LOGSpc << _m_chLogMSBuf << LOGSpc << Category << Text << std::endl;\
							LOGVarname.flush(); std::cout << _m_chLogBuf << LOGSpc <<                                       \
							_m_chLogMSBuf << LOGSpc << Category << Text << std::endl
#define LOG_P(Category,Text)	pthread_mutex_lock(&_ptLogMutex);															\
								LOG(Category,Text);																			\
								pthread_mutex_unlock(&_ptLogMutex);
//
#define LOG_CAT_I           LOG_NAME": <I> "
#define LOG_CAT_W           LOG_NAME": <W> "
#define LOG_CAT_E           LOG_NAME": <E> "
#define LOG_PATH			"./logs/" LOG_NAME"_Log.txt"
//"
#define RETVAL_OK           0
#define RETVAL_ERR          -1
#define LOG_CTRL_INIT		_lci(LOG_PATH)
#define LOG_CTRL_BIND_EXT_MUTEX(Mutex)	_ptLogMutex = Mutex
#define RETVAL_SET(value)	_iRetval = value
#define _lci(path)			_iRetval = RETVAL_OK;																			\
							LOGOPEN(path)
#endif // LOGGER_H

