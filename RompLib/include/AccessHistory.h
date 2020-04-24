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
  eSiblingReadRead = 0x3,
  eSiblingReadWrite = 0x4,
  eSiblingWriteWrite = 0x5,
  eNonSiblingReadRead = 0x6,
  eNonSiblingReadWrite = 0x7,
  eNonSiblingWriteWrite = 0x8,
  eMultiRec = 0x9,
};

enum RecordManageAction {
  eNoOp,
  eSkipAddCur,
  eDelHist,
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
