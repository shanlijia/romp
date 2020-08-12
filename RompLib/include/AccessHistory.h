#pragma once
#include <cstdint>
#include <memory>
#include <vector>

#include "PfqRWLock.h"
#include "Record.h"

namespace romp {

#define ACCESS_HISTORY_MASK 0x00000000ffffffff
#define ACCESS_HISTORY_SHIFT 32

enum AccessHistoryFlag {
  eDataRaceFound = 0x1,
  eMemoryRecycled = 0x2,
};

enum AccessHistoryState {
  eEmpty = 0x0,
  eSingleRead = 0x1,
  eSingleWrite = 0x2,
  eSiblingRR = 0x3,
  eSiblingRW = 0x4,
  eSiblingWW = 0x5,
  eNonSiblingRR = 0x6,
  eNonSiblingRW = 0x7,
  eNonSiblingWW = 0x8,
  eMultiRec = 0x9,
  eUndefinedState = 0xa,
  eAncestorChildWR = 0xb,
  eParentChildWR = 0xc,  
  eWRR= 0xd,
};

class AccessHistory {

public: 
  AccessHistory() : _state(0) { pfqRWLockInit(&_lock); }
  PfqRWLock& getLock();
  std::vector<Record>* getRecords();
  std::vector<Record>* peekRecords();   
  void setFlag(AccessHistoryFlag flag);
  void clearFlag(AccessHistoryFlag flag);
  void clearFlags();
  void clearAll();
  bool dataRaceFound() const;
  bool memIsRecycled() const;
  AccessHistoryState getState() const;   
  void setState(AccessHistoryState state);
  uint64_t getRawState() const; 
private:
  void _initRecords();
private:
  PfqRWLock _lock;
  uint64_t _state;  
  std::unique_ptr<std::vector<Record>> _records; 

};

}
