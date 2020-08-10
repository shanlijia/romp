#pragma once
#include <memory>
#include <unordered_map>

namespace romp {

/*
 * DupMemTable class is assocaited with each 
 * TaskData struct. It records memory addresses accessed by 
 * the same task in the same serial code segment. For example,
 * if an implicit task accesses a memory location X multiple 
 * times, these X accesses are serialized. 
 * The DupMemTable is implemented as an LRU cache.
 */

typedef struct HashNode {
  std::shared_ptr<HashNode> prev;
  std::shared_ptr<HashNode> next;
  uint64_t key;
  bool value;  
  HashNode(uint64_t key, bool value) {
    prev = nullptr;  
    next = nullptr;
    key = key;
    value = value;
  }	  
} HashNode;

using HashNodePtr = std::shared_ptr<HashNode>;

class DupMemTable {
public:
  DupMemTable(uint32_t capacity) { 
    _taskId = 0; 
    _capacity = capacity;
    _head = nullptr; 
    _tail = nullptr;
  }
  DupMemTable() { 
    _taskId = 0; 
    _capacity = 32; // set default memory 
    _head = nullptr;
    _tail = nullptr; 
  }
  bool isDupAccess(void* address, bool isWrite, uint64_t taskId);
  void clear(); 
private:
  void put(uint64_t key, bool value);
  bool get(uint64_t key, bool& value);
  void removeNode(HashNodePtr node);
  void addNode(HashNodePtr node);
private:
  uint32_t  _capacity;
  std::unordered_map<uint64_t, HashNodePtr> _table;    
  uint64_t _taskId;
  HashNodePtr _head;
  HashNodePtr _tail;
};


}
