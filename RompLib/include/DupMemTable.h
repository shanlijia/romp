#pragma once
#include <cstdint>

namespace romp {

/*
 * DupMemTable class is assocaited with each 
 * TaskData struct. It records memory addresses accessed by 
 * the same task in the same serial code segment. For example,
 * if an implicit task accesses a memory location X multiple 
 * times, these X accesses are serialized. 
 * The DupMemTable is implemented as an LRU cache. 
 * We use an array to store elements. We define the first element 
 * as the most recently accessed. When updating element, swap the 
 * element with the first element. We choose the last element as 
 * the victim.
 */
#define DUP_TABLE_CAPACITY 8


typedef struct DupElement {
  uint64_t addr;
  bool isWrite;
  DupElement() {
    addr = 0;
    isWrite = false;
  }
} DupElement;

class DupMemTable {
public:
  DupMemTable() { 
    _taskId = 0; 
    _elemNum = 0;
  }
  bool isDupAccess(void* address, bool isWrite, uint64_t taskId);
private:
  inline bool isFull();
  uint8_t _elemNum;
  void clear();
  void swing(int index);
  bool get(uint64_t addr, bool& recIsWrite);
  void put(uint64_t addr, bool isWrite);  
  inline void reduceCounter(int index);
  inline bool isEmpty(int index);
  DupElement _table[DUP_TABLE_CAPACITY]; 
  uint64_t _taskId;
};


}
