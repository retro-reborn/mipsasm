#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "mipsasm.h"

// Parse register name and return register number
int parse_register(const char *reg_str) {
    if (!reg_str) return -1;
    
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
    
    // Named registers - use lookup table
    static const struct {
        const char *name;
        int reg_num;
    } reg_table[] = {
        {"zero", REG_ZERO}, {"at", REG_AT},   
        {"v0", REG_V0},     {"v1", REG_V1},
        {"a0", REG_A0},     {"a1", REG_A1},   
        {"a2", REG_A2},     {"a3", REG_A3},
        {"t0", REG_T0},     {"t1", REG_T1},   
        {"t2", REG_T2},     {"t3", REG_T3},
        {"t4", REG_T4},     {"t5", REG_T5},   
        {"t6", REG_T6},     {"t7", REG_T7},
        {"s0", REG_S0},     {"s1", REG_S1},   
        {"s2", REG_S2},     {"s3", REG_S3},
        {"s4", REG_S4},     {"s5", REG_S5},   
        {"s6", REG_S6},     {"s7", REG_S7},
        {"t8", REG_T8},     {"t9", REG_T9},   
        {"k0", REG_K0},     {"k1", REG_K1},
        {"gp", REG_GP},     {"sp", REG_SP},   
        {"fp", REG_FP},     {"ra", REG_RA}
    };
    
    for (size_t i = 0; i < sizeof(reg_table)/sizeof(reg_table[0]); i++) {
        if (strcmp(reg_str, reg_table[i].name) == 0) {
            return reg_table[i].reg_num;
        }
    }
    
    return -1;
}

// Parse instruction mnemonic
instruction_type_t parse_instruction(const char *mnemonic) {
    if (!mnemonic) return INST_UNKNOWN;
    
    static const struct {
        const char *name;
        instruction_type_t type;
    } inst_table[] = {
        {"lui", INST_LUI},       {"li", INST_LI},         {"addiu", INST_ADDIU}, 
        {"addi", INST_ADDI},     {"sw", INST_SW},         {"lw", INST_LW},
        {"bnez", INST_BNEZ},     {"beqz", INST_BEQZ},     {"beq", INST_BEQ},
        {"bne", INST_BNE},       {"b", INST_B},           {"j", INST_J},
        {"jal", INST_JAL},       {"nop", INST_NOP},       {"andi", INST_ANDI},
        {"ori", INST_ORI},       {"xori", INST_XORI},     {"add", INST_ADD},
        {"sub", INST_SUB},       {"and", INST_AND},       {"or", INST_OR},
        {"xor", INST_XOR},       {"sll", INST_SLL},       {"srl", INST_SRL},
        {"sra", INST_SRA},       {"sllv", INST_SLLV},     {"srlv", INST_SRLV},
        {"srav", INST_SRAV},     {"slt", INST_SLT},       {"sltu", INST_SLTU},
        {"jr", INST_JR},         {"jalr", INST_JALR},     {"mfhi", INST_MFHI},
        {"mflo", INST_MFLO},     {"mthi", INST_MTHI},     {"mtlo", INST_MTLO},
        {"mult", INST_MULT},     {"multu", INST_MULTU},   {"div", INST_DIV},
        {"divu", INST_DIVU},     {"syscall", INST_SYSCALL}, {"break", INST_BREAK},
        {"slti", INST_SLTI},     {"sltiu", INST_SLTIU},   {"lb", INST_LB},
        {"lbu", INST_LBU},       {"lh", INST_LH},         {"lhu", INST_LHU},
        {"sb", INST_SB},         {"sh", INST_SH},         {"la", INST_LA},
        {"move", INST_MOVE}
    };
    
    for (size_t i = 0; i < sizeof(inst_table) / sizeof(inst_table[0]); i++) {
        if (strcmp(mnemonic, inst_table[i].name) == 0) {
            return inst_table[i].type;
        }
    }
    
    return INST_UNKNOWN;
}

