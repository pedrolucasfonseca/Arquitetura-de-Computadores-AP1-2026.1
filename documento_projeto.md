# Documentação do Projeto — Interpretador de Instruções com Sensor de Distância

**Disciplina:** Arquitetura de Computadores  
**Plataforma:** Arduino Mega 2560  
**Repositório:** https://github.com/pedrolucasfonseca/Arquitetura-de-Computadores-AP1-2026.1.git

---

## Identificação do Grupo

| Matrícula | Nome | Participação |
|-----------|------|--------------|
| 202507010719 | Marcus Vinicius da Cunha Martins Junior — Representante | TA |
| 202507010697 | Pedro Lucas Fonseca Vieira | TA |
| 202508549621 | Maria Eduarda Alves Cruz | TA |
| 202508549492  | Juan Lucas Pereira | TA |

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


---

## 4. Conjunto de Instruções (ISA)

Cada instrução possui um mnemônico, um opcode binário de 4 bits e uma descrição funcional.

| Decimal | Opcode | Mnemônico | Operando | Descrição |
|---------|--------|-----------|----------|-----------|
| 0  | 0000 | NOP    | —   | Não realiza operação |
| 1  | 0001 | READ   | —   | Lê o sensor de distância e armazena o resultado em ACC |
| 2  | 0010 | LOADK  | k | Carrega a constante k em ACC |
| 3  | 0011 | ADDK   | k | ACC = ACC + k |
| 4  | 0100 | SUBK   | k | ACC = ACC - k |
| 5  | 0101 | CMPK   | k | Compara ACC com k; se iguais, FLAG_Z = true |
| 6  | 0110 | LEDON  | n | Liga o LED n (1, 2 ou 3) |
| 7  | 0111 | LEDOFF | n | Desliga o LED n (1, 2 ou 3) |
| 8  | 1000 | BUZON  | —   | Liga o buzzer |
| 9  | 1001 | BUZOFF | —   | Desliga o buzzer |
| 10 | 1010 | DISP   | —   | Exibe o valor de ACC no display de 7 segmentos |
| 11 | 1011 | ALERT  | —   | Lê o sensor e executa resposta automática por faixa |
| 12 | 1100 | BINC   | —   | Exibe no Serial Monitor o opcode binário da instrução atual |
| 13 | 1101 | STORE  | x | Armazena ACC em MEM[x] |
| 14 | 1110 | LOADM  | x | Carrega MEM[x] em ACC |
| 15 | 1111 | HALT   | —   | Encerra a execução do programa |

---

## 5. Funcionalidades

### F01 — Entrada de Instruções por Teclado

*Elemento arquitetural:* Entrada / I/O

O sistema recebe instruções pelo teclado matricial 4×4. No modo LOAD, o usuário digita o opcode em decimal, pressiona B para separar do operando, e C para confirmar. A tecla D apaga o buffer. O sistema monta a string da instrução e a armazena no vetor programa[].

---

### F02 — Codificação de Mnemônico para Opcode

*Elemento arquitetural:* ISA / assembly

A função codificarOpcode() recebe o número decimal digitado e retorna o opcode binário de 4 bits correspondente, que é carregado em IR. O mapeamento segue exatamente a tabela da ISA definida na seção 4.

---

### F03 — Controle do Ciclo de Instrução (UC)

*Elemento arquitetural:* Unidade de Controle

A função executarCiclo() implementa o ciclo completo: busca a instrução em programa[PC], chama codificarOpcode(), carrega o resultado em IR, decodifica e chama a função de execução correspondente, atualiza PC e exibe o estado no Serial Monitor.

---

### F04 — Operações Aritméticas e de Comparação (ULA)

*Elemento arquitetural:* ULA

| Instrução | Operação |
|---|---|
| ADDK k | ACC = ACC + k |
| SUBK k | ACC = ACC - k |
| CMPK k | FLAG_Z = (ACC == k) |

---

### F05 — Leitura do Sensor de Distância

*Elemento arquitetural:* Entrada de dados

A função lerSensor() aciona o HC-SR04 via pino TRIG (40) e mede o tempo de retorno pelo pino ECHO (41), convertendo para centímetros. O valor é armazenado em ACC.

---

### F06 — Controle de LEDs

*Elemento arquitetural:* Saída

As instruções LEDON n e LEDOFF n ligam e desligam os LEDs nos pinos 42 (LED 1), 43 (LED 2) e 44 (LED 3).

---

### F07 — Controle de Buzzer

*Elemento arquitetural:* Saída

As instruções BUZON e BUZOFF ligam e desligam o buzzer no pino 45.

---

### F08 — Exibição no Display de 7 Segmentos

*Elemento arquitetural:* Saída

