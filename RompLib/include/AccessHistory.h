#pragma once
#include <atomic>
#include <cstdint>
#include <memory>
#include <vector>

#include "PfqRWLock.h"
#include "Record.h"

namespace romp {

enum AccessHistoryFlag {
  eDataRaceFound = 0x1,
  eMemoryRecycled = 0x2,
};

class AccessHistory {

public: 
  AccessHistory() : _state(0) { pfqRWLockInit(&_lock); }
  PfqRWLock& getLock();
  std::vector<Record>* getRecords();
  void setFlag(AccessHistoryFlag flag);
  void clearFlags();
  void clearFlag(AccessHistoryFlag flag);
  bool dataRaceFound() const;
  bool memIsRecycled() const;
  uint64_t getState() const;
  std::atomic_uint64_t numAccess;
  std::atomic_uint64_t numContendedAccess;
  std::atomic_uint64_t numMod;     
  std::atomic_uint64_t numContendedMod;
private:
  void _initRecords();
private:
  PfqRWLock _lock; 
  uint64_t _state;  
  std::unique_ptr<std::vector<Record>> _records; 
  
};

class LockGuard {
public:
  LockGuard(McsLock* lock, McsNode* node): _lock(lock), _node(node) {
    mcsLock(_lock, _node);
  }

  ~LockGuard() {
    mcsUnlock(_lock, _node);
  }
private:
  McsLock* _lock;
  McsNode* _node;
};

}
