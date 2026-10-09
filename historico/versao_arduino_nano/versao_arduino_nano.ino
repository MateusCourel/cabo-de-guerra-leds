// =====================================================
//  Cabo de Guerra de LEDs - Vermelho x Azul
//  VERSÃO ANTIGA (histórico) - Arduino Nano (ATmega328P)
//  Substituída pela versão ESP32 em /firmware. Mantida para
//  mostrar a evolução do projeto (ver README).
//  2 malhas WS2812B 16x16 SEPARADAS, cada uma no seu pino
//  Biblioteca: Adafruit NeoPixel (Gerenciador de Bibliotecas)
// =====================================================
#include <Adafruit_NeoPixel.h>

// ---------------- CONFIGURAÇÃO ----------------
#define PINO_MALHA1     6   // DIN da malha 1 (lado do vermelho)
#define PINO_MALHA2     7   // DIN da malha 2 (lado do azul)
#define PINO_VERMELHO   2   // botão vermelho (outro lado no GND)
#define PINO_AZUL       3   // botão azul (outro lado no GND)
#define LED_VERMELHO    8   // LED dentro do botão vermelho
#define LED_AZUL        9   // LED dentro do botão azul
#define PINO_BUZZER     5   // buzzer PASSIVO (+ no D5, - no GND)
#define SOM_LIGADO      1   // 0 = jogo sem som

#define LARGURA_MALHA   16  // colunas de cada malha
#define ALTURA          16  // linhas de cada malha

// Malhas 16x16 rígidas costumam ser numeradas por LINHA em zigue-zague.
// Se o GO aparecer embaralhado, teste POR_COLUNA 1 e/ou ZIGZAG 0
#define POR_COLUNA      0   // 0 = por linha, 1 = por coluna
#define ZIGZAG          1   // 1 = sentido alterna a cada linha/coluna

// Se uma malha estiver montada de cabeça para baixo, mude para 1
#define GIRAR_MALHA1    0
#define GIRAR_MALHA2    0

#define BRILHO          20  // 0 a 255 (cuidado com a corrente da fonte!)
#define COLUNAS_POR_TOQUE 1
#define VEZES_PISCAR    4
#define DEBOUNCE_MS     30

// Ordem das cores dos LEDs. Se o vermelho aparecer verde (ou o contrário),
// troque NEO_GRB por NEO_RGB
#define ORDEM_CORES     NEO_GRB

// 1 = ao ligar, faz um teste de cores em cada malha (para achar defeitos)
// Depois que estiver tudo certo, mude para 0
#define TESTE_CORES     1
// ----------------------------------------------

#ifndef LED_BUILTIN
#define LED_BUILTIN 13  // LED da própria placa no Nano
#endif

#define LARGURA         (LARGURA_MALHA * 2)
#define LEDS_POR_MALHA  (LARGURA_MALHA * ALTURA)

Adafruit_NeoPixel malha1(LEDS_POR_MALHA, PINO_MALHA1, ORDEM_CORES + NEO_KHZ800);
Adafruit_NeoPixel malha2(LEDS_POR_MALHA, PINO_MALHA2, ORDEM_CORES + NEO_KHZ800);

uint32_t VERMELHO, AZUL, AMARELO, BRANCO;

// A struct precisa ficar ANTES de qualquer função, senão a IDE do Arduino
// cria os protótipos automáticos antes dela e dá erro de compilação
struct Botao {
  uint8_t pino;
  uint8_t antes;
  unsigned long t;
};

Botao bv = {PINO_VERMELHO, HIGH, 0};
Botao ba = {PINO_AZUL, HIGH, 0};

bool apertou(Botao &b);
void sincronizar(Botao &b);

// ---------------- DESENHO ----------------
// Converte (x, y) dentro de UMA malha no índice do LED
uint16_t indice(uint8_t lx, uint8_t y) {
#if POR_COLUNA
  if (ZIGZAG && (lx & 1)) y = ALTURA - 1 - y;
  return (uint16_t)lx * ALTURA + y;
#else
  if (ZIGZAG && (y & 1)) lx = LARGURA_MALHA - 1 - lx;
  return (uint16_t)y * LARGURA_MALHA + lx;
#endif
}

