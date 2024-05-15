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

%module mdm

%{

#include "mdm/MultiDieManager.hh"
#include "ord/OpenRoad.hh"

mdm::MultiDieManager *
getMultiDieManager()
{
  return ord::OpenRoad::openRoad()->getMultiDieManager();
}

%}

%inline %{

void
set_3D_IC(int number_of_die)
{
  getMultiDieManager()->set3DIC(number_of_die);
}

void
makeShrunkLef()
{
  getMultiDieManager()->makeShrunkLefs();
}

void
multiDieDPL()
{
  getMultiDieManager()->multiDieDPL();
}

void
multiDieDPO()
{
  getMultiDieManager()->multiDieDPO();
}

void
runSemiLegalizer(char* targetDie="")
{
  getMultiDieManager()->runSemiLegalizer(targetDie);
}

void
constructSimpleExample1()
{
  getMultiDieManager()->constructSimpleExample1();
}

void
ICCADParse(char* testCase, bool siteDefined=false)
{
  getMultiDieManager()->ICCADParse(testCase, siteDefined);
}

void
timingTest()
{
  getMultiDieManager()->timingTest1();
}

void
timingTestOneDie()
{
  getMultiDieManager()->timingTestOneDie();
}

void
get3DHPWL()
{
  getMultiDieManager()->get3DHPWL();
}

void
getHPWL(char* dieInfo=nullptr)
{
  getMultiDieManager()->getHPWL(dieInfo);
}

void
setPartitionFile(char* partitionFile)
{
  getMultiDieManager()->setPartitionFile(partitionFile);
}

void
exportInstCoordinates(char* fileName)
{
  getMultiDieManager()->exportCoordinates(fileName);
}

void
importInstanceCoordinates(char* fileName)
{
  getMultiDieManager()->importCoordinates(fileName);
}

void
destroyOneDie(char* DIE)
{
  getMultiDieManager()->destroyOneDie(DIE);
}

void
readPartitionInfo(char* partitionFile)
{
  getMultiDieManager()->readPartitionInfo(partitionFile);
}

void
parseICCADOutput(char* filename, char* whichDie="")
{
  getMultiDieManager()->parseICCADOutput(filename, whichDie);
}

void
setICCADScale(int scale)
{
  getMultiDieManager()->setICCADScale(scale);
}

void
readCriticalCells(char* filename)
{
  getMultiDieManager()->readCriticalCells(filename);
}

void
setNetWeight(float weight)
{
  getMultiDieManager()->setNetWeight(weight);
}

%} // inline
