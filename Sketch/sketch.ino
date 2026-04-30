/*
 * ============================================================
 *  SISTEMA INTERPRETADOR DE INSTRUÇÕES COM SENSOR DE DISTÂNCIA
 *  Arduino Mega 2560
 * ============================================================
 *
 * Disciplina: Arquitetura de Computadores
 * Trabalho AP1 - 2026/1
 *
 *  202508549492 Juan Lucas Pereira - TA
 *  Marcus Vinicius - TA
 *
 * Descrição geral:
 *   Implementa um interpretador de instruções simulando os
 *   elementos de um processador: memória de programa, memória
 *   de dados, registradores (PC, IR, ACC), Unidade de Controle
 *   (UC) e Unidade Lógica e Aritmética (ULA).
 *
 *   Modo LOAD (#): armazena instruções sem executar.
 *   Modo RUN (R+U+N+#): executa uma instrução por vez com *.
 * ============================================================
 */

#include <Keypad.h>

// ============================================================
//  PINAGEM OBRIGATÓRIA (conforme especificação do trabalho)
// ============================================================
#define PIN_SEG_A  22   // display segmento a
#define PIN_SEG_B  23   // display segmento b
#define PIN_SEG_C  24   // display segmento c
#define PIN_SEG_D  25   // display segmento d
#define PIN_SEG_E  26   // display segmento e
#define PIN_SEG_F  27   // display segmento f
#define PIN_SEG_G  28   // display segmento g

#define PIN_ROW1   30   // teclado linha 1
#define PIN_ROW2   31   // teclado linha 2
#define PIN_ROW3   32   // teclado linha 3
#define PIN_ROW4   33   // teclado linha 4
#define PIN_COL1   34   // teclado coluna 1
#define PIN_COL2   35   // teclado coluna 2
#define PIN_COL3   36   // teclado coluna 3
#define PIN_COL4   37   // teclado coluna 4

#define PIN_TRIG   40   // sensor HC-SR04 trigger
#define PIN_ECHO   41   // sensor HC-SR04 echo

#define PIN_LED1   42   // LED 1 (verde)
#define PIN_LED2   43   // LED 2 (amarelo)
#define PIN_LED3   44   // LED 3 (vermelho — alerta)
#define PIN_BUZZER 45   // buzzer ativo

// ============================================================
//  TECLADO MATRICIAL 4x4
//  Layout:
//    1 2 3 A
//    4 5 6 B
//    7 8 9 C
//    * 0 # D
// ============================================================
const byte ROWS = 4;
const byte COLS = 4;

char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};

byte rowPins[ROWS] = {PIN_ROW1, PIN_ROW2, PIN_ROW3, PIN_ROW4};
byte colPins[COLS] = {PIN_COL1, PIN_COL2, PIN_COL3, PIN_COL4};

Keypad teclado = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// ============================================================
//  ELEMENTOS ARQUITETURAIS SIMULADOS (obrigatórios)
// ============================================================
int  MEM[16];           // memória de dados simulada (16 posições)
int  PC = 0;            // program counter — aponta instrução atual
byte IR = 0;            // instruction register — opcode corrente
int  ACC = 0;           // acumulador — armazena operandos e resultados
bool FLAG_Z = false;    // flag de zero — resultado de comparação (CMPK)
bool EXECUTANDO = true; // controle de execução do programa

// ============================================================
//  MEMÓRIA DE PROGRAMA
//  Armazena instruções no formato "OPCODE [OPERANDO]"
//  Ex.: "2 5" = LOADK 5 | "15" = HALT
// ============================================================
#define MAX_INSTRUCOES 32
String programa[MAX_INSTRUCOES];
int    totalInstrucoes = 0;
int    ponteiroCarga   = 0;

// ============================================================
//  MODOS DE OPERAÇÃO
// ============================================================
bool modoLOAD = false;
bool modoRUN  = false;
bool DEBUG_SERIAL = false;