// Parse immediate value (hex, decimal, or label)
int parse_immediate(const char *str, uint32_t *value) {
    if (!str || !value) return 0;
    
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
uint32_t encode_r_type(uint8_t op, uint8_t rs, uint8_t rt, uint8_t rd, uint8_t shamt, uint8_t func) {
    return ((uint32_t)op << 26) | ((uint32_t)rs << 21) | ((uint32_t)rt << 16) | 
           ((uint32_t)rd << 11) | ((uint32_t)shamt << 6) | (uint32_t)func;
}

// Encode I-type instruction
uint32_t encode_i_type(uint8_t op, uint8_t rs, uint8_t rt, uint16_t imm) {
    return ((uint32_t)op << 26) | ((uint32_t)rs << 21) | ((uint32_t)rt << 16) | (uint32_t)imm;
}

// Encode J-type instruction
uint32_t encode_j_type(uint8_t op, uint32_t target) {
    return ((uint32_t)op << 26) | (target & 0x3FFFFFF);
}

// Add label to context
int add_label(assembler_ctx_t *ctx, const char *name, uint32_t address) {
    if (ctx->label_count >= MAX_LABELS) return 0;
    
    strncpy(ctx->labels[ctx->label_count].name, name, sizeof(ctx->labels[ctx->label_count].name) - 1);
    ctx->labels[ctx->label_count].name[sizeof(ctx->labels[ctx->label_count].name) - 1] = '\0';
    ctx->labels[ctx->label_count].address = address;
    ctx->labels[ctx->label_count].resolved = 1;
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
    }
}

