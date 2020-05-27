#pragma once
namespace romp {

#define NUM_SDE_COUNTER 2

#define EVENT_REC_NUM_OVERFLOW 0
#define EVENT_MOD_NUM_COUNT 1	

#define REC_NUM_THRESHOLD 8
#define CONTENTION_THRESHOLD 2

void initPapiSde();
void finiStatsLog();

}