// x vai de 0 a 31: 0..15 = malha 1, 16..31 = malha 2
void pixel(int x, int y, uint32_t c) {
  if (x < 0 || x >= LARGURA || y < 0 || y >= ALTURA) return;
  uint8_t lx = x % LARGURA_MALHA;
  uint8_t ly = y;
  if (x < LARGURA_MALHA) {
    if (GIRAR_MALHA1) { lx = LARGURA_MALHA - 1 - lx; ly = ALTURA - 1 - ly; }
    malha1.setPixelColor(indice(lx, ly), c);
  } else {
    if (GIRAR_MALHA2) { lx = LARGURA_MALHA - 1 - lx; ly = ALTURA - 1 - ly; }
    malha2.setPixelColor(indice(lx, ly), c);
  }
}

void coluna(uint8_t x, uint32_t c) {
  for (uint8_t y = 0; y < ALTURA; y++) pixel(x, y, c);
}

void limpar() {
  malha1.clear();
  malha2.clear();
}

void encher(uint32_t c) {
  malha1.fill(c);
  malha2.fill(c);
}

void mostrar() {
  malha1.show();
  malha2.show();
}

// Fonte 5x7 (números da contagem) guardada na memória Flash (economiza RAM)
// Ordem: G, O, 3, 2, 1
const uint8_t FONTE[][7] PROGMEM = {
  {0b01110, 0b10001, 0b10000, 0b10111, 0b10001, 0b10001, 0b01111}, // G
  {0b01110, 0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b01110}, // O
  {0b11111, 0b00010, 0b00100, 0b00010, 0b00001, 0b10001, 0b01110}, // 3
  {0b01110, 0b10001, 0b00001, 0b00010, 0b00100, 0b01000, 0b11111}, // 2
  {0b00100, 0b01100, 0b00100, 0b00100, 0b00100, 0b00100, 0b01110}, // 1
};

int8_t indiceFonte(char ch) {
  switch (ch) {
    case 'G': return 0;
    case 'O': return 1;
    case '3': return 2;
    case '2': return 3;
    case '1': return 4;
  }
  return -1;
}

// Na malha 16x16 os números ficam com o dobro do tamanho (10 x 14)
#define ESCALA_TEXTO  ((ALTURA >= 14) ? 2 : 1)

// Escreve o texto centralizado na malha indicada (0 = malha 1, 1 = malha 2)
void texto(const char *txt, uint8_t m, uint32_t c) {
  const uint8_t e = ESCALA_TEXTO;
  uint8_t n = strlen(txt);
  int largTxt = (n * 6 - 1) * e;
  int x0 = m * LARGURA_MALHA + (LARGURA_MALHA - largTxt) / 2;
  int y0 = (ALTURA - 7 * e) / 2;
  for (uint8_t i = 0; i < n; i++) {
    int8_t f = indiceFonte(txt[i]);
    if (f < 0) continue;
    for (uint8_t r = 0; r < 7; r++) {
      uint8_t linha = pgm_read_byte(&FONTE[f][r]);
      for (uint8_t b = 0; b < 5; b++) {
        if (linha & (1 << (4 - b))) {
          for (uint8_t dy = 0; dy < e; dy++)
            for (uint8_t dx = 0; dx < e; dx++)
              pixel(x0 + (i * 6 + b) * e + dx, y0 + r * e + dy, c);
        }
      }
    }
  }
}

// Letras GRANDES (6 x 12, traço grosso) para o "GO" da malha 16x16
#define LETRA_L   6
#define LETRA_A   12
const uint8_t G_GRANDE[LETRA_A] PROGMEM = {
  0b011110,
  0b111111,
  0b110011,
  0b110000,
  0b110000,
  0b110111,
  0b110111,
  0b110011,
  0b110011,
  0b110011,
  0b111111,
  0b011110,
};
const uint8_t O_GRANDE[LETRA_A] PROGMEM = {
  0b011110,
  0b111111,
  0b110011,
  0b110011,
  0b110011,
  0b110011,
  0b110011,
  0b110011,
  0b110011,
  0b110011,
  0b111111,
  0b011110,
};

void letraGrande(const uint8_t *letra, int x0, int y0, uint32_t c) {
  for (uint8_t r = 0; r < LETRA_A; r++) {
    uint8_t linha = pgm_read_byte(&letra[r]);
    for (uint8_t b = 0; b < LETRA_L; b++) {
      if (linha & (1 << (LETRA_L - 1 - b))) pixel(x0 + b, y0 + r, c);
    }
  }
}

