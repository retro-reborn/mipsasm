#ifndef MIPSASM_H
#define MIPSASM_H

#include <stdint.h>

// Maximum assembly file size
#define MAX_ASM_SIZE 8192
#define MAX_OUTPUT_SIZE 4096
#define MAX_LABELS 256
#define MAX_LINE_LENGTH 256

// MIPS instruction types
typedef enum {
  INST_UNKNOWN = 0,
  INST_LUI,
  INST_LI,
  INST_ADDIU,
  INST_SW,
  INST_LW,
  INST_BNEZ,
  INST_BEQZ,
  INST_BEQ,
  INST_BNE,
  INST_B,
  INST_J,
  INST_JAL,
  INST_NOP,
  INST_ANDI,
  INST_ORI,
  INST_XORI,
  INST_ADDI,
  INST_ADD,
  INST_SUB,
  INST_AND,
  INST_OR,
  INST_XOR,
  INST_SLL,
  INST_SRL,
  INST_SRA,
  INST_SLLV,
  INST_SRLV,
  INST_SRAV,
  INST_SLT,
  INST_SLTI,
  INST_SLTU,
  INST_SLTIU,
  INST_JR,
  INST_JALR,
  INST_MFHI,
  INST_MFLO,
  INST_MTHI,
  INST_MTLO,
  INST_MULT,
  INST_MULTU,
  INST_DIV,
  INST_DIVU,
  INST_SYSCALL,
  INST_BREAK,
  INST_LB,
  INST_LBU,
  INST_LH,
  INST_LHU,
  INST_SB,
  INST_SH,
  INST_LA,
  INST_MOVE,
  INST_LABEL,
  INST_DIRECTIVE
} instruction_type_t;

// MIPS register mapping
typedef enum {
  REG_ZERO = 0,
  REG_AT = 1,
  REG_V0 = 2,
  REG_V1 = 3,
  REG_A0 = 4,
  REG_A1 = 5,
  REG_A2 = 6,
  REG_A3 = 7,
  REG_T0 = 8,
  REG_T1 = 9,
  REG_T2 = 10,
  REG_T3 = 11,
  REG_T4 = 12,
  REG_T5 = 13,
  REG_T6 = 14,
  REG_T7 = 15,
  REG_S0 = 16,
  REG_S1 = 17,
  REG_S2 = 18,
  REG_S3 = 19,
  REG_S4 = 20,
  REG_S5 = 21,
  REG_S6 = 22,
  REG_S7 = 23,
  REG_T8 = 24,
  REG_T9 = 25,
  REG_K0 = 26,
  REG_K1 = 27,
  REG_GP = 28,
  REG_SP = 29,
  REG_FP = 30,
  REG_RA = 31
} mips_register_t;

// Label structure
typedef struct {
  char name[64];
  uint32_t address;
  int resolved;
} label_t;

// Section types
typedef enum { SECTION_TEXT, SECTION_DATA } section_type_t;

// Assembler context
typedef struct {
  uint8_t *output;
  size_t output_size;
  uint32_t current_address;
  uint32_t text_address; // Starting address of text section
  uint32_t data_address; // Starting address of data section
  uint32_t text_size;    // Size of text section
  uint32_t data_size;    // Size of data section
  section_type_t current_section;
  label_t labels[MAX_LABELS];
  int label_count;
  int pass; // 1 for first pass (collect labels), 2 for second pass (resolve)
} assembler_ctx_t;

// Function prototypes
int mips_assemble(const char *source, uint8_t **output, size_t *output_size,
                  int verbose);
int parse_register(const char *reg_str);
uint32_t encode_r_type(uint8_t op, uint8_t rs, uint8_t rt, uint8_t rd,
                       uint8_t shamt, uint8_t func);
uint32_t encode_i_type(uint8_t op, uint8_t rs, uint8_t rt, uint16_t imm);
uint32_t encode_j_type(uint8_t op, uint32_t target);
instruction_type_t parse_instruction(const char *mnemonic);
int add_label(assembler_ctx_t *ctx, const char *name, uint32_t address);
int find_label(assembler_ctx_t *ctx, const char *name);
int parse_immediate(const char *str, uint32_t *value);
void handle_directive(assembler_ctx_t *ctx, const char *directive,
                      char **saveptr);
void estimate_directive_size(assembler_ctx_t *ctx, const char *directive);
void write_be32(assembler_ctx_t *ctx, uint32_t value);
int write_binary_file(const char *filename, const uint8_t *data, size_t size);

#endif // MIPSASM_H
