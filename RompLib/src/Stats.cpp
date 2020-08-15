#include "Stats.h"

#include <atomic>
#include <glog/logging.h>
#include <glog/raw_logging.h>
#include <stdlib.h>
#include <unordered_map>

#include "AccessHistory.h"

namespace romp {

void* sdeCounters[NUM_SDE_COUNTER];

std::atomic_long gNumCheckFuncCall;
std::atomic_long gNumBytesChecked;
std::atomic_long gNumAccessHistoryOverflow;
std::atomic_long gNoModRWCon;
std::atomic_long gNoModRRCon;
std::atomic_long gNoModNoCon;
std::atomic_long gModRWConUS;
std::atomic_long gModRWConUF;
std::atomic_long gModRRConUS;
std::atomic_long gModRRConUF;
std::atomic_long gModNoConUS;
std::atomic_long gModNoConUF;

std::unordered_map<void*, int> gAccessHistoryMap;

__attribute__((destructor))
void finiStatsLog() {
  LOG(INFO) << "gNumCheckFuncCall#" << gNumCheckFuncCall.load();
  LOG(INFO) << "gNumBytesChecked#" << gNumBytesChecked.load();  
  LOG(INFO) << "gNumAccessHistoryOverflow#" << gNumAccessHistoryOverflow.load(); 
  LOG(INFO) << "gNoModRWCon#" << gNoModRWCon.load();
  LOG(INFO) << "gNoModRRCon#" << gNoModRRCon.load();
  LOG(INFO) << "gNoModNoCon#" << gNoModNoCon.load();
  LOG(INFO) << "gModRWConUS#" << gModRWConUS.load();
  LOG(INFO) << "gModRWConUF#" << gModRWConUF.load();
  LOG(INFO) << "gModRRConUS#" << gModRRConUS.load();
  LOG(INFO) << "gModRRConUF#" << gModRRConUF.load();
  LOG(INFO) << "gModNoConUS#" << gModNoConUS.load(); 
  LOG(INFO) << "gModNoConUF#" << gModNoConUF.load(); 
  
  for (const auto& data : gAccessHistoryMap) {
    auto history = static_cast<AccessHistory*>(data.first);
    auto noModRWCon = history->noModRWCon.load();
    auto noModRRCon = history->noModRRCon.load();
    auto noModNoCon = history->noModNoCon.load();
    auto modRWConUS = history->modRWConUS.load(); 
    auto modRWConUF = history->modRWConUF.load();
    auto modRRConUS = history->modRRConUS.load();
    auto modRRConUF = history->modRRConUF.load();
    auto modNoConUS = history->modNoConUS.load();
    auto modNoConUF = history->modNoConUF.load();      
    auto numAccess = history->numAccess.load();
    LOG(INFO) << "object-history#" << history << "#"
              << "noModRWCon#" << noModRWCon << "#"
	      << "noModRRCon#" << noModRRCon << "#"
              << "noModNoCon#" << noModNoCon << "#"
              << "modRWConUS#" << modRWConUS << "#"
              << "modRWConUF#" << modRWConUF << "#"
              << "modRRConUS#" << modRRConUS << "#"
              << "modRRConUF#" << modRRConUF << "#"
              << "modNoConUS#" << modNoConUS << "#"
              << "modNoConUF#" << modNoConUF << "#"
              << "numAccess#" << numAccess << "#";
  }
}	

}

/*
 * Hook for papi_native_avail utility.
 * Should only be called by papi_native_avail.
 */
papi_handle_t papi_sde_hook_list_events(papi_sde_fptr_struct_t* fptrStruct) { 
  papi_handle_t sdeHandle;
  sdeHandle = fptrStruct->init("romp");  
  fptrStruct->create_counter(sdeHandle, romp::eventNames[0],
		  PAPI_SDE_DELTA, &romp::sdeCounters[0]);
  fptrStruct->describe_counter(sdeHandle, romp::eventNames[0],
        "Number of times the access history size is larger than threshold");
  fptrStruct->create_counter(sdeHandle, romp::eventNames[1],
		  PAPI_SDE_DELTA, &romp::sdeCounters[1]);
  fptrStruct->describe_counter(sdeHandle, romp::eventNames[1],
        "Number of times access history is modified");
  return sdeHandle;
}
