#include <systemc.h>

#include <iomanip>
#include <iostream>

#define clockcycle 10  // 10ns=100MHz

#include "top.h"

#define PRINT_PORT                                              \
    std::cout << "@" << std::setw(10) << sc_time_stamp() << ":" \
              << " ,P0o = " << std::setw(2) << std::hex         \
              << top.core->port0o.read().to_uint()              \
              << " ,P1o = " << std::setw(2) << std::hex         \
              << top.core->port1o.read().to_uint()              \
              << " ,P2o = " << std::setw(2) << std::hex         \
              << top.core->port2o.read().to_uint()              \
              << " ,P3o = " << std::setw(2) << std::hex         \
              << top.core->port3o.read().to_uint() << std::endl \
              << "           :"                                 \
              << " ,P0i = " << std::setw(2) << std::hex         \
              << top.core->port0i.read().to_uint()              \
              << " ,P1i = " << std::setw(2) << std::hex         \
              << top.core->port1i.read().to_uint()              \
              << " ,P2i = " << std::setw(2) << std::hex         \
              << top.core->port2i.read().to_uint()              \
              << " ,P3i = " << std::setw(2) << std::hex         \
              << top.core->port3i.read().to_uint() << std::endl \
              << std::endl

int sc_main(int argc, char *argv[]) {
    /* Usage */
    if (argc < 3) {
        printf("Usage:8051_iss filename imagename\n");
        return (1);
    }
    char *filename = argv[1];

    /* Module Instance */
    sc_clock               clk("CLOCK", clockcycle, 0.5, 1);
    sc_clock               slow_clk("SLOW_CLOCK", clockcycle * 8, 0.5, 1);
    sc_signal<sc_uint<1> > reset;
    sc_signal<sc_uint<1> > poff;
    top                    top("top");

    /* Signal Assign */
    top.clk(clk);
    top.slow_clk(slow_clk);
    top.reset(reset);
    top.poff(poff);

    /* VCD */
    sc_trace_file *fp;                      // Create VCD file
    fp = sc_create_vcd_trace_file("wave");  // open(fp), create wave.vcd file
    fp->set_time_unit(1, SC_NS);            // set tracing resolution to ns
    /* VCD::Add signals to trace*/
    sc_trace(fp, clk, "clk");
    sc_trace(fp, top.port0o, "CtrlFrom8051");
    sc_trace(fp, top.port1i, "CtrlFromEncoder");
    sc_trace(fp, top.port2o, "DataFrom8051");
    sc_trace(fp, top.port3i, "DataFromEncoder");

    /* Load ROM */
    if (!top.core->loadrom(filename)) {
        printf("%s file not found\n", filename);
        return (1);
    }

    /* Load Memory */
    filename = argv[2];
    {
        FILE *        FR;
        unsigned char s;
        int           i;

        FR = fopen(filename, "rb");
        if (FR == NULL) {
            printf("%s file not found\n", filename);
            return (1);
        } else {
            for (i = 0; i < 8 * 8; i++) {
                s                              = fgetc(FR);
                top.xrambig->mem[(0x3000) + i] = s;
            }
        }
        fclose(FR);
    }

    /* Count */
    int t;

    /* RESET */
    reset.write(1);
    for (t = 0; t < 10; t++) {
        sc_start(clockcycle, SC_NS);
    }
    reset.write(0);

    /* Log */
    VARS_8051 olds, news;
    top.core->save_vars(&olds);

    /* Log */
    bool dolog = false;

    bool         first   = false;
    unsigned int lastRec = 0x9f;
    bool         start   = false;
    bool         done    = false;
    bool         end     = false;

    for (; t < 30000; t++) {
#if 1
        if (!start && top.core->port0o.read() == CTRL_IN_START) {
            start = true;
            std::cout << "===========" << std::endl << std::endl;
            std::cout << "START_POINT" << std::endl;
            PRINT_PORT;
        }

        if (!done && top.core->port1i.read() == CTRL_OUT_DONE) {
            done = true;
            std::cout << "DONE_POINT" << std::endl;
            PRINT_PORT;
        }

        if (!end && top.core->port0o.read() == 0x9f &&
            top.core->port1i.read() == 0xfe) {
            end = true;
            std::cout << "END_POINT" << std::endl;
            PRINT_PORT;
            std::cout << "===========" << std::endl;
        }

        if (top.core->port0o.read() == 0x9f) {
            first = true;
        }

        if (first && top.core->port0o.read() == top.core->port2o.read()) {
            if (lastRec != top.core->port0o.read()) {
                lastRec = top.core->port0o.read();
                PRINT_PORT;
            }
        }

#else
        PRINT_PORT;
#endif

        if (dolog) {
            printf("%08d#", t);
            top.core->show_assembly();
        }

        sc_start(clockcycle, SC_NS);

        if (dolog) {
            top.core->save_vars(&news);
            show_debug(&news, &olds);
            memcpy(&olds, &news, sizeof(VARS_8051));
        }
    }

    sc_close_vcd_trace_file(fp);  // close(fp)
    return (0);
}
