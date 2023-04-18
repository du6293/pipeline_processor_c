#include <stdio.h>
#include <stdlib.h>

#include "rv32i.h"

int main (int argc, char *argv[]) {

	// get input arguments
	FILE *f_imem, *f_dmem;
	if (argc < 3) {
		printf("usage: %s imem_data_file dmem_data_file\n", argv[0]);
		exit(1);
	}

	if ( (f_imem = fopen(argv[1], "r")) == NULL ) {
		printf("Cannot find %s\n", argv[1]);
		exit(1);
	}
	if ( (f_dmem = fopen(argv[2], "r")) == NULL ) {
		printf("Cannot find %s\n", argv[2]);
		exit(1);
	}

	// memory data (global)
	uint32_t *reg_data;
	uint32_t *imem_data;
	uint32_t *dmem_data;

	reg_data = (uint32_t*)malloc(32*sizeof(uint32_t));
	imem_data = (uint32_t*)malloc(IMEM_DEPTH*sizeof(uint32_t));
	dmem_data = (uint32_t*)malloc(DMEM_DEPTH*sizeof(uint32_t));

	// initialize memory data
	int i, j, k;
	for (i = 0; i < 32; i++) reg_data[i] = 0;
	for (i = 0; i < IMEM_DEPTH; i++) imem_data[i] = 0;
	for (i = 0; i < DMEM_DEPTH; i++) dmem_data[i] = 0;

	uint32_t d, buf;
	i = 0;
	printf("\n*** Reading %s ***\n", argv[1]);
	while (fscanf(f_imem, "%1d", &buf) != EOF) {
		d = buf << 31;
		for (k = 30; k >= 0; k--) {
			if (fscanf(f_imem, "%1d", &buf) != EOF) {
				d |= buf << k;
			} else {
				printf("Incorrect format!!\n");
				exit(1);
			}
		}
		imem_data[i] = d;
		printf("imem[%03d]: %08X\n", i, imem_data[i]);
		i++;
	}

	i = 0;
	printf("\n*** Reading %s ***\n", argv[2]);
	while (fscanf(f_dmem, "%8x", &buf) != EOF) {
		dmem_data[i] = buf;
		printf("dmem[%03d]: %08X\n", i, dmem_data[i]);
		i++;
	}

	fclose(f_imem);
	fclose(f_dmem);

	
	
	/* instruction memory */
	Imem_in imem_in = {0};
	Imem_out imem_out = {0};
	
	/* register */
	Reg_in reg_in = {0};
	Reg_out reg_out= {0};
	
	/* alu */
	Alu_in alu_in = {0};
	Alu_out alu_out = {0};

	/* data memory */
	Dmem_in dmem_in = {0};
	Dmem_out dmem_out = {0};
	
	// pipeline registers
	Id id = {0};
	Ex ex = {0};
	Mem mem = {0};
	Wb wb = {0};

	/* use malloc */
	imem_in.imem_data = imem_data ; 
	reg_in.reg_data = reg_data;
	dmem_in.dmem_data = dmem_data;
	
	uint32_t cc = 2;	// clock count
	
	
	/* program counter */
	uint8_t 	pc_write 		= 0;
	uint32_t 	pc_curr 		= 0;
	uint32_t 	pc_next 		= 0;
	uint32_t 	pc_next_plus4 	= 0;
	uint32_t	pc_next_branch 	= 0;
	uint8_t 	pc_next_sel 	= 0;
	uint8_t 	branch_taken 	= 0;
	
	/* instruction memory */
	uint32_t 	imem_addr 		= 0;
	uint32_t 	inst 			= 0;
	
	uint8_t 	if_flush		= 0;
	uint8_t		if_stall		= 0;
//////////////////////////////////////////////////////////////////////	
	/* control unit */
	uint8_t 	opcode			= 0;
	uint8_t 	funct3			= 0;
	uint8_t 	funct7			= 0;
	
	uint8_t 	branch[6]		= {0};
	uint8_t 	alu_src			= 0;
	uint8_t 	mem_to_reg		= 0;
	uint8_t 	alu_op			= 0;
	uint8_t 	mem_read		= 0;
	uint8_t 	mem_write		= 0;
	uint8_t 	reg_write		= 0;
	
	/* immediate generator */
	uint32_t 	imm12			= 0;
	uint32_t	imm20			= 0;
	uint32_t 	imm32			= 0;
	uint32_t	imm32_branch	= 0;
	
	/* hazard detection unit */
	uint8_t		stall_by_load_use = 0;
	uint8_t		flush_by_branch	= 0;
	uint8_t		id_flush		= 0;
	uint8_t		id_stall		= 0;
	/* register */
	uint8_t		rs1				= 0;
	uint8_t		rs2				= 0;
	uint8_t		rd				= 0;
	uint32_t	rd_din			= 0;
/////////////////////////////////////////////////////////////////////////	
	/* alu */
	uint32_t	alu_in1			= 0;
	uint32_t	alu_in2			= 0;
	uint32_t 	alu_control		= 0;
	/* forwarding unit */
	uint8_t		forward_a		= 0;
	uint8_t		forward_b		= 0;
	uint32_t	alu_fwd_in1		= 0;
	uint32_t	alu_fwd_in2		= 0;
	
	/* branch unit */
	uint32_t	sub_for_branch	= 0;
	uint8_t		bu_zero			= 0;
	uint8_t		bu_sign			= 0;
////////////////////////////////////////////////////////////////////////////	
	/* data memory */
	uint32_t	dmem_addr		= 0;
	uint32_t	dmem_din		= 0;
	uint32_t	dmem_dout		= 0;
/////////////////////////////////////////////////////////////////////////////	


	while (cc < CLK_NUM) {
	/*5**************************************************************************************************************************************	
		/*wb register*/
		wb.pc 		   = mem.pc;
		wb.opcode	   = mem.opcode;
		wb.funct3	   = mem.funct3;
		wb.alu_result  = mem.alu_result;
		wb.dmem_dout   = dmem_out.dmem_dout;
		wb.rd		   = mem.rd;
		wb.reg_write   = mem.reg_write;
		wb.mem_to_reg  = mem.mem_to_reg;
		
	/////*write back*/////
		if (wb.mem_to_reg) {
			if (wb.opcode == 0x03 && wb.funct3 == 0x00) {                                        // lb 8bit
				if (((int32_t)wb.dmem_dout >> 7) &1 == 1) 		rd_din  = ((int32_t)wb.dmem_dout | 0xffffff00) ;  //dout[7:0]
				
				else                                			rd_din  = (wb.dmem_dout & 0x000000ff);
			}
			else if (wb.opcode == 0x03 && wb.funct3 == 0x01 ){                                      // lh 16bit
				if (((int32_t)wb.dmem_dout >> 15) &1 == 1) 		rd_din = ((int32_t)wb.dmem_dout | 0xffff0000) ;  //dou[15:0]
				
				else                                   			rd_din = (wb.dmem_dout & 0x0000ffff) ;
			
			}
			else if (wb.opcode == 0x03 && wb.funct3 == 0x02) 	rd_din = (wb.dmem_dout );    // lw 32bit
			else if (wb.opcode == 0x03 && wb.funct3 == 0x04) 	rd_din = (wb.dmem_dout & 0x000000ff);     // lbu
			else if (wb.opcode == 0x03 && wb.funct3 == 0x05) 	rd_din = (wb.dmem_dout & 0x0000ffff);     // lhu
		} else {
			if 		(wb.opcode == 0x33)					rd_din = wb.alu_result;  // r-type
			else if (wb.opcode == 0x13) 						rd_din = wb.alu_result;  // i-type
			else if (wb.opcode == 0x63) 						rd_din = wb.pc + 4; 	// sb-type
			else if (wb.opcode == 0x6f) 						rd_din = wb.pc + 4; 	 // jal
			else if (wb.opcode == 0x67) 						rd_din = wb.pc + 4;		//jalr
			else if (wb.opcode == 0x37)  						rd_din = wb.alu_result;  //lui
			else if (wb.opcode == 0x17) 						rd_din = wb.alu_result;  // auipc
			else									rd_din = wb.alu_result;	// etc
		}
		
		reg_in.rd_din = rd_din;
		reg_in.rd = wb.rd;
		reg_in.reg_write = wb.reg_write;
		reg_out = regfile(reg_in, reg_data); 
		
		
	/*4****************************************************************************************************************************************	
		/*mem register*/
		mem.pc			= ex.pc;
		mem.alu_result	= alu_out.alu_result;
		mem.rs2_dout	= alu_fwd_in2;
		mem.opcode 		= ex.opcode;
		mem.funct3		= ex.funct3;
		mem.rd			= ex.rd;
		mem.mem_read	= ex.mem_read;
		mem.mem_write	= ex.mem_write;
		mem.reg_write   = ex.reg_write;
		mem.mem_to_reg  = ex.mem_to_reg;
		
	/////*memory*/////
		// 10.data memory
		dmem_addr = mem.alu_result >> 2;
		if (mem.mem_write) {
			if (mem.funct3 == 0x0)		dmem_din = (mem.rs2_dout & 0x000000ff); // sb R[rs2][7:0]
			else if (mem.funct3 == 0x1) 	dmem_din = (mem.rs2_dout & 0x0000ffff); // sh R[rs2][15:0]
			else if (mem.funct3 == 0x2) 	dmem_din = (mem.rs2_dout); // sw R[rs2][31:0] 
		}
		
		dmem_in.dmem_addr = dmem_addr;
		dmem_in.dmem_din = dmem_din;
		dmem_in.mem_read = mem.mem_read;
		dmem_in.mem_write = mem.mem_write;
		
		dmem_out = dmem(dmem_in, dmem_data);
		
	/***********************************************************************************************************************************	
		/*ex register*/
		if (id_flush) {
			ex.pc		= 0;
			ex.rs1_dout = 0;
			ex.rs2_dout = 0;
			ex.imm32 	= 0;
			ex.imm32_branch = 0;
			ex.opcode  	= 0;
			ex.funct3 	= 0;
			ex.funct7	= 0;
			ex.rs1 		= 0;
			ex.rs2		= 0;
			ex.rd		= 0;
			ex.reg_write= 0;
			ex.mem_to_reg=0;
			ex.branch[0] = 0;
			ex.branch[1] = 0;
			ex.branch[2] = 0;
			ex.branch[3] = 0;
			ex.branch[4] = 0;
			ex.branch[5] = 0;
			ex.alu_src	= 0;
			ex.alu_op 	= 0;
			ex.mem_read	= 0;
			ex.mem_write= 0;
		}
		else if (id_stall) {
			ex.reg_write= 0;
			ex.mem_to_reg=0;
			ex.branch[0] = 0;
			ex.branch[1] = 0;
			ex.branch[2] = 0;
			ex.branch[3] = 0;
			ex.branch[4] = 0;
			ex.branch[5] = 0;
			ex.alu_src	= 0;
			ex.alu_op 	= 0;
			ex.mem_read	= 0;
			ex.mem_write= 0;
			ex.rd 		= rd;
		}
		else {
			ex.pc		= id.pc;
			ex.rs1_dout = reg_out.rs1_dout;
			ex.rs2_dout = reg_out.rs2_dout;
			ex.imm32 	= imm32;
			ex.imm32_branch = imm32_branch;
			ex.opcode  	= opcode;
			ex.funct3 	= funct3;
			ex.funct7	= funct7;
			ex.rs1 		= rs1;
			ex.rs2		= rs2;
			ex.rd		= rd;
			ex.reg_write = reg_write;
			ex.mem_to_reg = mem_to_reg;
			ex.branch[0] = branch[0];
			ex.branch[1] = branch[1];
			ex.branch[2] = branch[2];
			ex.branch[3] = branch[3];
			ex.branch[4] = branch[4];
			ex.branch[5] = branch[5];
			ex.alu_src	= alu_src;
			ex.alu_op 	= alu_op;
			ex.mem_read	= mem_read;
			ex.mem_write = mem_write;	
		}
		
		
	/////*execute*/////
		// 7. alu control
		
		if      (ex.alu_op == 0) 							alu_control = 2;  	 //load, store   
		else if (ex.alu_op == 1) 							alu_control = 7;	 // sb-type 
		else if (ex.alu_op == 3) 							alu_control = 10;    // lui
		else if (ex.alu_op == 2) {						 // i-type, r-type
			
			if     (ex.funct3 == 0x01) 						alu_control = 4;  //sll,sllli
			else if(ex.funct3 == 0x05 && ex.funct7 == 0x0) 	alu_control = 5 ; //srl,srli
			else if(ex.funct3 == 0x05 && ex.funct7 == 0x20) alu_control = 6 ;  // sra, srai signed right shift
			else if(ex.funct3 == 0x00) {
				if (ex.funct7 == 0x20) 						alu_control = 7 ; //sub
				else 								alu_control = 2 ; //add, addi
			}
			else if (ex.funct3 == 0x04) 					alu_control = 3; //xor,xori
			else if (ex.funct3 == 0x06) 					alu_control = 1; //or,ori
			else if (ex.funct3 == 0x07)					alu_control = 0; //and,andi
			else if (ex.funct3 == 0x02) 					alu_control = 8; //slt,slti
			else if (ex.funct3 == 0x03)					alu_control = 9; //sltu,sltiu
		}

		
		// 8. fowrarding unit
		if 		(forward_a == 0)		alu_fwd_in1 = ex.rs1_dout;
		else if (forward_a == 2)			alu_fwd_in1 = mem.alu_result;
		else if (forward_a == 1)			alu_fwd_in1 = (mem.mem_to_reg==1) ? wb.dmem_dout : (mem.reg_write == 1) ? wb.alu_result : 0	;
		
		if 		(forward_b == 0)		alu_fwd_in2 = ex.rs2_dout;
		else if (forward_b == 2)			alu_fwd_in2 = mem.alu_result;
		else if (forward_b == 1)			alu_fwd_in2 = (mem.mem_to_reg==1) ? wb.dmem_dout : (mem.reg_write == 1) ? wb.alu_result : 0	;
		
		
		if 		(mem.reg_write == 1 && mem.rd != 0 && mem.rd == ex.rs1)																	forward_a = 2;
		else if (wb.reg_write == 1 && (wb.rd != 0) && !(mem.reg_write == 1 && (mem.rd != 0) && (mem.rd == ex.rs1)) && (wb.rd == ex.rs1)) forward_a == 1;
		else 																															forward_a == 0;
		
		if 		(mem.reg_write == 1 && mem.rd != 0 && mem.rd == ex.rs2)																	forward_b = 2;
		else if (wb.reg_write == 1 && (wb.rd != 0) && !(mem.reg_write == 1 && (mem.rd != 0) && (mem.rd == ex.rs2)) && (wb.rd == ex.rs2)) forward_b == 1;
		else 																															forward_b == 0;
		
		alu_in1 = (ex.opcode == 0x17|| ex.opcode == 0x6f) ? ex.pc : (ex.opcode == 0x37) ? 0 : alu_fwd_in1;
		alu_in2 = (ex.alu_src == 1 && ex.opcode != 0x17) ? ex.imm32 : (ex.alu_src == 1 && ex.opcode == 0x17) ? ex.imm32_branch : alu_fwd_in2;
		
		alu_in.alu_in1 = alu_in1;
		alu_in.alu_in2 = alu_in2;
		alu_in.alu_control = alu_control;
		alu_out = alu(alu_in);
		
		
		
		
		// 9.branch unit
		sub_for_branch = alu_out.alu_result;
		bu_zero = (sub_for_branch == 0 )? 1 : 0;
		bu_sign = (sub_for_branch >> 31);
		
		if (ex.branch[0] == 1 && bu_zero == 1)					branch_taken = 1;//beq
		else if (ex.branch[1] == 1 && bu_zero == 0)				branch_taken = 1;//bne
		else if (ex.branch[2] == 1 && bu_zero == 0 && bu_sign == 1)		branch_taken = 1;//blt
		else if (ex.branch[3] == 1 && bu_sign == 0)				branch_taken = 1;//bge
		else if (ex.branch[4] == 1 && bu_zero == 0 && bu_sign == 1)		branch_taken = 1;//bltu
		else if (ex.branch[5] == 1 && bu_sign == 0)				branch_taken = 1;//bgeu
		else if (ex.opcode == 0x6f || ex.opcode == 0x67)			branch_taken = 1;//jal, jalr
		else									branch_taken = 0;//don't jump
		
		
	/*2****************************************************************************************************************************************************	
		/*id register*/
		if (if_flush)
		{
			id.pc = 0;
			id.inst = 0;
		}
		else if (!if_stall) {
			id.pc 	= pc_curr;
			id.inst = inst;
		}
		
		
	/////*instruction decode*/////	
		// 3. control uint
		
		opcode = id.inst & 0x0000007f;
		if (opcode == 0x37 || opcode == 0x17 ) 	  funct3 = 0;
		else 							  funct3 = (id.inst >> 12) & 0x00007;

		if (opcode == 0x33 ) 					  funct7 = (id.inst >> 25) & 0x7f;
		else 							  funct7 = 0; 
		
		
		if 	(opcode == 0x63 && funct3 == 0x0) 	  branch[0] = 1; // beq
		else 									  branch[0] = 0;
		
		if (opcode == 0x63 && funct3 == 0x1) 	  branch[1] = 1; // bne
		else 									  branch[1] = 0;
		
		if (opcode == 0x63 && funct3 == 0x4) 	  branch[2] = 1; // blt
		else 									  branch[2] = 0;
		
		if (opcode == 0x63 && funct3 == 0x5) 	  branch[3] = 1; // bge
		else 									  branch[3] = 0;
		
		if (opcode == 0x63 && funct3 == 0x6) 	  branch[4] = 1; // bltu
		else 									  branch[4] = 0;
		
		if (opcode == 0x63 && funct3 == 0x7) 	  branch[5] = 1; // bgeu
		else 									  branch[5] = 0;
		
		
		if (opcode == 0x03)		mem_read = 1; // load
		else 				mem_read = 0;
		
		if (opcode == 0x23)		mem_write = 1; // store
		else 				mem_write = 0;
		
		if (opcode == 0x03)		mem_to_reg = 1; // load
		else				mem_to_reg = 0;
		
		reg_write = (opcode == 0x03 || opcode == 0x33 || opcode == 0x13 || opcode == 0x63 || opcode == 0x67 || opcode == 0x37 || opcode == 0x6f || opcode == 0x17) ? 1 : 0;  // except for store 
		alu_src = (opcode == 0x3 || opcode == 0x23 || opcode == 0x13 || opcode == 0x67 || opcode == 0x6f || opcode == 0x37 || opcode == 0x17) ? 1 : 0 ;  // rs2_dout에 imm?  rs2
			
		
		
		if      (opcode == 0x03) 			alu_op = 0;         	// load -> add
		else if (opcode == 0x23) 			alu_op = 0;		// store -> add
		else if (opcode == 0x33) 			alu_op = 2;		// r-type  
		else if (opcode == 0x13) 			alu_op = 2;		// i-type
		else if (opcode == 0x63) 			alu_op = 1;          	// sb   -> sub
		else if (opcode == 0x37) 			alu_op = 3;		// lui
		else 	                 			alu_op = 0;         	// auipc, jal, jalr
			
		
		// 4. immediate generator
		
		if (opcode == 0x13){                /////// i-type 
		    imm12 = (id.inst >> 20) & 0xfff;  // inst[31:20]->imm[11:0] 
		}
		else if (opcode == 0x3){              ////load
			imm12 = (id.inst >> 20) &0xfff; // inst[31:20]->imm[11:0]
		}
		
		else if (opcode == 0x23){  ///// store
			imm12 = (((id.inst >> 25) & 0x7f)<<5 )| (id.inst>> 7) &0x1f ;  //imm[11:5] imm[4:0]
		}
		
		else if (opcode == 0x63){        ///////// sb-type  
			uint8_t	imm1 = ((id.inst >> 8) & 0xf);        // imm[4:1]
			uint8_t imm2 = ((id.inst >> 25) & 0x3f)<<4;   // imm[10:5]
			uint8_t imm3 = ((id.inst >> 7) & 0x1)<<10;   // imm[11]
			uint8_t imm4 = ((id.inst >> 31) & 0x1)<<11;    //imm[12]
			imm12 = imm1 | imm2 | imm3 | imm4;
		}
		else if (opcode == 0x6f){            // jal, uj-type 
			uint32_t	imm1 = ((id.inst >> 12) & 0xff)<<11; // imm[19:12]
			uint32_t	imm2 = ((id.inst >> 20) &  0x1)<<10;// imm[11]
			uint32_t	imm3 = ((id.inst >> 21) & 0x3ff)<<1;// imm[10:1]
			uint32_t	imm4 = ((id.inst >> 31) & 0x1)<<19;// imm[20]
			imm20 = imm1 | imm2 | imm3 | imm4;
			}
			
		else if (opcode == 0x67){     /////////  jalr
			imm12 = (id.inst >> 20) & 0xfff;
		}
		else if (opcode == 0x37){           // lui	
			imm20 = (id.inst >> 12) & 0xfffff;  // imm[31:12]
		}
		else if (opcode == 0x17){           // auipc
			imm20 = (id.inst >> 12) & 0xfffff;  // imm[31:12]
		}
		else imm12 = 0;
		

		if(opcode == 0x03 || opcode == 0x67){  // load,  jalr imm12 -> imm32
			if (((int32_t)imm12 >> 11) &1== 1)	imm32 = (imm12 & 0xfff) | 0xfffff000;
			else							 	imm32 = imm12 & 0x00000fff;
		}	
		else if (opcode == 0x23){  // store imm12 -> imm32
			if (((int32_t)imm12 >> 11) &1== 1)	imm32 = (imm12 & 0xfff) | 0xfffff000;
			else 								imm32 = (imm12) & 0x00000fff;
		}
		
		else if (opcode == 0x013){                                 // i-type imm12 -> imm32
			if (funct3 == 0x03) 				imm32 = imm12 & 0x00000fff;  // unsigned extension
			else {
				if (((int32_t)imm12>> 11) &1== 1)  imm32 = ( imm12 & 0xfff) | 0xfffff000;
				else 							imm32 = imm12 & 0x00000fff;
			}
		}
		else if (opcode == 0x63){                               // sb-type imm12 -> imm32
			if (funct3 == 0x06 || funct3 == 0x07) imm32 = imm12 & 0x00000fff;  // unsigned extension
			else {
				if (((int32_t)imm12 >> 11) &1== 1) imm32 =  ( imm12 & 0xfff) | 0xfffff000;
				else 							imm32 = imm12 & 0x00000fff;
			}
		}
		else if (opcode == 0x6f){                                 // jal, imm20 -> imm32
			if (( (int32_t)imm20 >> 19) &1 == 1) imm32 = (imm20 & 0xfffff) | 0xfff00000;
			else 								 imm32 = imm20 & 0x000fffff;
		}           
		else if (opcode == 0x37) 				imm32 = imm20 << 12;            // lui, imm20-> imm32
		else if (opcode == 0x17) 				imm32 = imm20 << 12;            // auipc, imm20 -> imm32



		// computing branch target
		if (opcode == 0x67)		 pc_next_branch = mem.alu_result;  // jalr
		else if (opcode == 0x6f) {   // jal
			imm32_branch = imm32 << 1;
			pc_next_branch = id.pc + imm32_branch;
		}
		else if (opcode == 0x63) {  // sb-type
			imm32_branch = imm32 << 1;
			pc_next_branch = id.pc + imm32_branch;
		}
		
		
		// 5. hazard detection unit

		if (opcode == 0x23||opcode == 0x33||opcode == 0x13||opcode ==0x63||opcode == 0x67)		rs1 = ((id.inst >> 15) & 0x1f); 
		// load, store, r-type, i-type, sb-type, jalr
		else if (opcode == 0x03)																rs1 = ((id.inst >> 15) & 0x1f);
		else 																									rs1 =0;
		
		if (opcode == 0x23||opcode ==0x33||opcode ==0x63)						rs2 = ((id.inst >> 20) & 0x0001f); // store, r-type, sb-type
		else																	rs2 = 0;
		
		if (opcode == 0x03||opcode == 0x33|| opcode == 0x13|| opcode ==0x37||opcode == 0x17||opcode == 0x6f|| opcode == 0x67)	rd = ((id.inst >>7) & 0x000001f); // load, r-type, i-type, lui,auipc,jal,jalr
		else																													rd = 0; 
		
		stall_by_load_use = (ex.mem_read) && ((ex.rd == rs1) || (ex.rd == rs2)) ? 1 : 0;
		flush_by_branch = branch_taken;
		
		id_flush = flush_by_branch;
		id_stall = stall_by_load_use;
		
		if_flush = flush_by_branch;
		if_stall = stall_by_load_use;
		
		pc_write = !(if_stall || if_flush);
		
		
		// 6. register
		reg_in.rs1 = rs1;
		reg_in.rs2 = rs2;
		reg_in.rd  = wb.rd;
		//reg_in.rd_din = rd_din;
		reg_in.reg_write = wb.reg_write; 
		
		reg_out = regfile(reg_in, reg_data);   // get rs1_dout, rs2_dout	
		
		/*1******************************************************************************************************************************************/	
			
	/////*instruction fetch*/////
		
		// 2.program counter
		pc_next_plus4 = pc_curr + 4;
		pc_next_sel = branch_taken;
		pc_next = (pc_next_sel==1) ? pc_next_branch : pc_next_plus4;
		if (cc > 2 && (pc_write==1)) 		pc_curr = pc_next;
		
		// 1.instruction memory
		imem_addr = pc_curr >> 2;
		imem_in.imem_addr = imem_addr;
		imem_out = imem(imem_in, imem_data);
		inst = imem_out.inst;

		
		
//		printf("cc : %d		opcode : %x		imm32 : %d \n",cc,opcode,imm32);
//		printf("pc_curr : %d	pc_next : %d \n", pc_curr, pc_next);
//		printf("rs1 : %d 		rs2 : %d		rd : %d \n", reg_in.rs1, reg_in.rs2, reg_in.rd);
//		printf("rs1_dout : %d 		rs2_dout : %d\n", reg_out.rs1_dout, reg_out.rs2_dout );
//		printf("forward_a : %d 		foward_b : %d \n", forward_a, forward_b);
//		printf("alu_fwd_in1 : %d	alu_fwd_in2 : %d\n", alu_fwd_in1, alu_fwd_in2);
//		printf("alu_in1 : %d	alu_in2 : %d	alu_result: %d\n", alu_in.alu_in1, alu_in.alu_in2, alu_out.alu_result);
//		printf(" \n" );

		
		cc++;

		
	}

	printf("********** RISCV_PIPELINE_SIMULATOR ***********\n");
	for (i = 0; i < 31; i++) {
		printf("RF[%02d]: %016X \n", i, reg_data[i]);
	}
		
	for (i = 0; i < 9; i++) {
		printf( "DMEM[%02d]: %016X \n", i, dmem_data[i]);
	}

	free(reg_data);
	free(imem_data);
	free(dmem_data);

	return 1;
}
