#include <stdint.h>

void mister_init(const char* misterHost, uint8_t lz4Frames, uint16_t mtu);
void mister_close(void);
void mister_set_mode(double pClock, uint16_t hActive, uint16_t hBegin, uint16_t hEnd, uint16_t hTotal, uint16_t vActive, uint16_t vBegin, uint16_t vEnd, uint16_t vTotal, uint8_t interlace);
char* mister_get_blit_buffer(void);
char* mister_get_audio_buffer(void);
void mister_blit(void);
void mister_audio(uint16_t audio_bytes);

int mister_get_field(void);
uint8_t mister_is_interlaced_fb(void);
uint8_t mister_is_connected(void);
