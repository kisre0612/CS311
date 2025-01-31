/***************************************************************/
/*                                                             */
/*   MIPS-32 Instruction Level Simulator                       */
/*                                                             */
/*   CS311 KAIST                                               */
/*   parse.c                                                   */
/*                                                             */
/***************************************************************/

#include <stdio.h>

#include "run.h"
#include "util.h"
#include "parse.h"

int text_size;
int data_size;

instruction parsing_instr(const char *buffer, const int index)
{
    instruction* instr = &INST_INFO[index];
	/* Implement this function */
	// called for every instruction in the input file and converts 
	// them into the 'instruction' type which is defined in util.h
	uint32_t word = fromBinary((char *) buffer);
	uint32_t addr_start = MEM_TEXT_START + index;

	mem_write_32(addr_start, word);
	SET_OPCODE(instr, word >> 26);
	if(OPCODE(instr) == 0) { 									// R-type instruction
		// addu, and, jr, nor, or, sltu, sll, srl, subu
		SET_RS(instr, (word & 0x03E00000) >> 21);
		SET_RT(instr, (word & 0x001F0000) >> 16);
		SET_RD(instr, (word & 0x0000F800) >> 11);
		SET_SHAMT(instr, (word & 0x000007C0) >> 6);
		SET_FUNC(instr, word & 0x0000003F);
	} else if(OPCODE(instr) == 2 || OPCODE(instr) == 3) {		// J-type instruction
		// j, jal
		instr->r_t.target = word & 0x03FFFFFF;
	} else {													// I-type instruction
		// addiu, andi, beq, bne, lui, lw, ori, sltiu, sw
		SET_RS(instr, (word & 0x03E00000) >> 21);
		SET_RT(instr, (word & 0x001F0000) >> 16);
		SET_IMM(instr, word & 0x0000FFFF);
	}

    return *instr;
}

void parsing_data(const char *buffer, const int index)
{
	/* Implement this function */
	// called for every data field in the input file and need to fill
	// the data into the 'simulated memory', use mem_read_32, mem_write_32
	uint32_t word = fromBinary((char *) buffer);
	uint32_t addr_start = MEM_DATA_START + index;
	mem_write_32(addr_start, word);
}

void print_parse_result()
{
    int i;
    printf("Instruction Information\n");

    for(i = 0; i < text_size/4; i++)
    {
	printf("INST_INFO[%d].value : %x\n",i, INST_INFO[i].value);
	printf("INST_INFO[%d].opcode : %d\n",i, INST_INFO[i].opcode);

	switch(INST_INFO[i].opcode)
	{
	    //Type I
	    case 0x9:		//(0x001001)ADDIU
	    case 0xc:		//(0x001100)ANDI
	    case 0xf:		//(0x001111)LUI	
	    case 0xd:		//(0x001101)ORI
	    case 0xb:		//(0x001011)SLTIU
	    case 0x23:		//(0x100011)LW	
	    case 0x2b:		//(0x101011)SW
	    case 0x4:		//(0x000100)BEQ
	    case 0x5:		//(0x000101)BNE
		printf("INST_INFO[%d].rs : %d\n",i, INST_INFO[i].r_t.r_i.rs);
		printf("INST_INFO[%d].rt : %d\n",i, INST_INFO[i].r_t.r_i.rt);
		printf("INST_INFO[%d].imm : %d\n",i, INST_INFO[i].r_t.r_i.r_i.imm);
		break;

    	    //TYPE R
	    case 0x0:		//(0x000000)ADDU, AND, NOR, OR, SLTU, SLL, SRL, SUBU  if JR
		printf("INST_INFO[%d].func_code : %d\n",i, INST_INFO[i].func_code);
		printf("INST_INFO[%d].rs : %d\n",i, INST_INFO[i].r_t.r_i.rs);
		printf("INST_INFO[%d].rt : %d\n",i, INST_INFO[i].r_t.r_i.rt);
		printf("INST_INFO[%d].rd : %d\n",i, INST_INFO[i].r_t.r_i.r_i.r.rd);
		printf("INST_INFO[%d].shamt : %d\n",i, INST_INFO[i].r_t.r_i.r_i.r.shamt);
		break;

    	    //TYPE J
	    case 0x2:		//(0x000010)J
	    case 0x3:		//(0x000011)JAL
		printf("INST_INFO[%d].target : %d\n",i, INST_INFO[i].r_t.target);
		break;

	    default:
		printf("Not available instruction\n");
		assert(0);
	}
    }

    printf("Memory Dump - Text Segment\n");
    for(i = 0; i < text_size; i+=4)
	printf("text_seg[%d] : %x\n", i, mem_read_32(MEM_TEXT_START + i));
    for(i = 0; i < data_size; i+=4)
	printf("data_seg[%d] : %x\n", i, mem_read_32(MEM_DATA_START + i));
    printf("Current PC: %x\n", CURRENT_STATE.PC);
}
