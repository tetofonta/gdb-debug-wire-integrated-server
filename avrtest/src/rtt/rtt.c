#include <rtt.h>
#include <string.h>

#ifdef BUILD_TYPE
#if BUILD_TYPE == Debug

typedef struct{
    uint8_t available : 1;
    uint8_t enabled : 1;
    uint8_t size : 6;
    uint8_t data[64];
} __attribute__((packed)) rtt_data;

volatile rtt_data rtt __attribute__((section(".dbgdata")));

void rtt_log(const char *buffer, uint8_t size){
    if (!rtt.enabled)
        return;
        
    size &= 0x3F;
    rtt.size = size;

    while(size--)
        *(rtt.data + size) = *(buffer + size);
        
    rtt.available = 1;
    asm volatile("break");
}

void rtt_init(void){
    rtt.available = 0;
    rtt.size = 0;
}

#endif
#endif