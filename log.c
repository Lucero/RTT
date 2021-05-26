#include "log.h"

uint32_t (*Log_GetMicrosecondStamp)(void);
uint32_t (*Log_GetMillisecondStamp)(void);

void Log_Init(void)
{
	RTT_Init();
	Log_GetMicrosecondStamp = NULL;
	Log_GetMillisecondStamp = NULL;
}
//
// 设置获取微秒时间戳
//
void Log_SetMicrosecondStamp(uint32_t (*GetMicrosecondStamp)(void))
{
	Log_GetMicrosecondStamp = GetMicrosecondStamp;
}
//
// 设置获取毫秒时间戳
//
void Log_SetMillisecondStamp(uint32_t (*GetMillisecondStamp)(void))
{
	Log_GetMillisecondStamp = GetMillisecondStamp;	
}

int Log_Output(uint8_t Level, const char* FileName, const char* FuncName, uint32_t LineNum, const char *sFormat, ...)
{
	int r;
	va_list ParamList;
	const char* strLevel = "";
	const char* strColor = "";
	const char* strCtrReset = RTT_CTRL_RESET;
	
	switch(Level)
	{
		case LOG_LEVEL_ERROR:
			strLevel = "ERROR";
			strColor = RTT_CTRL_TEXT_BRIGHT_RED;
			break;
		
		case LOG_LEVEL_WARNING:
			strLevel = "WARNING";
			strColor = RTT_CTRL_TEXT_BRIGHT_YELLOW;
			break;
		
		case LOG_LEVEL_INFO:
			strLevel = "INFO";
			strColor = RTT_CTRL_TEXT_BRIGHT_BLACK;
			break;
		
		case LOG_LEVEL_DEBUG:
			strLevel = "DEBUG";
			strColor = RTT_CTRL_TEXT_BRIGHT_BLUE;
			break;
		
		case LOG_LEVEL_VERBOSE:
			strLevel = "VERBOSE";
			strColor = RTT_CTRL_TEXT_BRIGHT_BLACK;
			break;
		
		default:
			break;
	}

	if (Log_GetMicrosecondStamp != NULL)
	{
		RTT_printf(0, "%s[%u.%03u.%03u] [%s] [%s] [%s] [%03u] ", 
			strColor,
			Log_GetMicrosecondStamp()/1000/1000, 
			Log_GetMicrosecondStamp()/1000%1000, 
			Log_GetMicrosecondStamp()%1000,
			strLevel,
			FileName, FuncName, LineNum);
	}
	else if (Log_GetMillisecondStamp != NULL)
	{
		RTT_printf(0, "%s[%u.%03u] [%s] [%s] [%s] [%03u] ", 
			strColor,
			Log_GetMillisecondStamp()/1000, 
			Log_GetMillisecondStamp()%1000,
			strLevel,
			FileName, FuncName, LineNum);
	}
	else
	{
		RTT_printf(0, "%s[%s] [%s] [%s] [%03u] ",
			strColor,
			strLevel,
			FileName, FuncName, LineNum);
	}
	
	va_start(ParamList, sFormat);
	r = RTT_vprintf(0, sFormat, &ParamList);
	va_end(ParamList);
	
	RTT_printf(0, "%s\n", strCtrReset);
	
	return r;
}
