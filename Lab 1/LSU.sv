import CORE_PKG::*;

module LSU (
  // General Inputs
  input logic clock,
  input logic reset,
  input logic data_gnt_i,

  // Inputs from decode
  input logic lsu_en_ip,                      // enable the LSU because it is a memory operation
  input load_store_func_code lsu_operator_ip, 

  // Input from ALU
  input logic alu_valid_ip,
  input logic [31:0] mem_addr_ip,             // address to access in mem for read/write

  // Input data from DRAM after load
  input logic [31:0] mem_data_ip,

  // Output to Decode
  output logic data_req_op,                  // validity of data address request
  output logic [31:0] load_mem_data_op      // data from load to sent to decode 
);

  logic valid_mem_operation;
  assign valid_mem_operation = data_gnt_i & lsu_en_ip & alu_valid_ip;

  always @(*) begin
    data_req_op = 1'b0; // default invalid operator
    load_mem_data_op = 32'hz; // default all 32 bits with zero

    if (valid_mem_operation == 1'b1) begin // check if the operation is a valid memory operation
      data_req_op = 1'b1; // set to valid operator
      case (lsu_operator_ip) // Creates cases for any type of instruction [LW, SW, ...]
        LW: begin
          /**
          * Here you will check to see if the address sent to memory is valid for load words 
          * 
          * 1. What do you need to check for in the memory address input to know if the instruction is valid or not? 
          *     - acomplished by checking if the 2 LSB are zero. This ensures that it is aligned properly
          *     - Though the processor already sets the data_req_op high, set it again to ensure proper functionality
          * 2. What do you need to set to inform the processor of a invalid address? 
          *     - If this is a invalid, set the data req_req_op from decode to false | 0
          */
          if (mem_addr_ip[1:0] == 2'b00) begin
            load_mem_data_op = mem_data_ip;
            data_req_op = 1'b1;
          end 
          else begin
            data_req_op = 1'b0;
          end
        end
        
        SW: begin
          /**
          * Here you will check to see if the address sent to memory is valid for store words 
          * 
          * 1. What do you need to check for in the memory address input to know if the instruction is valid or not? 
          *     - No need to send over data, so let decode that the function is valid
          * 2. What do you need to set to inform the processor of a invalid address? 
          */
          if (mem_addr_ip[1:0] == 2'b00) begin
            data_req_op = 1'b1;
          end 
          else begin
            data_req_op = 1'b0;
          end
        end
      endcase
    end
  end

endmodule