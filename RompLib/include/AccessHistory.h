#pragma once
#include <cstdint>
#include <memory>
#include <vector>

#include "McsLock.h"
#include "Record.h"

namespace romp {

#define ACCESS_HISTORY_MASK 0x00000000ffffffff
#define ACCESS_HISTORY_SHIFT 32

enum AccessHistoryFlag {
  eDataRaceFound = 0x1,
  eMemoryRecycled = 0x2,
};

/*
 * States for access history management
 */
enum AccessHistoryState {
  eEmpty = 0x0, 
  eSingleRead = 0x1,
  eSingleWrite = 0x2,
  eSiblingRR = 0x3,
  eSiblingRW = 0x4,
  eSiblingWW = 0x5,
  eNonSiblingRR = 0x6,
  eNonSiblingRW = 0x8,
  eNonSiblingWW = 0x8,
  eMultiRec = 0x9, // current not reachable
  eUndefinedState = 0xa,
  eAncestorChildWR = 0xb,
  eParentChildWR = 0xc,
  eWRR = 0xd, // W->R1, W->R2
};

enum RecordManageAction {
  eNoAction,
  eDelHistAddCur,
  eAddCur,
  eDelHist,
  eErrorAction,
  eDelAllAddCur,
  eDelOtherAddCur,
};

class AccessHistory {

public: 
  AccessHistory() : _state(0) { mcsInit(&_lock); }
  McsLock& getLock();
  std::vector<Record>* getRecords();
  void setFlag(AccessHistoryFlag flag);
  void clearFlag(AccessHistoryFlag flag);
  void clearAll();    
  bool dataRaceFound() const;
  bool memIsRecycled() const;
  AccessHistoryState getState() const;
  void setState(AccessHistoryState state);    
private:
  void _initRecords();
private:
  McsLock _lock; 
  uint64_t _state;  
  std::unique_ptr<std::vector<Record>> _records; 

};

}
