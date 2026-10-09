// =====================================================
//  Teste: arco-íris nas duas matrizes (ESP32)
//  Usado para confirmar que as duas matrizes recebem dados
//  e para ver a ordem física dos LEDs.
// =====================================================
#include <Adafruit_NeoPixel.h>

#define PINO_MALHA1  2     // GPIO2
#define PINO_MALHA2  4     // GPIO4
#define NUM_LEDS     256   // cada matriz 16x16
#define BRILHO       20    // deixe baixo para fontes de 2A
#define VELOCIDADE   10    // ms entre cada passo

Adafruit_NeoPixel malha1(NUM_LEDS, PINO_MALHA1, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel malha2(NUM_LEDS, PINO_MALHA2, NEO_GRB + NEO_KHZ800);

uint16_t inicio = 0;

void setup() {
  Serial.begin(115200);
  malha1.begin();
  malha2.begin();
  malha1.setBrightness(BRILHO);
  malha2.setBrightness(BRILHO);
  malha1.clear();
  malha2.clear();
  malha1.show();
  malha2.show();
  Serial.println("Rainbow iniciado nas 2 matrizes");
}

void loop() {
  for (int i = 0; i < NUM_LEDS; i++) {
    // As duas matrizes formam um arco-íris contínuo de 512 LEDs
    uint16_t cor1 = inicio + (i * 65536L / (NUM_LEDS * 2));
    uint16_t cor2 = inicio + ((i + NUM_LEDS) * 65536L / (NUM_LEDS * 2));
    malha1.setPixelColor(i, malha1.gamma32(malha1.ColorHSV(cor1)));
    malha2.setPixelColor(i, malha2.gamma32(malha2.ColorHSV(cor2)));
  }
  malha1.show();
  malha2.show();
  inicio += 256;
  delay(VELOCIDADE);
}
