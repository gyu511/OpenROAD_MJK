///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2018-2020, The Regents of the University of California
// All rights reserved.
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
// ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
///////////////////////////////////////////////////////////////////////////////

#include "mdm/MultiDieManager.hh"
namespace mdm {
using namespace std;
void MultiDieManager::twoDieDetailPlacement()
{
  vector<odb::dbDatabase*> targetDbSet;
  for (int i = 0; i < 2; ++i) {
    if (i == 0) {
      constructionDbForOneDie(TOP);
      detailPlacement();
      applyDetailPlacementResult();
      targetDb_ = nullptr;
    } else if (i == 1) {
      constructionDbForOneDie(BOTTOM);
      detailPlacement();
      applyDetailPlacementResult();
      targetDbSet.push_back(targetDb_);
      targetDb_ = nullptr;
    }
  }

  for (auto targetDb : targetDbSet) {
    odb::dbDatabase::destroy(targetDb);
  }
}

void MultiDieManager::constructionDbForOneDie(WHICH_DIE whichDie)
{
  int dieId = 0;
  if (whichDie == TOP) {
    dieId = 0;
  } else if (whichDie == BOTTOM) {
    dieId = 1;
  }

  vector<odb::dbInst*> instSetExclusive;
  for (auto chip : db_->getChips()) {
    auto block = chip->getBlock();
    for (auto inst : block->getInsts()) {
      auto partitionInfo = odb::dbIntProperty::find(inst, "whichDie");
      if (partitionInfo->getValue() != dieId) {
        instSetExclusive.push_back(inst);
      }
    }
  }
  targetDb_ = odb::dbDatabase::duplicate(db_);

  for (auto exclusiveInst : instSetExclusive) {
    auto inst = targetDb_->getChip()->getBlock()->findInst(
        exclusiveInst->getName().c_str());
    odb::dbInst::destroy(inst);
  }
}

void MultiDieManager::detailPlacement()
{
  auto* odp = new dpl::Opendp();
  odp->init(targetDb_, logger_);
  odp->detailedPlacement(0, 0);
}

void MultiDieManager::applyDetailPlacementResult()
{
  for (auto inst : targetDb_->getChip()->getBlock()->getInsts()) {
    auto originalInst
        = db_->getChip()->getBlock()->findInst(inst->getName().c_str());
    auto originalLocation = inst->getLocation();
    originalInst->setLocation(originalLocation.getX(), originalLocation.getY());
  }
}
}  // namespace mdm