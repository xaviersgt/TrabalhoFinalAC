#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#define TAMANHO_MEMORIA 8192
#define NUM_REGS 16
#define SP 14
#define PC 15

#define IO_CHAR_ENTRADA 0xF000
#define IO_CHAR_SAIDA   0xF001
#define IO_INT_ENTRADA  0xF002
#define IO_INT_SAIDA    0xF003

typedef struct {
    uint16_t regs[NUM_REGS];
    uint16_t memoria[TAMANHO_MEMORIA];
    bool mem_acessada[TAMANHO_MEMORIA];
    uint16_t ir;
    int flag_z;
    int flag_c;
} CPU;

int16_t extensao_sinal(uint16_t val, int bits) {
    uint16_t mask = 1 << (bits - 1);
    return (val ^ mask) - mask;
}

void verificar_memoria(uint16_t end) {
    if (end >= TAMANHO_MEMORIA) {
        printf("Erro: Acesso invalido a memoria fisica 0x%04hX\n", (unsigned short)end);
        exit(1);
    }
}

void imprimir_estado_cpu(CPU *cpu) {
    for (int i = 0; i < NUM_REGS; i++) {
        printf("R%d = 0x%04hX\n", i, (unsigned short)cpu->regs[i]);
    }

    printf("Z = %d\n", cpu->flag_z);
    printf("C = %d\n", cpu->flag_c);

    for (int i = 0; i < TAMANHO_MEMORIA; i++) {
        bool eh_pilha = (cpu->regs[SP] != 0x2000) &&
                        (i >= cpu->regs[SP]) &&
                        (i <= 0x1FFF);

        if (cpu->mem_acessada[i] && !eh_pilha) {
            printf("[ 0x%04hX ] = 0x%04hX\n",
                   (unsigned short)i,
                   (unsigned short)cpu->memoria[i]);
        }
    }

    if (cpu->regs[SP] != 0x2000) {
        int sp = cpu->regs[SP];
        if (sp < TAMANHO_MEMORIA) {
            for (int i = 0x1FFF; i >= sp; i--) {
                printf("[ 0x%04hX ] = 0x%04hX\n",
                       (unsigned short)i,
                       (unsigned short)cpu->memoria[i]);
            }
        }
    }
}



void cpu_inicializar(CPU *cpu) {
    memset(cpu->regs, 0, sizeof(cpu->regs));
    memset(cpu->memoria, 0, sizeof(cpu->memoria));
    memset(cpu->mem_acessada, 0, sizeof(cpu->mem_acessada));

    cpu->flag_z = 0;
    cpu->flag_c = 0;
    cpu->regs[SP] = 0x2000;
    cpu->regs[PC] = 0;
}



void carregar_programa(CPU *cpu, const char *arquivo) {
    FILE *f = fopen(arquivo, "r");
    if (!f) {
        perror("Erro ao abrir arquivo");
        exit(1);
    }

    uint16_t end, val;
    while (fscanf(f, "%hx %hx", &end, &val) == 2) {
        if (end < TAMANHO_MEMORIA)
            cpu->memoria[end] = val;
    }
    fclose(f);
}

