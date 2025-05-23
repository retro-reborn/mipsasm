#include "mipsasm.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int is_verbose = 0;

// Parse register name and return register number
int parse_register(const char *reg_str) {
  if (!reg_str)
    return -1;

  // Handle $ prefix
  if (reg_str[0] == '$') {
    reg_str++;
  }

  // Numeric register ($0-$31)
  if (isdigit(reg_str[0])) {
    int reg_num = atoi(reg_str);
    if (reg_num >= 0 && reg_num <= 31) {
      return reg_num;
    }
    return -1;
  }

  // Named registers
  static const struct {
    const char *name;
    int reg_num;
  } reg_table[] = {
      {"zero", REG_ZERO}, {"at", REG_AT}, {"v0", REG_V0}, {"v1", REG_V1},
      {"a0", REG_A0},     {"a1", REG_A1}, {"a2", REG_A2}, {"a3", REG_A3},
      {"t0", REG_T0},     {"t1", REG_T1}, {"t2", REG_T2}, {"t3", REG_T3},
      {"t4", REG_T4},     {"t5", REG_T5}, {"t6", REG_T6}, {"t7", REG_T7},
      {"s0", REG_S0},     {"s1", REG_S1}, {"s2", REG_S2}, {"s3", REG_S3},
      {"s4", REG_S4},     {"s5", REG_S5}, {"s6", REG_S6}, {"s7", REG_S7},
      {"t8", REG_T8},     {"t9", REG_T9}, {"k0", REG_K0}, {"k1", REG_K1},
      {"gp", REG_GP},     {"sp", REG_SP}, {"fp", REG_FP}, {"ra", REG_RA}};

  for (size_t i = 0; i < sizeof(reg_table) / sizeof(reg_table[0]); i++) {
    if (strcmp(reg_str, reg_table[i].name) == 0) {
      return reg_table[i].reg_num;
    }
  }

  return -1;
}

// Parse instruction mnemonic
instruction_type_t parse_instruction(const char *mnemonic) {
  if (!mnemonic)
    return INST_UNKNOWN;

  static const struct {
    const char *name;
    instruction_type_t type;
  } inst_table[] = {{"lui", INST_LUI},
                    {"li", INST_LI},
                    {"addiu", INST_ADDIU},
                    {"addi", INST_ADDI},
                    {"sw", INST_SW},
                    {"lw", INST_LW},
                    {"bnez", INST_BNEZ},
                    {"beqz", INST_BEQZ},
                    {"beq", INST_BEQ},
                    {"bne", INST_BNE},
                    {"b", INST_B},
                    {"j", INST_J},
                    {"jal", INST_JAL},
                    {"nop", INST_NOP},
                    {"andi", INST_ANDI},
                    {"ori", INST_ORI},
                    {"xori", INST_XORI},
                    {"add", INST_ADD},
                    {"sub", INST_SUB},
                    {"and", INST_AND},
                    {"or", INST_OR},
                    {"xor", INST_XOR},
                    {"sll", INST_SLL},
                    {"srl", INST_SRL},
                    {"sra", INST_SRA},
                    {"sllv", INST_SLLV},
                    {"srlv", INST_SRLV},
                    {"srav", INST_SRAV},
                    {"slt", INST_SLT},
                    {"sltu", INST_SLTU},
                    {"jr", INST_JR},
                    {"jalr", INST_JALR},
                    {"mfhi", INST_MFHI},
                    {"mflo", INST_MFLO},
                    {"mthi", INST_MTHI},
                    {"mtlo", INST_MTLO},
                    {"mult", INST_MULT},
                    {"multu", INST_MULTU},
                    {"div", INST_DIV},
                    {"divu", INST_DIVU},
                    {"syscall", INST_SYSCALL},
                    {"break", INST_BREAK},
                    {"slti", INST_SLTI},
                    {"sltiu", INST_SLTIU},
                    {"lb", INST_LB},
                    {"lbu", INST_LBU},
                    {"lh", INST_LH},
                    {"lhu", INST_LHU},
                    {"sb", INST_SB},
                    {"sh", INST_SH},
                    {"la", INST_LA},
                    {"move", INST_MOVE}};

  for (size_t i = 0; i < sizeof(inst_table) / sizeof(inst_table[0]); i++) {
    if (strcmp(mnemonic, inst_table[i].name) == 0) {
      return inst_table[i].type;
    }
  }

  return INST_UNKNOWN;
}

// Parse immediate value (hex, decimal, or label)
int parse_immediate(const char *str, uint32_t *value) {
  if (!str || !value)
    return 0;

  char *endptr;

  // Hex value (0x...)
  if (strncmp(str, "0x", 2) == 0 || strncmp(str, "0X", 2) == 0) {
    *value = strtoul(str, &endptr, 16);
    return (*endptr == '\0');
  }

  // Decimal value
  if (isdigit(str[0]) || str[0] == '-') {
    long long_val = strtol(str, &endptr, 10);
    *value = (uint32_t)long_val;
    return (*endptr == '\0');
  }

  // Label (will be resolved later)
  return 0;
}

// Encode R-type instruction
uint32_t encode_r_type(uint8_t op, uint8_t rs, uint8_t rt, uint8_t rd,
                       uint8_t shamt, uint8_t func) {
  return ((uint32_t)op << 26) | ((uint32_t)rs << 21) | ((uint32_t)rt << 16) |
         ((uint32_t)rd << 11) | ((uint32_t)shamt << 6) | (uint32_t)func;
}

// Encode I-type instruction
uint32_t encode_i_type(uint8_t op, uint8_t rs, uint8_t rt, uint16_t imm) {
  return ((uint32_t)op << 26) | ((uint32_t)rs << 21) | ((uint32_t)rt << 16) |
         (uint32_t)imm;
}

// Encode J-type instruction
uint32_t encode_j_type(uint8_t op, uint32_t target) {
  return ((uint32_t)op << 26) | (target & 0x3FFFFFF);
}

// Add label to context
int add_label(assembler_ctx_t *ctx, const char *name, uint32_t address) {
  if (ctx->label_count >= MAX_LABELS)
    return 0;

  strncpy(ctx->labels[ctx->label_count].name, name,
          sizeof(ctx->labels[ctx->label_count].name) - 1);
  ctx->labels[ctx->label_count]
      .name[sizeof(ctx->labels[ctx->label_count].name) - 1] = '\0';
  ctx->labels[ctx->label_count].address = address;
  ctx->labels[ctx->label_count].resolved = 1;

  if (is_verbose) {
    printf("Adding label '%s' at address 0x%08X (section: %s)\n", name, address,
           (ctx->current_section == SECTION_TEXT) ? "TEXT" : "DATA");
  }
  ctx->label_count++;

  return 1;
}

// Find label by name
int find_label(assembler_ctx_t *ctx, const char *name) {
  for (int i = 0; i < ctx->label_count; i++) {
    if (strcmp(ctx->labels[i].name, name) == 0) {
      return i;
    }
  }
  return -1;
}

