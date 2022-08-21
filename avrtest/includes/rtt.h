#ifndef __AVR_RTT__
#define __AVR_RTT__
#include <stdint.h>

#ifdef BUILD_TYPE
#if BUILD_TYPE == Debug


#ifndef __DEBUG_BUFFER_SIZE__
    #define __DEBUG_BUFFER_SIZE__ 64
#else
    #if __DEBUG_BUFFER_SIZE__ > 64
        #error "Buffer size mus be at most 64 bytes"
    #endif
#endif

/**
 * This call performs a rtt tranfer if rtt has been enabled from the debugger.
 * It will set the message size (limiting to how much space is available) and the available flag
 * then it will break waiting for the debugger to resume.
 *
 * burden of the debugger is to reset the flags and continue execution.
 */
void rtt_log(const char *buffer, uint8_t size);

/**
 * Initializes data structures for rtt operations.
 */
void rtt_init(void);

#else

inline void rtt_log(const char *buffer, uint8_t size){}
inline void rtt_init(void){}
inline void rtt_enable(void){}

#endif
#endif

#endif // __AVR_RTT__