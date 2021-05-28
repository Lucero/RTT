/*********************************************************************
*                                                                    *
*       RTT version: 0.1                                             *
*                                                                    *
**********************************************************************

---------------------------END-OF-HEADER------------------------------
File    : RTT.h
Purpose : Implementation of real-time transfer which allows
          real-time communication on targets which support debugger 
          memory accesses while the CPU is running.
Revision: $Rev: 0.1 $
----------------------------------------------------------------------
*/

#ifndef RTT_H
#define RTT_H

#include "RTT_Conf.h"

/*********************************************************************
*
*       Defines, defaults
*
**********************************************************************
*/
#ifndef RTT_USE_ASM
  #if (defined __SES_ARM)                       // Embedded Studio
    #define _CC_HAS_RTT_ASM_SUPPORT 1
  #elif (defined __CROSSWORKS_ARM)              // Rowley Crossworks
    #define _CC_HAS_RTT_ASM_SUPPORT 1
  #elif (defined __ARMCC_VERSION)               // ARM compiler
		#if (__ARMCC_VERSION >= 6000000)						// ARM compiler V6.0 and later is clang based
      #define _CC_HAS_RTT_ASM_SUPPORT 1
		#else
      #define _CC_HAS_RTT_ASM_SUPPORT 0
		#endif
  #elif (defined __GNUC__)                      // GCC
    #define _CC_HAS_RTT_ASM_SUPPORT 1
  #elif (defined __clang__)                     // Clang compiler
    #define _CC_HAS_RTT_ASM_SUPPORT 1
  #elif ((defined __IASMARM__) || (defined __ICCARM__))  // IAR assembler/compiler
    #define _CC_HAS_RTT_ASM_SUPPORT 1
  #else
    #define _CC_HAS_RTT_ASM_SUPPORT 0
  #endif
  #if ((defined __IASMARM__) || (defined __ICCARM__))  // IAR assembler/compiler
    //
    // IAR assembler / compiler
    //
    #if (__VER__ < 6300000)
      #define VOLATILE
    #else
      #define VOLATILE volatile
    #endif
    #if (defined __ARM7M__)                            // Needed for old versions that do not know the define yet
      #if (__CORE__ == __ARM7M__)                      // Cortex-M3
        #define _CORE_HAS_RTT_ASM_SUPPORT 1
      #endif
    #endif
    #if (defined __ARM7EM__)                           // Needed for old versions that do not know the define yet
      #if (__CORE__ == __ARM7EM__)                     // Cortex-M4/M7
        #define _CORE_HAS_RTT_ASM_SUPPORT 1
        #define _CORE_NEEDS_DMB 1
        #define RTT__DMB() asm VOLATILE ("DMB");
      #endif
    #endif
    #if (defined __ARM8M_BASELINE__)                   // Needed for old versions that do not know the define yet
      #if (__CORE__ == __ARM8M_BASELINE__)             // Cortex-M23
        #define _CORE_HAS_RTT_ASM_SUPPORT 0
        #define _CORE_NEEDS_DMB 1
        #define RTT__DMB() asm VOLATILE ("DMB");
      #endif
    #endif
    #if (defined __ARM8M_MAINLINE__)                   // Needed for old versions that do not know the define yet
      #if (__CORE__ == __ARM8M_MAINLINE__)             // Cortex-M33
        #define _CORE_HAS_RTT_ASM_SUPPORT 1
        #define _CORE_NEEDS_DMB 1
        #define RTT__DMB() asm VOLATILE ("DMB");
      #endif
    #endif
  #else
    //
    // GCC / Clang
    //
    #if (defined __ARM_ARCH_7M__)                 // Cortex-M3
      #define _CORE_HAS_RTT_ASM_SUPPORT 1
    #elif (defined __ARM_ARCH_7EM__)              // Cortex-M4/M7
      #define _CORE_HAS_RTT_ASM_SUPPORT 1
      #define _CORE_NEEDS_DMB           1
      #define RTT__DMB() __asm volatile ("dmb\n" : : :);
    #elif (defined __ARM_ARCH_8M_BASE__)          // Cortex-M23
      #define _CORE_HAS_RTT_ASM_SUPPORT 0
      #define _CORE_NEEDS_DMB           1
      #define RTT__DMB() __asm volatile ("dmb\n" : : :);
    #elif (defined __ARM_ARCH_8M_MAIN__)          // Cortex-M33
      #define _CORE_HAS_RTT_ASM_SUPPORT 1
      #define _CORE_NEEDS_DMB           1
      #define RTT__DMB() __asm volatile ("dmb\n" : : :);
    #else
      #define _CORE_HAS_RTT_ASM_SUPPORT 0
    #endif
  #endif
  //
  // If IDE and core support the ASM version, enable ASM version by default
  //
  #ifndef _CORE_HAS_RTT_ASM_SUPPORT
    #define _CORE_HAS_RTT_ASM_SUPPORT 0              // Default for unknown cores
  #endif
  #if (_CC_HAS_RTT_ASM_SUPPORT && _CORE_HAS_RTT_ASM_SUPPORT)
    #define RTT_USE_ASM                           (1)
  #else
    #define RTT_USE_ASM                           (0)
  #endif
