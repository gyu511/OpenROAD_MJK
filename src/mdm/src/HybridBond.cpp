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

#include "HybridBond.h"

namespace mdm {
HybridBond::HybridBond() = default;
HybridBond::~HybridBond() = default;
uint HybridBondInfo::getSizeX() const
{
  return sizeX_;
}
void HybridBondInfo::setSizeX(uint sizeX)
{
  HybridBondInfo::sizeX_ = sizeX;
}
uint HybridBondInfo::getSizeY() const
{
  return sizeY_;
}
void HybridBondInfo::setSizeY(uint sizeY)
{
  HybridBondInfo::sizeY_ = sizeY;
}
uint HybridBondInfo::getSpaceX() const
{
  return spaceX_;
}
void HybridBondInfo::setSpaceX(uint spaceX)
{
  HybridBondInfo::spaceX_ = spaceX;
}
uint HybridBondInfo::getSpaceY() const
{
  return spaceY_;
}
void HybridBondInfo::setSpaceY(uint spaceY)
{
  HybridBondInfo::spaceY_ = spaceY;
}
void HybridBondInfo::setHybridBondInfo(uint size_x,
                                       uint size_y,
                                       uint space_x,
                                       uint space_y)
{
  sizeX_ = size_x;
  sizeY_ = size_y;
  spaceX_ = space_x;
  spaceY_ = space_y;
}
}  // namespace mdm
