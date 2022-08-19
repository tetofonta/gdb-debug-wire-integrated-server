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

void rtt_log(const char *buffer, uint8_t size);
void rtt_init(void);
void rtt_enable(void);

#else

inline void rtt_log(const char *buffer, uint8_t size){}
inline void rtt_init(void){}
inline void rtt_enable(void){}

#endif
#endif

#endif // __AVR_RTT__