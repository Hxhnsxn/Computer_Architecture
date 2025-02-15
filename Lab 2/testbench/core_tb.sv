//***********************************************************
// ECE 3058 Architecture Concurrency and Energy in Computation
//
// RISCV Processor System Verilog Behavioral Model
//
// School of Electrical & Computer Engineering
// Georgia Institute of Technology
// Atlanta, GA 30332
//
//  Engineer:   Zou, Ivan
//  Module:     core_tb
//  Functionality:
//      This is the testbed for the 5 Stage Pipeline RISCV processor
//
//***********************************************************
`timescale 1ns / 1ns

module Core_tb;

// Clock and Reset signals to simulate as input into core
	logic clk = 1;
	logic mem_enable;
	logic reset;

	// local variables to display for testbench
	logic[6:0] cycle_count;
	
	integer i;
	initial
	begin
		cycle_count = 0;

		// do the simulation
		$dumpfile("Core_Simulation.vcd");

		// dump all the signals into the vcd waveforem file
		$dumpvars(0, Core_tb);

		reset = 1'b1;
		mem_enable = 1'b1;

		// Set the Test instructions and preset MEM and Regfile here if desired

		// Start Testbench Test Instructions. First instruction should always be a NOP

		#1 

    // NOP 
		core_proc.InstructionFetch_Module.InstructionMemory.instr_RAM[0] = 8'h00;
    core_proc.InstructionFetch_Module.InstructionMemory.instr_RAM[1] = 8'h00;
    core_proc.InstructionFetch_Module.InstructionMemory.instr_RAM[2] = 8'h00;
    core_proc.InstructionFetch_Module.InstructionMemory.instr_RAM[3] = 8'h00;

    // add x5, x5, x2
	// 	core_proc.InstructionFetch_Module.InstructionMemory.instr_RAM[4] = 8'h00;
    // core_proc.InstructionFetch_Module.InstructionMemory.instr_RAM[5] = 8'h22;
    // core_proc.InstructionFetch_Module.InstructionMemory.instr_RAM[6] = 8'h82;
    // core_proc.InstructionFetch_Module.InstructionMemory.instr_RAM[7] = 8'hb3;

    // sub x4, x12, x7
	// 	core_proc.InstructionFetch_Module.InstructionMemory.instr_RAM[8] = 8'h40;
    // core_proc.InstructionFetch_Module.InstructionMemory.instr_RAM[9] = 8'h76;
    // core_proc.InstructionFetch_Module.InstructionMemory.instr_RAM[10] = 8'h02;
    // core_proc.InstructionFetch_Module.InstructionMemory.instr_RAM[11] = 8'h33;


	// Task 1 Simple
	// NOP :>
		core_proc.InstructionFetch_Module.InstructionMemory.instr_RAM[12] = 32'h00000000; 
	core_proc.InstructionFetch_Module.InstructionMemory.instr_RAM[13] = 32'h00002483; // lw x9, 0(x0)
	core_proc.InstructionFetch_Module.InstructionMemory.instr_RAM[14] = 32'h002480b3; // add x1, x9, x2
	core_proc.InstructionFetch_Module.InstructionMemory.instr_RAM[15] = 32'h00402403; // lw x8, 4(x0)
	core_proc.InstructionFetch_Module.InstructionMemory.instr_RAM[16] = 32'h006401b3; // add x3, x8, x6
		#6 reset = 1'b0;

		#50 $finish;
	end

	always
		#1 clk <= clk + 1;

	always @(posedge clk) begin
		if (~reset)
			cycle_count <= cycle_count + 1;
	end

	Core core_proc(
		// Inputs
		.clock(clk),
		.reset(reset),
		.mem_en(mem_enable)
	);

endmodule
////***********************************************************
//// ECE 3058: Architecture, Concurrency, and Energy in Computation
////
//// RISC-V Processor SystemVerilog Behavioral Model
////
//// School of Electrical & Computer Engineering
//// Georgia Institute of Technology
//// Atlanta, GA 30332
////
////  Engineer:   Crombie, Ian; Cochran, Jack
////  Module:     core_tb
////  Functionality:
////      This is the testbench for a 5 stage pipelined RISC-V processor
////
////***********************************************************
//`timescale 1ns / 1ns
//
//module Core_tb;
//
//  // clock and reset signals
//  logic clk = 1;
//  logic reset;
//
//  // Loop variable for loading instructions
//  integer i;
//
//
//
//  // https://www.cs.sfu.ca/%7Eashriram/Courses/CS295/assets/notebooks/RISCV/RISCV_CARD.pdf
//  // https://luplab.gitlab.io/rvcodecjs/
//  // http://www.eg.bucknell.edu/~csci206/riscv-converter/
//
//
//  // Use the links above to add RISC-V instructions to the provided array in hex format.
//  // Remember, these resources are may be incorrect for some instruction types, so make sure to verify them with your RISC-V greencard
//  bit [31:0] test_instructions [] = {
//        32'h00000000, // NOP (keep as first instruction for simulation to work)
//        32'h05028293, // addi x5, x5, 80
//		32'h01428213, // addi x4, x5, 20
//		32'h00428533, // add x10, x5, x4
//		32'h40428633  // sub x12, x5, x4
//    };
//
//
//
//  
//  initial
//  begin
//    // dump waveform signals into a vcd waveform file
//    $dumpfile("Core_Simulation.vcd");
//    $dumpvars(0, Core_tb);
//	
//	reset = 1'b1;
//	
//	#1
//    // Loop over the array of instructions
//    for (i = 0; i < $size(test_instructions); i++)
//    begin
//	
//	  // Now load each byte in to memory manually
//      rv32_core.InstructionFetch_Module.InstructionMemory.instr_RAM[i * 4 + 0] = test_instructions[i][31:24];
//      rv32_core.InstructionFetch_Module.InstructionMemory.instr_RAM[i * 4 + 1] = test_instructions[i][23:16];
//      rv32_core.InstructionFetch_Module.InstructionMemory.instr_RAM[i * 4 + 2] = test_instructions[i][15:8];
//      rv32_core.InstructionFetch_Module.InstructionMemory.instr_RAM[i * 4 + 3] = test_instructions[i][7:0];
//    end
//	
//	#3 reset = 1'b0;
//	
//	
//    // end simulation after (# of instructions * 4ns(clk period))
//    #($size(test_instructions) * 4) $finish;
//  end
//
//
//always
//	#2 clk <= ~clk;
//
//
//
//  // instantiate the RISC-V core
//  Core rv32_core (
//         .clock(clk),
//         .reset(reset),
//         .mem_en(1'b1)
//       );
//
//endmodule
//