// Process a single line of assembly
static int process_line(assembler_ctx_t *ctx, const char *line) {
    char line_copy[MAX_LINE_LENGTH];
    char *token, *saveptr;
    uint32_t instruction = 0;
    
    // Copy line and remove comments
    strncpy(line_copy, line, sizeof(line_copy) - 1);
    line_copy[sizeof(line_copy) - 1] = '\0';
    
    char *comment = strstr(line_copy, "//");
    if (comment) *comment = '\0';
    
    // Skip empty lines and whitespace
    char *trimmed = line_copy;
    while (isspace(*trimmed)) trimmed++;
    if (*trimmed == '\0') return 1;
    
    // Check for label
    char *colon = strchr(trimmed, ':');
    if (colon) {
        *colon = '\0';
        if (ctx->pass == 1) {
            add_label(ctx, trimmed, ctx->current_address);
        }
        // Move past the label for instruction processing
        trimmed = colon + 1;
        while (isspace(*trimmed)) trimmed++;
        if (*trimmed == '\0') return 1;
    }
    
    // Skip directives (starts with .)
    if (trimmed[0] == '.') {
        if (ctx->pass == 2) {
            // Handle directives in pass 2
            handle_directive(ctx, trimmed + 1, &saveptr);
        }
        return 1;
    }
    
    // Parse instruction
    token = strtok_r(trimmed, " \t,", &saveptr);
    if (!token) return 1;
    
    instruction_type_t inst_type = parse_instruction(token);
    
    if (ctx->pass == 2) {  // Only generate code on second pass
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
                
                if (rt < 0) return 0;
                
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
                
                if (rt < 0) return 0;
                
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
                
                if (rt < 0 || rs < 0) return 0;
                
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
                if (rt < 0) return 0;
                
                // Parse offset(base) format
                char *paren = strchr(offset_base, '(');
                if (paren) {
                    *paren = '\0';
                    char *base_str = paren + 1;
                    char *end_paren = strchr(base_str, ')');
                    if (end_paren) *end_paren = '\0';
                    
                    int rs = parse_register(base_str);
                    uint32_t offset;
                    
                    if (rs < 0) return 0;
                    
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
                if (rs < 0) return 0;
                
                int label_idx = find_label(ctx, label_str);
                if (label_idx >= 0) {
                    int32_t offset = (int32_t)(ctx->labels[label_idx].address - (ctx->current_address + 4)) / 4;
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
                    int32_t offset = (int32_t)(ctx->labels[label_idx].address - (ctx->current_address + 4)) / 4;
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
                
                if (rt < 0 || rs < 0) return 0;
                
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
                
                if (rs < 0 || rt < 0) return 0;
                
                int label_idx = find_label(ctx, label_str);
                if (label_idx >= 0) {
                    int32_t offset = (int32_t)(ctx->labels[label_idx].address - (ctx->current_address + 4)) / 4;
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
                
                if (rs < 0 || rt < 0) return 0;
                
                int label_idx = find_label(ctx, label_str);
                if (label_idx >= 0) {
                    int32_t offset = (int32_t)(ctx->labels[label_idx].address - (ctx->current_address + 4)) / 4;
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
                if (rs < 0) return 0;
                
                int label_idx = find_label(ctx, label_str);
                if (label_idx >= 0) {
                    int32_t offset = (int32_t)(ctx->labels[label_idx].address - (ctx->current_address + 4)) / 4;
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
                if (rt < 0) return 0;
                
                // Parse offset(base) format
                char *paren = strchr(offset_base, '(');
                if (paren) {
                    *paren = '\0';
                    char *base_str = paren + 1;
                    char *end_paren = strchr(base_str, ')');
                    if (end_paren) *end_paren = '\0';
                    
                    int rs = parse_register(base_str);
                    if (rs < 0) return 0;
                    
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
                
                if (rd < 0 || rs < 0 || rt < 0) return 0;
                
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
                
                if (rd < 0 || rs < 0 || rt < 0) return 0;
                
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
                
                if (rd < 0 || rs < 0 || rt < 0) return 0;
                
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
                
                if (rd < 0 || rs < 0 || rt < 0) return 0;
                
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
                
                if (rd < 0 || rs < 0 || rt < 0) return 0;
                
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
                
                if (rd < 0 || rt < 0) return 0;
                if (!parse_immediate(sa_str, &sa) || sa > 31) return 0;
                
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
                
                if (rd < 0 || rt < 0) return 0;
                if (!parse_immediate(sa_str, &sa) || sa > 31) return 0;
                
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
                
                if (rd < 0 || rt < 0) return 0;
                if (!parse_immediate(sa_str, &sa) || sa > 31) return 0;
                
                instruction = encode_r_type(0, 0, rt, rd, sa, 0x03);
                write_be32(ctx, instruction);
                break;
            }
            
            case INST_JR: {
                char *rs_str = strtok_r(NULL, " \t,", &saveptr);
                
                int rs = parse_register(rs_str);
                if (rs < 0) return 0;
                
                instruction = encode_r_type(0, rs, 0, 0, 0, 0x08);
                write_be32(ctx, instruction);
                break;
            }
            
            case INST_JALR: {
                char *rs_str = strtok_r(NULL, " \t,", &saveptr);
                char *rd_str = strtok_r(NULL, " \t,", &saveptr);
                
                int rs = parse_register(rs_str);
                int rd = (rd_str) ? parse_register(rd_str) : 31; // default to $ra if no rd
                
                if (rs < 0 || rd < 0) return 0;
                
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
                
                if (code_str && !parse_immediate(code_str, &code)) return 0;
                
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
                
                if (rd < 0 || rs < 0) return 0;
                
                instruction = encode_r_type(0, rs, 0, rd, 0, 0x21); // ADDU
                write_be32(ctx, instruction);
                break;
            }
            
            case INST_LA: {
                // Pseudo-instruction: la $rt, label => lui $rt, upper(label) + ori $rt, $rt, lower(label)
                char *rt_str = strtok_r(NULL, " \t,", &saveptr);
                char *label_str = strtok_r(NULL, " \t,", &saveptr);
                
                int rt = parse_register(rt_str);
                if (rt < 0) return 0;
                
                int label_idx = find_label(ctx, label_str);
                if (label_idx >= 0) {
                    uint32_t addr = ctx->labels[label_idx].address;
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
                    return 0;
                }
                break;
            }
            
            case INST_LB: {
                char *rt_str = strtok_r(NULL, " \t,", &saveptr);
                char *offset_base = strtok_r(NULL, " \t,", &saveptr);
                
                int rt = parse_register(rt_str);
                if (rt < 0) return 0;
                
                // Parse offset(base) format
                char *paren = strchr(offset_base, '(');
                if (paren) {
                    *paren = '\0';
                    char *base_str = paren + 1;
                    char *end_paren = strchr(base_str, ')');
                    if (end_paren) *end_paren = '\0';
                    
                    int rs = parse_register(base_str);
                    if (rs < 0) return 0;
                    
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
                if (rt < 0) return 0;
                
                // Parse offset(base) format
                char *paren = strchr(offset_base, '(');
                if (paren) {
                    *paren = '\0';
                    char *base_str = paren + 1;
                    char *end_paren = strchr(base_str, ')');
                    if (end_paren) *end_paren = '\0';
                    
                    int rs = parse_register(base_str);
                    if (rs < 0) return 0;
                    
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
                if (rt < 0) return 0;
                
                // Parse offset(base) format
                char *paren = strchr(offset_base, '(');
                if (paren) {
                    *paren = '\0';
                    char *base_str = paren + 1;
                    char *end_paren = strchr(base_str, ')');
                    if (end_paren) *end_paren = '\0';
                    
                    int rs = parse_register(base_str);
                    if (rs < 0) return 0;
                    
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
                if (rt < 0) return 0;
                
                // Parse offset(base) format
                char *paren = strchr(offset_base, '(');
                if (paren) {
                    *paren = '\0';
                    char *base_str = paren + 1;
                    char *end_paren = strchr(base_str, ')');
                    if (end_paren) *end_paren = '\0';
                    
                    int rs = parse_register(base_str);
                    if (rs < 0) return 0;
                    
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
                if (rt < 0) return 0;
                
                // Parse offset(base) format
                char *paren = strchr(offset_base, '(');
                if (paren) {
                    *paren = '\0';
                    char *base_str = paren + 1;
                    char *end_paren = strchr(base_str, ')');
                    if (end_paren) *end_paren = '\0';
                    
                    int rs = parse_register(base_str);
                    if (rs < 0) return 0;
                    
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
                if (rt < 0) return 0;
                
                // Parse offset(base) format
                char *paren = strchr(offset_base, '(');
                if (paren) {
                    *paren = '\0';
                    char *base_str = paren + 1;
                    char *end_paren = strchr(base_str, ')');
                    if (end_paren) *end_paren = '\0';
                    
                    int rs = parse_register(base_str);
                    if (rs < 0) return 0;
                    
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
                
                if (rt < 0 || rs < 0) return 0;
                
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
                
                if (rt < 0 || rs < 0) return 0;
                
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
                
                if (rd < 0 || rs < 0 || rt < 0) return 0;
                
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
                
                if (rd < 0 || rs < 0 || rt < 0) return 0;
                
                instruction = encode_r_type(0, rs, rt, rd, 0, 0x2B);
                write_be32(ctx, instruction);
                break;
            }
            
            case INST_MULT: {
                char *rs_str = strtok_r(NULL, " \t,", &saveptr);
                char *rt_str = strtok_r(NULL, " \t,", &saveptr);
                
                int rs = parse_register(rs_str);
                int rt = parse_register(rt_str);
                
                if (rs < 0 || rt < 0) return 0;
                
                instruction = encode_r_type(0, rs, rt, 0, 0, 0x18);
                write_be32(ctx, instruction);
                break;
            }
            
            case INST_MULTU: {
                char *rs_str = strtok_r(NULL, " \t,", &saveptr);
                char *rt_str = strtok_r(NULL, " \t,", &saveptr);
                
                int rs = parse_register(rs_str);
                int rt = parse_register(rt_str);
                
                if (rs < 0 || rt < 0) return 0;
                
                instruction = encode_r_type(0, rs, rt, 0, 0, 0x19);
                write_be32(ctx, instruction);
                break;
            }
            
            case INST_DIV: {
                char *rs_str = strtok_r(NULL, " \t,", &saveptr);
                char *rt_str = strtok_r(NULL, " \t,", &saveptr);
                
                int rs = parse_register(rs_str);
                int rt = parse_register(rt_str);
                
                if (rs < 0 || rt < 0) return 0;
                
                instruction = encode_r_type(0, rs, rt, 0, 0, 0x1A);
                write_be32(ctx, instruction);
                break;
            }
            
            case INST_DIVU: {
                char *rs_str = strtok_r(NULL, " \t,", &saveptr);
                char *rt_str = strtok_r(NULL, " \t,", &saveptr);
                
                int rs = parse_register(rs_str);
                int rt = parse_register(rt_str);
                
                if (rs < 0 || rt < 0) return 0;
                
                instruction = encode_r_type(0, rs, rt, 0, 0, 0x1B);
                write_be32(ctx, instruction);
                break;
            }
            
            case INST_MFHI: {
                char *rd_str = strtok_r(NULL, " \t,", &saveptr);
                
                int rd = parse_register(rd_str);
                if (rd < 0) return 0;
                
                instruction = encode_r_type(0, 0, 0, rd, 0, 0x10);
                write_be32(ctx, instruction);
                break;
            }
            
            case INST_MFLO: {
                char *rd_str = strtok_r(NULL, " \t,", &saveptr);
                
                int rd = parse_register(rd_str);
                if (rd < 0) return 0;
                
                instruction = encode_r_type(0, 0, 0, rd, 0, 0x12);
                write_be32(ctx, instruction);
                break;
            }
            
            case INST_MTHI: {
                char *rs_str = strtok_r(NULL, " \t,", &saveptr);
                
                int rs = parse_register(rs_str);
                if (rs < 0) return 0;
                
                instruction = encode_r_type(0, rs, 0, 0, 0, 0x11);
                write_be32(ctx, instruction);
                break;
            }
            
            case INST_MTLO: {
                char *rs_str = strtok_r(NULL, " \t,", &saveptr);
                
                int rs = parse_register(rs_str);
                if (rs < 0) return 0;
                
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
                
                if (rd < 0 || rt < 0 || rs < 0) return 0;
                
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
                
                if (rd < 0 || rt < 0 || rs < 0) return 0;
                
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
                
                if (rd < 0 || rt < 0 || rs < 0) return 0;
                
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
void handle_directive(assembler_ctx_t *ctx, const char *directive, char **saveptr) {
    char *token;
    
    // Handle section directives (.text, .data)
    if (strcmp(directive, "text") == 0) {
        // TODO: handle .text section
        return;
    } else if (strcmp(directive, "data") == 0) {
        // TODO: handle .data section
        return;
    } else if (strcmp(directive, "word") == 0) {
        // .word directive - add 32-bit words
        while ((token = strtok_r(NULL, ", \t", saveptr)) != NULL) {
            uint32_t value;
            if (parse_immediate(token, &value)) {
                write_be32(ctx, value);
            } else {
                // Try to resolve as label
                int label_idx = find_label(ctx, token);
                if (label_idx >= 0) {
                    uint32_t addr = ctx->labels[label_idx].address;
                    write_be32(ctx, addr);
                }
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
                }
            }
        }
    } else if (strcmp(directive, "half") == 0 || strcmp(directive, "short") == 0) {
        // .half/.short directive - add 16-bit halfwords
        while ((token = strtok_r(NULL, ", \t", saveptr)) != NULL) {
            uint32_t value;
            if (parse_immediate(token, &value)) {
                if (ctx->output_size + 1 < MAX_OUTPUT_SIZE) {
                    // Big endian: high byte first
                    ctx->output[ctx->output_size++] = (uint8_t)((value >> 8) & 0xFF);
                    ctx->output[ctx->output_size++] = (uint8_t)(value & 0xFF);
                    ctx->current_address += 2;
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
                    }
                }
            }
        }
    } else if (strcmp(directive, "space") == 0 || strcmp(directive, "skip") == 0) {
        // .space/.skip directive - reserve specified number of bytes
        token = strtok_r(NULL, ", \t", saveptr);
        if (token) {
            uint32_t size;
            if (parse_immediate(token, &size)) {
                for (uint32_t i = 0; i < size && ctx->output_size < MAX_OUTPUT_SIZE; i++) {
                    ctx->output[ctx->output_size++] = 0;
                    ctx->current_address++;
                }
            }
        }
    } else if (strcmp(directive, "ascii") == 0 || strcmp(directive, "asciiz") == 0) {
        // .ascii/.asciiz directive - add ASCII string
        // Extract the entire quoted string
        token = strtok_r(NULL, "\"", saveptr);
        if (token) {
            for (size_t i = 0; token[i] != '\0' && ctx->output_size < MAX_OUTPUT_SIZE; i++) {
                ctx->output[ctx->output_size++] = token[i];
                ctx->current_address++;
            }
            
            // Add null terminator for .asciiz
            if (strcmp(directive, "asciiz") == 0 && ctx->output_size < MAX_OUTPUT_SIZE) {
                ctx->output[ctx->output_size++] = 0;
                ctx->current_address++;
            }
        }
    }
    // TODO: Handle other directives
}

// Main assembler function
int mips_assemble(const char *source, uint8_t **output, size_t *output_size) {
    assembler_ctx_t ctx = {0};
    char line[MAX_LINE_LENGTH];
    const char *line_start = source;
    const char *line_end;
    
    // Allocate output buffer
    ctx.output = malloc(MAX_OUTPUT_SIZE);
    if (!ctx.output) {
        fprintf(stderr, "Failed to allocate memory for output buffer\n");
        return 0;
    }
    memset(ctx.output, 0, MAX_OUTPUT_SIZE);
    
    // First pass: collect labels
    ctx.pass = 1;
    ctx.current_address = 0;
    ctx.output_size = 0;
    ctx.label_count = 0;
    
    line_start = source;
    while (*line_start) {
        line_end = strchr(line_start, '\n');
        if (!line_end) line_end = line_start + strlen(line_start);
        
        size_t line_len = line_end - line_start;
        if (line_len >= sizeof(line)) line_len = sizeof(line) - 1;
        
        strncpy(line, line_start, line_len);
        line[line_len] = '\0';
        
        if (!process_line(&ctx, line)) {
            free(ctx.output);
            return 0;
        }
        
        if (*line_end == '\n') line_end++;
        line_start = line_end;
    }
    
    // Second pass: generate code
    ctx.pass = 2;
    ctx.current_address = 0;
    ctx.output_size = 0;
    
    line_start = source;
    while (*line_start) {
        line_end = strchr(line_start, '\n');
        if (!line_end) line_end = line_start + strlen(line_start);
        
        size_t line_len = line_end - line_start;
        if (line_len >= sizeof(line)) line_len = sizeof(line) - 1;
        
        strncpy(line, line_start, line_len);
        line[line_len] = '\0';
        
        if (!process_line(&ctx, line)) {
            fprintf(stderr, "Error processing line (pass 2): %s\n", line);
            free(ctx.output);
            return 0;
        }
        
        if (*line_end == '\n') line_end++;
        line_start = line_end;
    }
    
    *output = ctx.output;
    *output_size = ctx.output_size;
    
    return 1;
}
