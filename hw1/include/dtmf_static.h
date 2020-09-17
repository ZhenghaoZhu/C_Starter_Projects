/*
 * DO NOT MODIFY THE CONTENTS OF THIS FILE.
 * IT WILL BE REPLACED DURING GRADING
 */
#ifndef DTMF_STATIC_H
#define DTMF_STATIC_H

#include "dtmf.h"

/*
 * Table of DTMF frequencies, in Hz.
 */
int dtmf_freqs[NUM_DTMF_FREQS] = { 697, 770, 852, 941, 1209, 1336, 1477, 1633 };

/*
 * Table mapping (row freq, col freq) pairs to DTMF symbol names.
 */
uint8_t dtmf_symbol_names[NUM_DTMF_ROW_FREQS][NUM_DTMF_COL_FREQS] = {
    {'1', '2', '3', 'A'}, // 0 4 | 0 5 | 0 6 | 0 7
    {'4', '5', '6', 'B'}, // 1 4 | 1 5 | 1 6 | 1 7
    {'7', '8', '9', 'C'}, // 2 4 | 2 5 | 2 7 | 2 7
    {'*', '0', '#', 'D'}  // 3 4 | 3 5 | 3 7 | 3 7
};

#endif
