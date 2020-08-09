#pragma once
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

typedef HashNode {
  HashNode* prev;
  HashNode* next;
  uint64_t key;
  bool value;  
  HashNode(uint64_t key, bool value) {
    prev = nullptr;  
    next = nullptr;
    key = key;
    value = value;
  }	  
} HashNode;

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
    _capacity = 32;
    _head = nullptr;
    _tail = nullptr; 
  }
  bool isDupAccess(void* address, bool isWrite, uint64_t taskId);
  void clear(); 
private:
  void put(uint64_t key, bool value);
  bool get(uint64_t key, bool& value);
  void removeNode(HashNode* node);
  void addNode(HashNode* node);
  void deleteNode(HashNode* node);
private:
  uint32_t  _capacity;
  std::unordered_map<uint64_t, HashNode*> _table;    
  uint64_t _taskId;
  HashNode* _head;
  HashNode* _tail;
};


}
