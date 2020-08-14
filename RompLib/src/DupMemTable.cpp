#include "DupMemTable.h"

#include <glog/logging.h>
#include <glog/raw_logging.h>

namespace romp {

void DupMemTable::removeNode(HashNodePtr node) {
  if (node->prev != nullptr) {
    node->prev->next = node->next;
  } else {
    _head = node->next;
  }
  if (node->next != nullptr) {
    node->next->prev = node->prev;
  } else {
    _tail = node->prev;
  }
}	

void DupMemTable::addNode(HashNodePtr node) {
  if (_tail != nullptr) {
    _tail->next = node;
  }
  node->prev = _tail;
  node->next = nullptr;
  _tail = node; 
  if (_head == nullptr) {
    _head = _tail;
  }
}

void DupMemTable::put(uint64_t key, bool value) {
  if (_table.find(key) != _table.end()) {
    auto node = _table[key];
    node->value = value;
    removeNode(node); 
    addNode(node);
  } else {
    if (_table.size() >= _capacity) {
      RAW_LOG(INFO, "table size larger than capacity, %d %d", 
		      _table.size(), _capacity);
      _table.erase(_head->key); // remove the least touched node
      removeNode(_head); 
    }
    auto node = std::make_shared<HashNode>(key, value);         
    RAW_LOG(INFO, "put(), starting to add node capacity: %d", _capacity);
    addNode(node);
    _table[key] = node;
  }
}

/*
 * Return false if there is no node associated with 'key'
 * Return true if there is node and put the node value in 'value'
 */
bool DupMemTable::get(uint64_t key, bool& value) {
  if (_table.find(key) == _table.end()) {
    return false; 
  }
  auto node = _table[key];
  value = node->value;
  removeNode(node);
  addNode(node);
  return true;
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

void DupMemTable::clear() {
  for (const auto& pair : _table) {
    auto node = pair.second;
    removeNode(node);
  }
  _tail = nullptr;
  _head = nullptr;
  _table.clear();
}

}
