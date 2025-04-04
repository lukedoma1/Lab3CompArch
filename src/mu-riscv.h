#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define FALSE 0
#define TRUE  1

/******************************************************************************/
/* RISCV memory layout                                                                                                                                      */
/******************************************************************************/
#define MEM_TEXT_BEGIN  0x00400000
#define MEM_TEXT_END      0x0FFFFFFF
/*Memory address 0x10000000 to 0x1000FFFF access by $gp*/
#define MEM_DATA_BEGIN  0x10010000
#define MEM_DATA_END   0x7FFFFFFF

#define MEM_KTEXT_BEGIN 0x80000000
#define MEM_KTEXT_END  0x8FFFFFFF

#define MEM_KDATA_BEGIN 0x90000000
#define MEM_KDATA_END  0xFFFEFFFF

/*stack and data segments occupy the same memory space. Stack grows backward (from higher address to lower address) */
#define MEM_STACK_BEGIN 0x7FFFFFFF
#define MEM_STACK_END  0x10010000

typedef struct {
	uint32_t begin, end;
	uint8_t *mem;
} mem_region_t;

/* memory will be dynamically allocated at initialization */
mem_region_t MEM_REGIONS[] = {
	{ MEM_TEXT_BEGIN, MEM_TEXT_END, NULL },
	{ MEM_DATA_BEGIN, MEM_DATA_END, NULL },
	{ MEM_KDATA_BEGIN, MEM_KDATA_END, NULL },
	{ MEM_KTEXT_BEGIN, MEM_KTEXT_END, NULL }
};

#define NUM_MEM_REGION 4
#define RISCV_REGS 32

typedef struct CPU_State_Struct {

  uint32_t PC;		                   /* program counter */
  uint32_t REGS[RISCV_REGS]; /* register file. */
  uint32_t HI, LO;                          /* special regs for mult/div. */
} CPU_State;

typedef struct CPU_Pipeline_Reg_Struct{
	uint32_t PC;
	uint32_t IR;
	uint32_t A;
	uint32_t B;
	uint32_t imm;
	uint32_t ALUOutput;
	uint32_t LMD;
	
} CPU_Pipeline_Reg;

/***************************************************************/
/* CPU State info.                                                                                                               */
/***************************************************************/

CPU_State CURRENT_STATE, NEXT_STATE;
int RUN_FLAG;	/* run flag*/
uint32_t INSTRUCTION_COUNT;
uint32_t CYCLE_COUNT;
uint32_t PROGRAM_SIZE; /*in words*/


/***************************************************************/
/* Pipeline Registers.                                                                                                        */
/***************************************************************/
CPU_Pipeline_Reg IF_ID;
CPU_Pipeline_Reg ID_EX;
CPU_Pipeline_Reg EX_MEM;
CPU_Pipeline_Reg MEM_WB;

char prog_file[32];

/***************************************************************/
/* Opcodes.                                                                                                        */
/***************************************************************/
#define R_OPCODE 0b0110011
#define IMM_ALU_OPCODE 0b0010011
#define LOAD_OPCODE 0b0000011
#define STORE_OPCODE 0b0100011
#define BRANCH_OPCODE 0b1100011
#define JUMP_OPCODE 0b1101111


/***************************************************************/
/* Bit Masks                                                                                                      */
/***************************************************************/
#define BIT_MASK_8 (0b11111111)
#define BIT_MASK_3 (BIT_MASK_8 >> 5)
#define BIT_MASK_5 (BIT_MASK_8 >> 3)
#define BIT_MASK_7 (BIT_MASK_8 >> 1)
#define BIT_MASK_12 ((BIT_MASK_8 << 4) | 0b1111)
#define BIT_MASK_16 ((BIT_MASK_8 << 8) | BIT_MASK_8)
#define BIT_MASK_20 (((BIT_MASK_8 << 12) | (BIT_MASK_8 << 8) | BIT_MASK_8))


/***************************************************************/
/* Instruction Macros                                                                                                    */
/***************************************************************/
#define GET_OPCODE(inst) ((inst) & BIT_MASK_7)
#define GET_FUNCT3(inst) (((inst) >> 12) & BIT_MASK_3)

/***************************************************************/
/* Data Hazard Help                                                                                                              */
/***************************************************************/
static int ENABLE_FORWARDING = 0;
static uint8_t previous_rd = 0;
static uint8_t double_previous_rd = 0;
static bool last_lw = false;
static bool double_last_lw = true;
static bool bubble = false;

/***************************************************************/
/* Function Declerations.                                                                                                */
/***************************************************************/
void help();
uint32_t mem_read_32(uint32_t address);
void mem_write_32(uint32_t address, uint32_t value);
void cycle();
void run(int num_cycles);
void runAll();
void mdump(uint32_t start, uint32_t stop) ;
void rdump();
void handle_command();
void reset();
void init_memory();
void load_program();
void handle_pipeline();
void DetectHazardsAndForward();
void WB();
void MEM();
void EX();
void ID();
void IF();
void show_pipeline();/*IMPLEMENT THIS*/
void initialize();
void print_program();

// print helpers
void print_instruction(uint32_t);
void print_command(uint32_t);
void handle_r_print(uint32_t bincmd);
void handle_s_print(uint32_t bincmd);
void handle_i_print(uint32_t bincmd);
void handle_b_print(uint32_t bincmd);
void handle_j_print(uint32_t bincmd);
void print_r_cmd(char* cmd_name, uint8_t rd, uint8_t rs1, uint8_t rs2);
void print_s_cmd(char* cmd_name, uint8_t rs2, uint8_t offset, uint8_t rs1);
void print_i_type1_cmd(char* cmd_name, uint8_t rd, uint8_t rs1, uint16_t imm);
void print_i_type2_cmd(char* cmd_name, uint8_t rd, uint8_t rs1, uint16_t imm);
void print_b_cmd(char* cmd_name, uint8_t rs1, uint8_t rs2, uint16_t imm);