// Write 32-bit big-endian value to output
void write_be32(assembler_ctx_t *ctx, uint32_t value) {
  if (ctx->output_size + 4 <= MAX_OUTPUT_SIZE) {
    ctx->output[ctx->output_size++] = (value >> 24) & 0xFF;
    ctx->output[ctx->output_size++] = (value >> 16) & 0xFF;
    ctx->output[ctx->output_size++] = (value >> 8) & 0xFF;
    ctx->output[ctx->output_size++] = value & 0xFF;
    ctx->current_address += 4;

    // Track section size
    if (ctx->current_section == SECTION_TEXT) {
      ctx->text_size += 4;
    } else {
      ctx->data_size += 4;
    }
  }
} // Process a single line of assembly
static int process_line(assembler_ctx_t *ctx, const char *line) {
  char line_copy[MAX_LINE_LENGTH];
  char *token, *saveptr;
  uint32_t instruction = 0;

  // Copy line and remove comments
  strncpy(line_copy, line, sizeof(line_copy) - 1);
  line_copy[sizeof(line_copy) - 1] = '\0';

  char *comment = strstr(line_copy, "//");
  if (comment)
    *comment = '\0';

  // Also handle hash comments (#)
  comment = strchr(line_copy, '#');
  if (comment)
    *comment = '\0';

  // Skip empty lines and whitespace
  char *trimmed = line_copy;
  while (isspace(*trimmed))
    trimmed++;
  if (*trimmed == '\0')
    return 1;

  // Check for label
  char *colon = strchr(trimmed, ':');
  if (colon) {
    // Store the label
    char label_name[64];
    strncpy(label_name, trimmed, colon - trimmed);
    label_name[colon - trimmed] = '\0';

    // Remove leading/trailing whitespace from label
    char *label_trim = label_name;
    while (isspace(*label_trim))
      label_trim++;

    // Add the label with the current address (which depends on the current
    // section)
    if (ctx->pass == 1) {
      add_label(ctx, label_trim, ctx->current_address);
    }
    // Move past the label for instruction processing
    trimmed = colon + 1;
    while (isspace(*trimmed))
      trimmed++;
    if (*trimmed == '\0')
      return 1;
  }

  // Skip directives (starts with .)
  if (trimmed[0] == '.') {
    // Extract the directive name
    char directive_copy[MAX_LINE_LENGTH];
    strncpy(directive_copy, trimmed + 1, sizeof(directive_copy) - 1);
    directive_copy[sizeof(directive_copy) - 1] = '\0';

    // Get just the directive name without params
    char *directive_name = strtok_r(directive_copy, " \t", &saveptr);
    if (directive_name) {
      // Always process section changes in both passes
      if (strcmp(directive_name, "text") == 0) {
        if (ctx->pass == 1) {
          if (is_verbose) {
            printf("Switching to TEXT section\n");
          }
          // In pass 1, remember where we are in each section
          if (ctx->current_section == SECTION_DATA) {
            ctx->data_size = ctx->current_address - ctx->data_address;
          }
        }

        ctx->current_section = SECTION_TEXT;
        if (ctx->pass == 1) {
          ctx->current_address = ctx->text_address + ctx->text_size;
        } else if (ctx->pass == 2) {
          ctx->current_address = ctx->text_address + ctx->text_size;
        }
        return 1;
      } else if (strcmp(directive_name, "data") == 0) {
        if (ctx->pass == 1) {
          if (is_verbose) {
            printf("Switching to DATA section\n");
          }
          // In pass 1, remember where we are in each section
          if (ctx->current_section == SECTION_TEXT) {
            ctx->text_size = ctx->current_address - ctx->text_address;
          }
        }

        ctx->current_section = SECTION_DATA;
        if (ctx->pass == 1) {
          ctx->current_address = ctx->data_address + ctx->data_size;
        } else if (ctx->pass == 2) {
          ctx->current_address = ctx->data_address + ctx->data_size;
        }
        return 1;
      }
    }

    if (ctx->pass == 1) {
      // In pass 1, estimate directive sizes
      estimate_directive_size(ctx, trimmed + 1);
    } else {
      // Handle directives in pass 2
      handle_directive(ctx, trimmed + 1, &saveptr);
    }
    return 1;
  }

  // Parse instruction
  token = strtok_r(trimmed, " \t,", &saveptr);
  if (!token)
    return 1;

  instruction_type_t inst_type = parse_instruction(token);

  if (ctx->pass == 2) { // Only generate code on second pass
    switch (inst_type) {
    case INST_NOP:
      instruction = 0x00000000;
      write_be32(ctx, instruction);
      break;

    case INST_LUI: {
      char *rt_str = strtok_r(NULL, " \t,", &saveptr);
      char *imm_str = strtok_r(NULL, " \t,", &saveptr);

      int rt = parse_register(rt_str);
      uint32_t imm;

      if (rt < 0)
        return 0;

      if (parse_immediate(imm_str, &imm)) {
        instruction = encode_i_type(0x0F, 0, rt, imm & 0xFFFF);
      } else {
        // Try to resolve as label
        int label_idx = find_label(ctx, imm_str);
        if (label_idx >= 0) {
          uint32_t addr = ctx->labels[label_idx].address;
          instruction = encode_i_type(0x0F, 0, rt, (addr >> 16) & 0xFFFF);
        } else {
          return 0;
        }
      }
      write_be32(ctx, instruction);
      break;
    }

    case INST_LI: {
      // li is a pseudo-instruction, expand to lui + ori if needed
      char *rt_str = strtok_r(NULL, " \t,", &saveptr);
      char *imm_str = strtok_r(NULL, " \t,", &saveptr);

      int rt = parse_register(rt_str);
      uint32_t imm;

      if (rt < 0)
        return 0;

      if (parse_immediate(imm_str, &imm)) {
        if (imm <= 0xFFFF) {
          // Small immediate, use ori with $zero
          instruction = encode_i_type(0x0D, 0, rt, imm & 0xFFFF);
          write_be32(ctx, instruction);
        } else {
          // Large immediate, use lui + ori
          instruction = encode_i_type(0x0F, 0, rt, (imm >> 16) & 0xFFFF);
          write_be32(ctx, instruction);
          if ((imm & 0xFFFF) != 0) {
            instruction = encode_i_type(0x0D, rt, rt, imm & 0xFFFF);
            write_be32(ctx, instruction);
          }
        }
      } else {
        return 0;
      }
      break;
    }

    case INST_ADDIU: {
      char *rt_str = strtok_r(NULL, " \t,", &saveptr);
      char *rs_str = strtok_r(NULL, " \t,", &saveptr);
      char *imm_str = strtok_r(NULL, " \t,", &saveptr);

      int rt = parse_register(rt_str);
      int rs = parse_register(rs_str);
      uint32_t imm;

      if (rt < 0 || rs < 0)
        return 0;

      if (parse_immediate(imm_str, &imm)) {
        instruction = encode_i_type(0x09, rs, rt, imm & 0xFFFF);
        write_be32(ctx, instruction);
      } else {
        return 0;
      }
      break;
    }

    case INST_SW: {
      char *rt_str = strtok_r(NULL, " \t,", &saveptr);
      char *offset_base = strtok_r(NULL, " \t,", &saveptr);

      int rt = parse_register(rt_str);
      if (rt < 0)
        return 0;

      // Parse offset(base) format
      char *paren = strchr(offset_base, '(');
      if (paren) {
        *paren = '\0';
        char *base_str = paren + 1;
        char *end_paren = strchr(base_str, ')');
        if (end_paren)
          *end_paren = '\0';

        int rs = parse_register(base_str);
        uint32_t offset;

        if (rs < 0)
          return 0;

        if (parse_immediate(offset_base, &offset)) {
          instruction = encode_i_type(0x2B, rs, rt, offset & 0xFFFF);
          write_be32(ctx, instruction);
        } else {
          return 0;
        }
      } else {
        return 0;
      }
      break;
    }

    case INST_BNEZ: {
      char *rs_str = strtok_r(NULL, " \t,", &saveptr);
      char *label_str = strtok_r(NULL, " \t,", &saveptr);

      int rs = parse_register(rs_str);
      if (rs < 0)
        return 0;

      int label_idx = find_label(ctx, label_str);
      if (label_idx >= 0) {
        int32_t offset = (int32_t)(ctx->labels[label_idx].address -
                                   (ctx->current_address + 4)) /
                         4;
        instruction = encode_i_type(0x05, rs, 0, offset & 0xFFFF);
        write_be32(ctx, instruction);
      } else {
        return 0;
      }
      break;
    }

    case INST_B: {
      char *label_str = strtok_r(NULL, " \t,", &saveptr);

      int label_idx = find_label(ctx, label_str);
      if (label_idx >= 0) {
        int32_t offset = (int32_t)(ctx->labels[label_idx].address -
                                   (ctx->current_address + 4)) /
                         4;
        instruction = encode_i_type(0x04, 0, 0, offset & 0xFFFF);
        write_be32(ctx, instruction);
      } else {
        return 0;
      }
      break;
    }

    case INST_ANDI: {
      char *rt_str = strtok_r(NULL, " \t,", &saveptr);
      char *rs_str = strtok_r(NULL, " \t,", &saveptr);
      char *imm_str = strtok_r(NULL, " \t,", &saveptr);

      int rt = parse_register(rt_str);
      int rs = parse_register(rs_str);
      uint32_t imm;

      if (rt < 0 || rs < 0)
        return 0;

      if (parse_immediate(imm_str, &imm)) {
        instruction = encode_i_type(0x0C, rs, rt, imm & 0xFFFF);
        write_be32(ctx, instruction);
      } else {
        return 0;
      }
      break;
    }

    case INST_BEQ: {
      char *rs_str = strtok_r(NULL, " \t,", &saveptr);
      char *rt_str = strtok_r(NULL, " \t,", &saveptr);
      char *label_str = strtok_r(NULL, " \t,", &saveptr);

      int rs = parse_register(rs_str);
      int rt = parse_register(rt_str);

      if (rs < 0 || rt < 0)
        return 0;

      int label_idx = find_label(ctx, label_str);
      if (label_idx >= 0) {
        int32_t offset = (int32_t)(ctx->labels[label_idx].address -
                                   (ctx->current_address + 4)) /
                         4;
        instruction = encode_i_type(0x04, rs, rt, offset & 0xFFFF);
        write_be32(ctx, instruction);
      } else {
        return 0;
      }
      break;
    }

    case INST_BNE: {
      char *rs_str = strtok_r(NULL, " \t,", &saveptr);
      char *rt_str = strtok_r(NULL, " \t,", &saveptr);
      char *label_str = strtok_r(NULL, " \t,", &saveptr);

      int rs = parse_register(rs_str);
      int rt = parse_register(rt_str);

      if (rs < 0 || rt < 0)
        return 0;

      int label_idx = find_label(ctx, label_str);
      if (label_idx >= 0) {
        int32_t offset = (int32_t)(ctx->labels[label_idx].address -
                                   (ctx->current_address + 4)) /
                         4;
        instruction = encode_i_type(0x05, rs, rt, offset & 0xFFFF);
        write_be32(ctx, instruction);
      } else {
        return 0;
      }
      break;
    }

    case INST_BEQZ: {
      char *rs_str = strtok_r(NULL, " \t,", &saveptr);
      char *label_str = strtok_r(NULL, " \t,", &saveptr);

      int rs = parse_register(rs_str);
      if (rs < 0)
        return 0;

      int label_idx = find_label(ctx, label_str);
      if (label_idx >= 0) {
        int32_t offset = (int32_t)(ctx->labels[label_idx].address -
                                   (ctx->current_address + 4)) /
                         4;
        instruction = encode_i_type(0x04, rs, 0, offset & 0xFFFF);
        write_be32(ctx, instruction);
      } else {
        return 0;
      }
      break;
    }

    case INST_J: {
      char *label_str = strtok_r(NULL, " \t,", &saveptr);

      int label_idx = find_label(ctx, label_str);
      if (label_idx >= 0) {
        uint32_t target = ctx->labels[label_idx].address >> 2;
        instruction = encode_j_type(0x02, target);
        write_be32(ctx, instruction);
      } else {
        return 0;
      }
      break;
    }

    case INST_JAL: {
      char *label_str = strtok_r(NULL, " \t,", &saveptr);

      int label_idx = find_label(ctx, label_str);
      if (label_idx >= 0) {
        uint32_t target = ctx->labels[label_idx].address >> 2;
        instruction = encode_j_type(0x03, target);
        write_be32(ctx, instruction);
      } else {
        return 0;
      }
      break;
    }

    case INST_LW: {
      char *rt_str = strtok_r(NULL, " \t,", &saveptr);
      char *offset_base = strtok_r(NULL, " \t,", &saveptr);

      int rt = parse_register(rt_str);
      if (rt < 0)
        return 0;

      // Parse offset(base) format
      char *paren = strchr(offset_base, '(');
      if (paren) {
        *paren = '\0';
        char *base_str = paren + 1;
        char *end_paren = strchr(base_str, ')');
        if (end_paren)
          *end_paren = '\0';

        int rs = parse_register(base_str);
        if (rs < 0)
          return 0;

        uint32_t offset;
        if (parse_immediate(offset_base, &offset)) {
          instruction = encode_i_type(0x23, rs, rt, offset & 0xFFFF);
          write_be32(ctx, instruction);
        } else {
          return 0;
        }
      } else {
        return 0;
      }
      break;
    }

    // R-type instructions
    case INST_ADD: {
      char *rd_str = strtok_r(NULL, " \t,", &saveptr);
      char *rs_str = strtok_r(NULL, " \t,", &saveptr);
      char *rt_str = strtok_r(NULL, " \t,", &saveptr);

      int rd = parse_register(rd_str);
      int rs = parse_register(rs_str);
      int rt = parse_register(rt_str);

      if (rd < 0 || rs < 0 || rt < 0)
        return 0;

      instruction = encode_r_type(0, rs, rt, rd, 0, 0x20);
      write_be32(ctx, instruction);
      break;
    }

    case INST_SUB: {
      char *rd_str = strtok_r(NULL, " \t,", &saveptr);
      char *rs_str = strtok_r(NULL, " \t,", &saveptr);
      char *rt_str = strtok_r(NULL, " \t,", &saveptr);

      int rd = parse_register(rd_str);
      int rs = parse_register(rs_str);
      int rt = parse_register(rt_str);

      if (rd < 0 || rs < 0 || rt < 0)
        return 0;

      instruction = encode_r_type(0, rs, rt, rd, 0, 0x22);
      write_be32(ctx, instruction);
      break;
    }

    case INST_AND: {
      char *rd_str = strtok_r(NULL, " \t,", &saveptr);
      char *rs_str = strtok_r(NULL, " \t,", &saveptr);
      char *rt_str = strtok_r(NULL, " \t,", &saveptr);

      int rd = parse_register(rd_str);
      int rs = parse_register(rs_str);
      int rt = parse_register(rt_str);

      if (rd < 0 || rs < 0 || rt < 0)
        return 0;

      instruction = encode_r_type(0, rs, rt, rd, 0, 0x24);
      write_be32(ctx, instruction);
      break;
    }

    case INST_OR: {
      char *rd_str = strtok_r(NULL, " \t,", &saveptr);
      char *rs_str = strtok_r(NULL, " \t,", &saveptr);
      char *rt_str = strtok_r(NULL, " \t,", &saveptr);

      int rd = parse_register(rd_str);
      int rs = parse_register(rs_str);
      int rt = parse_register(rt_str);

      if (rd < 0 || rs < 0 || rt < 0)
        return 0;

      instruction = encode_r_type(0, rs, rt, rd, 0, 0x25);
      write_be32(ctx, instruction);
      break;
    }

    case INST_XOR: {
      char *rd_str = strtok_r(NULL, " \t,", &saveptr);
      char *rs_str = strtok_r(NULL, " \t,", &saveptr);
      char *rt_str = strtok_r(NULL, " \t,", &saveptr);

      int rd = parse_register(rd_str);
      int rs = parse_register(rs_str);
      int rt = parse_register(rt_str);

      if (rd < 0 || rs < 0 || rt < 0)
        return 0;

      instruction = encode_r_type(0, rs, rt, rd, 0, 0x26);
      write_be32(ctx, instruction);
      break;
    }

    case INST_SLL: {
      char *rd_str = strtok_r(NULL, " \t,", &saveptr);
      char *rt_str = strtok_r(NULL, " \t,", &saveptr);
      char *sa_str = strtok_r(NULL, " \t,", &saveptr);

      int rd = parse_register(rd_str);
      int rt = parse_register(rt_str);
      uint32_t sa;

      if (rd < 0 || rt < 0)
        return 0;
      if (!parse_immediate(sa_str, &sa) || sa > 31)
        return 0;

      instruction = encode_r_type(0, 0, rt, rd, sa, 0);
      write_be32(ctx, instruction);
      break;
    }

    case INST_SRL: {
      char *rd_str = strtok_r(NULL, " \t,", &saveptr);
      char *rt_str = strtok_r(NULL, " \t,", &saveptr);
      char *sa_str = strtok_r(NULL, " \t,", &saveptr);

      int rd = parse_register(rd_str);
      int rt = parse_register(rt_str);
      uint32_t sa;

      if (rd < 0 || rt < 0)
        return 0;
      if (!parse_immediate(sa_str, &sa) || sa > 31)
        return 0;

      instruction = encode_r_type(0, 0, rt, rd, sa, 0x02);
      write_be32(ctx, instruction);
      break;
    }

    case INST_SRA: {
      char *rd_str = strtok_r(NULL, " \t,", &saveptr);
      char *rt_str = strtok_r(NULL, " \t,", &saveptr);
      char *sa_str = strtok_r(NULL, " \t,", &saveptr);

      int rd = parse_register(rd_str);
      int rt = parse_register(rt_str);
      uint32_t sa;

      if (rd < 0 || rt < 0)
        return 0;
      if (!parse_immediate(sa_str, &sa) || sa > 31)
        return 0;

      instruction = encode_r_type(0, 0, rt, rd, sa, 0x03);
      write_be32(ctx, instruction);
      break;
    }

    case INST_JR: {
      char *rs_str = strtok_r(NULL, " \t,", &saveptr);

      int rs = parse_register(rs_str);
      if (rs < 0)
        return 0;

      instruction = encode_r_type(0, rs, 0, 0, 0, 0x08);
      write_be32(ctx, instruction);
      break;
    }

    case INST_JALR: {
      char *rs_str = strtok_r(NULL, " \t,", &saveptr);
      char *rd_str = strtok_r(NULL, " \t,", &saveptr);

      int rs = parse_register(rs_str);
      int rd =
          (rd_str) ? parse_register(rd_str) : 31; // default to $ra if no rd

      if (rs < 0 || rd < 0)
        return 0;

      instruction = encode_r_type(0, rs, 0, rd, 0, 0x09);
      write_be32(ctx, instruction);
      break;
    }

    case INST_SYSCALL: {
      instruction = encode_r_type(0, 0, 0, 0, 0, 0x0C);
      write_be32(ctx, instruction);
      break;
    }

    case INST_BREAK: {
      char *code_str = strtok_r(NULL, " \t,", &saveptr);
      uint32_t code = 0;

      if (code_str && !parse_immediate(code_str, &code))
        return 0;

      instruction = encode_r_type(0, 0, 0, 0, 0, 0x0D);
      instruction |= (code & 0xFFFFF) << 6;
      write_be32(ctx, instruction);
      break;
    }

    case INST_MOVE: {
      // Pseudo-instruction: move $rd, $rs = addu $rd, $rs, $zero
      char *rd_str = strtok_r(NULL, " \t,", &saveptr);
      char *rs_str = strtok_r(NULL, " \t,", &saveptr);

      int rd = parse_register(rd_str);
      int rs = parse_register(rs_str);

      if (rd < 0 || rs < 0)
        return 0;

      instruction = encode_r_type(0, rs, 0, rd, 0, 0x21); // ADDU
      write_be32(ctx, instruction);
      break;
    }

    case INST_LA: {
      // Pseudo-instruction: la $rt, label => lui $rt, upper(label) + ori $rt,
      // $rt, lower(label)
      char *rt_str = strtok_r(NULL, " \t,", &saveptr);
      char *label_str = strtok_r(NULL, " \t,", &saveptr);

      int rt = parse_register(rt_str);
      if (rt < 0)
        return 0;

      int label_idx = find_label(ctx, label_str);
      if (label_idx >= 0) {
        uint32_t addr = ctx->labels[label_idx].address;

        if (is_verbose) {
          printf("  Loading address of label '%s': 0x%08X\n", label_str, addr);
        }
        uint16_t upper = (addr >> 16) & 0xFFFF;
        uint16_t lower = addr & 0xFFFF;

        // LUI rt, upper
        instruction = encode_i_type(0x0F, 0, rt, upper);
        write_be32(ctx, instruction);

        // ORI rt, rt, lower (only if lower != 0)
        if (lower != 0) {
          instruction = encode_i_type(0x0D, rt, rt, lower);
          write_be32(ctx, instruction);
        }
      } else {
        printf("  ERROR: Label '%s' not found\n", label_str);
        return 0;
      }
      break;
    }

    case INST_LB: {
      char *rt_str = strtok_r(NULL, " \t,", &saveptr);
      char *offset_base = strtok_r(NULL, " \t,", &saveptr);

      int rt = parse_register(rt_str);
      if (rt < 0)
        return 0;

      // Parse offset(base) format
      char *paren = strchr(offset_base, '(');
      if (paren) {
        *paren = '\0';
        char *base_str = paren + 1;
        char *end_paren = strchr(base_str, ')');
        if (end_paren)
          *end_paren = '\0';

        int rs = parse_register(base_str);
        if (rs < 0)
          return 0;

        uint32_t offset;
        if (parse_immediate(offset_base, &offset)) {
          instruction = encode_i_type(0x20, rs, rt, offset & 0xFFFF);
          write_be32(ctx, instruction);
        } else {
          return 0;
        }
      } else {
        return 0;
      }
      break;
    }

    case INST_LBU: {
      char *rt_str = strtok_r(NULL, " \t,", &saveptr);
      char *offset_base = strtok_r(NULL, " \t,", &saveptr);

      int rt = parse_register(rt_str);
      if (rt < 0)
        return 0;

      // Parse offset(base) format
      char *paren = strchr(offset_base, '(');
      if (paren) {
        *paren = '\0';
        char *base_str = paren + 1;
        char *end_paren = strchr(base_str, ')');
        if (end_paren)
          *end_paren = '\0';

        int rs = parse_register(base_str);
        if (rs < 0)
          return 0;

        uint32_t offset;
        if (parse_immediate(offset_base, &offset)) {
          instruction = encode_i_type(0x24, rs, rt, offset & 0xFFFF);
          write_be32(ctx, instruction);
        } else {
          return 0;
        }
      } else {
        return 0;
      }
      break;
    }

    case INST_LH: {
      char *rt_str = strtok_r(NULL, " \t,", &saveptr);
      char *offset_base = strtok_r(NULL, " \t,", &saveptr);

      int rt = parse_register(rt_str);
      if (rt < 0)
        return 0;

      // Parse offset(base) format
      char *paren = strchr(offset_base, '(');
      if (paren) {
        *paren = '\0';
        char *base_str = paren + 1;
        char *end_paren = strchr(base_str, ')');
        if (end_paren)
          *end_paren = '\0';

        int rs = parse_register(base_str);
        if (rs < 0)
          return 0;

        uint32_t offset;
        if (parse_immediate(offset_base, &offset)) {
          instruction = encode_i_type(0x21, rs, rt, offset & 0xFFFF);
          write_be32(ctx, instruction);
        } else {
          return 0;
        }
      } else {
        return 0;
      }
      break;
    }

    case INST_LHU: {
      char *rt_str = strtok_r(NULL, " \t,", &saveptr);
      char *offset_base = strtok_r(NULL, " \t,", &saveptr);

      int rt = parse_register(rt_str);
      if (rt < 0)
        return 0;

      // Parse offset(base) format
      char *paren = strchr(offset_base, '(');
      if (paren) {
        *paren = '\0';
        char *base_str = paren + 1;
        char *end_paren = strchr(base_str, ')');
        if (end_paren)
          *end_paren = '\0';

        int rs = parse_register(base_str);
        if (rs < 0)
          return 0;

        uint32_t offset;
        if (parse_immediate(offset_base, &offset)) {
          instruction = encode_i_type(0x25, rs, rt, offset & 0xFFFF);
          write_be32(ctx, instruction);
        } else {
          return 0;
        }
      } else {
        return 0;
      }
      break;
    }

    case INST_SB: {
      char *rt_str = strtok_r(NULL, " \t,", &saveptr);
      char *offset_base = strtok_r(NULL, " \t,", &saveptr);

      int rt = parse_register(rt_str);
      if (rt < 0)
        return 0;

      // Parse offset(base) format
      char *paren = strchr(offset_base, '(');
      if (paren) {
        *paren = '\0';
        char *base_str = paren + 1;
        char *end_paren = strchr(base_str, ')');
        if (end_paren)
          *end_paren = '\0';

        int rs = parse_register(base_str);
        if (rs < 0)
          return 0;

        uint32_t offset;
        if (parse_immediate(offset_base, &offset)) {
          instruction = encode_i_type(0x28, rs, rt, offset & 0xFFFF);
          write_be32(ctx, instruction);
        } else {
          return 0;
        }
      } else {
        return 0;
      }
      break;
    }

    case INST_SH: {
      char *rt_str = strtok_r(NULL, " \t,", &saveptr);
      char *offset_base = strtok_r(NULL, " \t,", &saveptr);

      int rt = parse_register(rt_str);
      if (rt < 0)
        return 0;

      // Parse offset(base) format
      char *paren = strchr(offset_base, '(');
      if (paren) {
        *paren = '\0';
        char *base_str = paren + 1;
        char *end_paren = strchr(base_str, ')');
        if (end_paren)
          *end_paren = '\0';

        int rs = parse_register(base_str);
        if (rs < 0)
          return 0;

        uint32_t offset;
        if (parse_immediate(offset_base, &offset)) {
          instruction = encode_i_type(0x29, rs, rt, offset & 0xFFFF);
          write_be32(ctx, instruction);
        } else {
          return 0;
        }
      } else {
        return 0;
      }
      break;
    }

    case INST_SLTI: {
      char *rt_str = strtok_r(NULL, " \t,", &saveptr);
      char *rs_str = strtok_r(NULL, " \t,", &saveptr);
      char *imm_str = strtok_r(NULL, " \t,", &saveptr);

      int rt = parse_register(rt_str);
      int rs = parse_register(rs_str);
      uint32_t imm;

      if (rt < 0 || rs < 0)
        return 0;

      if (parse_immediate(imm_str, &imm)) {
        instruction = encode_i_type(0x0A, rs, rt, imm & 0xFFFF);
        write_be32(ctx, instruction);
      } else {
        return 0;
      }
      break;
    }

    case INST_SLTIU: {
      char *rt_str = strtok_r(NULL, " \t,", &saveptr);
      char *rs_str = strtok_r(NULL, " \t,", &saveptr);
      char *imm_str = strtok_r(NULL, " \t,", &saveptr);

      int rt = parse_register(rt_str);
      int rs = parse_register(rs_str);
      uint32_t imm;

      if (rt < 0 || rs < 0)
        return 0;

      if (parse_immediate(imm_str, &imm)) {
        instruction = encode_i_type(0x0B, rs, rt, imm & 0xFFFF);
        write_be32(ctx, instruction);
      } else {
        return 0;
      }
      break;
    }

    case INST_SLT: {
      char *rd_str = strtok_r(NULL, " \t,", &saveptr);
      char *rs_str = strtok_r(NULL, " \t,", &saveptr);
      char *rt_str = strtok_r(NULL, " \t,", &saveptr);

      int rd = parse_register(rd_str);
      int rs = parse_register(rs_str);
      int rt = parse_register(rt_str);

      if (rd < 0 || rs < 0 || rt < 0)
        return 0;

      instruction = encode_r_type(0, rs, rt, rd, 0, 0x2A);
      write_be32(ctx, instruction);
      break;
    }

    case INST_SLTU: {
      char *rd_str = strtok_r(NULL, " \t,", &saveptr);
      char *rs_str = strtok_r(NULL, " \t,", &saveptr);
      char *rt_str = strtok_r(NULL, " \t,", &saveptr);

      int rd = parse_register(rd_str);
      int rs = parse_register(rs_str);
      int rt = parse_register(rt_str);

      if (rd < 0 || rs < 0 || rt < 0)
        return 0;

      instruction = encode_r_type(0, rs, rt, rd, 0, 0x2B);
      write_be32(ctx, instruction);
      break;
    }

    case INST_MULT: {
      char *rs_str = strtok_r(NULL, " \t,", &saveptr);
      char *rt_str = strtok_r(NULL, " \t,", &saveptr);

      int rs = parse_register(rs_str);
      int rt = parse_register(rt_str);

      if (rs < 0 || rt < 0)
        return 0;

      instruction = encode_r_type(0, rs, rt, 0, 0, 0x18);
      write_be32(ctx, instruction);
      break;
    }

    case INST_MULTU: {
      char *rs_str = strtok_r(NULL, " \t,", &saveptr);
      char *rt_str = strtok_r(NULL, " \t,", &saveptr);

      int rs = parse_register(rs_str);
      int rt = parse_register(rt_str);

      if (rs < 0 || rt < 0)
        return 0;

      instruction = encode_r_type(0, rs, rt, 0, 0, 0x19);
      write_be32(ctx, instruction);
      break;
    }

    case INST_DIV: {
      char *rs_str = strtok_r(NULL, " \t,", &saveptr);
      char *rt_str = strtok_r(NULL, " \t,", &saveptr);

      int rs = parse_register(rs_str);
      int rt = parse_register(rt_str);

      if (rs < 0 || rt < 0)
        return 0;

      instruction = encode_r_type(0, rs, rt, 0, 0, 0x1A);
      write_be32(ctx, instruction);
      break;
    }

    case INST_DIVU: {
      char *rs_str = strtok_r(NULL, " \t,", &saveptr);
      char *rt_str = strtok_r(NULL, " \t,", &saveptr);

      int rs = parse_register(rs_str);
      int rt = parse_register(rt_str);

      if (rs < 0 || rt < 0)
        return 0;

      instruction = encode_r_type(0, rs, rt, 0, 0, 0x1B);
      write_be32(ctx, instruction);
      break;
    }

    case INST_MFHI: {
      char *rd_str = strtok_r(NULL, " \t,", &saveptr);

      int rd = parse_register(rd_str);
      if (rd < 0)
        return 0;

      instruction = encode_r_type(0, 0, 0, rd, 0, 0x10);
      write_be32(ctx, instruction);
      break;
    }

    case INST_MFLO: {
      char *rd_str = strtok_r(NULL, " \t,", &saveptr);

      int rd = parse_register(rd_str);
      if (rd < 0)
        return 0;

      instruction = encode_r_type(0, 0, 0, rd, 0, 0x12);
      write_be32(ctx, instruction);
      break;
    }

    case INST_MTHI: {
      char *rs_str = strtok_r(NULL, " \t,", &saveptr);

      int rs = parse_register(rs_str);
      if (rs < 0)
        return 0;

      instruction = encode_r_type(0, rs, 0, 0, 0, 0x11);
      write_be32(ctx, instruction);
      break;
    }

    case INST_MTLO: {
      char *rs_str = strtok_r(NULL, " \t,", &saveptr);

      int rs = parse_register(rs_str);
      if (rs < 0)
        return 0;

      instruction = encode_r_type(0, rs, 0, 0, 0, 0x13);
      write_be32(ctx, instruction);
      break;
    }

    case INST_SLLV: {
      char *rd_str = strtok_r(NULL, " \t,", &saveptr);
      char *rt_str = strtok_r(NULL, " \t,", &saveptr);
      char *rs_str = strtok_r(NULL, " \t,", &saveptr);

      int rd = parse_register(rd_str);
      int rt = parse_register(rt_str);
      int rs = parse_register(rs_str);

      if (rd < 0 || rt < 0 || rs < 0)
        return 0;

      instruction = encode_r_type(0, rs, rt, rd, 0, 0x04);
      write_be32(ctx, instruction);
      break;
    }

    case INST_SRLV: {
      char *rd_str = strtok_r(NULL, " \t,", &saveptr);
      char *rt_str = strtok_r(NULL, " \t,", &saveptr);
      char *rs_str = strtok_r(NULL, " \t,", &saveptr);

      int rd = parse_register(rd_str);
      int rt = parse_register(rt_str);
      int rs = parse_register(rs_str);

      if (rd < 0 || rt < 0 || rs < 0)
        return 0;

      instruction = encode_r_type(0, rs, rt, rd, 0, 0x06);
      write_be32(ctx, instruction);
      break;
    }

    case INST_SRAV: {
      char *rd_str = strtok_r(NULL, " \t,", &saveptr);
      char *rt_str = strtok_r(NULL, " \t,", &saveptr);
      char *rs_str = strtok_r(NULL, " \t,", &saveptr);

      int rd = parse_register(rd_str);
      int rt = parse_register(rt_str);
      int rs = parse_register(rs_str);

      if (rd < 0 || rt < 0 || rs < 0)
        return 0;

      instruction = encode_r_type(0, rs, rt, rd, 0, 0x07);
      write_be32(ctx, instruction);
      break;
    }

    default:
      // Unknown instruction, skip
      return 1;
    }
  } else {
    // First pass: just advance address counter
    ctx->current_address += 4;

    // Handle li pseudo-instruction address calculation
    if (inst_type == INST_LI) {
      char *imm_str = strtok_r(NULL, " \t,", &saveptr);
      uint32_t imm;

      if (parse_immediate(imm_str, &imm)) {
        if (imm > 0xFFFF && (imm & 0xFFFF) != 0) {
          // li expands to two instructions
          ctx->current_address += 4;
        }
      }
    }
  }

  return 1;
}

