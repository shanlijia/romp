#include <experimental/filesystem>
#include <glog/logging.h>
#include <glog/raw_logging.h>
#include <limits.h>
#include <unistd.h>

#include "AccessHistory.h"
#include "Core.h"
#include "CoreUtil.h"
#include "DataSharing.h"
#include "Initialize.h"
#include "Label.h"
#include "LockSet.h"
#include "PfqRWLock.h"
#include "ShadowMemory.h"
#include "TaskData.h"
#include "ThreadData.h"

namespace fs = std::experimental::filesystem;

namespace romp {

using LabelPtr = std::shared_ptr<Label>;
using LockSetPtr = std::shared_ptr<LockSet>;

ShadowMemory<AccessHistory> shadowMemory;
/*
 *  This helper function is called when checkAccess() determines that 
 *  there exists intent to modify access history. It return true if 
 *  the reader lock cannot be directly upgraded to writer lock without 
 *  rolling back the traversal of access records. It return false if 
 *  the upgrade from reader lock to writer lock is a success and no 
 *  rolling back is required.
 */
bool upgradeHelper(bool& writeLockHeld, bool& readLockHeld, 
		   PfqRWLock* lock, PfqRWLockNode& me) {
  if (writeLockHeld) {
    // the writer lock is already held
    return false;
  }
  auto res = pfqUpgrade(lock, &me);
  writeLockHeld = true;
  readLockHeld = false;
  if (res == eAtomicUpgraded) {
    return false; 
  } else {
    return true;
  }
}


/*
 * Driver function to do data race checking and access history management.
 * We use reader-writer lock to enforce synchronization of shadow memory 
 * accesses.
 */
void checkDataRace(AccessHistory* accessHistory, const LabelPtr& curLabel, 
                   const LockSetPtr& curLockSet, const CheckInfo& checkInfo) {
  auto writeLockHeld = false;
  auto readLockHeld = false;  
  auto lockPtr = &(accessHistory->getLock());  
  PfqRWLockNode me;
  pfqRWLockReadLock(lockPtr); // acquire reader lock
  readLockHeld = true;

  auto curRecord = Record(checkInfo.isWrite, curLabel, curLockSet, 
          checkInfo.taskPtr, checkInfo.instnAddr, checkInfo.hwLock);
rollback:
  auto records = accessHistory->peekRecords();
  auto dataSharingType = checkInfo.dataSharingType;

  if (dataSharingType == eThreadPrivateBelowExit || 
          dataSharingType == eStaticThreadPrivate) {
    goto check_finish; 
  }
  if (records == nullptr) {
    // need to write 
    if (upgradeHelper(writeLockHeld, readLockHeld, lockPtr, me)) {
      goto rollback;
    } else {
      records = accessHistory->getRecords();
    }
  }
  if (accessHistory->dataRaceFound()) {
    /* 
     * data race has already been found on this memory location, romp only 
     * reports one data race on any memory location in one run. Once the data 
     * race is reported, romp clears the access history with respect to this
     * memory location and mark this memory location as found. Future access 
     * to this memory location does not go through data race checking.
     */
    if (!records->empty()) {  
      if (upgradeHelper(writeLockHeld, readLockHeld, lockPtr, me)) {
        goto rollback;
      }  else {
        records->clear();
      }
    }
    goto check_finish;   
  }
  if (accessHistory->memIsRecycled()) {
    /*
     * The memory slot is recycled because of the end of explicit task. 
     * reset the memory state flag and clear the access records.
     */
    if (upgradeHelper(writeLockHeld, readLockHeld, lockPtr, me)) {
      goto rollback;
    }
     accessHistory->clearFlags();
     records->clear();
  }
  if (records->empty()) {
    if (upgradeHelper(writeLockHeld, readLockHeld, lockPtr, me)) {
      goto rollback;
    }
    if (checkInfo.isWrite) {
      accessHistory->setState(eSingleWrite);
    } else {
      accessHistory->setState(eSingleRead);
    }
    records->push_back(curRecord); 
  } else {
    // check previous access records with current access
    auto isHistBeforeCurrent = false;
    int diffIndex;
    auto it = records->begin();
    std::vector<Record>::const_iterator cit;
    while (it != records->end()) {
      cit = it; 
      auto histRecord = *cit;
      auto histLabel = histRecord.getLabel();   
      isHistBeforeCurrent = happensBefore(histLabel, curLabel.get(), diffIndex);
      if (analyzeRaceCondition(histRecord, curRecord, isHistBeforeCurrent, 
                  diffIndex)) {
        gDataRaceFound = true;
        gNumDataRace++;
        if (gReportLineInfo) {
          McsNode node;	
          LockGuard recordGuard(&gDataRaceLock, &node);
          gDataRaceRecords.push_back(DataRaceInfo(histRecord.getInstnAddr(),
                                                  curRecord.getInstnAddr(),
                                                  checkInfo.byteAddress));
        } else if (gReportAtRuntime) {
          reportDataRace(histRecord.getInstnAddr(), curRecord.getInstnAddr(),
                         checkInfo.byteAddress);
        }
        accessHistory->setFlag(eDataRaceFound);  
	break;
      }
      auto [nextState, action] = manageAccessRecord(accessHistory, histRecord, curRecord, 
		                       isHistBeforeCurrent, diffIndex); 
      if (writeLockHeld) {
        // either the writer lock is already held, or the action is none
	accessHistory->setState(nextState);
        modifyAccessHistory(action, records, it, curRecord); 
      } else {
        if (upgradeHelper(writeLockHeld, readLockHeld, lockPtr, me)) {
          // have to go back to roll back state
          goto rollback;
	} else {
	  accessHistory->setState(nextState);
          modifyAccessHistory(action, records, it, curRecord);
	}
      }   
    }
  }
check_finish:
  if (writeLockHeld) {
    pfqRWLockWriteUnlock(lockPtr, &me);
  } else if (readLockHeld) {
    pfqRWLockReadUnlock(lockPtr); 
  }
}

extern "C" {

/** 
 * implement ompt_start_tool which is defined in OpenMP spec 5.0
 */
ompt_start_tool_result_t* ompt_start_tool(
        unsigned int ompVersion,
        const char* runtimeVersion) {
  ompt_data_t data;
  static ompt_start_tool_result_t startToolResult = { 
      &omptInitialize, &omptFinalize, data}; 
  char result[PATH_MAX];
  auto count = readlink("/proc/self/exe", result, PATH_MAX);
  if (count == 0) {
    LOG(FATAL) << "cannot get current executable path";
  }
  auto appPath = std::string(result, count);
  LOG(INFO) << "ompt_start_tool on executable: " << appPath;
  auto success = Dyninst::SymtabAPI::Symtab::openFile(gSymtabHandle, appPath);
  if (!success) {
    LOG(FATAL) << "cannot parse executable into symtab: " << appPath;
  }
  return &startToolResult;
}

void checkAccess(void* address,
                 uint32_t bytesAccessed,
                 void* instnAddr,
                 bool hwLock,
                 bool isWrite) {
  /*
  RAW_LOG(INFO, "address:%lx bytesAccessed:%u instnAddr: %lx hwLock: %u,"
                "isWrite: %u", address, bytesAccessed, instnAddr, 
                 hwLock, isWrite);
                 */
  if (!gOmptInitialized) {
    //RAW_LOG(INFO, "ompt not initialized yet");
    return;
  }
  AllTaskInfo allTaskInfo;
  int threadNum = -1;
  int taskType = -1;
  int teamSize = -1;
  void* curThreadData = nullptr;
  void* curParRegionData = nullptr;
  if (!prepareAllInfo(taskType, teamSize, threadNum, curParRegionData, 
              curThreadData, allTaskInfo)) {
    return;
  }
  if (taskType == ompt_task_initial) { 
    // don't check data race for initial task
    return;
  }
  // query data  
  auto dataSharingType = analyzeDataSharing(curThreadData, address, 
                                           allTaskInfo.taskFrame);
  if (!allTaskInfo.taskData->ptr) {
    RAW_LOG(WARNING, "pointer to current task data is null");
    return;
  }

  auto curTaskData = static_cast<TaskData*>(allTaskInfo.taskData->ptr);
  if (isDupMemAccess(curTaskData, isWrite, address)) {
    return;
  }
  curTaskData->exitFrame = allTaskInfo.taskFrame->exit_frame.ptr;
  auto& curLabel = curTaskData->label;
  auto& curLockSet = curTaskData->lockSet;
  
  CheckInfo checkInfo(allTaskInfo, bytesAccessed, instnAddr, 
          static_cast<void*>(curTaskData), taskType, isWrite, hwLock, 
          dataSharingType);
  for (uint64_t i = 0; i < bytesAccessed; ++i) {
    auto curAddress = reinterpret_cast<uint64_t>(address) + i;      
    auto accessHistory = shadowMemory.getShadowMemorySlot(curAddress);
    checkInfo.byteAddress = curAddress;
    checkDataRace(accessHistory, curLabel, curLockSet, checkInfo);
  }
}

}

}

