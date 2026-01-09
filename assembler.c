#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>

#define MAX_LINHAS 1024
#define MAX_LABELS 256

typedef struct {
    char nome[32];
    uint16_t endereco;
} Label;

Label labels[MAX_LABELS];
int qtd_labels = 0;
char linhas[MAX_LINHAS][256];
int qtd_linhas = 0;


void trim(char *s) {
    char *p = s;
    while (isspace(*p)) p++;
    memmove(s, p, strlen(p) + 1);
    for (int i = strlen(s) - 1; i >= 0 && isspace(s[i]); i--) s[i] = '\0';
}

int reg_num(char *r) {
    while (isspace(*r)) r++;
    if (tolower(r[0]) != 'r') return -1;
    return atoi(&r[1]);
}

int buscar_label(char *nome) {
    for (int i = 0; i < qtd_labels; i++) {
        if (strcmp(labels[i].nome, nome) == 0) return labels[i].endereco;
    }
    return -1;
}


int parse_int(char *s) {
    if (s[0] == '#') s++;
    return (int)strtol(s, NULL, 0);
}


void primeira_passagem() {
    uint16_t pc = 0;
    for (int i = 0; i < qtd_linhas; i++) {
        char linha[256];
        strcpy(linha, linhas[i]);
        trim(linha);
        if (linha[0] == '\0' || linha[0] == ';') continue;

        char *pos_dois_pontos = strchr(linha, ':');
        if (pos_dois_pontos) {
            char nome[32];
            sscanf(linha, "%[^:]", nome);
            trim(nome);
            strcpy(labels[qtd_labels].nome, nome);
            labels[qtd_labels].endereco = pc;
            qtd_labels++;
            
            char *resto = pos_dois_pontos + 1;
            trim(resto);
            if (*resto == '\0' || *resto == ';') continue; 
        }
        pc++;
    }
}


void segunda_passagem(FILE *out) {
    uint16_t pc = 0;
    for (int i = 0; i < qtd_linhas; i++) {
        char linha[256];
        strcpy(linha, linhas[i]);
        trim(linha);
        if (linha[0] == '\0' || linha[0] == ';') continue;

        char *pos_dois_pontos = strchr(linha, ':');
        char *conteudo = pos_dois_pontos ? pos_dois_pontos + 1 : linha;
        trim(conteudo);
        if (*conteudo == '\0' || *conteudo == ';') continue;

        uint16_t instr = 0;
        char op[16];
        sscanf(conteudo, "%s", op);


        if (isdigit(conteudo[0]) || (conteudo[0] == '0' && tolower(conteudo[1]) == 'x')) {
            instr = (uint16_t)strtol(conteudo, NULL, 0);
        }

        else if (strcmp(op, "MOV") == 0) {
            char rd[8], val_str[32];
            sscanf(conteudo, "MOV %[^,], %s", rd, val_str);
            instr |= (reg_num(rd) << 12) | 0x4;
            char *val_ptr = (val_str[0] == '#') ? &val_str[1] : val_str;
            if (isdigit(val_ptr[0])) instr |= ((parse_int(val_ptr) & 0xFF) << 4);
            else {
                int alvo = buscar_label(val_ptr);
                if (alvo != -1) instr |= ((alvo & 0xFF) << 4);
            }
        }

        else if (strcmp(op, "ADD")==0 || strcmp(op, "SUB")==0 || strcmp(op, "AND")==0 || strcmp(op, "OR")==0) {
            char rd[8], rm[8], rn[8];
            sscanf(conteudo, "%*s %[^,], %[^,], %s", rd, rm, rn);
            instr |= (reg_num(rd) << 12) | (reg_num(rm) << 8) | (reg_num(rn) << 4);
            if (strcmp(op, "ADD")==0) instr |= 0x5;
            else if (strcmp(op, "SUB")==0) instr |= 0x7;
            else if (strcmp(op, "AND")==0) instr |= 0x9;
            else instr |= 0xA;
        }

        else if (strcmp(op, "ADDI")==0 || strcmp(op, "SUBI")==0 || strcmp(op, "SHR")==0 || strcmp(op, "SHL")==0) {
            char rd[8], rm[8], imm_s[16];
            sscanf(conteudo, "%*s %[^,], %[^,], %s", rd, rm, imm_s);
            instr |= (reg_num(rd) << 12) | (reg_num(rm) << 8) | ((parse_int(imm_s) & 0xF) << 4);
            if (strcmp(op, "ADDI")==0) instr |= 0x6;
            else if (strcmp(op, "SUBI")==0) instr |= 0x8;
            else if (strcmp(op, "SHR")==0) instr |= 0xB;
            else instr |= 0xC;
        }

        else if (op[0] == 'J') {
            char lbl[32]; sscanf(conteudo, "%*s %s", lbl);
            char *l_ptr = (lbl[0] == '#') ? &lbl[1] : lbl;
            int alvo = buscar_label(l_ptr);
            int16_t off = (alvo != -1) ? alvo - (pc + 1) : 0;
            if (strcmp(op, "JMP") == 0) instr = ((off & 0xFFF) << 4) | 0x0;
            else {
                instr |= 0x1 | ((off & 0x3FF) << 4);
                if (strcmp(op, "JNE") == 0) instr |= (1 << 14);
                else if (strcmp(op, "JLT") == 0) instr |= (2 << 14);
                else if (strcmp(op, "JGE") == 0) instr |= (3 << 14);
            }
        }
        else if (strcmp(op, "LDR") == 0) {
            char rd[8], rm[8]; int imm;
            sscanf(conteudo, "LDR %[^,], [%[^,], #%d]", rd, rm, &imm);
            instr = (reg_num(rd) << 12) | (reg_num(rm) << 8) | ((imm & 0xF) << 4) | 0x2;
        }
        else if (strcmp(op, "STR") == 0) {
            char rn[8], rm[8]; int imm;
            sscanf(conteudo, "STR %[^,], [%[^,], #%d]", rn, rm, &imm);
            instr = ((imm & 0xF) << 12) | (reg_num(rm) << 8) | (reg_num(rn) << 4) | 0x3;
        }
        else if (strcmp(op, "PUSH") == 0) {
            char rn[8]; sscanf(conteudo, "PUSH %s", rn);
            instr = (reg_num(rn) << 4) | 0xE;
        }
        else if (strcmp(op, "POP") == 0) {
            char rd[8]; sscanf(conteudo, "POP %s", rd);
            instr = (reg_num(rd) << 12) | 0xF;
        }
        else if (strcmp(op, "HALT") == 0) instr = 0xFFFF;

        fprintf(out, "%04X %04X\n", pc, instr);
        pc++;
    }
}

int main(int argc, char *argv[]) {
    if (argc != 3) { printf("Uso: ./assembler input.asm output.hex\n"); return 1; }
    FILE *in = fopen(argv[1], "r");
    if (!in) { perror("Erro ao abrir arquivo de entrada"); return 1; }
    while (fgets(linhas[qtd_linhas], 256, in)) { qtd_linhas++; }
    fclose(in);
    printf("Linhas lidas: %d\n", qtd_linhas);
    if (qtd_linhas == 0) { printf("Erro: Arquivo de entrada vazio!\n"); return 1; }
    primeira_passagem();
    FILE *out = fopen(argv[2], "w");
    segunda_passagem(out);
    fclose(out);
    printf("Sucesso! Labels encontradas: %d. Arquivo %s gerado.\n", qtd_labels, argv[2]);
    return 0;
}