#endif

//
// We need to know if a DMB is needed to make sure that on Cortex-M7 etc.
// the order of accesses to the ring buffers is guaranteed
// Needed for: Cortex-M7, Cortex-M23, Cortex-M33
//
#ifndef _CORE_NEEDS_DMB
  #define _CORE_NEEDS_DMB 0
#endif

#ifndef RTT__DMB
  #if _CORE_NEEDS_DMB
    #error "Don't know how to place inline assembly for DMB"
  #else
    #define RTT__DMB()
  #endif
#endif

#ifndef RTT_CPU_CACHE_LINE_SIZE
  #define RTT_CPU_CACHE_LINE_SIZE (0)   // On most target systems where RTT is used, we do not have a CPU cache, therefore 0 is a good default here
#endif

#ifndef RTT_UNCACHED_OFF
  #if RTT_CPU_CACHE_LINE_SIZE
    #error "RTT_UNCACHED_OFF must be defined when setting RTT_CPU_CACHE_LINE_SIZE != 0"
  #else
    #define RTT_UNCACHED_OFF (0)
  #endif
#endif
#if RTT_USE_ASM
  #if RTT_CPU_CACHE_LINE_SIZE
    #error "RTT_USE_ASM is not available if RTT_CPU_CACHE_LINE_SIZE != 0"
  #endif
#endif

#ifndef RTT_ASM  // defined when RTT.h is included from assembly file
#include <stdlib.h>
#include <stdarg.h>

/*********************************************************************
*
*       Defines, fixed
*
**********************************************************************
*/

//
// Determine how much we must pad the control block to make it a multiple of a cache line in size
// Assuming: U8 = 1B
//           U16 = 2B
//           U32 = 4B
//           U8/U16/U32* = 4B
//
#if RTT_CPU_CACHE_LINE_SIZE    // Avoid division by zero in case we do not have any cache
  #define RTT__ROUND_UP_2_CACHE_LINE_SIZE(NumBytes) (((NumBytes + RTT_CPU_CACHE_LINE_SIZE - 1) / RTT_CPU_CACHE_LINE_SIZE) * RTT_CPU_CACHE_LINE_SIZE)
#else
  #define RTT__ROUND_UP_2_CACHE_LINE_SIZE(NumBytes) (NumBytes)
#endif
#define RTT__CB_SIZE                              (16 + 4 + 4 + (RTT_MAX_NUM_UP_BUFFERS * 24) + (RTT_MAX_NUM_DOWN_BUFFERS * 24))
#define RTT__CB_PADDING                           (RTT__ROUND_UP_2_CACHE_LINE_SIZE(RTT__CB_SIZE) - RTT__CB_SIZE)

/*********************************************************************
*
*       Types
*
**********************************************************************
*/

//
// Description for a circular buffer (also called "ring buffer")
// which is used as up-buffer (T->H)
//
typedef struct {
  const     char*    sName;         // Optional name. Standard names so far are: "Terminal", "SysView", "J-Scope_t4i4"
            char*    pBuffer;       // Pointer to start of buffer
            unsigned SizeOfBuffer;  // Buffer size in bytes. Note that one byte is lost, as this implementation does not fill up the buffer in order to avoid the problem of being unable to distinguish between full and empty.
            unsigned WrOff;         // Position of next item to be written by either target.
  volatile  unsigned RdOff;         // Position of next item to be read by host. Must be volatile since it may be modified by host.
            unsigned Flags;         // Contains configuration flags
} RTT_BUFFER_UP;

//
// Description for a circular buffer (also called "ring buffer")
// which is used as down-buffer (H->T)
//
typedef struct {
  const     char*    sName;         // Optional name. Standard names so far are: "Terminal", "SysView", "J-Scope_t4i4"
            char*    pBuffer;       // Pointer to start of buffer
            unsigned SizeOfBuffer;  // Buffer size in bytes. Note that one byte is lost, as this implementation does not fill up the buffer in order to avoid the problem of being unable to distinguish between full and empty.
  volatile  unsigned WrOff;         // Position of next item to be written by host. Must be volatile since it may be modified by host.
            unsigned RdOff;         // Position of next item to be read by target (down-buffer).
            unsigned Flags;         // Contains configuration flags
} RTT_BUFFER_DOWN;