void cpu_executar(CPU *cpu, int num_bps, int *breakpoints) {
    while (1) {
        for (int i = 0; i < num_bps; i++) {
            if (cpu->regs[PC] == breakpoints[i]) {
                imprimir_estado_cpu(cpu);
                getchar();
            }
        }

        if (cpu->regs[PC] >= TAMANHO_MEMORIA) {
            break;
        }

        cpu->ir = cpu->memoria[cpu->regs[PC]];
        uint16_t pc_atual = cpu->regs[PC];
        cpu->regs[PC]++;

        uint16_t opcode = cpu->ir & 0xF;
        uint16_t rd     = (cpu->ir >> 12) & 0xF;
        uint16_t rm     = (cpu->ir >> 8) & 0xF;
        uint16_t rn     = (cpu->ir >> 4) & 0xF;

        uint16_t imm4_baixo = (cpu->ir >> 4) & 0xF;
        uint16_t imm4_alto  = (cpu->ir >> 12) & 0xF;
        int16_t  imm8       = extensao_sinal((cpu->ir >> 4) & 0xFF, 8);
        int16_t  imm12      = extensao_sinal((cpu->ir >> 4) & 0x0FFF, 12);

        switch (opcode) {
            case 0x0:
                cpu->regs[PC] = cpu->regs[PC] + imm12;
                break;
            case 0x1:
            {
                uint16_t cond = (cpu->ir >> 14) & 0x3;
                int16_t imm10 = extensao_sinal((cpu->ir >> 4) & 0x03FF, 10);
                bool saltar = false;

                switch (cond) {
                    case 0: saltar = (cpu->flag_z == 1); break;
                    case 1: saltar = (cpu->flag_z == 0); break;
                    case 2: saltar = (cpu->flag_z == 0 && cpu->flag_c == 1); break;
                    case 3: saltar = (cpu->flag_z == 1 || cpu->flag_c == 0); break;
                }

                if (saltar) {
                    cpu->regs[PC] = cpu->regs[PC] + imm10;
                }
                break;
            }

            case 0x4:
                cpu->regs[rd] = imm8;
                break;

            case 0x5:
            {
                uint32_t res = (uint32_t)cpu->regs[rm] + (uint32_t)cpu->regs[rn];
                cpu->regs[rd] = (uint16_t)res;
                cpu->flag_z = (cpu->regs[rd] == 0);
                cpu->flag_c = (res > 0xFFFF);
                break;
            }

            case 0x6:
            {
                uint32_t res = (uint32_t)cpu->regs[rm] + imm4_baixo;
                cpu->regs[rd] = (uint16_t)res;
                cpu->flag_z = (cpu->regs[rd] == 0);
                cpu->flag_c = (res > 0xFFFF);
                break;
            }

            case 0x7:
                cpu->flag_c = (cpu->regs[rm] < cpu->regs[rn]);
                cpu->regs[rd] = cpu->regs[rm] - cpu->regs[rn];
                cpu->flag_z = (cpu->regs[rd] == 0);
                break;

            case 0x8:
                cpu->flag_c = (cpu->regs[rm] < imm4_baixo);
                cpu->regs[rd] = cpu->regs[rm] - imm4_baixo;
                cpu->flag_z = (cpu->regs[rd] == 0);
                break;

            case 0x9:
                cpu->regs[rd] = cpu->regs[rm] & cpu->regs[rn];
                cpu->flag_z = (cpu->regs[rd] == 0);
                break;

            case 0xA:
                cpu->regs[rd] = cpu->regs[rm] | cpu->regs[rn];
                cpu->flag_z = (cpu->regs[rd] == 0);
                break;

            case 0xB:
                cpu->regs[rd] = cpu->regs[rm] >> imm4_baixo;
                cpu->flag_z = (cpu->regs[rd] == 0);
                break;

            case 0xC:
                cpu->regs[rd] = cpu->regs[rm] << imm4_baixo;
                cpu->flag_z = (cpu->regs[rd] == 0);
                break;

            case 0xD:
                cpu->flag_z = (cpu->regs[rm] == cpu->regs[rn]);
                cpu->flag_c = (cpu->regs[rm] < cpu->regs[rn]);
                break;

            case 0x2:
            {
                uint16_t end = cpu->regs[rm] + imm4_baixo;

                if (end >= 0xF000) {
                    if (end == IO_CHAR_ENTRADA) {
                        int ch = getchar();
                        cpu->regs[rd] = (uint16_t)(unsigned char)ch;
                        printf("IN %c\n", (char)cpu->regs[rd]);
                    }
                    else if (end == IO_INT_ENTRADA) {
                        short tmp;
                        scanf("%hd", &tmp);
                        cpu->regs[rd] = (uint16_t)tmp;
                        printf("IN %hd\n", tmp);
                    }
                }
                else {
                    verificar_memoria(end);
                    cpu->mem_acessada[end] = true;
                    cpu->regs[rd] = cpu->memoria[end];
                }
                break;
            }

            case 0x3:
            {
                uint16_t end = cpu->regs[rm] + imm4_alto;
                uint16_t dado = cpu->regs[rn];

                if (end >= 0xF000) {
                    if (end == IO_CHAR_SAIDA) {
                        printf("OUT %c\n", (char)dado);
                    }
                    else if (end == IO_INT_SAIDA) {
                        printf("OUT %hd\n", (short)dado);
                    }
                }
                else {
                    verificar_memoria(end);
                    cpu->mem_acessada[end] = true;
                    cpu->memoria[end] = dado;
                }
                break;
            }

            case 0xE:
                cpu->regs[SP]--;
                verificar_memoria(cpu->regs[SP]);
                cpu->memoria[cpu->regs[SP]] = cpu->regs[rn];
                break;

            case 0xF:
                if (cpu->ir == 0xFFFF) {
                    imprimir_estado_cpu(cpu);
                    return;
                } else {
                    verificar_memoria(cpu->regs[SP]);
                    cpu->regs[rd] = cpu->memoria[cpu->regs[SP]];
                    cpu->regs[SP]++;
                }
                break;

            default:
                printf("Instrucao desconhecida: 0x%04hX em PC=0x%04hX\n",
                       (unsigned short)cpu->ir, (unsigned short)pc_atual);
                imprimir_estado_cpu(cpu);
                return;
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Uso: %s <arquivo.hex> [breakpoint1] [breakpoint2] ...\n", argv[0]);
        return 1;
    }

    CPU cpu;
    cpu_inicializar(&cpu);
    carregar_programa(&cpu, argv[1]);

    int num_bps = argc - 2;
    int *breakpoints = NULL;

    if (num_bps > 0) {
        breakpoints = (int*) malloc(num_bps * sizeof(int));
        if (!breakpoints) {
            printf("Erro de alocacao de memoria\n");
            return 1;
        }
        for (int i = 0; i < num_bps; i++) {
            breakpoints[i] = (int)strtol(argv[i + 2], NULL, 16);
        }
    }

    cpu_executar(&cpu, num_bps, breakpoints);

    if (breakpoints) free(breakpoints);
    return 0;
}
