#include "consensus.h"
static int consensus_flag = 0;


void consensus_grant() { consensus_flag = 1; }
void consensus_clear() { consensus_flag = 0; }
int consensus_ready() { return consensus_flag; }
