#ifndef _SHA256_H_
#define _SHA256_H_

#include <string>

#include "systemc.h"

#define ROTLEFT(a, b) (((a) << (b)) | ((a) >> (32 - (b))))
#define ROTRIGHT(a, b) (((a) >> (b)) | ((a) << (32 - (b))))
#define CH(x, y, z) (((x) & (y)) ^ (~(x) & (z)))
#define MAJ(x, y, z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))
#define EP0(x) (ROTRIGHT(x, 2) ^ ROTRIGHT(x, 13) ^ ROTRIGHT(x, 22))
#define EP1(x) (ROTRIGHT(x, 6) ^ ROTRIGHT(x, 11) ^ ROTRIGHT(x, 25))
#define SIG0(x) (ROTRIGHT(x, 7) ^ ROTRIGHT(x, 18) ^ ((x) >> 3))
#define SIG1(x) (ROTRIGHT(x, 17) ^ ROTRIGHT(x, 19) ^ ((x) >> 10))

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

/* States */
enum class FSM {
    IDLE = 0,
    REMAIN_LEN_CHECK,

    HASH_UPDATE_START,
    HASH_UPDATE_ITERATION,
    HASH_UPDATE_DONE,

    REQ_START_P1,
    REQ_START_P2,
    REQ_READING,
    REQ_DONE,

    TAIL_JOB_START,
    TAIL_JOB_REMAIN_LEN_CHECK,
    TAIL_JOB_NORMAL,
    TAIL_JOB_ONEMORE_P1,
    TAIL_JOB_ONEMORE_P2,
    TAIL_JOB_DONE,

    WAIT_FOR_READ,
};
std::string print_state(FSM state);

SC_MODULE(sha256) {
    sc_in_clk clk;

    sc_in<sc_uint<8>>  ctrlIn;
    sc_in<sc_uint<8>>  dataIn;
    sc_out<sc_uint<8>> ctrlOut;
    sc_out<sc_uint<8>> dataOut;

    sc_uint<64> ctxLen;  // Byte length of context

    sc_uint<32> hash[8];   // Hash Value
    sc_uint<8>  data[64];  // Read-in 512 bit data
    sc_uint<64> offset;    // Current Read Ptr

    bool        tailing;  // Work on Tail job
    bool        tailingMoreUpdate;
    sc_uint<64> tailLen;  // Tailing Length

    sc_uint<8>  count;
    sc_uint<8>  readSize;
    sc_uint<32> a, b, c, d, e, f, g, h;
    sc_uint<32> w[64];
    sc_uint<32> wsig0[64];
    sc_uint<32> wsig1[64];

    FSM currState;
    FSM nextState;

    void exec();

    SC_CTOR(sha256) {
        SC_THREAD(exec);
        sensitive << clk.pos();
    }
};

#endif