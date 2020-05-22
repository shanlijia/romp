#include "AccessHistory.h"

#include <glog/logging.h>
#include <glog/raw_logging.h>

namespace romp {

void AccessHistory::_initRecords() {
  _records = std::make_unique<std::vector<Record>>();
}

McsLock& AccessHistory::getLock() {
  return _lock;
}

/*
 * If records pointer has not been initialized, initialize the record first.
 * Then return the raw pointer to the records vector.
 * We assume the access history is under mutual exclusion.
 */
std::vector<Record>* AccessHistory::getRecords() {
  if (!_records.get()) {
    _initRecords();
  }
  return _records.get();
}

void AccessHistory::setFlag(AccessHistoryFlag flag) {
  _state |= flag;
}

void AccessHistory::clearFlag(AccessHistoryFlag flag) {
  _state &= ~flag; 
}

void AccessHistory::clearAll() {
  _state = 0; 
  _records->clear();
}

bool AccessHistory::dataRaceFound() const {
  return (_state & eDataRaceFound) == eDataRaceFound;
}

bool AccessHistory::memIsRecycled() const {
  return (_state & eMemoryRecycled) == eMemoryRecycled;
}

/*
 * Return access history state which is stored in the upper 32 bits of _state.
 */
AccessHistoryState AccessHistory::getState() const {
  return static_cast<AccessHistoryState>(_state >> ACCESS_HISTORY_SHIFT);
}

uint64_t AccessHistory::getRawState() const {
  return _state;
}
/*
 * Set access history state which is stored in the upper 32 bits of _state.
 */
void AccessHistory::setState(AccessHistoryState state) {
  _state &= ACCESS_HISTORY_MASK;
  _state |= static_cast<uint64_t>(state) << ACCESS_HISTORY_SHIFT;  
}

}
