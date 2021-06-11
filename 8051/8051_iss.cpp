#include <systemc.h>
// #include <conio.h>
#define clockcycle 10  // 10ns=100MHz
#include <iomanip>
#include <iostream>

#include "top.h"

#define PRINT_PORT                                                          \
    std::cout << "@" << std::setw(10) << sc_time_stamp() << ":"             \
              << " ,P0o = " << std::setw(2) << std::hex << top.core->port0o.read().to_uint() \
              << " ,P1o = " << std::setw(2) << std::hex << top.core->port1o.read().to_uint() \
              << " ,P2o = " << std::setw(2) << std::hex << top.core->port2o.read().to_uint() \
              << " ,P3o = " << std::setw(2) << std::hex << top.core->port3o.read().to_uint() \
              << std::endl                                                  \
              << "           :"                                             \
              << " ,P0i = " << std::setw(2) << std::hex << top.core->port0i.read().to_uint() \
              << " ,P1i = " << std::setw(2) << std::hex << top.core->port1i.read().to_uint() \
              << " ,P2i = " << std::setw(2) << std::hex << top.core->port2i.read().to_uint() \
              << " ,P3i = " << std::setw(2) << std::hex << top.core->port3i.read().to_uint() \
              << std::endl                                                  \
              << std::endl;

// DWORD WINAPI ShowWinThreadProc(LPVOID pParam);
// DWORD        ShowWinID = 0;

int sc_main(int argc, char *argv[]) {
    char *filename;
    bool  dolog;

    if (argc < 2) {
        printf("Usage:8051_iss filename imagename\n");
        return (1);
    }
    if (argc < 3) {
        printf("Usage:8051_iss filename imagename\n");
        return (1);
    }
    filename = argv[1];
    sc_clock clk("CLOCK", clockcycle, 0.5, 1);
    sc_clock slow_clk("SLOW_CLOCK", clockcycle, 0.5, 1);  // WA Added
    sc_signal<sc_uint<1> > reset;
    sc_signal<sc_uint<1> > poff;
    top                    top("top");
    top.clk(clk);
    top.slow_clk(slow_clk);  // WA Added
    top.reset(reset);
    top.poff(poff);

    if (!top.core->loadrom(filename)) {
        printf("%s file not found\n", filename);
        return (1);
    }
    filename = argv[2];
    {
        FILE *        FR;
        unsigned char s;
        int           i;
        FR = fopen(filename, "rb");
        if (FR == NULL) {
            printf("%s file not found\n", filename);
            return (1);
        }
        for (i = 0; i < 8 * 8; i++) {
            s                              = fgetc(FR);
            top.xrambig->mem[(0x3000) + i] = s;
        }
        fclose(FR);
    }

    ///////////////////////////////////Start Test
    int       t, k;
    int       score = 0;
    VARS_8051 olds, news;

    reset.write(1);
    for (t = 0; t < 10; t++) {
        sc_start(clockcycle, SC_NS);
    }
    reset.write(0);

    top.core->save_vars(&olds);
    dolog = false;

    bool first = false;
    for (; t < 30000; t++) {
        PRINT_PORT;

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

    //						//
    //    dump resoult		//
    //						//
    return (0);
}