// Write binary file
int write_binary_file(const char *filename, const uint8_t *data, size_t size) {
  FILE *file = fopen(filename, "wb");
  if (!file) {
    return 0;
  }

  size_t bytes_written = fwrite(data, 1, size, file);
  fclose(file);

  return (bytes_written == size);
}

// Handle assembler directives (.word, .byte, etc.)
void handle_directive(assembler_ctx_t *ctx, const char *directive,
                      char **saveptr) {
  char *token;
  char directive_copy[MAX_LINE_LENGTH];
  char *directive_name = NULL;

  // Extract the directive name
  strncpy(directive_copy, directive, sizeof(directive_copy) - 1);
  directive_copy[sizeof(directive_copy) - 1] = '\0';

  directive_name = strtok_r(directive_copy, " \t", saveptr);
  if (!directive_name)
    return;

  if (is_verbose) {
    printf("Processing directive: .%s\n", directive_name);
  }

  // Handle section directives (.text, .data) - although these are now handled
  // in process_line
  if (strcmp(directive_name, "text") == 0 ||
      strcmp(directive_name, "data") == 0) {
    // Already handled
    return;
  } else if (strcmp(directive_name, "org") == 0) {
    // .org directive - set the current address
    token = strtok_r(NULL, " \t", saveptr);
    if (token) {
      char *comment = strchr(token, '#');
      if (comment)
        *comment = '\0'; // Remove any trailing comment

      uint32_t address;
      if (parse_immediate(token, &address)) {
        if (is_verbose) {
          printf("  Setting address to 0x%08X for section %s\n", address,
                 ctx->current_section == SECTION_TEXT ? "TEXT" : "DATA");
        }

        // Update the section's base address (only on first use)
        if (ctx->current_section == SECTION_TEXT) {
          if (ctx->text_size == 0) {
            ctx->text_address = address;
          }
        } else {
          if (ctx->data_size == 0) {
            ctx->data_address = address;
          }
        }

        ctx->current_address = (ctx->current_section == SECTION_TEXT)
                                   ? ctx->text_address + ctx->text_size
                                   : ctx->data_address + ctx->data_size;
      }
    }
  } else if (strcmp(directive_name, "word") == 0) {
    // .word directive - add 32-bit words
    char *remaining =
        strtok_r(NULL, "\n", saveptr); // Get everything after the directive
    if (remaining) {
      char *comment = strchr(remaining, '#');
      if (comment)
        *comment = '\0'; // Remove any trailing comment

      char *saveptr2;
      token = strtok_r(remaining, ", \t", &saveptr2);
      while (token) {
        uint32_t value;
        if (parse_immediate(token, &value)) {
          if (is_verbose) {
            printf("  Adding word: 0x%08X\n", value);
          }
          write_be32(ctx, value);
        } else {
          // Try to resolve as label
          int label_idx = find_label(ctx, token);
          if (label_idx >= 0) {
            uint32_t addr = ctx->labels[label_idx].address;
            if (is_verbose) {
              printf("  Adding label address: %s = 0x%08X\n", token, addr);
            }
            write_be32(ctx, addr);
          } else {
            printf("  Warning: Could not resolve value: %s\n", token);
          }
        }
        token = strtok_r(NULL, ", \t", &saveptr2);
      }
    }
  } else if (strcmp(directive, "byte") == 0) {
    // .byte directive - add 8-bit bytes
    while ((token = strtok_r(NULL, ", \t", saveptr)) != NULL) {
      uint32_t value;
      if (parse_immediate(token, &value)) {
        if (ctx->output_size < MAX_OUTPUT_SIZE) {
          ctx->output[ctx->output_size++] = (uint8_t)(value & 0xFF);
          ctx->current_address++;

          // Track section size
          if (ctx->current_section == SECTION_TEXT) {
            ctx->text_size += 1;
          } else {
            ctx->data_size += 1;
          }
        }
      }
    }
  } else if (strcmp(directive, "half") == 0 ||
             strcmp(directive, "short") == 0) {
    // .half/.short directive - add 16-bit halfwords
    while ((token = strtok_r(NULL, ", \t", saveptr)) != NULL) {
      uint32_t value;
      if (parse_immediate(token, &value)) {
        if (ctx->output_size + 1 < MAX_OUTPUT_SIZE) {
          // Big endian: high byte first
          ctx->output[ctx->output_size++] = (uint8_t)((value >> 8) & 0xFF);
          ctx->output[ctx->output_size++] = (uint8_t)(value & 0xFF);
          ctx->current_address += 2;

          // Track section size
          if (ctx->current_section == SECTION_TEXT) {
            ctx->text_size += 2;
          } else {
            ctx->data_size += 2;
          }
        }
      }
    }
  } else if (strcmp(directive, "align") == 0) {
    // .align directive - align to specified boundary
    token = strtok_r(NULL, ", \t", saveptr);
    if (token) {
      uint32_t alignment;
      if (parse_immediate(token, &alignment)) {
        uint32_t mask = (1 << alignment) - 1;
        while (ctx->current_address & mask) {
          if (ctx->output_size < MAX_OUTPUT_SIZE) {
            ctx->output[ctx->output_size++] = 0;
            ctx->current_address++;

            // Track section size
            if (ctx->current_section == SECTION_TEXT) {
              ctx->text_size += 1;
            } else {
              ctx->data_size += 1;
            }
          }
        }
      }
    }
  } else if (strcmp(directive, "org") == 0) {
    // .org directive - set the current address
    token = strtok_r(NULL, ", \t", saveptr);
    if (token) {
      uint32_t address;
      if (parse_immediate(token, &address)) {
        ctx->current_address = address;

        // Update the section's base address
        if (ctx->current_section == SECTION_TEXT) {
          ctx->text_address = address;
        } else {
          ctx->data_address = address;
        }
      }
    }
  } else if (strcmp(directive, "space") == 0 ||
             strcmp(directive, "skip") == 0) {
    // .space/.skip directive - reserve specified number of bytes
    token = strtok_r(NULL, ", \t", saveptr);
    if (token) {
      uint32_t size;
      if (parse_immediate(token, &size)) {
        for (uint32_t i = 0; i < size && ctx->output_size < MAX_OUTPUT_SIZE;
             i++) {
          ctx->output[ctx->output_size++] = 0;
          ctx->current_address++;

          // Track section size
          if (ctx->current_section == SECTION_TEXT) {
            ctx->text_size += 1;
          } else {
            ctx->data_size += 1;
          }
        }
      }
    }
  } else if (strcmp(directive_name, "ascii") == 0 ||
             strcmp(directive_name, "asciiz") == 0) {
    // .ascii/.asciiz directive - add ASCII string
    // Extract the entire quoted string
    char *remaining =
        strstr(directive, directive_name) + strlen(directive_name);
    while (*remaining && isspace(*remaining))
      remaining++; // Skip whitespace

    if (*remaining == '"') {
      remaining++; // Skip opening quote
      char *end_quote = strchr(remaining, '"');
      if (end_quote) {
        *end_quote = '\0'; // Terminate at closing quote

        if (is_verbose) {
          printf("  Adding string: \"%s\"\n", remaining);
        }

        for (size_t i = 0;
             remaining[i] != '\0' && ctx->output_size < MAX_OUTPUT_SIZE; i++) {
          ctx->output[ctx->output_size++] = remaining[i];
          ctx->current_address++;

          // Track section size
          if (ctx->current_section == SECTION_TEXT) {
            ctx->text_size += 1;
          } else {
            ctx->data_size += 1;
          }
        }

        // Add null terminator for .asciiz
        if (strcmp(directive_name, "asciiz") == 0 &&
            ctx->output_size < MAX_OUTPUT_SIZE) {
          ctx->output[ctx->output_size++] = 0;
          ctx->current_address++;

          // Track section size
          if (ctx->current_section == SECTION_TEXT) {
            ctx->text_size += 1;
          } else {
            ctx->data_size += 1;
          }
        }
      }
    }
  }
}

