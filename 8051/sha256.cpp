#include "sha256.h"

std::string print_state(FSM state) {
    if (state == FSM::IDLE)
        return std::string("IDLE");
    else if (state == FSM::REMAIN_LEN_CHECK)
        return std::string("REMAIN_LEN_CHECK");
    else if (state == FSM::HASH_UPDATE_START)
        return std::string("HASH_UPDATE_START");
    else if (state == FSM::HASH_UPDATE_ITERATION)
        return std::string("HASH_UPDATE_ITERATION");
    else if (state == FSM::HASH_UPDATE_DONE)
        return std::string("HASH_UPDATE_DONE");
    else if (state == FSM::REQ_START_P1)
        return std::string("REQ_START_P1");
    else if (state == FSM::REQ_START_P2)
        return std::string("REQ_START_P2");
    else if (state == FSM::REQ_READING)
        return std::string("REQ_READING");
    else if (state == FSM::REQ_DONE)
        return std::string("REQ_DONE");
    else if (state == FSM::TAIL_JOB_START)
        return std::string("TAIL_JOB_START");
    else if (state == FSM::TAIL_JOB_REMAIN_LEN_CHECK)
        return std::string("TAIL_JOB_REMAIN_LEN_CHECK");
    else if (state == FSM::TAIL_JOB_NORMAL)
        return std::string("TAIL_JOB_NORMAL");
    else if (state == FSM::TAIL_JOB_ONEMORE_P1)
        return std::string("TAIL_JOB_ONEMORE_P1");
    else if (state == FSM::TAIL_JOB_ONEMORE_P2)
        return std::string("TAIL_JOB_ONEMORE_P2");
    else if (state == FSM::TAIL_JOB_DONE)
        return std::string("TAIL_JOB_DONE");
    else if (state == FSM::WAIT_FOR_READ)
        return std::string("WAIT_FOR_READ");
    else
        return std::string("UNKNOWN_ERROR");
}

static const sc_uint<32> k[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1,
    0x923f82a4, 0xab1c5ed5, 0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
    0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174, 0xe49b69c1, 0xefbe4786,
    0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147,
    0x06ca6351, 0x14292967, 0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
    0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85, 0xa2bfe8a1, 0xa81a664b,
    0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a,
    0x5b9cca4f, 0x682e6ff3, 0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
    0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2};

