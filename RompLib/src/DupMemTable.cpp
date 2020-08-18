#include "DupMemTable.h"

#include <climits>
#include <glog/logging.h>
#include <glog/raw_logging.h>

namespace romp {


void DupMemTable::clear() {
  DupElement empty;
  for (int i = 0; i < DUP_TABLE_CAPACITY; ++i) {
    _table[i] = empty;
  }	  
}

inline bool DupMemTable::isEmpty(int index) {
  // assume the integrity of the index
  return _table[index].addr == 0;
}

/*
 * Return true if a memory address that equals to addr is found.
 * Return false if it is not found. Set recIsWrite if the records shows 
 * that the access was a write.
 */
bool DupMemTable::get(uint64_t addr, bool& recIsWrite) {
  for (int i = 0; i < DUP_TABLE_CAPACITY; ++i) {
    auto elem = _table[i];
    if (elem.addr == addr) { // found
      recIsWrite = elem.isWrite;
      return true; 
    }
  }	  
  return false;
}

void DupMemTable::swing(int index) {
  if (index = 0) {
    return;
  }
  auto tmp = _table[index];
  for (int i = index - 1; i >= 0; i--) {
    _table[i + 1] = _table[i];
  }
  _table[0] = tmp;
}

inline bool DupMemTable::isFull() {
  return _elemNum == DUP_TABLE_CAPACITY;
}
/*
 * Record the memory address in the table. First, check if there is already 
 * an element having the same address. If not found, try finding  an empty slot 
 * If no empty slot is found, remove the slot with smallest counter number. 
 * If there is an empty slot, set the slot. And finally increment the counter.
 */
void DupMemTable::put(uint64_t addr, bool isWrite) {
  if (isFull()) {
   // we always replace once the table is full 
    for (int i = 0; i < DUP_TABLE_CAPACITY; ++i) {
      if (_table[i].addr == addr) {
        _table[i].isWrite = isWrite;
        swing(i); 
	return;
      }
    }
    // replace the victim
    DupElement element;
    element.addr = addr;
    element.isWrite = isWrite;
    _table[DUP_TABLE_CAPACITY - 1] = element; // swap out the victim
  } else {
    // scan to make sure there is not an existing element   
    auto firstEmptyIndex = -1;
    for (int i = 0; i < DUP_TABLE_CAPACITY; ++i) {
      if (isEmpty(i)) { 
        if (firstEmptyIndex == -1) {
          firstEmptyIndex = i;
	}
        continue;
      }
      if (_table[i].addr == addr) { // found the element
        _table[i].isWrite = isWrite;
	swing(i);
	return;
      }
    }
    // not found the element and has empty slot
    if (firstEmptyIndex < 0) {
      RAW_LOG(FATAL, "index is < 0");
    }
    DupElement element;
    element.addr = addr;
    element.isWrite = isWrite;
    _table[firstEmptyIndex] = element;
    swing(firstEmptyIndex);
    _elemNum++;
  }	  
}

/*
 * Return true if the memory access is considered as duplicated 
 * Return false if the memory access is considered as necessary for checking
 */
bool DupMemTable::isDupAccess(void* address, bool isWrite, uint64_t taskId) {
  auto addr = reinterpret_cast<uint64_t>(address);
  if (taskId != _taskId) { // task has mutated  
    clear();
    _taskId = taskId;  // update the task id 
    put(addr, isWrite); // put the new data 
    return false;
  }
  // same task id
  auto recIsWrite = false;
  if (get(addr, recIsWrite)) {
    // found the rec 
    put(addr, isWrite); // update the mem access
    if (!recIsWrite && isWrite) { // previous is a write
      return false; 
    } else {
      // prev: read, cur: read, or 
      // prev: write cur: write, or
      // prev: write cur: read
      // no need to recheck
      return true; 
    }
  } else {
    // no rec is found
    put(addr, isWrite);
    return false;         
  }
}


}
