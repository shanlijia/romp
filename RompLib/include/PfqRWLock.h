// * BeginRiceCopyright *****************************************************
//
// $HeadURL$
// $Id$
//
// --------------------------------------------------------------------------
// Part of HPCToolkit (hpctoolkit.org)
//
// Information about sources of support for research and development of
// HPCToolkit is at 'hpctoolkit.org' and in 'README.Acknowledgments'.
// --------------------------------------------------------------------------
//
// Copyright ((c)) 2002-2020, Rice University
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// * Redistributions of source code must retain the above copyright
//   notice, this list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright
//   notice, this list of conditions and the following disclaimer in the
//   documentation and/or other materials provided with the distribution.
//
// * Neither the name of Rice University (RICE) nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// This software is provided by RICE and contributors "as is" and any
// express or implied warranties, including, but not limited to, the
// implied warranties of merchantability and fitness for a particular
// purpose are disclaimed. In no event shall RICE or contributors be
// liable for any direct, indirect, incidental, special, exemplary, or
// consequential damages (including, but not limited to, procurement of
// substitute goods or services; loss of use, data, or profits; or
// business interruption) however caused and on any theory of liability,
// whether in contract, strict liability, or tort (including negligence
// or otherwise) arising in any way out of the use of this software, even
// if advised of the possibility of such damage.
//
// ******************************************************* EndRiceCopyright *

//******************************************************************************
//
// File:
//   $HeadURL$
//
// Purpose:
//   Define the API for a fair, phased reader-writer lock with local spinning
//
// Reference:
//   Björn B. Brandenburg and James H. Anderson. 2010. Spin-based reader-writer
//   synchronization for multiprocessor real-time systems. Real-Time Systems
//   46(1):25-87 (September 2010).  http://dx.doi.org/10.1007/s11241-010-9097-2
//
// Notes:
//   the reference uses a queue for arriving readers. on a cache coherent
//   machine, the local spinning property for waiting readers can be achieved
//   by simply using a cacheble flag. the implementation here uses that
//   simplification.
//
//******************************************************************************

#ifndef __pfq_rwlock_h__
#define __pfq_rwlock_h__



//******************************************************************************
// global includes
//******************************************************************************

#include <atomic>
#include <stdbool.h>



//******************************************************************************
// local includes
//******************************************************************************

#include "McsLock.h"


//******************************************************************************
// macros
//******************************************************************************

// align a variable at the start of a cache line
#define cache_aligned __attribute__((aligned(128)))



//******************************************************************************
// types
//******************************************************************************

typedef McsNode PfqRWLockNode;

typedef struct bigbool {
  std::atomic_bool bit cache_aligned = {false};
} bigbool;

typedef struct {
  //----------------------------------------------------------------------------
  // reader management
  //----------------------------------------------------------------------------
  std::atomic_int_least32_t  rin cache_aligned = {0};  // = 0
  std::atomic_int_least32_t rout cache_aligned = {0};  // = 0
  std::atomic_int_least32_t last cache_aligned = {0};  // = WRITER_PRESENT
  bigbool writer_blocking_readers[2]; // false

  //----------------------------------------------------------------------------
  // writer management
  //----------------------------------------------------------------------------
  McsLock wtail cache_aligned;  // init
  McsNode* whead cache_aligned;  // null

} PfqRWLock;

enum UpgradeResult {
  eAtomicUpgraded,
  eNonAtomicUpgraded,
};

//******************************************************************************
// interface operations
//******************************************************************************

void pfqRWLockInit(PfqRWLock *l);

void pfqRWLockReadLock(PfqRWLock *l);

void pfqRWLockReadUnlock(PfqRWLock *l);

void pfqRWLockWriteLock(PfqRWLock *l, PfqRWLockNode *me);

void pfqRWLockWriteUnlock(PfqRWLock *l, PfqRWLockNode *me);

UpgradeResult pfqUpgrade(PfqRWLock* l, PfqRWLockNode* me);
#endif
