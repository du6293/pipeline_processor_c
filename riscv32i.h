#ifndef __RV32I_H__
#define __RV32I_H__

// headers
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>


// defines
#define REG_WIDTH 32
#define IMEM_DEPTH 1024
#define DMEM_DEPTH 1024

// configs
#define CLK_NUM 45


/*  instruction memory */
typedef struct imem_input_t {
	uint32_t imem_addr;
	uint32_t *imem_data;
}Imem_in;

typedef struct imem_output_t {
	uint32_t inst;
}Imem_out;

Imem_out imem(Imem_in, uint32_t *imem_data);

/************************************************************************************************************************************/


/* register */
typedef struct regfile_input_t{
	uint8_t rs1;
	uint8_t rs2;
	uint8_t rd;
	uint32_t rd_din;
	uint32_t reg_write;
	uint32_t *reg_data;
}Reg_in;

typedef struct regfile_output_t{
	uint32_t rs1_dout;
	uint32_t rs2_dout;
}Reg_out;

Reg_out regfile(Reg_in, uint32_t *reg_data);


/* alu */
typedef struct alu_input_t{
	uint32_t alu_in1;
	uint32_t alu_in2;
	uint8_t alu_control;
}Alu_in;

typedef struct alu_output_t{
	uint32_t alu_result;
}Alu_out;

Alu_out alu(Alu_in);




/* data memory */
typedef struct dmem_input_t{
	uint32_t dmem_din;
	uint32_t dmem_addr;
	//uint32_t dmem_sz;
	uint8_t mem_write;
	uint8_t mem_read;
	uint32_t *dmem_data;
}Dmem_in;

typedef struct dmem_output_t{
	uint32_t dmem_dout;
}Dmem_out;

Dmem_out dmem(Dmem_in, uint32_t *dmem_data);



// structures for pipeline registers
typedef struct pipe_if_id_t {
	uint32_t pc;
	uint32_t inst;
}Id;


typedef struct pipe_id_ex_t {
	uint32_t pc;
	uint32_t rs1_dout;
	uint32_t rs2_dout;
	uint32_t imm32;
	uint32_t imm32_branch;
	uint8_t funct3;
	uint8_t funct7;
	uint8_t branch[6] ;
	uint8_t alu_src;
	uint8_t alu_op;
	uint8_t mem_read;
	uint8_t mem_write;	
	uint8_t opcode ;
	uint8_t rs1;
	uint8_t rs2;
	uint8_t rd;
	uint8_t reg_write;
	uint8_t mem_to_reg;
}Ex;

typedef struct pipe_ex_mem_t {
	uint32_t pc;
	uint32_t alu_result;
	uint32_t rs2_dout;
	uint8_t mem_read;
	uint8_t mem_write;
	uint8_t rd;
	uint8_t reg_write;
	uint8_t mem_to_reg;
	uint8_t opcode;
	uint8_t funct3;

}Mem;

typedef struct pipe_mem_wb_t {
	uint32_t pc;
	uint32_t alu_result;
	uint32_t dmem_dout;
	uint8_t opcode;
	uint8_t rd;
	uint8_t funct3;
	uint8_t reg_write;
	uint8_t mem_to_reg;
}Wb;

#endif
