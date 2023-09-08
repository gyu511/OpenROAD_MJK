# Copyright (c) 2021, The Regents of the University of California
# All rights reserved.
# 
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

# sta::define_cmd_args "run_mdm" {[-key1 key1] [-flag1] pos_arg1}

# Put helper functions in a separate namespace so they are not visible
# too users in the global namespace.

namespace eval mdm {
proc mdm_helper { } {
  puts "Helping 23/6"
}
}

sta::define_cmd_args "set_3D_IC" {[-die_number die_number]}

proc set_3D_IC { args } {
  sta::parse_key_args "set_3D_IC" args \
   keys {-die_number} flags {}

  set die_number $keys(-die_number)
  mdm::set_3D_IC $die_number
}

proc mdm::two_die_detail_placement { } {
  mdm::twoDieDetailPlace
}

proc mdm::makeShrunkLef { } {
  mdm::makeShrunkLef
}

