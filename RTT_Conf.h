﻿/*********************************************************************
*                                                                    *
*       RTT version: 0.1                                             *
*                                                                    *
**********************************************************************

---------------------------END-OF-HEADER------------------------------
File    : RTT_Conf.h
Purpose : Implementation of real-time transfer (RTT) which
          allows real-time communication on targets which support
          debugger memory accesses while the CPU is running.
Revision: $Rev: 0.1 $

*/

#ifndef RTT_CONF_H
#define RTT_CONF_H

#ifdef __IAR_SYSTEMS_ICC__
  #include <intrinsics.h>
#endif

/*********************************************************************
*
*       Defines, configurable
*
**********************************************************************
*/

//
// Take in and set to correct values for Cortex-A systems with CPU cache
//
//#define RTT_CPU_CACHE_LINE_SIZE            (32)           // Largest cache line size (in bytes) in the current system
//#define RTT_UNCACHED_OFF                   (0xFB000000)   // Address alias where RTT CB and buffers can be accessed uncached
//
// Most common case:
// Up-channel 0: RTT
// Up-channel 1: SystemView
//
#ifndef   RTT_MAX_NUM_UP_BUFFERS
  #define RTT_MAX_NUM_UP_BUFFERS             (/*#RTT_MAX_NUM_UP_BUFFERS#*/)            // Max. number of up-buffers (T->H) available on this target    (Default: 3)
#endif
//
// Most common case:
// Down-channel 0: RTT
// Down-channel 1: SystemView
//
#ifndef   RTT_MAX_NUM_DOWN_BUFFERS
  #define RTT_MAX_NUM_DOWN_BUFFERS           (/*#RTT_MAX_NUM_UP_BUFFERS#*/)            // Max. number of down-buffers (H->T) available on this target  (Default: 3)
#endif

#ifndef   RTT_PRINTF_BUFFER_SIZE
  #define RTT_PRINTF_BUFFER_SIZE             (64u)          // Size of buffer for RTT printf to bulk-send chars via RTT     (Default: 64)
#endif

#ifndef   RTT_MODE_DEFAULT
  #define RTT_MODE_DEFAULT                   RTT_MODE_NO_BLOCK_SKIP // Mode for pre-initialized terminal channel (buffer 0)
#endif

/*********************************************************************
*
*       RTT memcpy configuration
*
*       memcpy() is good for large amounts of data,
*       but the overhead is big for small amounts, which are usually stored via RTT.
*       With RTT_MEMCPY_USE_BYTELOOP a simple byte loop can be used instead.
*
*       RTT_MEMCPY() can be used to replace standard memcpy() in RTT functions.
*       This is may be required with memory access restrictions,
*       such as on Cortex-A devices with MMU.
*/
#ifndef   RTT_MEMCPY_USE_BYTELOOP
  #define RTT_MEMCPY_USE_BYTELOOP            0              // 0: Use memcpy/RTT_MEMCPY, 1: Use a simple byte-loop
#endif
//
// Example definition of RTT_MEMCPY to external memcpy with GCC toolchains and Cortex-A targets
//
//#if ((defined __SES_ARM) || (defined __CROSSWORKS_ARM) || (defined __GNUC__)) && (defined (__ARM_ARCH_7A__))
//  #define RTT_MEMCPY(pDest, pSrc, NumBytes)      memcpy((pDest), (pSrc), (NumBytes))
//#endif

//
// Target is not allowed to perform other RTT operations while string still has not been stored completely.
// Otherwise we would probably end up with a mixed string in the buffer.
// If using  RTT from within interrupts, multiple tasks or multi processors, define the RTT_LOCK() and RTT_UNLOCK() function here.
//
// RTT_MAX_INTERRUPT_PRIORITY can be used in the sample lock routines on Cortex-M3/4.
// Make sure to mask all interrupts which can send RTT data, i.e. generate SystemView events, or cause task switches.
// When high-priority interrupts must not be masked while sending RTT data, RTT_MAX_INTERRUPT_PRIORITY needs to be adjusted accordingly.
// (Higher priority = lower priority number)
// Default value for embOS: 128u
// Default configuration in FreeRTOS: configMAX_SYSCALL_INTERRUPT_PRIORITY: ( configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << (8 - configPRIO_BITS) )
// In case of doubt mask all interrupts: 1 << (8 - BASEPRI_PRIO_BITS) i.e. 1 << 5 when 3 bits are implemented in NVIC
// or define RTT_LOCK() to completely disable interrupts.
//
#ifndef   RTT_MAX_INTERRUPT_PRIORITY
  #define RTT_MAX_INTERRUPT_PRIORITY         (0x20)   // Interrupt priority to lock on RTT_LOCK on Cortex-M3/4 (Default: 0x20)
