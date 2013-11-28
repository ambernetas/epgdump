#include "util.h"

void dump_hex(const char *prefix, uint8_t *buf, int size)
{
        int             i;
        unsigned char   ch;
        char            sascii[17];
        char            linebuffer[16*4+1];

        sascii[16] = 0x0;

        for(i = 0; i < size; i++) {
                ch = buf[i];
                if (i%16 == 0) {
                        sprintf(linebuffer, "%04x ", i);
                }
                sprintf(&linebuffer[(i%16)*3], "%02x ", ch);
                if (ch >= ' ' && ch <= '}')
                        sascii[i%16]=ch;
                else
                        sascii[i%16]='.';

                if (i%16 == 15)
                        fprintf(stderr, "%s %04x: %s  %s\n", prefix, i-0xf, linebuffer, sascii);
        }

        /* i++ after loop */
        if (i%16 != 0) {
                for(; i%16 != 0; i++) {
                        sprintf(&linebuffer[(i%16)*3], "   ");
                        sascii[i%16]=' ';
                }

                fprintf(stderr, "%s %04x: %s  %s\n", prefix, i-0xf-1, linebuffer, sascii);
        }
}

