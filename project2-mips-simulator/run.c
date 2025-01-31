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
instruction* get_inst_info(uint32_t pc) 
{ 
    return &INST_INFO[(pc - MEM_TEXT_START) >> 2];
}

/***************************************************************/
/*                                                             */
/* Procedure: process_instruction                              */
/*                                                             */
/* Purpose: Process one instrction                             */
/*                                                             */
/***************************************************************/
void process_instruction(){
	/* Implement this function */
    instruction* instr = get_inst_info(CURRENT_STATE.PC);
    CURRENT_STATE.PC += 4;                  // PC of the next instruction
    unsigned char rs, rt, rd, shamt;
    short funct, imm;
    uint32_t target_jump;
    if(OPCODE(instr) == 0) {
		// R-type : addu, and, jr, nor, or, sltu, sll, srl, subu
		rs = RS(instr), rt = RT(instr), rd = RD(instr), shamt = SHAMT(instr), funct = FUNC(instr);
        if(funct == 33) {                   // addu : R[rd] = R[rs] + R[rt]
            CURRENT_STATE.REGS[rd] = CURRENT_STATE.REGS[rs] + CURRENT_STATE.REGS[rt];
        } else if(funct == 36) {            // and : R[rd] = R[rs] & R[rt]
            CURRENT_STATE.REGS[rd] = CURRENT_STATE.REGS[rs] & CURRENT_STATE.REGS[rt];
        } else if(funct == 8) {             // jr : PC = R[rs]
            CURRENT_STATE.PC = CURRENT_STATE.REGS[rs];
        } else if(funct == 39) {            // nor : R[rd] = ~(R[rs] | R[rt])
            CURRENT_STATE.REGS[rd] = ~(CURRENT_STATE.REGS[rs] | CURRENT_STATE.REGS[rt]);
        } else if(funct == 37) {            // or : R[rd] = (R[rs] | R[rt])
            CURRENT_STATE.REGS[rd] = (CURRENT_STATE.REGS[rs] | CURRENT_STATE.REGS[rt]);
        } else if(funct == 43) {            // sltu : R[rd] = (R[rs] < R[rt]) ? 1 : 0
            CURRENT_STATE.REGS[rd] = (CURRENT_STATE.REGS[rs] < CURRENT_STATE.REGS[rt]) ? 1 : 0;
        } else if(funct == 0) {             // sll : R[rd] = R[rt] << shamt
            CURRENT_STATE.REGS[rd] = CURRENT_STATE.REGS[rt] << shamt;
        } else if(funct == 2) {             // srl : R[rd] = R[rt] >> shamt
            CURRENT_STATE.REGS[rd] = CURRENT_STATE.REGS[rt] >> shamt;
        } else if(funct == 35) {            // subu : R[rd] = R[rs] - R[rt]
            CURRENT_STATE.REGS[rd] = CURRENT_STATE.REGS[rs] - CURRENT_STATE.REGS[rt];
        } else {                            // not a R-type instruction
            printf("Error : Not a R-type instruction.\n");
            exit(0);
        }
	} else if(OPCODE(instr) == 2 || OPCODE(instr) == 3) {
		// J-type : j, jal
        if(OPCODE(instr) == 2) {            // j : PC = JumpAddr
            CURRENT_STATE.PC = (TARGET(instr) << 2) | ((CURRENT_STATE.PC >> 28) << 28);
        } else if(OPCODE(instr) == 3) {     // jal : R[31] = PC + 4; (ignore delay slot) PC = JumpAddr
            CURRENT_STATE.REGS[31] = CURRENT_STATE.PC;
            CURRENT_STATE.PC = (TARGET(instr) << 2) | ((CURRENT_STATE.PC >> 28) << 28);
        } else {                            // not a J-type instruction
            // can't be here
            printf("Error : Not a J-type instruction.\n");
            exit(0);
        }
	} else {
		// I-type : addiu, andi, beq, bne, lui, lw, ori, sltiu, sw
		rs = RS(instr), rt = RT(instr), imm = IMM(instr);
        int32_t SignExtImm = (int32_t) imm;
        uint32_t ZeroExtImm = (uint32_t)(unsigned short) imm;
        if(OPCODE(instr) == 9) {            // addiu : R[rt] = R[rs] + SignExtImm
            CURRENT_STATE.REGS[rt] = CURRENT_STATE.REGS[rs] + SignExtImm;
        } else if(OPCODE(instr) == 12) {    // andi : R[rt] = R[rs] & ZeroExtImm
            CURRENT_STATE.REGS[rt] = CURRENT_STATE.REGS[rs] & ZeroExtImm;
        } else if(OPCODE(instr) == 4) {     // beq : if(R[rs] == R[rt]) PC = PC + 4 + BranchAddr
            if(CURRENT_STATE.REGS[rs] == CURRENT_STATE.REGS[rt]) {
                CURRENT_STATE.PC += SignExtImm << 2;
            }
        } else if(OPCODE(instr) == 5) {     // bne : if(R[rs] != R[rt]) PC = PC + 4 + BranchAddr
            if(CURRENT_STATE.REGS[rs] != CURRENT_STATE.REGS[rt]) {
                CURRENT_STATE.PC += SignExtImm << 2;
            }
        } else if(OPCODE(instr) == 15) {    // lui : R[rt] = {imm, 16'b0}
            CURRENT_STATE.REGS[rt] = imm << 16;
        } else if(OPCODE(instr) == 35) {    // lw : R[rt] = M[R[rs] + SignExtImm]
            CURRENT_STATE.REGS[rt] = mem_read_32(CURRENT_STATE.REGS[rs] + SignExtImm);
        } else if(OPCODE(instr) == 13) {    // ori : R[rt] = R[rs] | ZeroExtImm
            CURRENT_STATE.REGS[rt] = CURRENT_STATE.REGS[rs] | ZeroExtImm;
        } else if(OPCODE(instr) == 11) {    // sltiu : R[rt] = (R[rs] < SignExtImm) ? 1 : 0
            CURRENT_STATE.REGS[rt] = (CURRENT_STATE.REGS[rs] < SignExtImm) ? 1 : 0;
        } else if(OPCODE(instr) == 43) {    // sw : M[R[rs] + SignExtImm] = R[rt]
            mem_write_32(CURRENT_STATE.REGS[rs] + SignExtImm, CURRENT_STATE.REGS[rt]);
        } else {                            // not a I-type instruction
            printf("Error : Not a I-type instruction.\n");
            exit(0);
        }
	}

    RUN_BIT = (((CURRENT_STATE.PC - MEM_TEXT_START) >> 2) < NUM_INST) ? TRUE : FALSE;
}