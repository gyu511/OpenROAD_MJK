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
#ifndef TEST_CASE_MANAGER_H
#define TEST_CASE_MANAGER_H

#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include "odb/db.h"

namespace mdm {
class MultiDieManager;
class ICCADOutputParser
{
  enum DIE
  {
    TOP,
    BOTTOM
  };

 public:
  void setDb(odb::dbDatabase* db) { db_ = db; }
  void parseOutput(std::ifstream& outputFile);
  void applyCoordinates(odb::dbBlock* targetBlock = nullptr);
  void makePartitionFile(const std::string& fileName);
  void setScale(int scale) { scale_ = scale; }

 private:
  std::pair<int, int> getTerminalCoordinate(const std::string& terminalName)
  {
    return terminalCoordinateMap_[terminalName];
  }

  std::pair<int, int> getCellCoordinate(const std::string& terminalName)
  {
    return instCoordinateMap_[terminalName];
  }

  bool parsed_ = false;
  std::vector<std::string> instList_;
  std::vector<std::string> terminalList_;
  std::unordered_map<std::string, DIE> instPartitionMap_;
  std::unordered_map<std::string, std::pair<int, int>> instCoordinateMap_;
  std::unordered_map<std::string, std::pair<int, int>> terminalCoordinateMap_;
  odb::dbDatabase* db_;
  int scale_ = 1;
};
class TestCaseManager
{
  struct LibPinInfo
  {
    std::string pinName;
    int pinLocationX;
    int pinLocationY;
  };
  struct LibCellInfo
  {
    std::string name;
    int width;
    int height;
    int pinNumber;
    bool isMacro;
    std::vector<LibPinInfo> libPinInfos;
  };
  struct TechInfo
  {
    std::string name;
    int libCellNum;
    std::vector<LibCellInfo> libCellInfos;
  };
  struct RowInfo
  {
   public:
    int startX = 0;
    int startY = 0;
    int rowWidth = 0;
    int rowHeight = 0;
    int repeatCount = 0;
  };
  struct DieInfo
  {
    std::string tech_name;
    int lowerLeftX;
    int lowerLeftY;
    int upperRightX;
    int upperRightY;
    int max_util;
    RowInfo rowInfo;
    TechInfo* techInfo;
  };
  struct TerminalInfo
  {
    int sizeX;
    int sizeY;
    int spacing_size;
    int cost;
  };
  struct InstanceInfo
  {
    std::string instName;
    std::string libCellName;
  };
  struct ConnectedPinInfo
  {
    std::string instanceName;
    std::string libPinName;
  };
  struct NetInfo
  {
    std::string netName;
    int pinNum;
    std::vector<ConnectedPinInfo> connectedPinsInfos;
  };
  struct BenchInformation
  {
    std::vector<TechInfo> techInfos;
    std::vector<InstanceInfo> instance_infos;
    std::vector<NetInfo> netInfos;
    std::vector<DieInfo> dieInfos;
    TerminalInfo terminalInfo{};
  };
  struct HybridBond  // equals to `terminal`
  {
    int x;
    int y;
    odb::dbNet* net;
  };

 public:
  enum DIE_ID
  {
    TOP_DIE,
    BOTTOM_DIE
  };

  enum TESTCASE
  {
    ICCAD2022_CASE1,
    ICCAD2022_CASE2,
    ICCAD2022_CASE3,
    ICCAD2022_CASE4,
    ICCAD2022_CASE2_H,
    ICCAD2022_CASE3_H,
    ICCAD2022_CASE4_H,

    ICCAD2023_CASE1,
    ICCAD2023_CASE2,
    ICCAD2023_CASE3,
    ICCAD2023_CASE4,
    ICCAD2023_CASE2_H,
    ICCAD2023_CASE3_H,
    ICCAD2023_CASE4_H,

    NA,

    NET_CARD,
    MP_GROUP,
    MEGA_BOOM
  };

  void setMDM(MultiDieManager* mdm);
  void ICCADContest(TESTCASE testCase, MultiDieManager* mdmPtr);

  // Belows are utils
  void parseICCADOutput(const std::string& fileName,
                        const char* whichDieChar = "");
  void setScale(int scale);
  int getScale() { return scale_; }
  bool isICCADParsed() { return isICCADParsed_; };
  void rowConstruction();

 private:
  void ICCADContest2022(const std::string& inputFileName,
                        MultiDieManager* mdManager);
  void ICCADContest2023(const std::string& inputFileName,
                        MultiDieManager* mdManager);
  void constructDB(MultiDieManager* mdManager);

  /**
   * Get the input file name and the contest year of the input file
   * */
  std::pair<std::string, int> fetchInputFileInfo(TESTCASE testCase) const;

  ICCADOutputParser iccadOutputParser_;
  bool isICCADParsed_ = false;
  BenchInformation benchInformation_;
  int instanceNumber_;
  int netNumber_;
  std::pair<RowInfo, RowInfo> rowInfos_;
  std::pair<int, int> maxUtils_;
  MultiDieManager* mdm_ = nullptr;
  TESTCASE testcase_;
  int scale_ = 1;
};

}  // namespace mdm

#endif  // TEST_CASE_MANAGER_H
