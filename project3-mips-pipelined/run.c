/***************************************************************/
/*                                                             */
/*   MIPS-32 Instruction Level Simulator                       */
/*                                                             */
/*   CS311 KAIST                                               */
/*   run.c                                                     */
/*                                                             */
/***************************************************************/

#include <stdio.h>

#include "util.h"
#include "run.h"


/***************************************************************/
/*                                                             */
/* Procedure: get_inst_info                                    */
/*                                                             */
/* Purpose: Read insturction information                       */
/*                                                             */
/***************************************************************/
instruction* get_inst_info(uint32_t pc) { 
    return &INST_INFO[(pc - MEM_TEXT_START) >> 2];
}


/***************************************************************/
/*                                                             */
/* Procedure: process_instruction                              */
/*                                                             */
/* Purpose: Process one instrction                             */
/*                                                             */
/* TODO: Implement 5-stage pipeplined MIPS simulator           */
/*                                                             */
/***************************************************************/
void process_instruction(){
	/** Your implementation here */
	WB_Stage();
	MEM_Stage();
	EX_Stage();
	ID_Stage();
	IF_Stage();
}

void IF_Stage(){
	if(!check_run_bit(IF_STAGE)) return;
	if(CURRENT_STATE.MEM_WB_FORWARD_REG) {
		CURRENT_STATE.PC = CURRENT_STATE.IF_PC + 4;
		CURRENT_STATE.IF_PC = 0;
		return;
	}	
	if(CURRENT_STATE.IF_ID_INST) {
		CURRENT_STATE.PIPE[IF_STAGE] = 0;
		CURRENT_STATE.IF_ID_INST = 0;
		CURRENT_STATE.IF_ID_NPC = 0;
		return;
	}
	if(CURRENT_STATE.JUMP_PC) {
		CURRENT_STATE.IF_ID_NPC = 0;
		CURRENT_STATE.PIPE[IF_STAGE] = CURRENT_STATE.PC;
		CURRENT_STATE.PC = CURRENT_STATE.JUMP_PC;
		CURRENT_STATE.JUMP_PC = 0;
		return;
	}
	CURRENT_STATE.PIPE[IF_STAGE] = CURRENT_STATE.PC;
	CURRENT_STATE.IF_ID_NPC = CURRENT_STATE.PC;
	if(!CURRENT_STATE.IF_PC) CURRENT_STATE.PC = CURRENT_STATE.PC + 4;
}

void ID_Stage() {
	if(!check_run_bit(ID_STAGE)) return;
	if(CURRENT_STATE.MEM_WB_FORWARD_REG) return;
	instruction *inst = get_inst_info(CURRENT_STATE.IF_ID_NPC);
	uint32_t opcode = OPCODE(inst); 
	uint32_t rs = RS(inst);
	uint32_t rt = RT(inst);
	uint32_t rd = RD(inst);
	uint32_t imm = IMM(inst);
	uint32_t shamt = SHAMT(inst);
	uint32_t func = FUNC(inst);
	CURRENT_STATE.PIPE[ID_STAGE] = CURRENT_STATE.IF_ID_NPC;
	CURRENT_STATE.ID_EX_NPC = CURRENT_STATE.IF_ID_NPC;
	CURRENT_STATE.ID_EX_OPCODE = opcode;
	CURRENT_STATE.ID_EX_RS = rs;
	CURRENT_STATE.ID_EX_RT = rt;
	CURRENT_STATE.ID_EX_RD = rd;
	CURRENT_STATE.ID_EX_SHAMT = shamt;
	CURRENT_STATE.ID_EX_IMM = imm;
	CURRENT_STATE.ID_EX_FUNC = func;
	if(opcode == 0x2 || opcode == 0x3) {									// j, jal (J-type)
		CURRENT_STATE.JUMP_PC = TARGET(inst) * 4;
		CURRENT_STATE.ID_EX_DEST = 1;
		if(opcode == 0x3) CURRENT_STATE.REGS[31] = CURRENT_STATE.PC;		// CURRENT_STATE.PC incremented already.
	} else if(opcode == 0x0 && func == 0x8) {								// jr (R-type)
		CURRENT_STATE.JUMP_PC = CURRENT_STATE.REGS[RS(inst)];
		CURRENT_STATE.ID_EX_DEST = 1;
	} else CURRENT_STATE.ID_EX_DEST = 0;									// other inst.
}

