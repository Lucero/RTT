#ifndef __LOG_H_
#define __LOG_H_

#include "stdint.h"
#include "stdbool.h"
#include "string.h"
#include "RTT.h"

/* output log's level */
#define	LOG_LEVEL_DISABLE	(0)
#define	LOG_LEVEL_ERROR		(1)
#define	LOG_LEVEL_WARNING	(2)
#define	LOG_LEVEL_INFO		(3)
#define	LOG_LEVEL_DEBUG		(4)
#define	LOG_LEVEL_VERBOSE	(5)

void Log_Init(void);
//
// 设置获取微秒时间戳
//
void Log_SetMicrosecondStamp(uint32_t (*GetMicrosecondStamp)(void));
//
// 设置获取毫秒时间戳
//
void Log_SetMillisecondStamp(uint32_t (*GetMillisecondStamp)(void));
int Log_Output(uint8_t Level, const char* FileName, const char* FuncName, uint32_t LineNum, const char *sFormat, ...);
//
//设置 LOG 输出等级
//
#define LOG_LEVEL			(LOG_LEVEL_VERBOSE)


#if LOG_LEVEL >= LOG_LEVEL_ERROR
    #define LogErr(...)			Log_Output(LOG_LEVEL_ERROR, __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)
#else
    #define LogErr(...)			((void)0);
#endif
#if LOG_LEVEL >= LOG_LEVEL_WARNING
    #define LogWarning(...)		Log_Output(LOG_LEVEL_WARNING, __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)
#else
    #define LogWarning(...)		((void)0);
#endif
#if LOG_LEVEL >= LOG_LEVEL_INFO
    #define LogInfo(...)		Log_Output(LOG_LEVEL_INFO, __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)
#else
    #define LogInfo(...)		((void)0);
#endif
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    #define LogDbg(...)			Log_Output(LOG_LEVEL_DEBUG, __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)
#else
    #define LogDbg(...)			((void)0);
#endif
#if LOG_LEVEL >= LOG_LEVEL_VERBOSE
    #define LogVerbose(...)		Log_Output(LOG_LEVEL_VERBOSE, __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)
#else
    #define LogVerbose(...)		((void)0);
#endif

#endif



