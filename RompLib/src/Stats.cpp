#include "Stats.h"
#include <glog/logging.h>
#include <glog/raw_logging.h>
#include <stdlib.h>

#include "papi_sde_interface.h"

namespace romp {

void* gNumRecOverflowCntr;

static const char * eventNames[1] = {
  "REC_NUM_OVERFLOW_COUNT"
};

void initPapiSde() {
  papi_handle_t sdeHandle;
  sdeHandle = papi_sde_init("romp");
  papi_sde_create_counter(sdeHandle, eventNames[0],
		  PAPI_SDE_DELTA, &gNumRecOverflowCntr); 
  LOG(INFO) << "papi sde events initialized";
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
		  PAPI_SDE_DELTA, &romp::gNumRecOverflowCntr);
  fptrStruct->describe_counter(sdeHandle, romp::eventNames[0],
        "Number of times the access history size is larger than threshold");
  return sdeHandle;
}