// Debug: print section info
void print_section_info(assembler_ctx_t *ctx) {
  printf("TEXT: base=0x%08X size=%u bytes\n", ctx->text_address,
         ctx->text_size);
  printf("DATA: base=0x%08X size=%u bytes\n", ctx->data_address,
         ctx->data_size);
  printf("Total output size: %zu bytes\n", ctx->output_size);
  printf("Label count: %d\n", ctx->label_count);

  // List some labels if any
  if (ctx->label_count > 0) {
    printf("Labels:\n");
    for (int i = 0; i < ctx->label_count && i < 10; i++) {
      printf("  %s: 0x%08X\n", ctx->labels[i].name, ctx->labels[i].address);
    }
    if (ctx->label_count > 10) {
      printf("  (and %d more...)\n", ctx->label_count - 10);
    }
  }
}

// Estimate directive size for pass 1 address calculation
void estimate_directive_size(assembler_ctx_t *ctx, const char *directive) {
  char directive_copy[MAX_LINE_LENGTH];
  char *saveptr = NULL;

  // Extract the directive name
  strncpy(directive_copy, directive, sizeof(directive_copy) - 1);
  directive_copy[sizeof(directive_copy) - 1] = '\0';

  // Remove any comments
  char *comment = strchr(directive_copy, '#');
  if (comment)
    *comment = '\0';

  char *directive_name = strtok_r(directive_copy, " \t", &saveptr);
  if (!directive_name)
    return;

  if (is_verbose) {
    printf("Estimating directive in pass 1: .%s\n", directive_name);
  }

  if (strcmp(directive_name, "word") == 0) {
    // Count number of comma-separated values
    char *remaining = saveptr;
    int word_count = 0;

    // Skip leading whitespace in remaining text
    while (*remaining && isspace(*remaining))
      remaining++;

    char *token = strtok_r(remaining, ",", &saveptr);
    while (token) {
      word_count++;
      token = strtok_r(NULL, ",", &saveptr);
    }

    // Each word is 4 bytes
    int size = word_count * 4;

    if (is_verbose) {
      printf("  Estimated %d words (%d bytes)\n", word_count, size);
    }

    ctx->current_address += size;
    if (ctx->current_section == SECTION_TEXT) {
      ctx->text_size += size;
    } else {
      ctx->data_size += size;
    }
  } else if (strcmp(directive_name, "byte") == 0) {
    // Count byte items
    char *remaining = saveptr;
    int byte_count = 0;

    // Skip whitespace
    while (*remaining && isspace(*remaining))
      remaining++;

    char *token = strtok_r(remaining, ",", &saveptr);
    while (token) {
      byte_count++;
      token = strtok_r(NULL, ",", &saveptr);
    }

    if (is_verbose) {
      printf("  Estimated %d bytes\n", byte_count);
    }
    ctx->current_address += byte_count;
    if (ctx->current_section == SECTION_TEXT) {
      ctx->text_size += byte_count;
    } else {
      ctx->data_size += byte_count;
    }
  } else if (strcmp(directive_name, "half") == 0 ||
             strcmp(directive_name, "short") == 0) {
    // Count half-word items
    char *remaining = saveptr;
    int half_count = 0;

    while (*remaining && isspace(*remaining))
      remaining++;

    char *token = strtok_r(remaining, ",", &saveptr);
    while (token) {
      half_count++;
      token = strtok_r(NULL, ",", &saveptr);
    }

    int size = half_count * 2; // Each half-word is 2 bytes

    if (is_verbose) {
      printf("  Estimated %d half-words (%d bytes)\n", half_count, size);
    }

    ctx->current_address += size;
    if (ctx->current_section == SECTION_TEXT) {
      ctx->text_size += size;
    } else {
      ctx->data_size += size;
    }
  } else if (strcmp(directive_name, "ascii") == 0 ||
             strcmp(directive_name, "asciiz") == 0) {
    // Find string between quotes
    char *str_start = strchr(directive, '"');
    if (str_start) {
      str_start++; // Skip opening quote
      char *str_end = strchr(str_start, '"');
      if (str_end) {
        int str_len = str_end - str_start;
        // Add null terminator for asciiz
        if (strcmp(directive_name, "asciiz") == 0) {
          str_len++;
        }

        if (is_verbose) {
          printf("  Estimated string length: %d bytes\n", str_len);
        }
        ctx->current_address += str_len;
        if (ctx->current_section == SECTION_TEXT) {
          ctx->text_size += str_len;
        } else {
          ctx->data_size += str_len;
        }
      }
    }
  } else if (strcmp(directive_name, "space") == 0 ||
             strcmp(directive_name, "skip") == 0) {
    char *size_str = strtok_r(NULL, " \t", &saveptr);
    if (size_str) {
      uint32_t size;
      if (parse_immediate(size_str, &size)) {
        if (is_verbose) {
          printf("  Estimated space: %u bytes\n", size);
        }
        ctx->current_address += size;
        if (ctx->current_section == SECTION_TEXT) {
          ctx->text_size += size;
        } else {
          ctx->data_size += size;
        }
      }
    }
  }
  // Note: align directive is not estimated here since it depends on runtime
  // address
}

