cd ../../src/mdm/test

puts "tclCmd: set db"
set db [ord::get_db]

set chip [odb::dbChip_create $db]
read_lef -tech -lib timing/example1.lef
set top_block [odb::dbBlock_create $chip topHierBlock]

read_lef -tech_name childLef1 -tech -lib timing/example1-1.lef
read_lef -tech_name childLef2 -tech -lib timing/example1-2.lef

read_liberty timing/example1_slow.lib

mdm::timingTest

gui::design_created

set clk_name core_clock
set clk_port_name clkPort
set clk_period 1.0
set clk_io_pct 0.2

set clk_port [get_ports $clk_port_name]
create_clock -name $clk_name -period $clk_period $clk_port

set_wire_rc -signal -layer metal3
set_wire_rc -clock  -layer metal5
estimate_parasitics -placement

report_checks