// ============================================================
//  OPCODES (ISA — 4 bits, decimal 0..15)
// ============================================================
#define OP_NOP    0
#define OP_READ   1
#define OP_LOADK  2
#define OP_ADDK   3
#define OP_SUBK   4
#define OP_CMPK   5
#define OP_LEDON  6
#define OP_LEDOFF 7
#define OP_BUZON  8
#define OP_BUZOFF 9
#define OP_DISP   10
#define OP_ALERT  11
#define OP_BINC   12
#define OP_STORE  13
#define OP_LOADM  14
#define OP_HALT   15

// Mnemônicos para exibição no Serial Monitor
const char* MNEMONICOS[] = {
  "NOP","READ","LOADK","ADDK","SUBK","CMPK",
  "LEDON","LEDOFF","BUZON","BUZOFF","DISP",
  "ALERT","BINC","STORE","LOADM","HALT"
};

// ============================================================
//  DISPLAY DE 7 SEGMENTOS — padrões (cátodo comum)
//  Bits: dp g f e d c b a  (bit7..bit0)
// ============================================================
const byte SEG7[] = {
  0b00111111,  // 0
  0b00000110,  // 1
  0b01011011,  // 2
  0b01001111,  // 3
  0b01100110,  // 4
  0b01101101,  // 5
  0b01111101,  // 6
  0b00000111,  // 7
  0b01111111,  // 8
  0b01101111,  // 9
  0b01111001,  // E  (índice 10 — overflow)
  0b01000000,  // -  (índice 11 — negativo)
  0b00000000   // apagado (índice 12)
};

// ============================================================
//  BUFFER DE ENTRADA
//  Acumula o que o usuário digita pelo teclado
// ============================================================
String bufferEntrada = "";

// ============================================================
//  PROTÓTIPOS DE FUNÇÕES
// ============================================================
void processarTecla(char tecla);
void entrarModoLOAD();
void sairModoLOAD();
void armazenarInstrucao(String instrucao);
void iniciarRUN();
void executarProximaInstrucao();
byte obterOpcode(String instrucao);
int  obterOperando(String instrucao);
bool temOperando(byte op);
void exibirEstado();
void acenderDisplay(byte padrao);
void exibirNoDisplay(int valor);
int  lerSensor();
void ligarLED(int n);
void desligarLED(int n);
int  pinLED(int n);
int ultimoEstadoALERT = -1;

// ============================================================
//  SETUP
// ============================================================
void setup() {
  Serial.begin(9600);

  // Configura pinos do display como saída
  for (int p = PIN_SEG_A; p <= PIN_SEG_G; p++) {
    pinMode(p, OUTPUT);
    digitalWrite(p, LOW);
  }

  // Configura LEDs como saída
  pinMode(PIN_LED1, OUTPUT); digitalWrite(PIN_LED1, LOW);
  pinMode(PIN_LED2, OUTPUT); digitalWrite(PIN_LED2, LOW);
  pinMode(PIN_LED3, OUTPUT); digitalWrite(PIN_LED3, LOW);

  // Configura buzzer como saída
  pinMode(PIN_BUZZER, OUTPUT); digitalWrite(PIN_BUZZER, LOW);

  // Configura sensor
  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);

  // Inicializa memória de dados com zeros
  for (int i = 0; i < 16; i++) MEM[i] = 0;

  Serial.println("==============================================");
  Serial.println(" INTERPRETADOR DE INSTRUCOES - Arduino Mega");
  Serial.println("==============================================");
  Serial.println("Comandos:");
  Serial.println("  #         -> entra/sai do modo LOAD");
  Serial.println("  R U N #   -> inicia execucao (RUN)");
  Serial.println("  *         -> executa proxima instrucao");
  Serial.println("----------------------------------------------");
  Serial.println("No modo LOAD:");
  Serial.println("  Digits 0-9 -> digita opcode/operando");
  Serial.println("  B          -> espaco entre opcode e operando");
  Serial.println("  C          -> confirma instrucao (ENTER)");
  Serial.println("  D          -> apaga buffer");
  Serial.println("==============================================");
}

// ============================================================
//  LOOP PRINCIPAL
// ============================================================
void loop() {
  char tecla = teclado.getKey();
  if (tecla) {
    processarTecla(tecla);
  }
}