void EX_Stage() {
	if(!check_run_bit(EX_STAGE)) return;
	instruction *inst = get_inst_info(CURRENT_STATE.ID_EX_NPC);				// get instruction for execution
	uint32_t r1 = CURRENT_STATE.REGS[CURRENT_STATE.ID_EX_RS];
	uint32_t r2 = CURRENT_STATE.REGS[CURRENT_STATE.ID_EX_RT]; 
	CURRENT_STATE.PIPE[EX_STAGE] = CURRENT_STATE.ID_EX_NPC;
	CURRENT_STATE.EX_MEM_NPC = CURRENT_STATE.ID_EX_NPC;
	
	if(CURRENT_STATE.MEM_WB_FORWARD_REG) CURRENT_STATE.MEM_WB_FORWARD_REG = 0;
	else {
		if(CURRENT_STATE.forward_signal) {									// forward?
			if(CURRENT_STATE.EX_MEM_FORWARD_REG && CURRENT_STATE.ID_EX_RS == RD(inst)) r1 = CURRENT_STATE.EX_MEM_ALU_OUT;
			if(CURRENT_STATE.EX_MEM_FORWARD_REG && CURRENT_STATE.ID_EX_RT == RD(inst)) r2 = CURRENT_STATE.EX_MEM_ALU_OUT;
			if(CURRENT_STATE.ID_EX_RS == CURRENT_STATE.MEM_WB_DEST) {
				r1 = CURRENT_STATE.MEM_WB_MEM_OUT;
				CURRENT_STATE.MEM_WB_FORWARD_REG = 1;
				CURRENT_STATE.PIPE[EX_STAGE] = 0;
				CURRENT_STATE.EX_MEM_NPC = 0;
				return;
			}	
			if(CURRENT_STATE.ID_EX_RT == CURRENT_STATE.MEM_WB_DEST) {
				r2 = CURRENT_STATE.MEM_WB_MEM_OUT;
				CURRENT_STATE.MEM_WB_FORWARD_REG = 1;
				CURRENT_STATE.PIPE[EX_STAGE] = 0;
				CURRENT_STATE.EX_MEM_NPC = 0;
				return;
			}
		} else {
			if(CURRENT_STATE.EX_MEM_FORWARD_REG && CURRENT_STATE.ID_EX_RS == CURRENT_STATE.EX_MEM_DEST) r1 = CURRENT_STATE.EX_MEM_ALU_OUT;
			if(CURRENT_STATE.EX_MEM_FORWARD_REG && CURRENT_STATE.ID_EX_RT == CURRENT_STATE.EX_MEM_DEST) r2 = CURRENT_STATE.EX_MEM_ALU_OUT;
		}
	}
	operation(r1, r2);														// ALU operation
}

void MEM_Stage() {
	if(!check_run_bit(MEM_STAGE)) return;
	instruction *inst = get_inst_info(CURRENT_STATE.EX_MEM_NPC);			// get inst.
	CURRENT_STATE.PIPE[MEM_STAGE] = CURRENT_STATE.EX_MEM_NPC;
	CURRENT_STATE.MEM_WB_NPC = CURRENT_STATE.EX_MEM_NPC;
	CURRENT_STATE.MEM_WB_ALU_OUT = CURRENT_STATE.EX_MEM_ALU_OUT;
	CURRENT_STATE.forward_signal = 0;
	
	// store word, load word
	if(OPCODE(inst) == 0x2B) mem_write_32(CURRENT_STATE.EX_MEM_ALU_OUT, CURRENT_STATE.EX_MEM_W_VALUE);
	if(OPCODE(inst) == 0x23) {
		CURRENT_STATE.MEM_WB_MEM_OUT = mem_read_32(CURRENT_STATE.EX_MEM_ALU_OUT);
		CURRENT_STATE.forward_signal = 1;
	}
	
	if(CURRENT_STATE.EX_MEM_BR_TAKE) {
		flush_pipeline();													// if branch is taken, flush the pipeline
		CURRENT_STATE.PC = CURRENT_STATE.EX_MEM_BR_TARGET;
	}
	
	CURRENT_STATE.MEM_WB_DEST = CURRENT_STATE.EX_MEM_DEST;
	CURRENT_STATE.MEM_WB_BR_TAKE = CURRENT_STATE.EX_MEM_BR_TAKE;
	CURRENT_STATE.EX_MEM_BR_TAKE = 0;
}