//
// RTT control block which describes the number of buffers available
// as well as the configuration for each buffer
//
//
typedef struct {
  char                    acID[16];                                 // Initialized to "XXXXXX RTT"
  int                     MaxNumUpBuffers;                          // Initialized to RTT_MAX_NUM_UP_BUFFERS (type. 2)
  int                     MaxNumDownBuffers;                        // Initialized to RTT_MAX_NUM_DOWN_BUFFERS (type. 2)
  RTT_BUFFER_UP           aUp[RTT_MAX_NUM_UP_BUFFERS];              // Up buffers, transferring information up from target via debug probe to host
  RTT_BUFFER_DOWN         aDown[RTT_MAX_NUM_DOWN_BUFFERS];          // Down buffers, transferring information down from host via debug probe to target
#if RTT__CB_PADDING
  unsigned char           aDummy[RTT__CB_PADDING];
#endif
} RTT_CB;

/*********************************************************************
*
*       Global data
*
**********************************************************************
*/
extern RTT_CB _RTT;

/*********************************************************************
*
*       RTT API functions
*
**********************************************************************
*/
#ifdef __cplusplus
  extern "C" {
#endif
int          RTT_AllocDownBuffer         (const char* sName, void* pBuffer, unsigned BufferSize, unsigned Flags);
int          RTT_AllocUpBuffer           (const char* sName, void* pBuffer, unsigned BufferSize, unsigned Flags);
int          RTT_ConfigUpBuffer          (unsigned Channel,  const char* sName, void* pBuffer, unsigned BufferSize, unsigned Flags);
int          RTT_ConfigDownBuffer        (unsigned Channel,  const char* sName, void* pBuffer, unsigned BufferSize, unsigned Flags);
int          RTT_GetKey                  (void);
unsigned     RTT_HasData                 (unsigned Channel);
int          RTT_HasKey                  (void);
unsigned     RTT_HasDataUp               (unsigned Channel);
void         RTT_Init                    (void);
unsigned     RTT_Read                    (unsigned Channel, void* pBuffer, unsigned BufferSize);
unsigned     RTT_ReadNoLock              (unsigned Channel, void* pData,   unsigned BufferSize);
int          RTT_SetNameDownBuffer       (unsigned Channel, const char* sName);
int          RTT_SetNameUpBuffer         (unsigned Channel, const char* sName);
int          RTT_SetFlagsDownBuffer      (unsigned Channel, unsigned Flags);
int          RTT_SetFlagsUpBuffer        (unsigned Channel, unsigned Flags);
int          RTT_WaitKey                 (void);
unsigned     RTT_Write                   (unsigned Channel, const void* pBuffer, unsigned NumBytes);
unsigned     RTT_WriteNoLock             (unsigned Channel, const void* pBuffer, unsigned NumBytes);
unsigned     RTT_WriteSkipNoLock         (unsigned Channel, const void* pBuffer, unsigned NumBytes);
unsigned     RTT_ASM_WriteSkipNoLock     (unsigned Channel, const void* pBuffer, unsigned NumBytes);
unsigned     RTT_WriteString             (unsigned Channel, const char* s);
void         RTT_WriteWithOverwriteNoLock(unsigned Channel, const void* pBuffer, unsigned NumBytes);
unsigned     RTT_PutChar                 (unsigned Channel, char c);
unsigned     RTT_PutCharSkip             (unsigned Channel, char c);
unsigned     RTT_PutCharSkipNoLock       (unsigned Channel, char c);
unsigned     RTT_GetAvailWriteSpace      (unsigned Channel);
unsigned     RTT_GetBytesInBuffer        (unsigned Channel);
//
// Function macro for performance optimization
//
#define      RTT_HASDATA(n)       (((RTT_BUFFER_DOWN*)((char*)&_RTT.aDown[n] + RTT_UNCACHED_OFF))->WrOff - ((RTT_BUFFER_DOWN*)((char*)&_RTT.aDown[n] + RTT_UNCACHED_OFF))->RdOff)

#if RTT_USE_ASM
  #define RTT_WriteSkipNoLock  RTT_ASM_WriteSkipNoLock
#endif

/*********************************************************************
*
*       RTT transfer functions to send RTT data via other channels.
*
**********************************************************************
*/
unsigned     RTT_ReadUpBuffer            (unsigned Channel, void* pBuffer, unsigned BufferSize);
unsigned     RTT_ReadUpBufferNoLock      (unsigned Channel, void* pData, unsigned BufferSize);
unsigned     RTT_WriteDownBuffer         (unsigned Channel, const void* pBuffer, unsigned NumBytes);
unsigned     RTT_WriteDownBufferNoLock   (unsigned Channel, const void* pBuffer, unsigned NumBytes);

#define      RTT_HASDATA_UP(n)    (((RTT_BUFFER_UP*)((char*)&_RTT.aUp[n] + RTT_UNCACHED_OFF))->WrOff - ((RTT_BUFFER_UP*)((char*)&_RTT.aUp[n] + RTT_UNCACHED_OFF))->RdOff)   // Access uncached to make sure we see changes made by the J-Link side and all of our changes go into HW directly

/*********************************************************************
*
*       RTT "Terminal" API functions
*
**********************************************************************
*/
int     RTT_SetTerminal        (unsigned char TerminalId);
int     RTT_TerminalOut        (unsigned char TerminalId, const char* s);

/*********************************************************************
*
*       RTT printf functions (require RTT_printf.c)
*
**********************************************************************
*/
int RTT_printf(unsigned Channel, const char * sFormat, ...);
int RTT_vprintf(unsigned Channel, const char * sFormat, va_list * pParamList);

#ifdef __cplusplus
  }