O display é de *catodo comum*, acionado diretamente pelos pinos 22 a 28 (segmentos a–g). A instrução DISP exibe o valor de ACC em decimal (0–9). Valores fora desse intervalo são tratados conforme a seção 6.

Tabela de segmentos (catodo comum — 1 = aceso):

| Dígito | a | b | c | d | e | f | g |
|--------|---|---|---|---|---|---|---|
| 0 | 1 | 1 | 1 | 1 | 1 | 1 | 0 |
| 1 | 0 | 1 | 1 | 0 | 0 | 0 | 0 |
| 2 | 1 | 1 | 0 | 1 | 1 | 0 | 1 |
| 3 | 1 | 1 | 1 | 1 | 0 | 0 | 1 |
| 4 | 0 | 1 | 1 | 0 | 0 | 1 | 1 |
| 5 | 1 | 0 | 1 | 1 | 0 | 1 | 1 |
| 6 | 1 | 0 | 1 | 1 | 1 | 1 | 1 |
| 7 | 1 | 1 | 1 | 0 | 0 | 0 | 0 |
| 8 | 1 | 1 | 1 | 1 | 1 | 1 | 1 |
| 9 | 1 | 1 | 1 | 1 | 0 | 1 | 1 |
| E (overflow) | 1 | 0 | 0 | 1 | 1 | 1 | 1 |
| – (negativo) | 0 | 0 | 0 | 0 | 0 | 0 | 1 |

---

### F09 — Memória Simulada

*Elemento arquitetural:* Memória

O vetor MEM[16] armazena valores inteiros. STORE x grava ACC em MEM[x]; LOADM x lê MEM[x] e carrega em ACC.

---

### F10 — Resposta Automática por Faixa de Distância (ALERT)

*Elemento arquitetural:* Controle / processamento

| Faixa de distância | Comportamento |
|---|---|
| Distância < 10 cm | Liga buzzer + liga LED 1 (alerta) |
| 10 cm ≤ distância < 20 cm | Liga LED 1 (alerta); buzzer desligado |
| Distância ≥ 20 cm | Mantém buzzer e LED 1 desligados |

---

### F11 — Finalização por HALT

*Elemento arquitetural:* Controle

HALT define EXECUTANDO = false, interrompendo o ciclo. Novos * são ignorados. O Serial Monitor exibe [HALT] Execucao encerrada.

---

## 6. Tratamento de Resultados Não Representáveis

O display é limitado a 1 dígito decimal (0–9).

### 6.1 Overflow

Ocorre quando o resultado a exibir via DISP é *maior que 9*.

| Dispositivo | Comportamento |
|---|---|
| Display | Exibe E (segmentos a, d, e, f, g acesos) |
| Serial Monitor | [ERRO] Overflow: valor X nao representavel no display. |

Situações possíveis: resultado de ADDK ou valor carregado por LOADK acima de 9.

### 6.2 Resultado Negativo

Ocorre quando o resultado de uma operação é *menor que 0*.

| Dispositivo | Comportamento |
|---|---|
| Display | Exibe – (segmento g aceso) |
| Serial Monitor | [ERRO] Valor negativo: resultado X nao representavel no display. |

Situações possíveis: resultado de SUBK quando o operando é maior que ACC.

---

## 7. Descrição do Hardware

### 7.1 Componentes Utilizados

| Componente | Especificação |
|---|---|
| Microcontrolador | Arduino Mega 2560 (ATmega2560) |
| Teclado | Matricial 4×4 |
| Sensor de distância | HC-SR04 |
| LEDs | 3 × LED 5 mm |
| Buzzer | Buzzer ativo |
| Display | 7 segmentos, 1 dígito, *catodo comum* |
| Resistores para LEDs | 220 Ω a 330 Ω (um por LED) |
| Resistores para display | 220 Ω a 330 Ω (um por segmento) |
| Protoboard | Padrão de laboratório |
| Jumpers | Padrão de laboratório |

### 7.2 Regras de Conexão

- Cada LED possui resistor em série
- Cada segmento do display possui resistor em série
- O buzzer é conectado a pino digital de saída
- O sensor HC-SR04 utiliza pino para TRIG e pino para ECHO
- O teclado matricial é ligado diretamente a pinos digitais do Arduino
- O display é acionado exclusivamente por saídas digitais, sem driver dedicado

---

## 8. Pinagem do Arduino

Pinagem obrigatória conforme especificação do professor:

| Pino | Elemento |
|------|----------|
| 22 | Display — segmento a |
| 23 | Display — segmento b |
| 24 | Display — segmento c |
| 25 | Display — segmento d |
| 26 | Display — segmento e |
| 27 | Display — segmento f |
| 28 | Display — segmento g |
| 29 | Display — ponto decimal (opcional) |
| 30 | Teclado — linha 1 |
| 31 | Teclado — linha 2 |
| 32 | Teclado — linha 3 |
| 33 | Teclado — linha 4 |
| 34 | Teclado — coluna 1 |
| 35 | Teclado — coluna 2 |
| 36 | Teclado — coluna 3 |
| 37 | Teclado — coluna 4 |
| 40 | Sensor HC-SR04 — TRIG |
| 41 | Sensor HC-SR04 — ECHO |
| 42 | LED 1 |
| 43 | LED 2 |
| 44 | LED 3 |
| 45 | Buzzer |

---

## 9. Organização do Sketch

### 9.1 Estrutura de Funções

| Função | Responsabilidade |
|---|---|
| lerTeclado() | Leitura e debounce do teclado matricial |
| montarInstrucao() | Monta a string da instrução a partir das teclas pressionadas |
| codificarOpcode() | Converte decimal para opcode binário de 4 bits; carrega em IR |
| armazenarInstrucao() | Armazena instrução no vetor programa[] (modo LOAD) |
| executarCiclo() | UC: implementa busca → decodificação → execução |
| lerSensor() | Aciona HC-SR04 e retorna distância em cm |
| exibirDisplay() | Aciona segmentos do display conforme valor de ACC |
| controleLED() | Liga ou desliga LED conforme instrução e operando |
| controleBuzzer() | Liga ou desliga o buzzer |
| executarADDK() | ULA: soma operando a ACC |
| executarSUBK() | ULA: subtrai operando de ACC |
| executarCMPK() | ULA: compara ACC com operando e atualiza FLAG_Z |
| executarALERT() | Lê sensor e aciona saídas conforme faixa de distância |
| exibirEstado() | Exibe PC, IR, ACC, FLAG_Z no Serial Monitor |
| tratarErro() | Trata overflow e resultado negativo no display e Serial Monitor |

### 9.2 Boas Práticas Adotadas

- Registradores simulados declarados globalmente com comentários arquiteturais
- Nomes de variáveis e funções significativos e em português técnico
- Código indentado de forma consistente
- Entrada, codificação, controle, processamento e saída separados em funções distintas
- Nenhuma lógica de negócio concentrada no loop()

### 9.3 Cabeçalho do Sketch

cpp
/*
| 202508549492  | Juan Lucas Pereira | TA |
| 202507010719 | Marcus Vinicius da Cunha Martins Junior | TA |
| 202507010697 | Pedro Lucas Fonseca Vieira | TA |
| 202508549621 | Maria Eduarda Alves Cruz | TA |
*/


---

## 10. Setup de Testes

| Teste | O que será demonstrado |
|---|---|
| T01 — Codificação | Digitar mnemônico e exibir opcode binário no Serial Monitor |
| T02 — Leitura do sensor | Executar READ e mostrar valor em ACC |
| T03 — Operações da ULA | Executar ADDK e SUBK, explicando o papel de ACC |
| T04 — Comparação | Executar CMPK e mostrar atualização de FLAG_Z |
| T05 — Memória | Executar STORE e LOADM, explicando o vetor MEM |
| T06 — Saída visual e sonora | Executar LEDON, LEDOFF, BUZON e BUZOFF |
| T07 — ALERT | Demonstrar os três comportamentos por faixa de distância |
| T08 — Display | Exibir valores válidos e explicar a limitação a 1 dígito |
| T09 — Overflow | Resultado > 9: display exibe E, Serial Monitor informa overflow |
| T10 — Resultado negativo | Resultado < 0: display exibe –, Serial Monitor informa erro |
| T11 — HALT | Demonstrar interrupção correta da execução |
| T12 — Conceitual oral | Identificar no código: memória, PC, IR, ACC, UC e ULA |

---

## 11. Entregáveis

| # | Entregável | Status |
|---|---|---|
| 1 | Documentação do projeto em Markdown | ✅ Este documento |
| 2 | Sketch do Arduino comentado | ✅ https://github.com/pedrolucasfonseca/Arquitetura-de-Computadores-AP1-2026.1/tree/main/Sketch |
| 3 | Protótipo físico montado em protoboard e funcional | ✅ Entregue em aula |
| 4 | Repositório GitHub com documentação e código | ✅ https://github.com/pedrolucasfonseca/Arquitetura-de-Computadores-AP1-2026.1.git |
| 5 | Histórico de commits correspondente ao desenvolvimento | ✅ https://github.com/pedrolucasfonseca/Arquitetura-de-Computadores-AP1-2026.1/commits/main/ |

*Repositório:* https://github.com/claytonjasilva — nome a definir com o professor.

---

Documentação elaborada conforme os requisitos técnicos da disciplina de Arquitetura de Computadores — AP1 2026/1.