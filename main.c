/*
 * =================================================================================
 * TRABALHO FINAL - ARQUITETURA DE COMPUTADORES (SIMULADOR RISC 16-BIT)
 * =================================================================================
 *
 * ESPECIFICAÇÕES GERAIS:
 * 1. Palavra de dados/instrução: 16 bits.
 * 2. Memória: 16KB (endereços 0x0000 a 0x2000). Mapeada como array de 8192 posições.
 * 3. Registradores: 16 (R0-R15).
 * - R14 = SP (Stack Pointer) -> Inicia em 0x2000.
 * - R15 = PC (Program Counter).
 * 4. Flags: Zero (Z) e Carry (C). Apenas instruções da ULA modificam as flags.
 *
 * =================================================================================
 * TODO LIST (O QUE PRECISA SER FEITO):
 * =================================================================================
 *
 * [ ] 1. ESTRUTURAS DE DADOS (STRUCTS)
 * - Criar struct 'CPU' contendo:
 * - uint16_t reg[16];        // R0 a R15
 * - uint16_t memory[8192];   // Memória RAM (0x0000 - 0x2000)
 * - uint16_t ir;             // Instruction Register
 * - int flag_z;              // Flag Zero
 * - int flag_c;              // Flag Carry
 * - bool mem_access[8192];   // (Dica PDF) Para rastrear posições acessadas p/ o print final.
 *
 * [ ] 2. LEITURA DO ARQUIVO (LOADER)
 * - Abrir arquivo .txt ou .hex passado por argumento.
 * - Ler linhas no formato "<endereço> <conteúdo>" (hexadecimal).
 * - Preencher o array 'memory' da CPU com esses valores.
 * - Armazenar breakpoints (se houver, passados via argv).
 *
 * [ ] 3. CICLO DE EXECUÇÃO (Loop Principal)
 * - Loop while(1) até encontrar instrução HALT.
 * - 3.1. BUSCA (FETCH):
 * IR = memory[PC]
 * old_pc = PC;  // Guardar p/ debug/breakpoints
 * PC = PC + 1;  // IMPORTANTE: PC incrementa ANTES de executar [PDF Pag 5]
 *
 * - 3.2. DECODIFICAÇÃO (DECODE):
 * Extrair Opcode (bits 0-3).
 * Extrair Operandos (Rd, Rm, Rn, Imediato) dependendo do tipo da instrução.
 * Dica: Fazer extensão de sinal para imediatos em JMP, J<cond> e MOV [PDF Pag 5].
 *
 * - 3.3. EXECUÇÃO (EXECUTE) - SWITCH CASE NO OPCODE:
 * - JUMP (JMP, JEQ, JNE, JLT, JGE): Verificar flags Z/C e somar PC + Imediato.
 * - MEMÓRIA (LDR, STR):
 * * Atenção: STR salva na memória, LDR carrega p/ registrador.
 * * IMPORTANTE: Verificar Mapeamento de E/S (Endereços >= 0xF000).
 * - 0xF000/0xF001: Char I/O (getchar/putchar).
 * - 0xF002/0xF003: Int I/O (scanf/printf).
 * - MOVIMENTAÇÃO (MOV): Rd = Imediato.
 * - ULA (ADD, SUB, AND, OR, SHL, SHR, CMP):
 * * Realizar operação.
 * * ATUALIZAR FLAGS Z e C (apenas aqui!).
 * * CMP é um SUB que não salva resultado, só flags.
 * - PILHA (PUSH, POP):
 * * PUSH: SP--, Mem[SP] = Rn.
 * * POP: Rd = Mem[SP], SP++.
 * - CONTROLE (HALT): Break no loop.
 *
 * [ ] 4. SISTEMA DE SAÍDA (DUMP)
 * - Criar função para imprimir o estado da máquina (chamada no HALT ou Breakpoint).
 * - Imprimir R0-R15 em Hexa.
 * - Imprimir Flags Z e C.
 * - Imprimir conteúdo da Memória de Dados (apenas posições marcadas como acessadas).
 * - Imprimir Pilha (se SP != 0x2000, imprimir de SP até o fundo).
 *
 * [ ] 5. IMPLEMENTAR FUNÇÕES AUXILIARES (Dicas do PDF)
 * - Extensão de sinal (converter 6 bits ou 8 bits para 16 bits signed).
 * - Verificação de breakpoints.
 *
 * =================================================================================
 */


#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#define MEMORY_SIZE 8192
#define NUM_REGS 16

#define SP 14
#define PC 15

typedef struct {
    uint16_t reg[NUM_REGS];
    uint16_t memory[MEMORY_SIZE];
    bool mem_access[MEMORY_SIZE];
    uint16_t ir;
    int flag_z;
    int flag_c;
} CPU;

/* ================= AUX ================= */

int16_t sign_extend(uint16_t val, int bits) {
    uint16_t mask = 1 << (bits - 1);
    return (val ^ mask) - mask;
}

uint16_t zero_extend(uint16_t val) {
    return val;
}

void mem_check(uint16_t addr) {
    if (addr >= MEMORY_SIZE) {
        printf("Erro: acesso invalido a memoria 0x%04X\n", addr);
        exit(1);
    }
}

/* ================= DUMP ================= */

