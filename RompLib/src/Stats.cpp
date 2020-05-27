#include "Stats.h"

#include <atomic>
#include <glog/logging.h>
#include <glog/raw_logging.h>
#include <stdlib.h>
#include <unordered_map>

#include <papi_sde_interface.h>

#include "AccessHistory.h"

namespace romp {

void* sdeCounters[NUM_SDE_COUNTER];
std::atomic_long gNumCheckAccessCall;
std::atomic_long gNumModAccessHistory;
std::atomic_long gNumAccessHistoryOverflow;
std::atomic_long gNumDupMemAccess;
std::unordered_map<void*, int> gAccessHistoryMap;

static const char * eventNames[NUM_SDE_COUNTER] = {
  "REC_NUM_CNT",
  "MOD_NUM_CNT",
};

__attribute__((constructor))
void initPapiSde() {
  papi_handle_t sdeHandle;
  sdeHandle = papi_sde_init("romp");
  for (int i = 0; i < NUM_SDE_COUNTER; ++i) {
    papi_sde_create_counter(sdeHandle, eventNames[i], PAPI_SDE_DELTA, 
		    &sdeCounters[i]);
  }
  LOG(INFO) << "papi sde events initialized";
}

__attribute__((destructor))
void finiStatsLog() {
  LOG(INFO) << "num check access called: " << gNumCheckAccessCall.load();
  LOG(INFO) << "access history modification: " << gNumModAccessHistory.load();
  LOG(INFO) << "access history threshold: " << REC_NUM_THRESHOLD;
  LOG(INFO) << "access history overflow: " << gNumAccessHistoryOverflow.load();
  LOG(INFO) << "num dup memory access: " << gNumDupMemAccess.load();
  for (const auto& data : gAccessHistoryMap) {
    auto history = static_cast<AccessHistory*>(data.first);
    LOG(INFO) << "contention on history# " << history
	      << " contention time# " << history->numContention.load()
	      << " num access# " << history->numAccess.load() 
	      << " num mod# " << history->numMod.load();
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
