/////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2019, The Regents of the University of California
// All rights reserved.
//
// BSD 3-Clause License
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
///////////////////////////////////////////////////////////////////////////////

#ifndef OPENROAD_SRC_MDM_INCLUDE_MDM_MULTIDIEMANAGER_H_
#define OPENROAD_SRC_MDM_INCLUDE_MDM_MULTIDIEMANAGER_H_
#include <tcl.h>

#include "../../../par/include/par/PartitionMgr.h"
#include "dpl/Opendp.h"
#include "gpl/Replace.h"
#include "odb/db.h"
#include "par/PartitionMgr.h"

namespace mdm {
class MultiDieManager
{
 public:
  MultiDieManager();
  ~MultiDieManager();

  void init(odb::dbDatabase* db,
            utl::Logger* logger,
            par::PartitionMgr* partition_mgr,
            gpl::Replace* replace,
            dpl::Opendp* opendp);

  void setParam1(double param1);
  void setFlag1(bool flag1);
  void getFlag1();

 private:
  odb::dbDatabase* db_;
  utl::Logger* logger_;
  par::PartitionMgr* partition_mgr_;
  gpl::Replace* replace_;
  dpl::Opendp* opendp_;


  double param1_;
  bool flag1_;
};
}  // namespace mdm

#endif  // OPENROAD_SRC_MDM_INCLUDE_MDM_MULTIDIEMANAGER_H_