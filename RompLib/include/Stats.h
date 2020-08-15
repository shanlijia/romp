#pragma once
namespace romp {

#define REC_NUM_THRESHOLD 8
#define CONTENTION_THRESHOLD 2

void finiStatsLog();

enum CounterType {
  eNoModRWCon = 1,
  eNoModRRCon = 2,
  eNoModNoCon= 3,
  eModRWConUS = 4,
  eModRWConUF = 5,
  eModRRConUS = 6,//has mod intention,reader-reader contention, upgrade success
  eModRRConUF = 7,
  eModNoConUS = 8,
  eModNoConUF = 9,      
  eUndefCounter = 10,
};

}
