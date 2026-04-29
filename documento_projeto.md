# Documentação do Projeto — Interpretador de Instruções com Sensor de Distância

**Disciplina:** Arquitetura de Computadores  
**Plataforma:** Arduino Mega 2560  
**Repositório:** https://github.com/pedrolucasfonseca/Arquitetura-de-Computadores-AP1-2026.1.git

---

## Identificação do Grupo

| Matrícula | Nome | Participação |
|-----------|------|--------------|
| [MATRÍCULA] | [NOME] — Representante | TA |
| [MATRÍCULA] | [NOME] | TA |
| 202508549621 | Maria Eduarda Alves Cruz | TA |
| [MATRÍCULA] | [NOME] | TA |

---

## Sumário

1. [Descrição Geral do Projeto](#1-descrição-geral-do-projeto)
2. [Elementos Arquiteturais Implementados](#2-elementos-arquiteturais-implementados)
3. [Modos de Operação](#3-modos-de-operação)
4. [Conjunto de Instruções (ISA)](#4-conjunto-de-instruções-isa)
5. [Funcionalidades](#5-funcionalidades)
6. [Tratamento de Resultados Não Representáveis](#6-tratamento-de-resultados-não-representáveis)
7. [Descrição do Hardware](#7-descrição-do-hardware)
8. [Pinagem do Arduino](#8-pinagem-do-arduino)
9. [Organização do Sketch](#9-organização-do-sketch)
10. [Setup de Testes](#10-setup-de-testes)
11. [Entregáveis](#11-entregáveis)

---

## 1. Descrição Geral do Projeto

### 1.1 Escopo

O projeto consiste no desenvolvimento de um sistema embarcado baseado no **Arduino Mega 2560** que implementa um **interpretador de instruções**. O sistema recebe comandos por meio de um **teclado matricial 4×4**, interpreta esses comandos no formato **mnemônico**, converte-os internamente para seus respectivos **opcodes binários de 4 bits** e executa as ações físicas e lógicas correspondentes.

O projeto evidencia os seguintes elementos de Arquitetura de Computadores:

- **Entrada:** teclado matricial para inserção de instruções
- **Unidade de Controle (UC):** lógica de busca, codificação, decodificação e controle de execução das instruções
- **Unidade Lógica e Aritmética (ULA):** operações aritméticas (`ADDK`, `SUBK`) e de comparação (`CMPK`)
- **Memória:** vetor `MEM[16]` para armazenamento de dados simulado
- **Registradores simulados:** variáveis `PC`, `IR` e `ACC`
- **Saída:** LEDs, buzzer, display de 7 segmentos e Serial Monitor
- **Sensor de distância:** fonte de dados para processamento

O sistema opera segundo o modelo de **programa armazenado**, implementando o **ciclo de instrução** (busca → decodificação → execução) em coerência com os conceitos da disciplina.

### 1.2 Não Escopo

Não fazem parte do escopo deste projeto:

- Implementação de eletrônica avançada
- Uso de CI controlador dedicado para display
- Implementação fiel de ISA de processadores comerciais
- Comunicação em rede
- Uso de componentes não previstos no laboratório

### 1.3 Objetivos

- Consolidar os conceitos de programa armazenado, ciclo de instrução, memória, registradores, ISA, representação binária e entrada/saída
- Relacionar abstrações arquiteturais estudadas em sala com uma implementação prática em Arduino
- Exercitar a organização estruturada de código com clareza, modularidade e documentação

---

## 2. Elementos Arquiteturais Implementados

Os elementos arquiteturais abaixo são implementados como variáveis globais no sketch, com nomes padronizados e comentários explicativos.

```cpp
int  MEM[16];           // Memória de dados simulada (16 posições)
int  PC = 0;            // Program Counter: índice da instrução corrente
byte IR = 0;            // Instruction Register: armazena o opcode da instrução atual
int  ACC = 0;           // Acumulador: armazena operandos e resultados de operações
bool FLAG_Z = false;    // Flag de zero: resultado de comparação (CMPK)
bool EXECUTANDO = true; // Controle de execução: false quando HALT é processado
```

### Correspondência com a arquitetura estudada

| Variável / Estrutura | Elemento arquitetural | Descrição |
|---|---|---|
| `MEM[16]` | Memória de dados | Armazena valores intermediários via `STORE` e `LOADM` |
| `PC` | Program Counter | Controla qual instrução será buscada a seguir |
| `IR` | Instruction Register | Recebe o opcode binário da instrução corrente |
| `ACC` | Acumulador | Usado pela ULA e pelas instruções de entrada/saída |
| `FLAG_Z` | Flag de comparação | Atualizada por `CMPK`; indica igualdade entre `ACC` e operando |
| `EXECUTANDO` | Controle de fluxo | Encerra o ciclo de instrução quando `HALT` é executado |
| `programa[]` | Memória de programa | Vetor de strings que armazena as instruções carregadas no modo LOAD |
| Função `executarCiclo()` | Unidade de Controle (UC) | Implementa o ciclo busca → decodificação → execução |
| Funções `executarADDK()`, `executarSUBK()`, `executarCMPK()` | ULA | Realizam operações aritméticas e de comparação |

---

## 3. Modos de Operação

O sistema opera em dois modos distintos, caracterizando o conceito de **programa armazenado**.

### 3.1 Modo de Entrada de Programa (LOAD)

O modo LOAD é ativado e encerrado pela tecla `#` do teclado.

**Ao entrar no modo LOAD, o sistema:**
- Limpa o vetor de memória de programa
- Zera o ponteiro de carga (índice de armazenamento)
- Aguarda instruções do usuário

**Durante o modo LOAD:**
- O usuário digita instruções no formato `opcode_decimal [operando]`
- A tecla `B` funciona como espaço, separando o opcode do operando
- A tecla `C` confirma a instrução (funciona como ENTER)
- A tecla `D` apaga o buffer atual, permitindo redigitar
- Cada instrução confirmada é armazenada na próxima posição do vetor `programa[]`
- O Serial Monitor exibe o mnemônico correspondente para confirmação visual
- As instruções **não são executadas** nesta etapa

**Ao pressionar `#` novamente, o modo LOAD é encerrado.**

#### Mapeamento do teclado no modo LOAD

| Tecla | Função |
|-------|--------|
| `0`–`9` | Dígitos do opcode ou operando |
| `B` | Espaço — separa opcode do operando |
| `C` | Confirma (ENTER) — armazena a instrução |
| `D` | Apaga o buffer atual |
| `#` | Entra / sai do modo LOAD |

#### Exemplo de entrada de programa

O programa abaixo em mnemônico:

```
LOADK 5
ADDK  3
DISP
HALT
```

É inserido no teclado como (opcode decimal `B` operando `C`):

```
2 B 5 C
3 B 3 C
10 C
15 C
```

O Serial Monitor exibe durante o carregamento:

```
[LOAD] Instrucao 0: LOADK 5
[LOAD] Instrucao 1: ADDK 3
[LOAD] Instrucao 2: DISP
[LOAD] Instrucao 3: HALT
[LOAD] Modo encerrado. 4 instrucoes carregadas.
```

### 3.2 Modo de Execução (RUN)

A execução é iniciada pelo comando `RUN` digitado no teclado e avança uma instrução por vez a cada pressionamento de `*`.

**Ao receber `RUN`, o sistema:**
- Inicializa `PC = 0`
- Define `EXECUTANDO = true`
- Entra em modo de execução, aguardando comandos

**A cada pressionamento de `*`, o sistema executa um ciclo de instrução completo:**

1. **Busca:** `instrucao ← programa[PC]`
2. **Codificação:** converte o opcode decimal para binário de 4 bits
3. **Carga no IR:** `IR ← opcode`
4. **Decodificação:** identifica a instrução a executar
5. **Execução:** chama a função correspondente
6. **Atualização:** `ACC`, `MEM`, `FLAG_Z` e dispositivos de saída, conforme aplicável
7. **Incremento:** `PC = PC + 1`
8. **Exibição:** estado interno mostrado no Serial Monitor

**Após cada instrução, o sistema aguarda novo `*`.**

#### Exemplo de saída no Serial Monitor durante RUN

```
PC: 0 | IR: 0010 (LOADK) | ACC: 5 | FLAG_Z: 0
PC: 1 | IR: 0011 (ADDK)  | ACC: 8 | FLAG_Z: 0
PC: 2 | IR: 1010 (DISP)  | ACC: 8 | FLAG_Z: 0
PC: 3 | IR: 1111 (HALT)  | ACC: 8 | FLAG_Z: 0
[HALT] Execucao encerrada.
```

### 3.3 Condição de Parada

A execução é encerrada exclusivamente quando a instrução `HALT` (opcode `1111`) é processada. Após isso, `EXECUTANDO = false` e novos comandos `*` são ignorados até que um novo `RUN` seja emitido.

### 3.4 Fluxo Geral

```
# (pressiona)
  └─> Modo LOAD ativado
        └─> Usuário digita instruções (opcode B operando C)
              └─> # (pressiona novamente)
                    └─> Modo LOAD encerrado

RUN (digita)
  └─> PC = 0, aguarda *
        └─> * (pressiona)
              └─> Executa 1 instrução → exibe estado interno
                    └─> * (pressiona) → repete...
                          └─> HALT → EXECUTANDO = false → encerrado
```