#include "Stats.h"

#include <atomic>
#include <glog/logging.h>
#include <glog/raw_logging.h>
#include <stdlib.h>
#include <string>
#include <unordered_map>

#include "AccessHistory.h"

namespace romp {

std::atomic_long statCounters[COUNTER_NUM];

std::string counterNames[] = { 
	                "undef",
	                "no-mod-rw-con#", 
                        "no-mod-rr-con#",
                        "no-mod-no-con#",
                        "mod-rwcon-us-nocon#",
                        "mod-rwcon-us-wrcon#",
                        "mod-rwcon-uf-nocon#",
                        "mod-rwcon-uf-wrcon#",
                        "mod-rrcon-us-nocon#",
                        "mod-rrcon-us-wrcon#",
                        "mod-rrcon-uf-nocon#",
                        "mod-rrcon-uf-wrcon#",
                        "mod-nocon-us-nocon#",
                        "mod-nocon-us-wrcon#",
                        "mod-nocon-uf-nocon#",
                        "mod-nocon-uf-wrcon#",
                       };

std::atomic_long gNumCheckFuncCall;
std::atomic_long gNumBytesChecked;
std::atomic_long gNumAccessHistoryOverflow;

__attribute__((destructor))
void finiStatsLog() {
  auto checkFunCall = gNumCheckFuncCall.load();
  auto bytesChecked = gNumBytesChecked.load();
  auto historyOverflow = gNumAccessHistoryOverflow.load();
  LOG(INFO) << "check-func-call#" << checkFunCall;
  LOG(INFO) << "bytes-checked#" << bytesChecked;
  LOG(INFO) << "history-overflow#" << historyOverflow;
  for (int i = 1; i < COUNTER_NUM; ++i) {
    LOG(INFO) << counterNames[i] << statCounters[i].load();
  }
  auto checkRatio = (float)checkFunCall / (float)bytesChecked;
  auto noModCount = 0;
  for (int i = 1; i <=3; ++i) {
    noModCount += statCounters[i].load(); 
  }
  auto modRatio = (float)(checkFunCall - noModCount) / (float)(checkFunCall);
  LOG(INFO) << "check-ratio#" << checkRatio;
  LOG(INFO) << "mod-ratio#" << modRatio;
  auto readOnlyNoConRatio = (float)(statCounters[eNoModNoCon].load()) / checkFunCall;
  auto modNoConRatio = (float)(statCounters[eModNoConUSNoCon].load()) / checkFunCall;
  LOG(INFO) << "readonly-no-con-ratio#" << readOnlyNoConRatio;
  LOG(INFO) << "mod-no-con-ratio#" << modNoConRatio;
  
}	

}