void WB_Stage() {
	if(!check_run_bit(WB_STAGE)) return;
	instruction* inst = get_inst_info(CURRENT_STATE.MEM_WB_NPC);			// get inst. for write back
	uint32_t opcode = OPCODE(inst);
	uint32_t func = FUNC(inst);
	CURRENT_STATE.PIPE[WB_STAGE] = CURRENT_STATE.MEM_WB_NPC;
	switch(opcode) {
		case 0:
			if(func == 0x8) break;
			CURRENT_STATE.REGS[CURRENT_STATE.MEM_WB_DEST] = CURRENT_STATE.MEM_WB_ALU_OUT;
			break;
		case 2:
			break;
		case 3:
			break;
		case 4:
			break;
		case 5:
			break;
		case 35:
			CURRENT_STATE.REGS[CURRENT_STATE.MEM_WB_DEST] = CURRENT_STATE.MEM_WB_MEM_OUT;
			break;
		case 43:
			break;
		default:
			CURRENT_STATE.REGS[CURRENT_STATE.MEM_WB_DEST] = CURRENT_STATE.MEM_WB_ALU_OUT;
	}
	if ((CURRENT_STATE.EX_MEM_NPC - MEM_TEXT_START >= 4*NUM_INST) && (CURRENT_STATE.EX_MEM_NPC != 0)) RUN_BIT = FALSE;
	INSTRUCTION_COUNT++;
}   

int check_run_bit(int stage) {
	if(stage == 0) {
		if(CURRENT_STATE.PC - MEM_TEXT_START >= 4 * NUM_INST) {
			CURRENT_STATE.IF_ID_NPC = CURRENT_STATE.PC;
			CURRENT_STATE.PIPE[IF_STAGE] = 0;
			return 0;
		} else return 1;
	} else if(stage == 1) {
		if(CURRENT_STATE.IF_ID_NPC - MEM_TEXT_START >= 4 * NUM_INST || !CURRENT_STATE.IF_ID_NPC) {
			CURRENT_STATE.ID_EX_NPC = CURRENT_STATE.IF_ID_NPC;
			CURRENT_STATE.PIPE[ID_STAGE] = 0;
			return 0;
		} else return 1;
	} else if(stage == 2) {
		if(CURRENT_STATE.ID_EX_NPC - MEM_TEXT_START >= 4 * NUM_INST || !CURRENT_STATE.ID_EX_NPC) {
			CURRENT_STATE.EX_MEM_NPC = CURRENT_STATE.ID_EX_NPC;
			CURRENT_STATE.PIPE[EX_STAGE] = 0;
			return 0;
		} else return 1;
	} else if(stage == 3) {
		if(CURRENT_STATE.EX_MEM_NPC - MEM_TEXT_START >= 4 * NUM_INST || !CURRENT_STATE.EX_MEM_NPC) {
			CURRENT_STATE.MEM_WB_NPC = CURRENT_STATE.EX_MEM_NPC;
			CURRENT_STATE.PIPE[MEM_STAGE] = 0;
			return 0;
		} else return 1;
	} else if(stage == 4) {
		if(!CURRENT_STATE.MEM_WB_NPC) {
			CURRENT_STATE.PIPE[WB_STAGE] = 0;
			return 0;
		} else return 1;
	} else return 1;
}

