#pragma once
namespace romp {

#define COUNTER_NUM 16
#define REC_NUM_THRESHOLD 8
#define CONTENTION_THRESHOLD 2

void finiStatsLog();

enum CounterType {
  eUndefCounter = 0,

  eNoModRWCon = 1,
  eNoModRRCon = 2,
  eNoModNoCon= 3,

  eModRWConUSNoCon = 4,
  eModRWConUSWRCon = 5,
  eModRWConUFNoCon = 6,
  eModRWConUFWRCon = 7,

  eModRRConUSNoCon = 8,
  eModRRConUSWRCon = 9,
  eModRRConUFNoCon = 10,
  eModRRConUFWRCon = 11,
 
  eModNoConUSNoCon = 12,
  eModNoConUSWRCon = 13,
  eModNoConUFNoCon = 14,
  eModNoConUFWRCon = 15,

};

}
