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