// ============================================================
//  F01 — ENTRADA DE INSTRUÇÕES
//  Interpreta cada tecla conforme o modo atual do sistema.
// ============================================================
void processarTecla(char tecla) {

  // '#' alterna modo LOAD / confirma saída do LOAD
  if (tecla == '#') {
    if (!modoLOAD && !modoRUN) {
      // Verifica se o usuário digitou "RUN" antes do '#'
      if (bufferEntrada == "RUN") {
        bufferEntrada = "";
        iniciarRUN();
      } else {
        // Entra no modo LOAD
        bufferEntrada = "";
        entrarModoLOAD();
      }
    } else if (modoLOAD) {
      // Confirma última instrução pendente e sai do LOAD
      if (bufferEntrada.length() > 0) {
        armazenarInstrucao(bufferEntrada);
        bufferEntrada = "";
      }
      sairModoLOAD();
    }
    return;
  }

  // '*' executa próxima instrução no modo RUN
  if (tecla == '*') {
    if (modoRUN) {
      executarProximaInstrucao();
    } else {
      Serial.println("[AVISO] Digite R U N # para iniciar a execucao.");
    }
    return;
  }

  // Letras R, U, N formam o comando RUN fora do modo LOAD
  if (!modoLOAD && (tecla == 'A' || tecla == 'B' || tecla == 'C')) {
      if (tecla == 'A') bufferEntrada += 'R';
      if (tecla == 'B') bufferEntrada += 'U';
      if (tecla == 'C') bufferEntrada += 'N';
      Serial.print(bufferEntrada == "R" ? "\nDigitando: R" :
                  bufferEntrada == "RU" ? "U" : "N");
      return;
  }
  // Modo LOAD: acumula dígitos e comandos de edição
  if (modoLOAD) {
    if (tecla >= '0' && tecla <= '9') {
      bufferEntrada += tecla;
      Serial.print(tecla);
    } else if (tecla == 'B') {
      // 'B' = espaço (separa opcode do operando)
      if (bufferEntrada.length() > 0) {
        bufferEntrada += ' ';
        Serial.print(' ');
      }
    } else if (tecla == 'C') {
      // 'C' = confirmar instrução (ENTER)
      if (bufferEntrada.length() > 0) {
        Serial.println();
        armazenarInstrucao(bufferEntrada);
        bufferEntrada = "";
      }
    } else if (tecla == 'D') {
      // 'D' = apagar buffer
      bufferEntrada = "";
      Serial.println(" [APAGADO]");
    }
  }
}

// ============================================================
//  Ativa o modo LOAD e limpa a memória de programa
// ============================================================
void entrarModoLOAD() {
  modoLOAD        = true;
  modoRUN         = false;
  EXECUTANDO      = false;
  ponteiroCarga   = 0;
  totalInstrucoes = 0;
  for (int i = 0; i < MAX_INSTRUCOES; i++) programa[i] = "";

  Serial.println("\n>>> MODO LOAD ATIVADO <<<");
  Serial.println("Digite opcode [B operando] C para cada instrucao.");
  Serial.println("Pressione # para finalizar.");
}

// ============================================================
//  Desativa o modo LOAD e lista o programa armazenado
// ============================================================
void sairModoLOAD() {
  modoLOAD = false;

  Serial.println("\n>>> MODO LOAD ENCERRADO <<<");
  Serial.print("Instrucoes carregadas: ");
  Serial.println(totalInstrucoes);
  Serial.println("--- Programa ---");
  for (int i = 0; i < totalInstrucoes; i++) {
    byte op  = obterOpcode(programa[i]);
    int  arg = obterOperando(programa[i]);
    Serial.print("  [");
    Serial.print(i);
    Serial.print("] ");
    if (op <= 15) {
      Serial.print(MNEMONICOS[op]);
      if (temOperando(op)) { Serial.print(" "); Serial.print(arg); }
    }
    Serial.println();
  }
  Serial.println("---");
  Serial.println("Para executar: pressione A, B, C e depois #  (R-U-N #)");
}

