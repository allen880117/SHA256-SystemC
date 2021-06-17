#include <io8051.h>

void init_iss(void) {
    P0 = 85;
    // while(P0!=170);
    P2 = 0;
    P3 = 0;
    P0 = 0;
    P0 = 150;

    P0 = 0;
    P1 = 0;
    P2 = 0;
    P3 = 0;
}

#define CTRL_OUT_IDLE (0x00)
#define CTRL_OUT_CLEAR (0xCC)
#define CTRL_OUT_BUSY (0x01)
#define CTRL_OUT_DONE (0x02)
#define CTRL_OUT_REQ_POSTFIX (0x03)
#define CTRL_OUT_RESULT_POSTFIX (0x06)

#define CTRL_IN_RESET (0x00)
#define CTRL_IN_CLEAR (0xCC)
#define CTRL_IN_START (0xC0)
#define CTRL_IN_DATA_PREFIX (0x40)
#define CTRL_IN_RESULT_PREFIX (0x80)

#define CTRL_IN P0
#define CTRL_OUT P1
#define DATA_IN P2
#define DATA_OUT P3

void main(void) {
    // char context[] = "abcdefg";
    // char context[] = "1092-Hardware-Software-Co-Design-Class-0617_a3s2f1as68df1as9df8a4sd6f1asdf9a8sdf1as6df531ASDF68as1d9f8s1df956789kbu";
    char context[] = "0123456789abcdeffedcba98765432100123456789abcdeffedcba98765432100123456789abcdeffedcba98765432100123456789abcdeffedcba98765432100123456789abcdeffedcba98765432100123456789abcdeffedcba98765432100123456789abcdeffedcba98765432100123456789abcdeffedcba987654321";
    
    char result[32];
    unsigned int i;
    
    init_iss();

    CTRL_IN = CTRL_IN_RESET;  // reset encoder

    while (CTRL_OUT != CTRL_OUT_DONE) {
        if (CTRL_OUT == CTRL_OUT_IDLE) {
            // DATA_IN = 7; // Write Data First
            // DATA_IN = 115; // Write Data First
            DATA_IN = 255;  // Write Data First

            CTRL_IN = CTRL_IN_START;  // Then Control Signal

        } else if ((CTRL_OUT & 0x03) == CTRL_OUT_REQ_POSTFIX) {
            DATA_IN = (unsigned char)context[P3];  // Write Data First

            CTRL_IN = (CTRL_IN_DATA_PREFIX |
                       ((CTRL_OUT & (~0x03)) >> 2));  // Then Control Signal
        } else if (CTRL_OUT == CTRL_OUT_CLEAR) {
            CTRL_IN = CTRL_IN_CLEAR;
        }
    }

    while ((CTRL_OUT & 0x03) == CTRL_OUT_DONE) {
        if (CTRL_OUT == CTRL_OUT_DONE) {
            i       = 0;
            CTRL_IN = (CTRL_IN_RESULT_PREFIX | i);
        } else if ((CTRL_OUT & 0x07) == CTRL_OUT_RESULT_POSTFIX) {
            unsigned int idx = ((CTRL_OUT & 0xF8) >> 3);
            result[idx]      = DATA_OUT;

            i++;
            if (i == 32)
                break;
            else
                CTRL_IN = (CTRL_IN_RESULT_PREFIX | i);
        }
    }

    i = 0;
    while (i < 32) {
        CTRL_IN = result[i];
        DATA_IN = result[i];
        i++;
    }

    P3   = 0x00;  // unable
    PCON = 2;     // Set a power down control signal to 8051
    P2   = 0;
    P3   = 0;
}