#endif

/*********************************************************************
*
*       RTT lock configuration for Embedded Studio,
*       Rowley CrossStudio and GCC
*/
#if ((defined(__SES_ARM) || defined(__SES_RISCV) || defined(__CROSSWORKS_ARM) || defined(__GNUC__) || defined(__clang__)) && !defined (__CC_ARM) && !defined(WIN32))
  #if (defined(__ARM_ARCH_6M__) || defined(__ARM_ARCH_8M_BASE__))
    #define RTT_LOCK()  {                                                                           \
                                    unsigned int _RTT__LockState;                                   \
                                  __asm volatile ("mrs   %0, primask  \n\t"                         \
                                                  "movs  r1, #1       \n\t"                         \
                                                  "msr   primask, r1  \n\t"                         \
                                                  : "=r" (_RTT__LockState)                          \
                                                  :                                                 \
                                                  : "r1", "cc"                                      \
                                                  );

    #define RTT_UNLOCK()          __asm volatile ("msr   primask, %0  \n\t"                         \
                                                  :                                                 \
                                                  : "r" (_RTT__LockState)                           \
                                                  :                                                 \
                                                  );                                                \
                        }
  #elif (defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__) || defined(__ARM_ARCH_8M_MAIN__))
    #ifndef   RTT_MAX_INTERRUPT_PRIORITY
      #define RTT_MAX_INTERRUPT_PRIORITY   (0x20)
    #endif
    #define RTT_LOCK()  {                                                                           \
                                    unsigned int _RTT__LockState;                                   \
                                  __asm volatile ("mrs   %0, basepri  \n\t"                         \
                                                  "mov   r1, %1       \n\t"                         \
                                                  "msr   basepri, r1  \n\t"                         \
                                                  : "=r" (_RTT__LockState)                          \
                                                  : "i"(RTT_MAX_INTERRUPT_PRIORITY)                 \
                                                  : "r1", "cc"                                      \
                                                  );

    #define RTT_UNLOCK()          __asm volatile ("msr   basepri, %0  \n\t"                         \
                                                  :                                                 \
                                                  : "r" (_RTT__LockState)                           \
                                                  :                                                 \
                                                  );                                                \
                        }

  #elif defined(__ARM_ARCH_7A__)
    #define RTT_LOCK() {                                                       \
                                 unsigned int _RTT__LockState;                 \
                                 __asm volatile ("mrs r1, CPSR \n\t"           \
                                                 "mov %0, r1 \n\t"             \
                                                 "orr r1, r1, #0xC0 \n\t"      \
                                                 "msr CPSR_c, r1 \n\t"         \
                                                 : "=r" (_RTT__LockState)      \
                                                 :                             \
                                                 : "r1", "cc"                  \
                                                 );

    #define RTT_UNLOCK()        __asm volatile ("mov r0, %0 \n\t"              \
                                                "mrs r1, CPSR \n\t"            \
                                                "bic r1, r1, #0xC0 \n\t"       \
                                                "and r0, r0, #0xC0 \n\t"       \
                                                "orr r1, r1, r0 \n\t"          \
                                                "msr CPSR_c, r1 \n\t"          \
                                                :                              \
                                                : "r" (_RTT__LockState)        \
                                                : "r0", "r1", "cc"             \
                                                );                             \
                        }
  #elif defined(__riscv) || defined(__riscv_xlen)
    #define RTT_LOCK()  {                                                      \
                                 unsigned int _RTT__LockState;                 \
                                 __asm volatile ("csrr  %0, mstatus  \n\t"     \
                                                 "csrci mstatus, 8   \n\t"     \
                                                 "andi  %0, %0,  8   \n\t"     \
                                                 : "=r" (_RTT__LockState)      \
                                                 :                             \
                                                 :                             \
                                                );

  #define RTT_UNLOCK()           __asm volatile ("csrr  a1, mstatus  \n\t"     \
                                                 "or    %0, %0, a1   \n\t"     \
                                                 "csrs  mstatus, %0  \n\t"     \
                                                 :                             \
                                                 : "r"  (_RTT__LockState)      \
                                                 : "a1"                        \
                                                );                             \
                        }
  #else
    #define RTT_LOCK()
    #define RTT_UNLOCK()
  #endif