// ============================================================
//  Armazena instrução validada na memória de programa
// ============================================================
void armazenarInstrucao(String instrucao) {
  instrucao.trim();
  if (instrucao.length() == 0) return;
  if (ponteiroCarga >= MAX_INSTRUCOES) {
    Serial.println("[ERRO] Memoria de programa cheia!");
    return;
  }
  byte op = obterOpcode(instrucao);
  if (op > 15) {
    Serial.println("[ERRO] Instrucao invalida: " + instrucao);
    return;
  }
  programa[ponteiroCarga] = instrucao;
  Serial.print("[LOAD] [");
  Serial.print(ponteiroCarga);
  Serial.print("] -> ");
  Serial.print(MNEMONICOS[op]);
  if (temOperando(op)) { Serial.print(" "); Serial.print(obterOperando(instrucao)); }
  Serial.println();
  ponteiroCarga++;
  totalInstrucoes = ponteiroCarga;
}

// ============================================================
//  F03 — UNIDADE DE CONTROLE: inicializa modo RUN
// ============================================================
void iniciarRUN() {
  if (totalInstrucoes == 0) {
    Serial.println("[AVISO] Nenhum programa carregado. Use # para entrar no LOAD.");
    return;
  }
  modoLOAD   = false;
  modoRUN    = true;
  EXECUTANDO = true;
  PC         = 0;

  Serial.println("\n>>> MODO RUN ATIVADO <<<");
  Serial.println("Pressione * para executar cada instrucao.");
  Serial.println("----------------------------------------------");
}

// ============================================================
//  F03 — UNIDADE DE CONTROLE: ciclo de instrução completo
//
//  Etapas (a cada acionamento de '*'):
//    1. Busca:        instrucao ← programa[PC]
//    2. Codificação:  IR ← opcode da instrucao
//    3. Decodificação: interpreta IR
//    4. Execução:     realiza a operação
//    5. Atualização:  PC++, exibe estado
// ============================================================
void executarProximaInstrucao() {
  if (!EXECUTANDO) {
    Serial.println("[HALT] Programa encerrado. Digite R U N # para reiniciar.");
    return;
  }
  if (PC >= totalInstrucoes) {
    Serial.println("[FIM] Fim do programa sem HALT.");
    EXECUTANDO = false; modoRUN = false;
    return;
  }

  // ETAPA 1 — Busca
  String instrucaoAtual = programa[PC];

  // ETAPA 2 — Codificação: carrega opcode em IR
  IR = obterOpcode(instrucaoAtual);

  // ETAPA 3+4 — Decodificação e Execução
  int operando = obterOperando(instrucaoAtual);

  switch (IR) {

    case OP_NOP:
      // Não realiza operação
      break;

    case OP_READ:
      // F05 — Lê sensor ultrassônico → ACC
      ACC = lerSensor();
      break;

    case OP_LOADK:
      // Carrega constante no acumulador
      ACC = operando;
      break;

    case OP_ADDK:
      // F04 (ULA) — Soma: ACC ← ACC + operando
      ACC = ACC + operando;
      break;

    case OP_SUBK:
      // F04 (ULA) — Subtração: ACC ← ACC - operando
      ACC = ACC - operando;
      break;

    case OP_CMPK:
      // F04 (ULA) — Comparação: FLAG_Z ← (ACC == operando)
      FLAG_Z = (ACC == operando);
      break;

    case OP_LEDON:
      // F06 — Liga LED especificado pelo operando
      ligarLED(operando);
      break;

    case OP_LEDOFF:
      // F06 — Desliga LED especificado pelo operando
      desligarLED(operando);
      break;

    case OP_BUZON:
      tone(PIN_BUZZER, 1000); // 1000 Hz
      Serial.println("[BUZON] Buzzer ligado.");
      break;

    case OP_BUZOFF:
      noTone(PIN_BUZZER);
      Serial.println("[BUZOFF] Buzzer desligado.");
      break;

    case OP_DISP:
      // F08 — Exibe ACC no display de 7 segmentos
      exibirNoDisplay(ACC);
      break;

    case OP_ALERT:
      // F10 — Resposta automática por faixa de distância
      executarALERT();
      break;

    case OP_BINC:
      // F02 — Exibe opcode de IR em binário no Serial Monitor
      Serial.print("[BINC] IR em binario: ");
      for (int b = 3; b >= 0; b--) Serial.print((IR >> b) & 1);
      Serial.print(" (decimal: "); Serial.print(IR); Serial.println(")");
      break;

    case OP_STORE:
      // F09 — Armazena ACC em MEM[operando]
      if (operando >= 0 && operando < 16) {
        MEM[operando] = ACC;
        Serial.print("[STORE] MEM["); Serial.print(operando);
        Serial.print("] = "); Serial.println(ACC);
      } else {
        Serial.println("[ERRO] STORE: endereco invalido (0-15).");
      }
      break;

    case OP_LOADM:
      // F09 — Carrega MEM[operando] em ACC
      if (operando >= 0 && operando < 16) {
        ACC = MEM[operando];
        Serial.print("[LOADM] ACC = MEM["); Serial.print(operando);
        Serial.print("] = "); Serial.println(ACC);
      } else {
        Serial.println("[ERRO] LOADM: endereco invalido (0-15).");
      }
      break;

    case OP_HALT:
      // F11 — Encerra a execução
      EXECUTANDO = false;
      modoRUN    = false;

      // garantir estado neutro do sistema
      desligarLED(1);
      desligarLED(2);
      desligarLED(3);
      noTone(PIN_BUZZER);
      acenderDisplay(SEG7[12]);
      
      Serial.println("[HALT] Execucao encerrada.");
      break;

    default:
      Serial.println("[ERRO] Opcode desconhecido.");
      break;
  }

  // ETAPA 5 — Exibe estado interno e incrementa PC
  exibirEstado();
  if (EXECUTANDO) PC++;
}