void sha256::exec() {
    /* State Initialization */
    currState = FSM::IDLE;
    nextState = FSM::IDLE;

    /* Thread */
    while (true) {
        /* State Work & Next State Logic */
        if (this->currState == FSM::IDLE) {
            /* Common Jobs */
            /* Reset Some Variables */
            tailing           = false;
            tailingMoreUpdate = false;
            count             = 0;
            readSize          = 64;
            offset            = 0;  // Current Read Length
            hash[0] = 0x6a09e667, hash[1] = 0xbb67ae85;
            hash[2] = 0x3c6ef372, hash[3] = 0xa54ff53a;
            hash[4] = 0x510e527f, hash[5] = 0x9b05688c;
            hash[6] = 0x1f83d9ab, hash[7] = 0x5be0cd19;

            /* Next currState logic selection */
            if (ctrlIn.read() == CTRL_IN_START) {
                /* Start Encoding
                /* Get Message Length */
                ctxLen = dataIn.read();
                /* Raise Busy */
                ctrlOut.write(CTRL_OUT_BUSY);
                /* Assign Next State */
                nextState = FSM::REMAIN_LEN_CHECK;
            } else {
                /* Raise IDLE */
                ctrlOut.write(CTRL_OUT_IDLE);
                /* Assign Next State */
                nextState = FSM::IDLE;
            }

        } else if (this->currState == FSM::REMAIN_LEN_CHECK) {
            if (offset + 64 > ctxLen) {
                /* Do Tail Jobs */
                nextState = FSM::TAIL_JOB_START;
            } else {
                /* Go to Request Phase */
                nextState = FSM::REQ_START_P1;
            }
        } else if (this->currState == FSM::REQ_START_P1) {
            if (readSize != 0) {
                /* Ask to Clear the Control Signal from 8051 */
                ctrlOut.write(CTRL_OUT_CLEAR);
                /* Check Response */
                if (ctrlIn.read() == CTRL_IN_CLEAR) {
                    nextState = FSM::REQ_START_P2;
                } else {
                    nextState = FSM::REQ_START_P1;
                }
            } else {
                /* No need to read */
                nextState = FSM::REQ_DONE;
            }
        } else if (this->currState == FSM::REQ_START_P2) {
            /* Common Jobs*/
            count = 0;
            /* Raise for first byte */
            ctrlOut.write((count << 2) | CTRL_OUT_REQ_POSTFIX);
            dataOut.write(offset + count);
            /* Go to req A reading */
            nextState = FSM::REQ_READING;

        } else if (this->currState == FSM::REQ_READING) {
            /* Check Valid of Ctrl Message */
            if (ctrlIn.read() == (CTRL_IN_DATA_PREFIX | count)) {
                /* Write Data in */
                data[count] = dataIn.read();
                /* End condition check */
                if (count == readSize - 1) {
                    ctrlOut.write(CTRL_OUT_BUSY);  // Local Address
                    dataOut.write(0x00);           // Global Address
                    nextState = FSM::REQ_DONE;
                } else {
                    /* count increment */
                    count++;
                    /* Send out Signal */
                    ctrlOut.write((count << 2) |
                                  CTRL_OUT_REQ_POSTFIX);  // Local Address
                    dataOut.write(offset + count);        // Global Address
                    nextState = FSM::REQ_READING;
                }
            } else {
                /* Not yet */
                nextState = FSM::REQ_READING;
            }

        } else if (this->currState == FSM::REQ_DONE) {
            /* Reset to Busy State */
            ctrlOut.write(CTRL_OUT_BUSY);
            /* Offset Increment */
            offset += 64;
            /* Check Whether Tailing Job Read */
            if (tailing) {
                nextState = FSM::TAIL_JOB_REMAIN_LEN_CHECK;
            } else {
                nextState = FSM::HASH_UPDATE_START;
            }

        } else if (this->currState == FSM::HASH_UPDATE_START) {
            /* Common jobs for initialization */
            /* Reset to Busy State */
            ctrlOut.write(CTRL_OUT_BUSY);
            /* Assign temp values */
            a = hash[0], b = hash[1], c = hash[2], d = hash[3];
            e = hash[4], f = hash[5], g = hash[6], h = hash[7];
            /* Calculate W[0~15] */
            for (size_t i = 0, j = 0; i < 16; i++, j += 4) {
                sc_uint<32> tmp_wi = (data[j] << 24) | (data[j + 1] << 16) |
                                     (data[j + 2] << 8) | (data[j + 3]);
                w[i]     = tmp_wi;
                wsig0[i] = SIG0(tmp_wi);
                wsig1[i] = SIG1(tmp_wi);
            }
            /* Reset Count */
            count = 0;
            /* Next currState */
            nextState = FSM::HASH_UPDATE_ITERATION;

        } else if (this->currState == FSM::HASH_UPDATE_ITERATION) {
            /* Iteration Job */
            {
                /* Temporary Wi */
                sc_uint<32> tmp_wi = w[count];

                /* Temporal Summation */
                sc_uint<32> t1 = h + EP1(e) + CH(e, f, g) + k[count] + tmp_wi;
                sc_uint<32> t2 = EP0(a) + MAJ(a, b, c);

                /* Swap Values*/
                h = g;
                g = f;
                f = e;
                e = d + t1;
                d = c;
                c = b;
                b = a;
                a = t1 + t2;

                /* Forward Calculation(For Reducing the Critical Path) */
                if (count < 48) {
                    sc_uint<32> tmp_wi_16 = wsig1[count + 14] + w[count + 9] +
                                            wsig0[count + 1] + tmp_wi;
                    w[count + 16]     = tmp_wi_16;
                    wsig0[count + 16] = SIG0(tmp_wi_16);
                    wsig1[count + 16] = SIG1(tmp_wi_16);
                }
            }
            /* Next State Logic */
            if (count == 63) {
                nextState = FSM::HASH_UPDATE_DONE;
            } else {
                nextState = FSM::HASH_UPDATE_ITERATION;
            }
            /* Count Increment */
            count++;

        } else if (this->currState == FSM::HASH_UPDATE_DONE) {
            /* Accumulate */
            hash[0] += a, hash[1] += b, hash[2] += c, hash[3] += d;
            hash[4] += e, hash[5] += f, hash[6] += g, hash[7] += h;

            /* State Transition */
            if (tailing) {
                if (tailingMoreUpdate)
                    nextState = FSM::TAIL_JOB_ONEMORE_P2;
                else
                    nextState = FSM::TAIL_JOB_DONE;
            } else {
                nextState = FSM::REMAIN_LEN_CHECK;
            }

        } else if (this->currState == FSM::TAIL_JOB_START) {
            /* Reset to Busy State */
            ctrlOut.write(CTRL_OUT_BUSY);
            /* Set tailing to true */
            tailing = true;
            /* Set Essential Variables */
            tailLen  = ctxLen - offset;
            readSize = tailLen;  // Change Readsize
            /* Read last data */
            nextState = FSM::REQ_START_P1;

        } else if (this->currState == FSM::TAIL_JOB_REMAIN_LEN_CHECK) {
            /* Check need one more update or not */
            if (tailLen <= 64 - 1 - 8) {
                tailingMoreUpdate = false;
                /* Next State */
                nextState = FSM::TAIL_JOB_NORMAL;
            } else {
                tailingMoreUpdate = true;
                /* Next State */
                nextState = FSM::TAIL_JOB_ONEMORE_P1;
            }

        } else if (this->currState == FSM::TAIL_JOB_NORMAL) {
            /* No more Update */
            tailingMoreUpdate = false;
            /* Append 1 at tail of message */
            data[tailLen] = 0x80;
            /* Fill Other chunks with zero */
            for (size_t i = tailLen + 1; i < 64 - 8; i++) {
                data[i] = 0x00;
            }
            /* Fill the length of context at the end of chunk */
            sc_uint<64> bit_length = ctxLen * 8;
            data[56]               = bit_length.range(63, 56);
            data[57]               = bit_length.range(55, 48);
            data[58]               = bit_length.range(47, 40);
            data[59]               = bit_length.range(39, 32);
            data[60]               = bit_length.range(31, 24);
            data[61]               = bit_length.range(23, 16);
            data[62]               = bit_length.range(15, 8);
            data[63]               = bit_length.range(7, 0);
            /* Next State Logic */
            nextState = FSM::HASH_UPDATE_START;

        } else if (this->currState == FSM::TAIL_JOB_ONEMORE_P1) {
            /* Need more update */
            tailingMoreUpdate = true;
            /* Append 1 at tail of message */
            data[tailLen] = 0x80;
            /* Next State Logic */
            nextState = FSM::HASH_UPDATE_START;

        } else if (this->currState == FSM::TAIL_JOB_ONEMORE_P2) {
            /* No more Update */
            tailingMoreUpdate = false;
            /* Fill Other chunks with zero */
            for (size_t i = 0; i < 64 - 8; i++) {
                data[i] = 0x00;
            }
            /* Fill the length of context at the end of chunk */
            sc_uint<64> bit_length = ctxLen * 8;
            data[56]               = bit_length.range(63, 56);
            data[57]               = bit_length.range(55, 48);
            data[58]               = bit_length.range(47, 40);
            data[59]               = bit_length.range(39, 32);
            data[60]               = bit_length.range(31, 24);
            data[61]               = bit_length.range(23, 16);
            data[62]               = bit_length.range(15, 8);
            data[63]               = bit_length.range(7, 0);
            /* Next State Logic */
            nextState = FSM::HASH_UPDATE_START;

        } else if (this->currState == FSM::TAIL_JOB_DONE) {
            /* Raise Done */
            ctrlOut.write(CTRL_OUT_DONE);
            /* Next State */
            nextState = FSM::WAIT_FOR_READ;

        } else if (this->currState == FSM::WAIT_FOR_READ) {
            if (ctrlIn.read().range(7, 5) == (CTRL_IN_RESULT_PREFIX >> 5)) {
                sc_uint<8> idx = ctrlIn.read().range(4, 0);
                ctrlOut.write((idx << 3) | CTRL_OUT_RESULT_POSTFIX);

                sc_uint<8> hashIdx       = idx >> 2;
                sc_uint<8> hashRangeUp   = ((0x03 - (idx & 0x03)) << 3) + 7;
                sc_uint<8> hashRangeDown = ((0x03 - (idx & 0x03)) << 3);
                dataOut.write(hash[hashIdx].range(hashRangeUp, hashRangeDown));
            }
            /* Hold State */
            nextState = FSM::WAIT_FOR_READ;

        } else {
            nextState = FSM::IDLE;
        }

        /* Assign Next State */
        if (ctrlIn.read() == CTRL_IN_RESET) {
            currState = FSM::IDLE;
        } else {
            currState = nextState;
        }

        /* End Wait */
        wait();
    }
}