#endif

#endif // ifndef(RTT_ASM)

/*********************************************************************
*
*       Defines
*
**********************************************************************
*/

//
// Operating modes. Define behavior if buffer is full (not enough space for entire message)
//
#define RTT_MODE_NO_BLOCK_SKIP         (0)     // Skip. Do not block, output nothing. (Default)
#define RTT_MODE_NO_BLOCK_TRIM         (1)     // Trim: Do not block, output as much as fits.
#define RTT_MODE_BLOCK_IF_FIFO_FULL    (2)     // Block: Wait until there is space in the buffer.
#define RTT_MODE_MASK                  (3)

//
// Control sequences, based on ANSI.
// Can be used to control color, and clear the screen
//
#define RTT_CTRL_RESET                "\x1B[0m"         // Reset to default colors
#define RTT_CTRL_CLEAR                "\x1B[2J"         // Clear screen, reposition cursor to top left

#define RTT_CTRL_TEXT_BLACK           "\x1B[2;30m"
#define RTT_CTRL_TEXT_RED             "\x1B[2;31m"
#define RTT_CTRL_TEXT_GREEN           "\x1B[2;32m"
#define RTT_CTRL_TEXT_YELLOW          "\x1B[2;33m"
#define RTT_CTRL_TEXT_BLUE            "\x1B[2;34m"
#define RTT_CTRL_TEXT_MAGENTA         "\x1B[2;35m"
#define RTT_CTRL_TEXT_CYAN            "\x1B[2;36m"
#define RTT_CTRL_TEXT_WHITE           "\x1B[2;37m"

#define RTT_CTRL_TEXT_BRIGHT_BLACK    "\x1B[1;30m"
#define RTT_CTRL_TEXT_BRIGHT_RED      "\x1B[1;31m"
#define RTT_CTRL_TEXT_BRIGHT_GREEN    "\x1B[1;32m"
#define RTT_CTRL_TEXT_BRIGHT_YELLOW   "\x1B[1;33m"
#define RTT_CTRL_TEXT_BRIGHT_BLUE     "\x1B[1;34m"
#define RTT_CTRL_TEXT_BRIGHT_MAGENTA  "\x1B[1;35m"
#define RTT_CTRL_TEXT_BRIGHT_CYAN     "\x1B[1;36m"
#define RTT_CTRL_TEXT_BRIGHT_WHITE    "\x1B[1;37m"

#define RTT_CTRL_BG_BLACK             "\x1B[24;40m"
#define RTT_CTRL_BG_RED               "\x1B[24;41m"
#define RTT_CTRL_BG_GREEN             "\x1B[24;42m"
#define RTT_CTRL_BG_YELLOW            "\x1B[24;43m"
#define RTT_CTRL_BG_BLUE              "\x1B[24;44m"
#define RTT_CTRL_BG_MAGENTA           "\x1B[24;45m"
#define RTT_CTRL_BG_CYAN              "\x1B[24;46m"
#define RTT_CTRL_BG_WHITE             "\x1B[24;47m"

#define RTT_CTRL_BG_BRIGHT_BLACK      "\x1B[4;40m"
#define RTT_CTRL_BG_BRIGHT_RED        "\x1B[4;41m"
#define RTT_CTRL_BG_BRIGHT_GREEN      "\x1B[4;42m"
#define RTT_CTRL_BG_BRIGHT_YELLOW     "\x1B[4;43m"
#define RTT_CTRL_BG_BRIGHT_BLUE       "\x1B[4;44m"
#define RTT_CTRL_BG_BRIGHT_MAGENTA    "\x1B[4;45m"
#define RTT_CTRL_BG_BRIGHT_CYAN       "\x1B[4;46m"
#define RTT_CTRL_BG_BRIGHT_WHITE      "\x1B[4;47m"


#endif

/*************************** End of file ****************************/
