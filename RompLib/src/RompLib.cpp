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
#include "Stats.h"
#include "TaskData.h"
#include "ThreadData.h"

#include "papi_sde_interface.h"

namespace fs = std::experimental::filesystem;

namespace romp {

using LabelPtr = std::shared_ptr<Label>;
using LockSetPtr = std::shared_ptr<LockSet>;

ShadowMemory<AccessHistory> shadowMemory;
extern void* sdeCounters[NUM_SDE_COUNTER];
extern std::atomic_long gNumCheckAccessCall;
extern std::atomic_long gNumContendedAccess;
extern std::atomic_long gNumModAccessHistory;
extern std::atomic_long gNumContendedMod;
extern std::atomic_long gNumAccessHistoryOverflow;
extern std::atomic_long gNumDupMemAccess;
extern std::unordered_map<void*, int> gAccessHistoryMap;
McsLock gMapLock;

/*
 * return true if atomic upgrade fails and needs to roll back
 */
bool upgradeHelper(bool& writeLockHeld, bool& readLockHeld, 
		  PfqRWLock* lock, bool& wwContended, 
		  PfqRWLockNode& me) {
  if (writeLockHeld) {
    return false;
  }
  wwContended = false;
  auto res = pfqUpgrade(lock, &me);
  writeLockHeld = true;
  readLockHeld = false;
  if (res == eAtomicUpgraded) {
    return false;
  } else {
    wwContended = true;
    return true;
  }
}


void recordContendedAccess(bool& contendAccessRecorded, AccessHistory* accessHistory) {
  if (!contendAccessRecorded) {
    gNumContendedAccess++;   
    accessHistory->numContendedAccess++;
    McsNode mapNode;
    mcsLock(&gMapLock, &mapNode);
    gAccessHistoryMap[(void*)accessHistory]++;  
    mcsUnlock(&gMapLock, &mapNode);
  }
  contendAccessRecorded = true;
}

void recordContendedMod(bool& contendModRecorded, AccessHistory* accessHistory) {
  if (!contendModRecorded) {
    gNumContendedMod++;
    accessHistory->numContendedMod++;
  }
  contendModRecorded = true;
}

void recordModEvent(bool& modRecorded, AccessHistory* accessHistory) {
  if (!modRecorded) {
    gNumModAccessHistory++;
    accessHistory->numMod++;
  }
  modRecorded = true;
}
/*
 * Driver function to do data race checking and access history management.
 */
void checkDataRace(AccessHistory* accessHistory, const LabelPtr& curLabel, 
                   const LockSetPtr& curLockSet, const CheckInfo& checkInfo) {
  gNumCheckAccessCall++;
  bool readWriteContend = false;
  bool writeWriteContend = false;   
  bool writeLockHeld = false;
  bool readLockHeld = false;
  bool contendAccessRecorded = false;
  bool contendModRecorded = false;
  bool modRecorded = false;
  auto lockPtr = &(accessHistory->getLock());   
  PfqRWLockNode me;
  if (pfqRWLockReadLock(lockPtr)) {
    readWriteContend = true;
  }
  readLockHeld = true;
  accessHistory->numAccess++; 
  // now we acquired the read lock
  int rollBackCnt = 0;
rollback_label:
  RAW_LOG(INFO, "check data race called %lx %d", accessHistory, rollBackCnt);
  if (readWriteContend || writeWriteContend) {
    recordContendedAccess(contendAccessRecorded, accessHistory);
  }
  auto dataSharingType = checkInfo.dataSharingType;
  if (dataSharingType == eThreadPrivateBelowExit || 
          dataSharingType == eStaticThreadPrivate) {
    RAW_LOG(INFO, "check data race called %lx 3", accessHistory);
    if (writeLockHeld) {
      RAW_LOG(INFO, "check data race called %lx 4", accessHistory);
      pfqRWLockWriteUnlock(lockPtr, &me);
    } else if (readLockHeld) {
      RAW_LOG(INFO, "check data race called %lx 5", accessHistory);
      pfqRWLockReadUnlock(lockPtr);
    }
    return;
  }
  RAW_LOG(INFO, "check data race called %lx 1", accessHistory);
  auto records = accessHistory->getRecords();
  if (records->size() > REC_NUM_THRESHOLD) {
    //papi_sde_inc_counter(sdeCounters[EVENT_REC_NUM_OVERFLOW], 1);     
    gNumAccessHistoryOverflow++;
  }
  if (accessHistory->dataRaceFound()) {
    RAW_LOG(INFO, "check data race called %lx 2", accessHistory);
    /* 
     * data race has already been found on this memory location, romp only 
     * reports one data race on any memory location in one run. Once the data 
     * race is reported, romp clears the access history with respect to this
     * memory location and mark this memory location as found. Future access 
     * to this memory location does not go through data race checking.
     */
    if (!records->empty()) {
      //papi_sde_inc_counter(sdeCounters[EVENT_MOD_NUM_COUNT], 1); 
      if (upgradeHelper(writeLockHeld, readLockHeld,
			      lockPtr, writeWriteContend, me)) {
        RAW_LOG(INFO, "check data race called %lx 2", accessHistory);
	goto rollback_label;
      } else {
        records->clear();
        recordModEvent(modRecorded, accessHistory);
        pfqRWLockWriteUnlock(lockPtr, &me);
	return;
      }
    }
    // direct return here
    if (writeLockHeld) {
      pfqRWLockWriteUnlock(lockPtr, &me);
    } else if (readLockHeld) {
      pfqRWLockReadUnlock(lockPtr);
    }
    return;
   }
   if (accessHistory->memIsRecycled()) {
    /*
     * The memory slot is recycled because of the end of explicit task. 
     * reset the memory state flag and clear the access records.
     */
     if (upgradeHelper(writeLockHeld, readLockHeld, lockPtr, writeWriteContend, me)) {
       goto rollback_label;
     } else {
       accessHistory->clearFlags();
       records->clear();
       recordModEvent(modRecorded, accessHistory);
      }
    } 
  /*
  if (isDupMemAccess(checkInfo)) {
    gNumDupMemAccess++;
    return;
  }
  */
   auto curRecord = Record(checkInfo.isWrite, curLabel, curLockSet, 
          checkInfo.taskPtr, checkInfo.instnAddr, checkInfo.hwLock);
   if (records->empty()) {
    // no access record, add current access to the record
     if (upgradeHelper(writeLockHeld, readLockHeld, lockPtr, writeWriteContend, me)) {
       goto rollback_label;
     } else {
       records->push_back(curRecord);
       recordModEvent(modRecorded, accessHistory);
     }
     if (readWriteContend || writeWriteContend) {
       recordContendedMod(contendModRecorded, accessHistory);
     }
   } else {
    // check previous access records with current access
    auto isHistBeforeCurrent = false;
    auto it = records->begin();
    std::vector<Record>::const_iterator cit;
    auto skipAddCur = false;
    int diffIndex;
    bool modFlag = false;
    while (it != records->end()) {
      cit = it; 
      auto histRecord = *cit;
      if (histRecord.getLabel() == nullptr) {
        RAW_LOG(WARNING, "hist record label is null, size: %d", records->size());
      }
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
      auto decision = manageAccessRecord(histRecord, curRecord, 
              isHistBeforeCurrent, diffIndex);
      if (decision == eSkipAddCur) {
        skipAddCur = true;
      }
      if (decision != eNoOp) {
      //  papi_sde_inc_counter(sdeCounters[EVENT_MOD_NUM_COUNT], 1); 
	modFlag = true;
      }
      if (decision == eNoOp || writeLockHeld) {
        // either just proceeds to next record or 
	// writer lock has already been held
        modifyAccessHistory(decision, records, it);
      } else {
        // acquire writer lock 
        if (upgradeHelper(writeLockHeld, readLockHeld, lockPtr, writeWriteContend, me)) {
          goto rollback_label;
        } else {
	  // acquired the lock
          modifyAccessHistory(decision, records, it);
        }
      }
    }
    if (!skipAddCur) {
      if (upgradeHelper(writeLockHeld, readLockHeld, lockPtr, writeWriteContend, me)) {
        goto rollback_label;
      } else {
        records->push_back(curRecord); 
	// don't record the mod here
      }
    }
    if (modFlag) {
      recordModEvent(modRecorded, accessHistory);
      if (readWriteContend || writeWriteContend) {
        recordContendedMod(contendModRecorded, accessHistory);
      }
    }
  }
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