#endif

/*********************************************************************
*
*       RTT lock configuration for IAR EWARM
*/
#ifdef __ICCARM__
  #if (defined (__ARM6M__)          && (__CORE__ == __ARM6M__))             ||                      \
      (defined (__ARM8M_BASELINE__) && (__CORE__ == __ARM8M_BASELINE__))
    #define RTT_LOCK()  {                                                                           \
                            unsigned int _RTT__LockState;                                           \
                            _RTT__LockState = __get_PRIMASK();                                      \
                            __set_PRIMASK(1);

    #define RTT_UNLOCK()    __set_PRIMASK(_RTT__LockState);                                         \
                        }
  #elif (defined (__ARM7EM__)         && (__CORE__ == __ARM7EM__))          ||                      \
        (defined (__ARM7M__)          && (__CORE__ == __ARM7M__))           ||                      \
        (defined (__ARM8M_MAINLINE__) && (__CORE__ == __ARM8M_MAINLINE__))  ||                      \
        (defined (__ARM8M_MAINLINE__) && (__CORE__ == __ARM8M_MAINLINE__))
    #ifndef   RTT_MAX_INTERRUPT_PRIORITY
      #define RTT_MAX_INTERRUPT_PRIORITY   (0x20)
    #endif
    #define RTT_LOCK()  {                                                                           \
                            unsigned int _RTT__LockState;                                           \
                            _RTT__LockState = __get_BASEPRI();                                      \
                            __set_BASEPRI(RTT_MAX_INTERRUPT_PRIORITY);

    #define RTT_UNLOCK()    __set_BASEPRI(_RTT__LockState);                                         \
                        }
  #elif (defined (__ARM7A__) && (__CORE__ == __ARM7A__))                    ||                      \
        (defined (__ARM7R__) && (__CORE__ == __ARM7R__))
    #define RTT_LOCK() {                                                                            \
                                 unsigned int _RTT__LockState;                                      \
                                 __asm volatile ("mrs r1, CPSR \n\t"                                \
                                                 "mov %0, r1 \n\t"                                  \
                                                 "orr r1, r1, #0xC0 \n\t"                           \
                                                 "msr CPSR_c, r1 \n\t"                              \
                                                 : "=r" (_RTT__LockState)                           \
                                                 :                                                  \
                                                 : "r1", "cc"                                       \
                                                 );

    #define RTT_UNLOCK()        __asm volatile ("mov r0, %0 \n\t"                                   \
                                                "mrs r1, CPSR \n\t"                                 \
                                                "bic r1, r1, #0xC0 \n\t"                            \
                                                "and r0, r0, #0xC0 \n\t"                            \
                                                "orr r1, r1, r0 \n\t"                               \
                                                "msr CPSR_c, r1 \n\t"                               \
                                                :                                                   \
                                                : "r" (_RTT__LockState)                             \
                                                : "r0", "r1", "cc"                                  \
                                                );                                                  \
                            }
  #endif
#endif

/*********************************************************************
*
*       RTT lock configuration for IAR RX
*/
#ifdef __ICCRX__
  #define RTT_LOCK()    {                                                                           \
                            unsigned long _RTT__LockState;                                          \
                            _RTT__LockState = __get_interrupt_state();                              \
                            __disable_interrupt();

  #define RTT_UNLOCK()      __set_interrupt_state(_RTT__LockState);                                 \
                        }
#endif

/*********************************************************************
*
*       RTT lock configuration for IAR RL78
*/
#ifdef __ICCRL78__
  #define RTT_LOCK()    {                                                                           \
                            __istate_t _RTT__LockState;                                             \
                            _RTT__LockState = __get_interrupt_state();                              \
                            __disable_interrupt();

  #define RTT_UNLOCK()      __set_interrupt_state(_RTT__LockState);                                 \
                        }
