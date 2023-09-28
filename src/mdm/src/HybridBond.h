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

#ifndef OPENROAD_SRC_MDM_SRC_HYBRIDBOND_H_
#define OPENROAD_SRC_MDM_SRC_HYBRIDBOND_H_
#include <iostream>

#include "odb/db.h"
namespace mdm {
class HybridBond
{
 public:
  HybridBond();
  ~HybridBond();

 private:
  odb::dbBox shape_;
  odb::dbNet* connected_net_{};
};
class HybridBondInfo
{
 public:
  void setHybridBondInfo(uint size_x, uint size_y, uint space_x, uint space_y);
  uint getSizeX() const;
  void setSizeX(uint sizeX);
  uint getSizeY() const;
  void setSizeY(uint sizeY);
  uint getSpaceX() const;
  void setSpaceX(uint spaceX);
  uint getSpaceY() const;
  void setSpaceY(uint spaceY);

 private:
  uint sizeX_;
  uint sizeY_;
  uint spaceX_;
  uint spaceY_;
};

}  // namespace mdm
#endif  // OPENROAD_SRC_MDM_SRC_HYBRIDBOND_H_
