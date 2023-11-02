cd ../../src/mdm/test

puts "tclCmd: set db"
set db [ord::get_db]

puts "tclCmd: set chip"
set chip [odb::dbChip_create $db]

puts "tclCmd: set tech"
set tech [odb::dbTech_create $db topHierTech]

puts "tclCmd: top_block"
set top_block [odb::dbBlock_create $chip topHierBlock]


puts "tclCmd: read_lef -tech_name top -tech -lib timing/example1-1.lef"
read_lef -tech_name childLef1 -tech -lib timing/example1-1.lef
puts "tclCmd: read_lef -tech_name bottom -tech -lib timing/example1-2.lef"
read_lef -tech_name childLef2 -tech -lib timing/example1-2.lef


## is below valid?
puts "tclCmd: read_liberty timing/example1_slow.lib"
read_liberty timing/example1_slow.lib


# Make def data
# read_def timing/example1.def
puts "tclCmd: mdm::test"
mdm::timingTest

gui::design_created

set clk_name core_clock
set clk_port_name clkPort
set clk_period 1.0
set clk_io_pct 0.2

set clk_port [get_ports $clk_port_name]
create_clock -name $clk_name -period $clk_period $clk_port

set non_clock_inputs [lsearch -inline -all -not -exact [all_inputs] $clk_port]

set_input_delay  [expr $clk_period * $clk_io_pct] -clock $clk_name $non_clock_inputs
set_output_delay [expr $clk_period * $clk_io_pct] -clock $clk_name [all_outputs]

##########################################################

#puts "tclCmd: read_spef timing/example1.dspef"
#read_spef timing/example1.dspef

#puts "tclCmd: create_clock -name clk -period 10 {clk1 clk2 clk3}"
#create_clock -name clk -period 10 {clk1 clk2 clk3}

#puts "tclCmd: set_input_delay -clock clk 0 {in1 in2}"
#set_input_delay -clock clk 0 {in1 in2}

#puts "tclCmd: set_output_delay -clock clk 0 out"
#set_output_delay -clock clk 0 out

report_checks