// ============================================================
//  F02 — CODIFICAÇÃO: extrai opcode (número antes do espaço)
// ============================================================
byte obterOpcode(String instrucao) {
  instrucao.trim();
  int espaco = instrucao.indexOf(' ');
  String parte = (espaco >= 0) ? instrucao.substring(0, espaco) : instrucao;
  int op = parte.toInt();
  if (op < 0 || op > 15) return 255;
  return (byte)op;
}

// ============================================================
//  Extrai operando (número após o espaço)
// ============================================================
int obterOperando(String instrucao) {
  instrucao.trim();
  int espaco = instrucao.indexOf(' ');
  if (espaco < 0) return 0;
  return instrucao.substring(espaco + 1).toInt();
}

// ============================================================
//  Retorna true se o opcode exige operando
// ============================================================
bool temOperando(byte op) {
  return (op == OP_LOADK || op == OP_ADDK  || op == OP_SUBK  ||
          op == OP_CMPK  || op == OP_LEDON || op == OP_LEDOFF ||
          op == OP_STORE || op == OP_LOADM);
}

// ============================================================
//  Exibe estado interno no Serial Monitor após cada instrução
// ============================================================
void exibirEstado() {
  Serial.print("PC: "); Serial.print(PC);
  Serial.print(" | IR: ");
  if (IR <= 15) Serial.print(MNEMONICOS[IR]);
  else          Serial.print("???");
  if (temOperando(IR)) { Serial.print(" "); Serial.print(obterOperando(programa[PC])); }
  Serial.print(" | ACC: "); Serial.print(ACC);
  Serial.print(" | FLAG_Z: "); Serial.println(FLAG_Z ? "1" : "0");
}

// ============================================================
//  F08 — DISPLAY: envia padrão de bits para os segmentos
// ============================================================
void acenderDisplay(byte padrao) {
  digitalWrite(PIN_SEG_A, (padrao >> 0) & 1);
  digitalWrite(PIN_SEG_B, (padrao >> 1) & 1);
  digitalWrite(PIN_SEG_C, (padrao >> 2) & 1);
  digitalWrite(PIN_SEG_D, (padrao >> 3) & 1);
  digitalWrite(PIN_SEG_E, (padrao >> 4) & 1);
  digitalWrite(PIN_SEG_F, (padrao >> 5) & 1);
  digitalWrite(PIN_SEG_G, (padrao >> 6) & 1);
}

