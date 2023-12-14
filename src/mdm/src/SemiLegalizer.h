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

#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include "odb/db.h"

namespace mdm {
class MultiDieManager;
class SemiLegalizer
{
  using RowCluster = std::vector<odb::dbInst*>;

 public:
  void setDb(odb::dbDatabase* db) { db_ = db; }
  void run(bool abacus = true);

 private:
  /**
   * \Refer:
   * [Abacus] Fast Legalization of Standard Cell Circuits with Minimal Movement
   * [DREAMPlace] https://github.com/limbo018/DREAMPlace/tree/master/dreamplace/ops/abacus_legalize
   * This is the simple and fast legalizer
   * */
  void runAbacus();
  void runAbacus(odb::dbBlock* block);

  void doSemiLegalize();

  /**
   * Assume it has single height rows
   * This will adjust the row capacity
   * */
  void doSemiLegalize(odb::dbBlock* block);

  /**
   * \deprecated
   * This function attaches the instances in the same row tightly together,
   * considering the wire length force
   * */
  void clingingRow();

  /**
   * 1. Place cells from left to right considering overlap
   * 2. If the cell over the die, then shift the cell cluster to left.
   * */
  void shiftLegalize();
  void shiftCellsToLeft(RowCluster& cellRow, odb::dbInst* inst, int idx);
  void adjustRowCapacity();
  int degreeOfExcess(const std::vector<odb::dbInst*>& row);

  bool utilCheck();

  odb::dbDatabase* db_;
  odb::dbBlock* targetBlock_;
};
}  // namespace mdm