// Pinta a malha inteira com a cor de fundo e escreve "GO" grande por cima
void telaGO(uint8_t m, uint32_t fundo, uint32_t letra) {
  for (uint8_t x = 0; x < LARGURA_MALHA; x++) coluna(m * LARGURA_MALHA + x, fundo);
  int largGO = LETRA_L * 2 + 2;  // 6 + 2 + 6 = 14
  int x0 = m * LARGURA_MALHA + (LARGURA_MALHA - largGO) / 2;
  int y0 = (ALTURA - LETRA_A) / 2;
  letraGrande(G_GRANDE, x0, y0, letra);
  letraGrande(O_GRANDE, x0 + LETRA_L + 2, y0, letra);
}

// ---------------- BOTÕES ----------------
// (a struct Botao fica lá no topo do arquivo)

// true só no instante em que o botão é apertado (com debounce)
bool apertou(Botao &b) {
  uint8_t v = digitalRead(b.pino);
  unsigned long agora = millis();
  if (v != b.antes && agora - b.t > DEBOUNCE_MS) {
    b.antes = v;
    b.t = agora;
    return v == LOW;
  }
  return false;
}

// Ignora o que foi apertado enquanto não estávamos lendo
void sincronizar(Botao &b) {
  b.antes = digitalRead(b.pino);
  b.t = millis();
}

// ---------------- SOM ----------------
// Notas (Hz)
#define NOTA_C5  523
#define NOTA_E5  659
#define NOTA_G5  784
#define NOTA_A5  880
#define NOTA_C6  1047
#define NOTA_E6  1319
#define NOTA_A4  440
#define NOTA_E4  330

// Toca sem travar o programa (o som segue enquanto o jogo roda)
void bip(unsigned int freq, unsigned long dur) {
#if SOM_LIGADO
  tone(PINO_BUZZER, freq, dur);
#endif
}

// Toca e espera terminar
void nota(unsigned int freq, unsigned long dur) {
  bip(freq, dur);
  delay(dur + 20);
}

void somLigou() {
  nota(NOTA_C5, 80);
  nota(NOTA_E5, 80);
  nota(NOTA_G5, 80);
  nota(NOTA_C6, 150);
}

// Som de "batida" quando as cores se encontram: desce de agudo para grave
void somEncontro() {
  for (int f = 1500; f >= 200; f -= 50) {
    bip(f, 20);
    delay(20);
  }
  noTone(PINO_BUZZER);
}

// ---------------- LEDs DOS BOTÕES ----------------
void ledsBotoes(bool vermelho, bool azul) {
  digitalWrite(LED_VERMELHO, vermelho ? HIGH : LOW);
  digitalWrite(LED_AZUL, azul ? HIGH : LOW);
}

// ---------------- ETAPAS DO JOGO ----------------
void telaInicio() {
  limpar();
  telaGO(0, VERMELHO, AZUL);   // malha do vermelho: fundo vermelho, GO azul
  telaGO(1, AZUL, VERMELHO);   // malha do azul: fundo azul, GO vermelho
  mostrar();
  sincronizar(bv);
  sincronizar(ba);
  unsigned long tPisca = millis();
  bool aceso = true;
  ledsBotoes(true, true);
  while (true) {
    if (apertou(bv) || apertou(ba)) {
      ledsBotoes(false, false);
      nota(NOTA_C6, 60);  // "ok, vamos começar"
      return;
    }
    // LEDs dos botões piscam devagar chamando para jogar
    if (millis() - tPisca > 500) {
      tPisca = millis();
      aceso = !aceso;
      ledsBotoes(aceso, aceso);
    }
    delay(5);
  }
}

void contagem() {
  const char *numeros[] = {"3", "2", "1"};
  for (uint8_t i = 0; i < 3; i++) {
    limpar();
    texto(numeros[i], 0, AMARELO);
    texto(numeros[i], 1, AMARELO);
    mostrar();
    bip(NOTA_A5, 150);  // bip em cada número
    delay(1000);
  }
  limpar();
  mostrar();
  bip(NOTA_E6, 400);    // bip longo e agudo: VALENDO!
  sincronizar(bv);
  sincronizar(ba);
}