void dump_cpu(CPU *cpu) {
    for (int i = 0; i < NUM_REGS; i++)
        printf("R%d = 0x%04X\n", i, cpu->reg[i]);

    printf("Z = %d\n", cpu->flag_z);
    printf("C = %d\n", cpu->flag_c);
}

/* ================= INIT ================= */

void cpu_init(CPU *cpu) {
    for (int i = 0; i < NUM_REGS; i++)
        cpu->reg[i] = 0;

    for (int i = 0; i < MEMORY_SIZE; i++) {
        cpu->memory[i] = 0;
        cpu->mem_access[i] = false;
    }

    cpu->flag_z = cpu->flag_c = 0;
    cpu->reg[SP] = 0x2000;
    cpu->reg[PC] = 0;
}

/* ================= LOADER ================= */

void load_program(CPU *cpu, const char *file) {
    FILE *f = fopen(file, "r");
    if (!f) {
        perror("Erro ao abrir arquivo");
        exit(1);
    }

    uint16_t addr, val;
    while (fscanf(f, "%hx %hx", &addr, &val) == 2)
        if (addr < MEMORY_SIZE)
            cpu->memory[addr] = val;

    fclose(f);
}

/* ================= EXEC ================= */

void cpu_run(CPU *cpu) {
    while (1) {
        uint16_t old_pc = cpu->reg[PC];
        mem_check(old_pc);

        cpu->ir = cpu->memory[old_pc];
        cpu->reg[PC]++;

        if (cpu->ir == 0xFFFF) {
            dump_cpu(cpu);
            return;
        }

        uint16_t opcode = cpu->ir & 0xF;
        uint16_t rd = (cpu->ir >> 12) & 0xF;
        uint16_t rm = (cpu->ir >> 8) & 0xF;
        uint16_t rn = (cpu->ir >> 4) & 0xF;

        uint16_t imm4  = cpu->ir >> 4 & 0xF;
        int16_t imm8   = sign_extend((cpu->ir >> 4) & 0xFF, 8);
        int16_t imm12  = sign_extend(cpu->ir >> 4, 12);

        switch (opcode) {

        case 0x4: // MOV
            cpu->reg[rd] = imm8;
            cpu->flag_z = (cpu->reg[rd] == 0);
            break;

        case 0x5: { // ADD
            uint32_t r = cpu->reg[rm] + cpu->reg[rn];
            cpu->reg[rd] = r;
            cpu->flag_z = (cpu->reg[rd] == 0);
            cpu->flag_c = (r > 0xFFFF);
            break;
        }

        case 0x6: { // ADDI
            uint32_t r = cpu->reg[rm] + imm4;
            cpu->reg[rd] = r;
            cpu->flag_z = (cpu->reg[rd] == 0);
            cpu->flag_c = (r > 0xFFFF);
            break;
        }

        case 0x7: { // SUB
            cpu->flag_c = (cpu->reg[rm] < cpu->reg[rn]);
            cpu->reg[rd] = cpu->reg[rm] - cpu->reg[rn];
            cpu->flag_z = (cpu->reg[rd] == 0);
            break;
        }

        case 0x8: { // SUBI
            cpu->flag_c = (cpu->reg[rm] < imm4);
            cpu->reg[rd] = cpu->reg[rm] - imm4;
            cpu->flag_z = (cpu->reg[rd] == 0);
            break;
        }

        case 0x9: // OR
            cpu->reg[rd] = cpu->reg[rm] | cpu->reg[rn];
            cpu->flag_z = (cpu->reg[rd] == 0);
            break;

        case 0xA: // MUL
            cpu->reg[rd] = cpu->reg[rm] * cpu->reg[rn];
            cpu->flag_z = (cpu->reg[rd] == 0);
            cpu->flag_c = 0;
            break;

        case 0xB: // AND
            cpu->reg[rd] = cpu->reg[rm] & cpu->reg[rn];
            cpu->flag_z = (cpu->reg[rd] == 0);
            break;

        case 0xC: // SHL
            cpu->reg[rd] = cpu->reg[rm] << imm4;
            cpu->flag_z = (cpu->reg[rd] == 0);
            break;

        case 0xD: { // CMP
            cpu->flag_z = (cpu->reg[rm] == cpu->reg[rn]);
            cpu->flag_c = (cpu->reg[rm] < cpu->reg[rn]);
            break;
        }

        case 0x0: // JMP
            cpu->reg[PC] = old_pc + imm12;
            break;

        case 0x1: { // JCOND
            uint16_t cond = (cpu->ir >> 14) & 0x3;
            if ((cond == 0 && cpu->flag_z) ||
                (cond == 1 && !cpu->flag_z) ||
                (cond == 2 && !cpu->flag_z && cpu->flag_c) ||
                (cond == 3 && (cpu->flag_z || !cpu->flag_c)))
                cpu->reg[PC] = old_pc + imm12;
            break;
        }

        default:
            printf("Instrucao invalida: 0x%04X\n", cpu->ir);
            dump_cpu(cpu);
            return;
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Uso: %s programa.txt\n", argv[0]);
        return 1;
    }

    CPU cpu;
    cpu_init(&cpu);
    load_program(&cpu, argv[1]);
    cpu_run(&cpu);
    return 0;
}
