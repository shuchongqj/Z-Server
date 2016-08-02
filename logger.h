#ifndef LOGGER_H
#define LOGGER_H

//=== ЛОГГЕР ===

// LOGDECL              - вставить в заголовок или исходник (для глобальной видимости).
// LOGOPEN(Filename)    - вставить для инициализации.
// LOGCLOSE             - вставить для завершения.
// LOG(Category,Text)   - отправка сообщения.

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
#ifndef WIN32
#define LOGDECL             std::fstream LOGVarname; char m_chLogBuf[80]; char m_chLogMSBuf[8];                             \
                            time_t LogTimeNow; timeval LogTimeval
#else
#define LOGDECL             std::fstream LOGVarname; char m_chLogBuf[80]; char m_chLogMSBuf[8];                             \
                            time_t LogTimeNow; timeval LogTimeval;                                                          \
                            int gettimeofday(timeval * tp, struct timezone * tzp){                                          \
                                SYSTEMTIME system_time; FILETIME file_time; GetSystemTime(&system_time);                    \
                                SystemTimeToFileTime(&system_time, &file_time);                                             \
                                tp->tv_usec = (long)(system_time.wMilliseconds); tzp = tzp; return 0;}
#endif
#define LOGOPEN(Filename)   LOGVarname.open(Filename, std::ios_base::in|std::ios_base::out|std::ios_base::trunc)
#define LOGCLOSE            LOGVarname.close()
#define LOG(Category,Text)  LogTimeNow = time(0);                                                                           \
                            gettimeofday(&LogTimeval, NULL);                                                                \
                            strftime(m_chLogBuf, sizeof(m_chLogBuf), LOGTimeFormat, localtime(&LogTimeNow));                \
                            sprintf(m_chLogMSBuf, LOGMsFormat, LogTimeval.tv_usec);                                         \
                            LOGVarname << m_chLogBuf << LOGSpc << m_chLogMSBuf << LOGSpc << Category << Text << std::endl;  \
                            LOGVarname.flush(); std::cout << m_chLogBuf << LOGSpc <<                                        \
                            m_chLogMSBuf << LOGSpc << Category << Text << std::endl
//
#define LOG_CAT_I           LOG_NAME": <I> "
#define LOG_CAT_W           LOG_NAME": <W> "
#define LOG_CAT_E           LOG_NAME": <E> "
#define LOG_PATH			"./logs/" LOG_NAME"_Log.txt"
//"
#define RETVAL_OK               0
#define RETVAL_ERR              -1
#define LOG_CTRL_INIT			_lci(LOG_PATH)
#define RETVAL_SET(value)		_uiRetval = value
#define LOG_CTRL_EXIT           LOGCLOSE;							\
								return _uiRetval
#define _lci(path)				unsigned int _uiRetval = RETVAL_OK;	\
								LOGOPEN(path)
#endif // LOGGER_H

