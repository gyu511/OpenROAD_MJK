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
#include "stt/SteinerTreeBuilder.h"
#include "triton_route/TritonRoute.h"
#include "grt/GlobalRouter.h"

namespace mdm {
using namespace std;
void MultiDieManager::multiDieDPL()
{
  auto odp = new dpl::Opendp();
  odp->init(db_, logger_);
  odp->detailedPlacement(0, 0, "", false, db_->getChip()->getBlock());
  for (auto block : db_->getChip()->getBlock()->getChildren()) {
    odp->detailedPlacement(0, 0, "", false, block);
  }
}

void MultiDieManager::multiDieDPO()
{
  auto dpo = new dpo::Optdp;
  auto odp = new dpl::Opendp();
  odp->init(db_, logger_);
  dpo->init(db_, logger_, odp);
  dpo->improvePlacement(1, 0, 0, db_->getChip()->getBlock(), false);
  for (auto block : db_->getChip()->getBlock()->getChildren()) {
    dpo->improvePlacement(1, 0, 0, block, false);
  }
}

void MultiDieManager::multiDieDRT()
{
  // set the guide file
//  for (auto block : db_->getChip()->getBlock()->getChildren()) {
//    auto grt = new grt::GlobalRouter();
//        grt->init(db_, logger_);
//
//  }

  // Default values based on TCL file for drt
  const char* outputMazeFile = "";
  const char* outputDrcFile = "";
  std::optional<int> drcReportIterStepOpt;
  const char* outputCmapFile = "";
  const char* outputGuideCoverageFile = "";
  const char* dbProcessNode = "";
  bool enableViaGen = true;
  int drouteEndIter = -1;
  const char* viaInPinBottomLayer = "";
  const char* viaInPinTopLayer = "";
  int orSeed = -1;
  double orK = 0.0;
  const char* bottomRoutingLayer = "";
  const char* topRoutingLayer = "";
  int verbose = 1;
  bool cleanPatches = false;
  bool noPa = false;
  bool singleStepDR = false;
  int minAccessPoints = -1;
  bool saveGuideUpdates = false;
  const char* repairPDNLayerName = "";

  for (auto block : db_->getChip()->getBlock()->getChildren()) {
    auto router = new triton_route::TritonRoute();
    auto stt = new stt::SteinerTreeBuilder();
    stt->init(db_, logger_);
    router->init(nullptr, db_, logger_, nullptr, stt);
    router->setParams({outputMazeFile,
                       outputDrcFile,
                       drcReportIterStepOpt,
                       outputCmapFile,
                       outputGuideCoverageFile,
                       dbProcessNode,
                       enableViaGen,
                       drouteEndIter,
                       viaInPinBottomLayer,
                       viaInPinTopLayer,
                       orSeed,
                       orK,
                       bottomRoutingLayer,
                       topRoutingLayer,
                       verbose,
                       cleanPatches,
                       !noPa,
                       singleStepDR,
                       minAccessPoints,
                       saveGuideUpdates,
                       repairPDNLayerName});
    router->main(block);
    delete stt;
    delete router;
  }
}

void MultiDieManager::runSemiLegalizer(char* targetDie)
{
  SemiLegalizer semiLegalizer{};
  semiLegalizer.setDb(db_);
  semiLegalizer.run(true, targetDie);
}

}  // namespace mdm