void flush_pipeline() {														// flush IF, ID, EX stages
	CURRENT_STATE.PIPE[IF_STAGE] = 0;
	CURRENT_STATE.PIPE[ID_STAGE] = 0;
	CURRENT_STATE.PIPE[EX_STAGE] = 0;
	CURRENT_STATE.IF_ID_INST = 1;
	CURRENT_STATE.IF_ID_NPC = 0;
	CURRENT_STATE.ID_EX_NPC = 0;
	CURRENT_STATE.EX_MEM_NPC = 0;
}

void operation(uint32_t reg1, uint32_t reg2) {
	instruction *inst = get_inst_info(CURRENT_STATE.ID_EX_NPC);				// ALU operation with inst.
	uint32_t opcode = CURRENT_STATE.ID_EX_OPCODE;
	uint32_t imm = CURRENT_STATE.ID_EX_IMM;
	uint32_t shamt = CURRENT_STATE.ID_EX_SHAMT;
	uint32_t func = CURRENT_STATE.ID_EX_FUNC;
	switch(opcode) {
		// R-type: addu, and, jr, nor, or, sltu, sll, srl, subu
		case 0x0:
			switch(func) {
				case 0x21:													// addu: R[rd] = R[rs] + R[rt]
					CURRENT_STATE.EX_MEM_FORWARD_REG = 1;
					CURRENT_STATE.EX_MEM_ALU_OUT = reg1 + reg2;
					CURRENT_STATE.EX_MEM_DEST = RD(inst);
					break;
				case 0x24:													// and : R[rd] = R[rs] & R[rt]
					CURRENT_STATE.EX_MEM_FORWARD_REG = 1;
					CURRENT_STATE.EX_MEM_ALU_OUT = reg1 & reg2;
					CURRENT_STATE.EX_MEM_DEST = RD(inst);
					break;
				case 0x8:													// jr: PC = R[rs]
					CURRENT_STATE.EX_MEM_FORWARD_REG = 0;
					break;
				case 0x27:													// nor: R[rd] = ~(R[rs] | R[rt])
					CURRENT_STATE.EX_MEM_FORWARD_REG = 1;
					CURRENT_STATE.EX_MEM_ALU_OUT = ~(reg1 | reg2);
					CURRENT_STATE.EX_MEM_DEST = RD(inst);
					break;
				case 0x25:													// or: R[rd] = (R[rs] | R[rt])
					CURRENT_STATE.EX_MEM_FORWARD_REG = 1;
					CURRENT_STATE.EX_MEM_ALU_OUT = reg1 | reg2;
					CURRENT_STATE.EX_MEM_DEST = RD(inst);
					break;
				case 0x2B:													// sltu: R[rd] = (R[rs] < R[rt]) ? 1 : 0
					CURRENT_STATE.EX_MEM_FORWARD_REG = 1;
					CURRENT_STATE.EX_MEM_DEST = RD(inst);
					if (reg1 < reg2) CURRENT_STATE.EX_MEM_ALU_OUT = 1;	
					else CURRENT_STATE.EX_MEM_ALU_OUT = 0;
					break;
				case 0x0:													// sll: R[rd] = R[rt] << shamt
					CURRENT_STATE.EX_MEM_FORWARD_REG = 1;
					CURRENT_STATE.EX_MEM_ALU_OUT = reg2 << shamt;
					CURRENT_STATE.EX_MEM_DEST = RD(inst);
					break;
				case 0x2:													// srl: R[rd] = R[rt] >> shamt
					CURRENT_STATE.EX_MEM_FORWARD_REG = 1;
					CURRENT_STATE.EX_MEM_ALU_OUT = reg2 >> shamt;
					CURRENT_STATE.EX_MEM_DEST = RD(inst);
					break;
				case 0x23:													// subu: R[rd] = R[rs] - R[rt]
					CURRENT_STATE.EX_MEM_FORWARD_REG = 1;
					CURRENT_STATE.EX_MEM_ALU_OUT = reg1 - reg2;
					CURRENT_STATE.EX_MEM_DEST = RD(inst);
					break;
				default:
					printf("Error: not an appropriate instruction.\n");		// can't be here
					exit(0);
				}
				break;
		// J-type: j, jal
		case 0x2:															// j: PC = JumpAddr
			CURRENT_STATE.EX_MEM_FORWARD_REG = 0;
			break;
		case 0x3:															// jal: R[31] = PC + 4; (ignore delay slot) PC = JumpAddr
			CURRENT_STATE.EX_MEM_FORWARD_REG = 0;
			break;
		// I-type: addiu, andi, beq, bne, lui, lw, ori, sltiu, sw
		case 0x9:															// addiu: R[rt] = R[rs] + SignExtImm
			CURRENT_STATE.EX_MEM_FORWARD_REG = 1;
			CURRENT_STATE.EX_MEM_ALU_OUT = reg1 + imm;
			CURRENT_STATE.EX_MEM_DEST = RT(inst);
			break;
		case 0xC:															// andi: R[rt] = R[rs] & ZeroExtImm
			CURRENT_STATE.EX_MEM_FORWARD_REG = 1;
			CURRENT_STATE.EX_MEM_ALU_OUT = reg1 & imm;
			CURRENT_STATE.EX_MEM_DEST = RT(inst);
			break;
		case 0x4:															// beq: if(R[rs] == R[rt]) PC = PC + 4 + BranchAddr
			CURRENT_STATE.EX_MEM_FORWARD_REG = 0;
			if (reg1 == reg2) {
				CURRENT_STATE.EX_MEM_BR_TARGET = CURRENT_STATE.ID_EX_NPC + 4 + imm * 4;
				CURRENT_STATE.EX_MEM_BR_TAKE = 1;
			}
			break;
		case 0x5:															// bne: if(R[rs] != R[rt]) PC = PC + 4 + BranchAddr
			CURRENT_STATE.EX_MEM_FORWARD_REG = 0;
			if (reg1 != reg2) {
				CURRENT_STATE.EX_MEM_BR_TARGET = CURRENT_STATE.ID_EX_NPC + imm * 4 + 4;
				CURRENT_STATE.EX_MEM_BR_TAKE = 1;
			}
			break;
		case 0xF:															// lui: R[rt] = {imm, 16'b0}
			CURRENT_STATE.EX_MEM_FORWARD_REG = 1;
			CURRENT_STATE.EX_MEM_ALU_OUT = ((uint32_t) imm) << 16;
			CURRENT_STATE.EX_MEM_DEST = RT(inst);
			break;
		case 0x23:															// lw: R[rt] = M[R[rs] + SignExtImm]
			CURRENT_STATE.EX_MEM_FORWARD_REG = 1;
			CURRENT_STATE.EX_MEM_ALU_OUT = reg1 + imm;
			CURRENT_STATE.EX_MEM_DEST = RT(inst);
			instruction* instr = get_inst_info(CURRENT_STATE.IF_ID_NPC);
			if(CURRENT_STATE.EX_MEM_DEST == RS(instr) || CURRENT_STATE.EX_MEM_DEST == RT(instr)) CURRENT_STATE.IF_PC = CURRENT_STATE.PC;
			break;
		case 0xD:															// ori: R[rt] = R[rs] | ZeroExtImm
			CURRENT_STATE.EX_MEM_FORWARD_REG = 1;
			CURRENT_STATE.EX_MEM_ALU_OUT = reg1 | imm;
			CURRENT_STATE.EX_MEM_DEST = RT(inst);
			break;
		case 0xB:															// sltiu : R[rt] = (R[rs] < SignExtImm) ? 1 : 0
			CURRENT_STATE.EX_MEM_FORWARD_REG = 1;
			CURRENT_STATE.EX_MEM_DEST = RT(inst);
			if (reg1 < ((uint32_t) imm)) CURRENT_STATE.EX_MEM_ALU_OUT = 1;
			else CURRENT_STATE.EX_MEM_ALU_OUT = 0;
			break;
		case 0x2B:															// sw : M[R[rs] + SignExtImm] = R[rt]
			CURRENT_STATE.EX_MEM_FORWARD_REG = 0;
			CURRENT_STATE.EX_MEM_ALU_OUT = reg1 + imm;
			CURRENT_STATE.EX_MEM_W_VALUE = reg2;
			break;
		default:
			printf("Error: not an appropriate instruction.\n");				// can't be here
			exit(0);
	}
}