#endif

/*********************************************************************
*
*       RTT lock configuration for KEIL ARM
*/
#ifdef __CC_ARM
  #if (defined __TARGET_ARCH_6S_M)
    #define RTT_LOCK()  {                                                                           \
							unsigned int _RTT__LockState;                                           \
							register unsigned char _RTT__PRIMASK __asm( "primask");                 \
							_RTT__LockState = _RTT__PRIMASK;                                        \
							_RTT__PRIMASK = 1u;                                                     \
							__schedule_barrier();

    #define RTT_UNLOCK()    _RTT__PRIMASK = _RTT__LockState;                                        \
                             __schedule_barrier();                                                  \
                        }
  #elif (defined(__TARGET_ARCH_7_M) || defined(__TARGET_ARCH_7E_M))
    #ifndef   RTT_MAX_INTERRUPT_PRIORITY
      #define RTT_MAX_INTERRUPT_PRIORITY   (0x20)
    #endif
    #define RTT_LOCK()  {                                                                           \
                            unsigned int _RTT__LockState;                                           \
                            register unsigned char BASEPRI __asm( "basepri");                       \
                            _RTT__LockState = BASEPRI;                                              \
                            BASEPRI = RTT_MAX_INTERRUPT_PRIORITY;                                   \
                            __schedule_barrier();

    #define RTT_UNLOCK()    BASEPRI = _RTT__LockState;                                              \
                            __schedule_barrier();                                                   \
                        }
  #endif
#endif

/*********************************************************************
*
*       RTT lock configuration for TI ARM
*/
#ifdef __TI_ARM__
  #if defined (__TI_ARM_V6M0__)
    #define RTT_LOCK()  {                                                                           \
                            unsigned int _RTT__LockState;                                           \
                            _RTT__LockState = __get_PRIMASK();                                      \
                            __set_PRIMASK(1);

    #define RTT_UNLOCK()   __set_PRIMASK(_RTT__LockState);                                          \
                        }
  #elif (defined (__TI_ARM_V7M3__) || defined (__TI_ARM_V7M4__))
    #ifndef   RTT_MAX_INTERRUPT_PRIORITY
      #define RTT_MAX_INTERRUPT_PRIORITY   (0x20)
    #endif
    #define RTT_LOCK()  {                                                                           \
                            unsigned int _RTT__LockState;                                           \
                            _RTT__LockState = _set_interrupt_priority(RTT_MAX_INTERRUPT_PRIORITY);

    #define RTT_UNLOCK()    _set_interrupt_priority(_RTT__LockState);                               \
                        }
  #endif
#endif

/*********************************************************************
*
*       RTT lock configuration for CCRX
*/
#ifdef __RX
  #include <machine.h>
  #define RTT_LOCK()    {                                                                           \
                                unsigned long _RTT__LockState;                                      \
                                _RTT__LockState = get_psw() & 0x010000;                             \
                                clrpsw_i();

  #define RTT_UNLOCK()          set_psw(get_psw() | _RTT__LockState);                               \
                        }
#endif

/*********************************************************************
*
*       RTT lock configuration for embOS Simulation on Windows
*       (Can also be used for generic RTT locking with embOS)
*/
#if defined(WIN32) || defined(RTT_LOCK_EMBOS)

void OS_SIM_EnterCriticalSection(void);
void OS_SIM_LeaveCriticalSection(void);

#define RTT_LOCK()      {                                                                           \
                            OS_SIM_EnterCriticalSection();

#define RTT_UNLOCK()        OS_SIM_LeaveCriticalSection();                                          \
                        }
#endif

/*********************************************************************
*
*       RTT lock configuration fallback
*/
#ifndef   RTT_LOCK
  #define RTT_LOCK()                // Lock RTT (nestable)   (i.e. disable interrupts)
#endif

#ifndef   RTT_UNLOCK
  #define RTT_UNLOCK()              // Unlock RTT (nestable) (i.e. enable previous interrupt lock state)
#endif

#endif
/*************************** End of file ****************************/
