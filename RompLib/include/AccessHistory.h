#pragma once
#include <atomic>
#include <cstdint>
#include <memory>
#include <vector>

#include "McsLock.h"
#include "Record.h"

namespace romp {

enum AccessHistoryFlag {
  eDataRaceFound = 0x1,
  eMemoryRecycled = 0x2,
};

class AccessHistory {

public: 
  AccessHistory() : _state(0) { mcsInit(&_lock); }
  McsLock& getLock();
  std::vector<Record>* getRecords();
  void setFlag(AccessHistoryFlag flag);
  void clearFlags();
  void clearFlag(AccessHistoryFlag flag);
  bool dataRaceFound() const;
  bool memIsRecycled() const;
  uint64_t getState() const;
  std::atomic_uint64_t numContention;
  std::atomic_uint64_t numAccess;
  std::atomic_uint64_t numMod;
private:
  void _initRecords();
private:
  McsLock _lock; 
  uint64_t _state;  
  std::unique_ptr<std::vector<Record>> _records; 
  
};

class LockGuard {
public:
  LockGuard(McsLock* lock, McsNode* node): _lock(lock), _node(node) {
    mcsLock(_lock, _node);
  }

  LockGuard(AccessHistory* history, McsNode* node) {
    _lock = &(history->getLock());
    _node = node;
    if (!mcsTryLock(_lock, _node)) {
      history->numContention++; // if we experienced a contention, record it
      mcsLock(_lock, _node);
    }
  }

  ~LockGuard() {
    mcsUnlock(_lock, _node);
  }
private:
  McsLock* _lock;
  McsNode* _node;
};

}
