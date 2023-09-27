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

#include <iostream>
#include <string>
#include <vector>

#include "HybridBond.h"
#include "dpl/Opendp.h"
#include "gpl/Replace.h"
#include "odb/db.h"
#include "par/PartitionMgr.h"

namespace mdm {
class MultiDieManager;
class HybridBond;
class HybridBondInfo;

class SwitchInstanceHelper
{
 public:
  static int findAssignedDieId(odb::dbInst* inst);
  static std::pair<odb::dbBlock*, odb::dbLib*> findTargetDieAndLib(
      MultiDieManager* manager,
      int dieID);
  static void inheritPlacementInfo(odb::dbInst* originalInst,
                                   odb::dbInst* newInst);
  static void inheritNetInfo(odb::dbInst* originalInst, odb::dbInst* newInst);
  static void inheritProperties(odb::dbInst* originalInst,
                                odb::dbInst* newInst);
  static void inheritGroupInfo(odb::dbInst* originalInst, odb::dbInst* newInst);
};

class MultiDieManager
{
  friend class SwitchInstanceHelper;

 public:
  MultiDieManager();
  ~MultiDieManager();

  void init(odb::dbDatabase* db,
            utl::Logger* logger,
            par::PartitionMgr* partition_mgr,
            gpl::Replace* replace,
            dpl::Opendp* opendp);

  void set3DIC(int number_of_die,
               uint hybrid_bond_x = 10,
               uint hybrid_bond_y = 10,
               uint hybrid_bond_space_x = 10,
               uint hybrid_bond_space_y = 10,
               float area_ratio = 0.5);
  /**
   * \brief
   * Make shrunk lef files.
   * If the area_ratio is 0.5,
   * then the first lef ratio is 1.0, the second lef ratio is 0.5,
   * the third lef ratio is 0.25 .. \n
   * This affects only library information.
   * This will not affect tech information, because the multi several tech is
   * not supported completely in odb.
   * */
  void makeShrunkLefs();

  void twoDieDetailPlacement();

 private:
  odb::dbTech* makeNewTech(const std::string& tech_name);
  void makeShrunkLib(const std::string& which_die,
                     double shrunk_ratio,
                     odb::dbTech* tech__);

  /**
   * \brief
   * Partition instances into different die.
   * This will consider the size of technologies and the utilization of each
   * die.
   *
   * 1. Read partition information from file.
   * 2. Make sub-blocks as much as partition IDs.
   * 3. Put the instances into the sub-blocks.
   * */
  void splitInstances();

  /**
   * This function is responsible for creating sub blocks (or dies) in a
   * multi-die design. For each technology in the design database, a new die is
   * created with a unique name. The first technology, which is the one parsed
   * at the TCL level, is skipped.
   */
  void makeSubBlocks();

  /**
   * Return the n_th lib
   * */
  odb::dbLib* findLibByPartitionInfo(int value);

  /**
   * Reading function for partitioning information in `par` is not work in
   * proper way. So make this function as temporal solution.
   * */
  void readPartitionInfo(std::string file_name);

  /**
   * Below functions are for detail placement.
   * */
  enum WHICH_DIE
  {
    TOP,
    BOTTOM
  };
  void constructionDbForOneDie(WHICH_DIE which_die);
  void detailPlacement();
  void applyDetailPlacementResult();
  void switchInstanceToAssignedDie(odb::dbInst* originalInst);

  odb::dbDatabase* db_{};
  utl::Logger* logger_{};
  par::PartitionMgr* partition_mgr_{};
  gpl::Replace* replace_{};
  dpl::Opendp* opendp_{};

  int number_of_die_{};
  float shrink_area_ratio{};
  std::vector<float> shrink_length_ratios_;

  // Hybrid Bond information --> This would be absorbed in odb later.
  std::vector<HybridBond> hybridbond_set_;
  HybridBondInfo hybrid_bond_info_{};

  // temporal variables
  odb::dbDatabase* target_db_{};
};

}  // namespace mdm

#endif  // OPENROAD_SRC_MDM_INCLUDE_MDM_MULTIDIEMANAGER_H_
