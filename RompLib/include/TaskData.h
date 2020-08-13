#pragma once
#include <atomic>
#include <memory>
#include <unordered_map>
#include <vector>

#include "DupMemTable.h"

namespace romp {
class Label;
class LockSet;

/*
 * TaskData struct records information related to a task.
 * A pointer to this struct is stored in openmp runtime 
 * data structure and could be retrieved through ompt query 
 * functions.
 */
typedef struct TaskData {
  std::shared_ptr<Label> label;
  std::shared_ptr<LockSet> lockSet;
  bool inReduction;
  std::vector<void*> childExpTaskData;
  std::atomic_uint64_t taskId; 
  std::unique_ptr<DupMemTable> dupTable; 
  void* exitFrame; 
  int expLocalId; // if the task is explicit, store its local id in par region
  bool isMutexTask;
  bool isExplicitTask; 
  TaskData() {
    label = nullptr;
    lockSet = nullptr;
    inReduction = false;
    exitFrame = nullptr;
    expLocalId = 0;
    isMutexTask = false;
    isExplicitTask = false;
    taskId = 0;
    dupTable = std::make_unique<DupMemTable>();
  }

} TaskData;

}