// Main assembler function
int mips_assemble(const char *source, uint8_t **output, size_t *output_size,
                  int verbose) {
  assembler_ctx_t ctx = {0};
  char line[MAX_LINE_LENGTH];
  const char *line_start = source;
  const char *line_end;

  is_verbose = verbose;

  // Debug: Print source length
  if (is_verbose) {
    printf("Source length: %zu bytes\n", strlen(source));
  }

  // Allocate output buffer
  ctx.output = malloc(MAX_OUTPUT_SIZE);
  if (!ctx.output) {
    fprintf(stderr, "Failed to allocate memory for output buffer\n");
    return 0;
  }
  memset(ctx.output, 0, MAX_OUTPUT_SIZE);

  // Initialize section addresses
  // Default: text at 0x00400000 (typical MIPS program start)
  //          data at 0x10010000 (typical MIPS data segment start)
  ctx.text_address = 0x00400000;
  ctx.data_address = 0x10010000;
  ctx.text_size = 0;
  ctx.data_size = 0;

  // First pass: collect labels
  ctx.pass = 1;
  ctx.current_address = ctx.text_address; // Start in text section by default
  ctx.current_section = SECTION_TEXT;
  ctx.output_size = 0;
  ctx.label_count = 0;

  line_start = source;
  while (*line_start) {
    line_end = strchr(line_start, '\n');
    if (!line_end)
      line_end = line_start + strlen(line_start);

    size_t line_len = line_end - line_start;
    if (line_len >= sizeof(line))
      line_len = sizeof(line) - 1;

    strncpy(line, line_start, line_len);
    line[line_len] = '\0';

    if (!process_line(&ctx, line)) {
      free(ctx.output);
      return 0;
    }

    if (*line_end == '\n')
      line_end++;
    line_start = line_end;
  }

  // After pass 1, save the label table
  if (is_verbose) {
    printf("\nCompleted pass 1:\n");
    print_section_info(&ctx);
  }

  // Second pass: generate code
  ctx.pass = 2;
  ctx.current_address = ctx.text_address; // Reset to text section
  ctx.current_section = SECTION_TEXT;
  ctx.output_size = 0;
  ctx.text_size = 0;
  ctx.data_size = 0;

  line_start = source;
  while (*line_start) {
    line_end = strchr(line_start, '\n');
    if (!line_end)
      line_end = line_start + strlen(line_start);

    size_t line_len = line_end - line_start;
    if (line_len >= sizeof(line))
      line_len = sizeof(line) - 1;

    strncpy(line, line_start, line_len);
    line[line_len] = '\0';

    if (!process_line(&ctx, line)) {
      fprintf(stderr, "Error processing line (pass 2): %s\n", line);
      free(ctx.output);
      return 0;
    }

    if (*line_end == '\n')
      line_end++;
    line_start = line_end;
  }

  *output = ctx.output;
  *output_size = ctx.output_size;

  // Debug info
  if (is_verbose) {
    print_section_info(&ctx);
  }

  return 1;
}
