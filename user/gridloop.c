#include "usys.h"

/* Long-running ring-3 demo for the preemptive scheduler.
   Counts up, printing progress periodically. With true preemption
   the counter survives timer preempts and the run eventually completes;
   with the old cooperative restart model it would never finish. */

void _start(void) {
    char buf[16];
    unsigned long count = 0;
    unsigned long target = 4000000ul;
    unsigned long next_print = 100000ul;

    sys_write("gridloop: starting long count to 4000000\n");

    while (count < target) {
        count++;
        if (count == next_print) {
            sys_write("gridloop: ");
            unsigned long v = count;
            char tmp[16];
            int n = 0;
            if (v == 0) {
                tmp[n++] = '0';
            } else {
                int t = 0;
                char rev[16];
                while (v > 0) {
                    rev[t++] = (char)('0' + (v % 10));
                    v /= 10;
                }
                while (t > 0) {
                    tmp[n++] = rev[--t];
                }
            }
            for (int i = 0; i < n && i < 15; ++i) {
                buf[i] = tmp[i];
            }
            buf[n] = '\0';
            sys_write(buf);
            sys_write("\n");
            next_print += 100000ul;
        }
    }

    sys_write("gridloop: complete\n");
    sys_exit(0);
}