void jogar(uint8_t &v, uint8_t &a) {
  v = 0;  // colunas do vermelho (enchem da esquerda, malha 1 -> malha 2)
  a = 0;  // colunas do azul (enchem da direita, malha 2 -> malha 1)
  while (v + a < LARGURA) {
    bool mudou = false;
    if (apertou(bv)) {
      for (uint8_t k = 0; k < COLUNAS_POR_TOQUE && v + a < LARGURA; k++) {
        coluna(v, VERMELHO);
        v++;
      }
      bip(600, 15);   // clique do vermelho (mais grave)
      mudou = true;
    }
    if (apertou(ba)) {
      for (uint8_t k = 0; k < COLUNAS_POR_TOQUE && v + a < LARGURA; k++) {
        coluna(LARGURA - 1 - a, AZUL);
        a++;
      }
      bip(900, 15);   // clique do azul (mais agudo)
      mudou = true;
    }
    if (mudou) mostrar();
    // LED do botão acende enquanto ele está apertado
    ledsBotoes(digitalRead(PINO_VERMELHO) == LOW, digitalRead(PINO_AZUL) == LOW);
  }
  ledsBotoes(false, false);
}

void mostrarVencedor(uint8_t v, uint8_t a) {
  somEncontro();  // ~0,6 s, enquanto mostra onde se encontraram
  delay(200);
  uint32_t c;
  if (v > a)      c = VERMELHO;
  else if (a > v) c = AZUL;
  else            c = BRANCO;  // empate
  bool ganhouV = (v >= a);  // no empate os dois piscam
  bool ganhouA = (a >= v);
  bool empate = (v == a);
  // Uma nota a cada piscada: vitória sobe, empate desce
  const unsigned int vitoria[] = {NOTA_C5, NOTA_E5, NOTA_G5, NOTA_C6};
  const unsigned int velha[]   = {NOTA_A4, NOTA_A4, NOTA_A4, NOTA_E4};
  for (uint8_t i = 0; i < VEZES_PISCAR; i++) {
    encher(c);
    mostrar();
    ledsBotoes(ganhouV, ganhouA);  // LED do botão vencedor pisca junto
    bip(empate ? velha[i % 4] : vitoria[i % 4], (i == VEZES_PISCAR - 1) ? 380 : 250);
    delay(400);
    limpar();
    mostrar();
    ledsBotoes(false, false);
    delay(400);
  }
}

// ---------------- SETUP / LOOP ----------------
void setup() {
  pinMode(PINO_VERMELHO, INPUT_PULLUP);
  pinMode(PINO_AZUL, INPUT_PULLUP);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(LED_VERMELHO, OUTPUT);
  pinMode(LED_AZUL, OUTPUT);
  pinMode(PINO_BUZZER, OUTPUT);
  ledsBotoes(false, false);

  malha1.begin();
  malha2.begin();

  // Se faltou RAM para os LEDs, o LED da placa pisca sem parar
  if (malha1.numPixels() == 0 || malha2.numPixels() == 0) {
    while (true) {
      digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
      delay(150);
    }
  }

  VERMELHO = Adafruit_NeoPixel::Color(BRILHO, 0, 0);
  AZUL     = Adafruit_NeoPixel::Color(0, 0, BRILHO);
  AMARELO  = Adafruit_NeoPixel::Color(BRILHO, BRILHO * 180 / 255, 0);
  BRANCO   = Adafruit_NeoPixel::Color(BRILHO, BRILHO, BRILHO);

  limpar();
  mostrar();

#if TESTE_CORES
  // Malha 1: vermelho, verde, azul. Depois a malha 2: vermelho, verde, azul.
  uint32_t cores[] = {VERMELHO, Adafruit_NeoPixel::Color(0, BRILHO, 0), AZUL};
  for (uint8_t m = 0; m < 2; m++) {
    for (uint8_t i = 0; i < 3; i++) {
      limpar();
      if (m == 0) malha1.fill(cores[i]); else malha2.fill(cores[i]);
      mostrar();
      delay(1000);
    }
  }
  limpar();
  mostrar();
#endif

  somLigou();  // musiquinha ao ligar o jogo
}

void loop() {
  uint8_t v, a;
  telaInicio();
  contagem();
  jogar(v, a);
  mostrarVencedor(v, a);
}
