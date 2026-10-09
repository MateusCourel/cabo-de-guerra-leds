// =====================================================
//  Teste: manter as duas matrizes apagadas (ESP32)
//  Usado no diagnóstico: se as matrizes estavam brancas e
//  apagam com este código, o sinal de dados está chegando.
// =====================================================
#include <Adafruit_NeoPixel.h>

Adafruit_NeoPixel malha1(256, 2, NEO_GRB + NEO_KHZ800);  // GPIO2
Adafruit_NeoPixel malha2(256, 4, NEO_GRB + NEO_KHZ800);  // GPIO4

void setup() {
  Serial.begin(115200);
  malha1.begin();
  malha2.begin();
}

void loop() {
  // Reenvia "tudo apagado" 10x por segundo, assim funciona
  // mesmo se a fonte das matrizes for ligada depois do ESP32
  malha1.clear();
  malha2.clear();
  malha1.show();
  malha2.show();
  Serial.println("apagado");
  delay(100);
}
