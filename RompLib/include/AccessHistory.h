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
/*
 * We apply reader lock guard.
 */
public:
  LockGuard(PfqRWLock* lock): _lock(lock) {
    pfqRWLockReadLock(lock);
  }

  LockGuard(AccessHistory* history, bool& readWriteContend, bool rolledBack) {
    _lock = &(history->getLock());
    _rolledBack = rolledBack;
    readWriteContend = false;
    if (!rolledBack) {
      if (pfqRWLockReadLock(_lock)) {
        readWriteContend = true;  
      }
    } e
      
    }
  }

  ~LockGuard() {
    pfqRWLockReadUnlock(_lock);
  }

private:
  PfqRWLock* _lock;
  bool _rolledBack;

};

}
