#include <stdio.h>
#include <string.h>
#include <systemc.h>

#include "sha256.h"

#define PRINTLN(FORMAT, ...)                                           \
    printf("%8s: " FORMAT "\n", (sc_time_stamp().to_string().c_str()), \
           __VA_ARGS__)

#define PRINT_RESULT                                                          \
    PRINTLN("%08x %08x %08x %08x %08x %08x %08x %08x", dev.hash[0].to_uint(), \
            dev.hash[1].to_uint(), dev.hash[2].to_uint(),                     \
            dev.hash[3].to_uint(), dev.hash[4].to_uint(),                     \
            dev.hash[5].to_uint(), dev.hash[6].to_uint(),                     \
            dev.hash[7].to_uint())

#define clockcycle 10

#define PRINT_STATE                                              \
    PRINTLN("-> curr: %s", print_state(dev.currState).c_str()); \
    PRINTLN("-> next: %s", print_state(dev.nextState).c_str());

int sc_main(int argc, char *argv[]) {
    /* Signals Setup */
    sc_clock              clk("CLOCK", clockcycle, SC_NS, 0.5);
    sc_signal<sc_uint<8>> ctrlIn;
    sc_signal<sc_uint<8>> dataIn;
    sc_signal<sc_uint<8>> ctrlOut;
    sc_signal<sc_uint<8>> dataOut;

    sha256 dev("SHA256");
    dev.clk(clk);
    dev.dataIn(dataIn);
    dev.dataOut(dataOut);
    dev.ctrlIn(ctrlIn);
    dev.ctrlOut(ctrlOut);

    /* Start Test */
    char context[] =
        // "acdefgdddasdfacdefgdddasdfacdefgdddasdfacdefgdddasdfacdefgdddasdfacdef"
        // "gdddasdfacdefgdddasdfacdefgdddasdfacdefgdddasdfacdefgdddasdfacdefgddda"
        // "sdf";
        // "abcdefgh01234567abcdefgh01234567abcdefgh01234567abcdefgh01234567abcdef"
        // "gh01234567abcdefgh01234567abcdefgh01234567abcdefgh01234567abcdefghijkl"
        // "mnopqrst";
        // "dfhaseklha wefha weofj awopejf apwefja pwoefjapowejif apwoeifj "
        // "apwoefjiapw oefjapwoef jaopwefj aopwejf opawejfpoa wiejf paowejif "
        // "paowejif paowejf  aopwejf";
        "abcdefgh01234567abcdefgh01234567abcdefgh01234567abcdefgh01234567abcdef"
        "gh01234567abcdefgh01234567abcdefgh01234567abcdefgh01234567";

    size_t ctxLen = strlen(context);
    size_t counter = 0;
    bool   pp      = true;
    while (dev.ctrlOut.read() != CTRL_OUT_DONE && counter < 5000) {
        PRINTLN("%s", "===========================");
        PRINTLN("Port status before %4lu run", counter);
        PRINTLN("%s", "===========================");
        PRINTLN("-> CTRL_IN :  %02x, DATA_IN :  %02x",
                dev.ctrlIn.read().to_uint(), dev.dataIn.read().to_uint());
        PRINTLN("-> CTRL_OUT:  %02x, DATA_OUT:  %02x",
                dev.ctrlOut.read().to_uint(), dev.dataOut.read().to_uint());

        /* Simulate the Sudden Reset */
        if (counter == 300) ctrlIn.write(CTRL_IN_RESET);

        /* Run */
        sc_start(clockcycle, SC_NS);

        /* I/O Condition */
        if (dev.ctrlOut.read() == CTRL_OUT_IDLE) {
            // You should change Data first
            // Then raise Ctrl signal
            // Since Ctrl signal is also ack signal for
            // that Data signal is ready
            if (pp)
                dataIn.write(ctxLen);
            else
                ctrlIn.write(CTRL_IN_START);
            pp = !pp;
        } else if (dev.ctrlOut.read().range(1, 0) == CTRL_OUT_REQ_POSTFIX) {
            // You should change Data first
            // Then raise Ctrl signal
            // Since Ctrl signal is also ack signal for
            // that Data signal is ready
            if (pp)
                dataIn.write((unsigned char)context[dataOut.read()]);
            else
                ctrlIn.write(CTRL_IN_DATA_PREFIX |
                              (dev.ctrlOut.read().range(7, 2)));
            pp = !pp;
        } else if (dev.ctrlOut.read() == CTRL_OUT_CLEAR){
            ctrlIn.write(CTRL_IN_CLEAR);
        }

        /* After run */
        PRINTLN("%s",
                "=============================================================="
                "==========");
        PRINTLN(" %lu run:", counter);
        PRINT_STATE;
        PRINTLN("%s",
                "=============================================================="
                "==========");

        /* While Increment */
        counter++;
    }

    unsigned int result[32];
    counter           = 0;
    size_t done_count = 0;
    while (dev.ctrlOut.read().range(1, 0) == CTRL_OUT_DONE) {
        PRINTLN("%s", "===========================");
        PRINTLN("Port status before %4lu run", done_count);
        PRINTLN("%s", "===========================");
        PRINTLN("-> CTRL_IN :  %02x, DATA_IN :  %02x",
                dev.ctrlIn.read().to_uint(), dev.dataIn.read().to_uint());
        PRINTLN("-> CTRL_OUT:  %02x, DATA_OUT:  %02x",
                dev.ctrlOut.read().to_uint(), dev.dataOut.read().to_uint());

        /* Run */
        sc_start(clockcycle, SC_NS);

        if (dev.ctrlOut.read() == CTRL_OUT_DONE) {
            /* Start To Read Out Data */
            counter = 0;
            ctrlIn.write(CTRL_IN_RESULT_PREFIX | counter);
        } else if (dev.ctrlOut.read().range(2, 0) == CTRL_OUT_RESULT_POSTFIX) {
            /* Read Out Every Data */
            size_t idx  = dev.ctrlOut.read().range(7, 3);
            result[idx] = dev.dataOut.read();

            counter++;
            if (counter == 32)
                break;
            else
                ctrlIn.write(CTRL_IN_RESULT_PREFIX | counter);
        }

        /* After run */
        PRINTLN("%s",
                "=============================================================="
                "==========");
        PRINTLN(" After Done : %lu run:", done_count);
        PRINT_STATE;
        PRINTLN("%s",
                "=============================================================="
                "==========");

        /* While Increment */
        done_count++;
    }

    PRINTLN("%s",
            "=============================================================="
            "==========");
    PRINTLN("%s at %lu runs", "DONE", counter);
    PRINTLN("Original Context:\n %s", context);
    for (int i = 0; i < 32; i++) {
        printf("%02x", result[i]);
    }
    printf("\n");
    PRINTLN("%s",
            "=============================================================="
            "==========");
    PRINT_RESULT;

    /* Return */
    return 0;
}

// acdefgdddasdfacdefgdddasdfacdefgdddasdfacdefgdddasdfacdefgdddasdfacdefgdddasdfacdefgdddasdfacdefgdddasdfacdefgdddasdfacdefgdddasdfacdefgdddasdf
// dfhaseklha wefha weofj awopejf apwefja pwoefjapowejif apwoeifj apwoefjiapw
// oefjapwoef jaopwefj aopwejf opawejfpoa wiejf paowejif paowejif paowejf
// aopwejf