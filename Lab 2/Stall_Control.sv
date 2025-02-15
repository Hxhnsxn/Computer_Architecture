//***********************************************************
// ECE 3058 Architecture Concurrency and Energy in Computation
//
// RISCV Processor System Verilog Behavioral Model
//
// School of Electrical & Computer Engineering
// Georgia Institute of Technology
// Atlanta, GA 30332
//
//  Module:     core_tb
//  Functionality:
//      Stall Controller for a 5 Stage RISCV Processor
//
//***********************************************************
import CORE_PKG::*;

module Stall_Control (
  input logic reset, 

  input logic [6:0] ID_instr_opcode_ip,
  input logic [4:0] ID_src1_addr_ip,
  input logic [4:0] ID_src2_addr_ip,

  //The destination register from the different stages
  input logic [4:0] EX_reg_dest_ip,  // destination register from EX pipe
  input logic [4:0] LSU_reg_dest_ip,
  input logic [4:0] WB_reg_dest_ip,
  input logic WB_write_reg_en_ip,

  // The opcode of the current instr. in ID/EX
  input [6:0] EX_instr_opcode_ip,

  output logic stall_op
);

  always_comb begin
    stall_op = 1'b0;
    case(ID_instr_opcode_ip) 

      OPCODE_OP: begin
        
        /**
        * Task 1
        * 
        * Here you will need to decide when to pull the stall control logic high. 
        * 
        * 1. Load to use stalls
        * 2. Stalls when reading and writing from Register File
        * For Register Register instructions, what registers are relevant
        */
        if ((EX_instr_opcode_ip == OPCODE_LOAD) && // Checks if the opcode in EX is a load
          (ID_src1_addr_ip === EX_reg_dest_ip) && // checking if rs1 is being modified by the previous instr for a load
          (EX_reg_dest_ip !== 7'b0)) begin // checks if the modified Execution register is not zero
            stall_op = 1'b1; // stall
          end
        // same as previous but for rs2
        if ((EX_instr_opcode_ip == OPCODE_LOAD) && 
          (ID_src2_addr_ip === EX_reg_dest_ip) && 
          (EX_reg_dest_ip !== 7'b0)) begin 
            stall_op = 1'b1; // stall
          end

        if ((ID_src1_addr_ip === WB_reg_dest_ip) && // make sure that the modified register is in the WB stage
         (WB_write_reg_en_ip) && // confirm that WB is in fact writing back ...
         (WB_reg_dest_ip !== 5'b0) && // confirm that the modified register is not the zero register
         (ID_src1_addr_ip !== EX_reg_dest_ip) && // Ensures that we get the most recent data - if in EX we must stall
         (ID_src1_addr_ip !== LSU_reg_dest_ip)) begin // Same logic as the Execute stage check - need to get the most recent modified instance from the wb stage
          stall_op = 1'b1; // stall
        end
        // same logic as previous if-block but for rs2
        if ((ID_src2_addr_ip === WB_reg_dest_ip) && (WB_write_reg_en_ip) && (WB_reg_dest_ip !== 5'b0) &&
         (ID_src2_addr_ip !== EX_reg_dest_ip) && (ID_src2_addr_ip !== LSU_reg_dest_ip)) begin
          stall_op = 1'b1; // stall
        end
      end

      OPCODE_OPIMM: begin

        /**
        * Task 1
        * 
        * Here you will need to decide when to pull the stall control logic high. 
        * 
        * 1. Load to use stalls
        * 2. Stalls when reading and writing from Register File
        * For Register Immedite instructions, what registers are relevant
        */
        if ((EX_instr_opcode_ip == OPCODE_LOAD) && // Checks if the opcode in EX is a load
          (ID_src1_addr_ip === EX_reg_dest_ip)) begin // checking if rs1 is being modified by the previous instr for a load
          // (ID_src1_addr_ip) && // checking if the register is in use | if it is not zero
          // (EX_reg_dest_ip))  // checks if the modified Execution register is not zero
            stall_op = 1'b1; // stall
          end
        if ((ID_src1_addr_ip === WB_reg_dest_ip) && // make sure that the modified register is in the WB stage
         (WB_write_reg_en_ip) && // confirm WB is active
         (WB_reg_dest_ip !== 5'b0) && // confirm that the modified register is not the zero register
         (ID_src1_addr_ip !== EX_reg_dest_ip) && // Ensures that we get the most recent data - if in EX we must stall
         (ID_src1_addr_ip !== LSU_reg_dest_ip)) begin // Same logic as the Execute stage check - need to get the most recent modified instance from the wb stage
          stall_op = 1'b1; // stall
         end
      end

      default: begin
        stall_op = 1'b0;
      end
    endcase
  end

endmodule
