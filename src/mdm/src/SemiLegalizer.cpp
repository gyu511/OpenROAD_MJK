///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2023, The Regents of the University of California
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

#include "SemiLegalizer.h"
namespace mdm {

void SemiLegalizer::doSemiLegalize()
{
  doSemiLegalize(db_->getChip()->getBlock());
  for (auto block : db_->getChip()->getBlock()->getChildren()) {
    doSemiLegalize(block);
  }
}
void SemiLegalizer::doSemiLegalize(odb::dbBlock* block)
{
  targetBlock_ = block;
  if (!utilCheck()) {
    return;
  }
  adjustRowCapacity();
  shiftLegalize();
}
void SemiLegalizer::clingingRow()
{
  std::vector<RowCluster> rowClusters;
  auto numRows = targetBlock_->getRows().size();
  auto rowHeight = (*targetBlock_->getRows().begin())->getBBox().dy();

  rowClusters.resize(numRows);
  auto instSet = targetBlock_->getInsts();

  int yMin = (*targetBlock_->getRows().begin())->getBBox().yMin();
  for (auto inst : instSet) {
    auto instY = inst->getLocation().y();
    int rowIdx = (instY - yMin) / rowHeight;
    inst->setLocation(inst->getLocation().x(), rowIdx * rowHeight + yMin);
    if (rowIdx >= 0 && rowIdx < numRows) {
      rowClusters[rowIdx].push_back(inst);
    } else {
      assert(0);
    }
  }

  for (auto& rowCluster : rowClusters) {
    if (rowCluster.empty()) {
      continue;
    }
    std::sort(rowCluster.begin(),
              rowCluster.end(),
              [](const odb::dbInst* a, const odb::dbInst* b) {
                return a->getLocation().x() < b->getLocation().x();
              });

    // set the start point

    auto dieWidth = db_->getChip()->getBlock()->getDieArea().dx();
    int powerDirection = 0;
    int powerFactors = 0;
    int widthSum = 0;

    for (auto inst : rowCluster) {
      widthSum += inst->getMaster()->getWidth();
      for (auto term : inst->getITerms()) {
        if (!term->getNet()) {
          continue;
        }
        auto net = term->getNet();
        powerDirection
            = net->getTermBBox().xCenter() - term->getBBox().xCenter();
        powerFactors += 1;
      }
    }
    powerDirection /= powerFactors;
    auto powerRatio = powerDirection / dieWidth;

    auto margin = dieWidth - widthSum;
    auto marginHalf = margin / 2;
    int startPoint = marginHalf - marginHalf * powerRatio;
    int cursor = startPoint;
    for (auto inst : rowCluster) {
      inst->setLocation(cursor, inst->getLocation().y());
      cursor += inst->getMaster()->getWidth();
    }
  }
}
void SemiLegalizer::shiftLegalize()
{
  std::vector<RowCluster> rowClusters;
  auto numRows = targetBlock_->getRows().size();
  auto rowHeight = (*targetBlock_->getRows().begin())->getBBox().dy();

  rowClusters.resize(numRows);
  auto instSet = targetBlock_->getInsts();

  int yMin = (*targetBlock_->getRows().begin())->getBBox().yMin();
  for (auto inst : instSet) {
    auto instY = inst->getLocation().y();
    int rowIdx = (instY - yMin) / rowHeight;
    inst->setLocation(inst->getLocation().x(), rowIdx * rowHeight + yMin);
    if (rowIdx >= 0 && rowIdx < numRows) {
      rowClusters[rowIdx].push_back(inst);
    } else {
      assert(0);
    }
  }

  for (auto& rowCluster : rowClusters) {
    std::sort(rowCluster.begin(),
              rowCluster.end(),
              [](const odb::dbInst* a, const odb::dbInst* b) {
                return a->getLocation().x() < b->getLocation().x();
              });
    int cursor = targetBlock_->getDieArea().xMin();
    int cellRowIdx = 0;
    for (auto inst : rowCluster) {
      if (cursor > inst->getLocation().x()) {
        // the target cell is overlapped -> shift until not overlapped
        inst->setLocation(cursor, inst->getLocation().y());
      }
      cursor = inst->getBBox()->xMax();

      /* If the cell over the row, then shift the cell cluster to left. */
      if (cursor > targetBlock_->getDieArea().xMax()) {
        // current instance is out from the die
        shiftCellsToLeft(rowCluster, inst, cellRowIdx);
      }
      cellRowIdx++;
    }
  }
}
void SemiLegalizer::shiftCellsToLeft(SemiLegalizer::RowCluster& cellRow,
                                     odb::dbInst* inst,
                                     int idx)
{
  /* Get the shift candidates
   * `candidates` means,
   * ---------------------------------------------------------
   * ... ┌─────┐      ┌─────┐------┌─────┐-----┌─────┐---┌──────────────┐
   * ... │     │      │ c3  │      │ c2  │     │ c1  │   │              │
   * ... └─────┘      └─────┘------└─────┘-----└─────┘---└──────────────┘
   *                         <---->       <--->       <->    <---------->
   *                        spacing3     spacing2   spacing1 stickOutWidth
   *                                <------------------->
   *                                 c1, c2 are candidates
   *                         because Σ spacing_i < stickOutWidth until c3
   * ---------------------------------------------------------
   *                                                         |
   *                                                    Die end point
   * */
  std::vector<odb::dbInst*> candidates;
  int stickOutWidth
      = inst->getBBox()->xMax() - targetBlock_->getDieArea().xMax();
  int sumOfSpace = 0;
  int spacing;
  int cursor = inst->getLocation().x();
  candidates.push_back(inst);
  while (sumOfSpace < stickOutWidth) {
    idx -= 1;
    if (idx == -1) {
      spacing = cursor - 0;
      candidates.push_back(nullptr);
    } else {
      inst = cellRow.at(idx);
      spacing = cursor - inst->getBBox()->xMax();
      candidates.push_back(inst);
      cursor = inst->getLocation().x();
    }
    sumOfSpace += spacing;
  }
  candidates.pop_back();

  // Place candidates cells from right to left
  cursor = targetBlock_->getDieArea().xMax();
  for (auto inst : candidates) {
    int cellWidth = inst->getMaster()->getWidth();
    int placePoint = cursor - cellWidth;
    inst->setLocation(placePoint, inst->getLocation().y());
    cursor = inst->getLocation().x();
  }
}
void SemiLegalizer::adjustRowCapacity()
{
  // [.] Setting the Row Clusters
  std::vector<RowCluster> rowClusters;
  auto numRows = targetBlock_->getRows().size();
  auto rowHeight = (*targetBlock_->getRows().begin())->getBBox().dy();
  auto rowWidth = (*targetBlock_->getRows().begin())->getBBox().dx();

  rowClusters.resize(numRows);
  auto instSet = targetBlock_->getInsts();

  int yMin = (*targetBlock_->getRows().begin())->getBBox().yMin();
  for (auto inst : instSet) {
    auto instY = inst->getLocation().y();
    int rowIdx = (instY - yMin) / rowHeight;
    inst->setLocation(inst->getLocation().x(), rowIdx * rowHeight + yMin);
    if (rowIdx >= 0 && rowIdx < numRows) {
      rowClusters[rowIdx].push_back(inst);
    } else {
      assert(0);
    }
  }
  // sort with x coordinate of instances for each row
  for (auto& rowCluster : rowClusters) {
    std::sort(rowCluster.begin(),
              rowCluster.end(),
              [](const odb::dbInst* a, const odb::dbInst* b) {
                return a->getLocation().x() < b->getLocation().x();
              });
  }
  // [.] Check capacity for the rows and adjust them
  // Scan the rows from bottom to top.
  // If it is not solved all, then
  // scan the rows from top the bottom.
  // Repeat above.
  enum DIRECTION
  {
    UPWARD,
    DOWNWARD
  };
  bool direction = UPWARD;
  bool solved = false;
  while (!solved) {
    solved = true;

    int rowClusterMaxIdx = static_cast<int>(rowClusters.size());
    int startIdx = direction == UPWARD ? 0 : rowClusterMaxIdx - 1;
    int endIdx = direction == UPWARD ? rowClusterMaxIdx : -1;
    int step = direction == UPWARD ? 1 : -1;

    for (int i = startIdx; direction == UPWARD ? (i < endIdx) : (i > endIdx);
         i += step) {
      auto& rowCluster = rowClusters.at(i);
      int excess = degreeOfExcess(rowCluster);
      if (excess > 0) {
        solved = false;
      }
      uint reducedWidth = 0;
      while (excess > reducedWidth) {
        auto smallestInst = rowCluster.at(0);
        rowCluster.erase(
            std::remove(rowCluster.begin(), rowCluster.end(), smallestInst),
            rowCluster.end());

        int targetRowIdx;
        if (direction == UPWARD) {
          targetRowIdx = (i == rowClusters.size() - 1) ? i - 1 : i + 1;
        } else {
          targetRowIdx = (i == 0) ? i + 1 : i - 1;
        }
        rowClusters.at(targetRowIdx).push_back(smallestInst);
        smallestInst->setLocation(smallestInst->getLocation().x(),
                                  targetRowIdx * rowHeight + yMin);
        reducedWidth += smallestInst->getMaster()->getWidth();
      }
    }
    if (direction == UPWARD) {
      direction = DOWNWARD;
    } else {
      direction = UPWARD;
    }
  }
}
int SemiLegalizer::degreeOfExcess(const std::vector<odb::dbInst*>& row)
{
  int rowWidth = (*targetBlock_->getRows().begin())->getBBox().dx();

  int totalWidth = 0;
  for (auto inst : row) {
    totalWidth += inst->getMaster()->getWidth();
  }

  if (totalWidth > rowWidth) {
    return totalWidth - rowWidth;
  }
  return 0;
}
bool SemiLegalizer::utilCheck()
{
  int64_t sumArea = 0;
  for (auto inst : targetBlock_->getInsts()) {
    sumArea += inst->getMaster()->getWidth() * inst->getMaster()->getHeight();
  }
  if (sumArea > targetBlock_->getDieArea().area()) {
    return false;
  }
  return true;
}
}  // namespace mdm