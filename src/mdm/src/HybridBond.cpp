#include "HybridBond.h"

namespace mdm {
HybridBond::HybridBond() = default;
HybridBond::~HybridBond() = default;
uint HybridBondInfo::getSizeX() const
{
  return size_x_;
}
void HybridBondInfo::setSizeX(uint size_x)
{
  HybridBondInfo::size_x_ = size_x;
}
uint HybridBondInfo::getSizeY() const
{
  return size_y_;
}
void HybridBondInfo::setSizeY(uint size_y)
{
  HybridBondInfo::size_y_ = size_y;
}
uint HybridBondInfo::getSpaceX() const
{
  return space_x_;
}
void HybridBondInfo::setSpaceX(uint space_x)
{
  HybridBondInfo::space_x_ = space_x;
}
uint HybridBondInfo::getSpaceY() const
{
  return space_y_;
}
void HybridBondInfo::setSpaceY(uint space_y)
{
  HybridBondInfo::space_y_ = space_y;
}
void HybridBondInfo::setHybridBondInfo(uint size_x,
                                       uint size_y,
                                       uint space_x,
                                       uint space_y)
{
  size_x_ = size_x;
  size_y_ = size_y;
  space_x_ = space_x;
  space_y_ = space_y;
}
}  // namespace mdm
