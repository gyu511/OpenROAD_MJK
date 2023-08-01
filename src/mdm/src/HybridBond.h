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
  void setSizeX(uint size_x);
  uint getSizeY() const;
  void setSizeY(uint size_y);
  uint getSpaceX() const;
  void setSpaceX(uint space_x);
  uint getSpaceY() const;
  void setSpaceY(uint space_y);

 private:
  uint size_x_;
  uint size_y_;
  uint space_x_;
  uint space_y_;
};

}  // namespace mdm
#endif  // OPENROAD_SRC_MDM_SRC_HYBRIDBOND_H_