// ============================================================
//  F08 — DISPLAY: exibe valor com tratamento de overflow/negativo
// ============================================================
void exibirNoDisplay(int valor) {
  if (valor < 0) {
    acenderDisplay(SEG7[11]); // exibe '-'
    Serial.println("[DISPLAY] Valor negativo. Exibindo '-'.");
  } else if (valor > 9) {
    acenderDisplay(SEG7[10]); // exibe 'E'
    Serial.print("[DISPLAY] Overflow! Valor=");
    Serial.print(valor); Serial.println(" > 9. Exibindo 'E'.");
  } else {
    acenderDisplay(SEG7[valor]);
    Serial.print("[DISPLAY] Exibindo: "); Serial.println(valor);
  }
}

// ============================================================
//  F05 — SENSOR: lê distância em cm com HC-SR04
// ============================================================
int lerSensor() {
  digitalWrite(PIN_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(PIN_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_TRIG, LOW);

  long duracao = pulseIn(PIN_ECHO, HIGH, 30000);
  int dist = (int)(duracao / 58);

  if (DEBUG_SERIAL) {
    Serial.print("[READ] Distancia: ");
    Serial.print(dist);
    Serial.println(" cm");
  }

  return dist;
}

// ============================================================
//  F06 — LEDs: obtem pino pelo número do LED (1, 2 ou 3)
// ============================================================
int pinLED(int n) {
  if (n == 1) return PIN_LED1;
  if (n == 2) return PIN_LED2;
  if (n == 3) return PIN_LED3;
  return -1;
}

void ligarLED(int n) {
  int p = pinLED(n);
  if (p < 0) return;

  digitalWrite(p, HIGH);

  if (DEBUG_SERIAL) {
    Serial.print("[LEDON] LED ");
    Serial.println(n);
  }
}

void desligarLED(int n) {
  int p = pinLED(n);
  if (p < 0) return;

  digitalWrite(p, LOW);

  if (DEBUG_SERIAL) {
    Serial.print("[LEDOFF] LED ");
    Serial.println(n);
  }
}

// ============================================================
//  F10 — ALERT: resposta automática por faixa de distância
//    < 10 cm         → buzzer + LED 3 (alerta)
//    10–19 cm        → só LED 3 (alerta)
//    >= 20 cm        → tudo desligado (normal)
// ============================================================
void executarALERT() {
  Serial.println("[ALERT] Monitorando... pressione * para sair.");

  ultimoEstadoALERT = -1;

  while (true) {
    char tecla = teclado.getKey();
    if (tecla == '*') {
      desligarLED(2);
      desligarLED(3);
      noTone(PIN_BUZZER);
      acenderDisplay(SEG7[12]);
      Serial.println("[ALERT] Monitoramento encerrado.");
      return;
    }

    int dist = lerSensor();

    int estado;

    if (dist < 10) estado = 0;
    else if (dist < 20) estado = 1;
    else estado = 2;

    if (estado == ultimoEstadoALERT) {
      delay(300);
      continue; // 🔥 não faz nada repetido
    }

    ultimoEstadoALERT = estado;

    // reset SÓ quando muda estado
    desligarLED(2);
    desligarLED(3);
    noTone(PIN_BUZZER);

    if (estado == 0) {
      ligarLED(3);
      tone(PIN_BUZZER, 1000);
      exibirNoDisplay(dist);
      Serial.println("[ALERT] PERIGO! < 10 cm.");
    }
    else if (estado == 1) {
      ligarLED(2);
      acenderDisplay(SEG7[12]);
      Serial.println("[ALERT] ATENCAO! 10-19 cm.");
    }
    else {
      acenderDisplay(SEG7[12]);
      Serial.println("[ALERT] NORMAL. >= 20 cm.");
    }

    delay(300);
  }
}