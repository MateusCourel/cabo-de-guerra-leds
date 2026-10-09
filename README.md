# 🔴🔵 Cabo de Guerra de LEDs

Jogo para dois jogadores feito com **ESP32** e **duas matrizes de LED WS2812B 16x16** (512 LEDs). Quem apertar o botão mais rápido empurra a sua cor pela tela. Quando as duas "cobras" de LEDs se encontram, vence quem tiver ocupado mais espaço.

<!-- Coloque as fotos em docs/fotos/ com estes nomes (veja docs/fotos/LEIA-ME.md) -->
<p align="center">
  <img src="docs/fotos/jogo.jpg" alt="Jogo funcionando" width="45%">
  <img src="docs/fotos/tela-go.jpg" alt="Tela inicial com GO" width="45%">
</p>

---

## 👥 Equipe

Projeto feito em equipe:

| Quem | O que fez |
|---|---|
| **Professor Ricardo Rall** | Ideia do projeto e compra dos materiais |
| **Davi Rodrigo de Miranda** | Montagem eletrônica dos componentes |
| **Mateus Antonio Courel** ([@MateusCourel](https://github.com/MateusCourel)) | Programação |

---

## 🎮 Como funciona

1. **Tela inicial:** cada matriz mostra um **"GO"** grande. A matriz do vermelho tem fundo vermelho e GO azul, a do azul é o contrário. Os LEDs dos botões piscam chamando para jogar.
2. **Contagem:** ao apertar qualquer botão, aparece **3, 2, 1** com um bip em cada número.
3. **Partida:** cada apertada faz a "cobra" do jogador andar **3 LEDs**. Ela corre linha por linha, fazendo um "L" no fim de cada linha, e sobe pela matriz em direção ao adversário.
4. **Encontro:** quando as duas cobras se encontram, vence quem tiver mais LEDs. As matrizes **piscam 4 vezes** na cor do vencedor, com uma musiquinha de vitória (ou branco, em caso de empate).
5. O jogo volta para a tela inicial.

---

## 🧰 Hardware

| Componente | Quantidade |
|---|---|
| ESP32 (DevKit) | 1 |
| Matriz de LED WS2812B 16x16 | 2 |
| Fonte 5V 2A | 2 (uma por matriz) |
| Botão arcade com LED (vermelho e azul) | 2 |
| Buzzer passivo | 1 |
| Resistor 330 Ω (linha de dados) | 2 |

📌 Ligações completas, tabela de pinos e consumo de corrente: **[docs/ligacoes.md](docs/ligacoes.md)**

| Componente | Pino do ESP32 |
|---|---|
| Matriz 1 (vermelho) | GPIO2 |
| Matriz 2 (azul) | GPIO4 |
| Botão vermelho / azul | GPIO18 / GPIO19 |
| LED do botão vermelho / azul | GPIO21 / GPIO22 |
| Buzzer | GPIO23 |

---

## 💻 Como rodar

1. Instale a **Arduino IDE** e o suporte ao ESP32 (*Ferramentas → Placa → Gerenciador de Placas → "esp32"*).
2. Instale a biblioteca **Adafruit NeoPixel** (*Ferramentas → Gerenciar Bibliotecas*).
3. Abra [`firmware/cabo_de_guerra_esp32/cabo_de_guerra_esp32.ino`](firmware/cabo_de_guerra_esp32/cabo_de_guerra_esp32.ino).
4. Selecione a placa **ESP32 Dev Module** e a porta COM.
5. Clique em **Carregar**. Se der erro de gravação, baixe o *Upload Speed* para 115200 e desligue as fontes das matrizes durante o upload.

### Ajustes rápidos (topo do código)

| Configuração | Para que serve |
|---|---|
| `BRILHO` | Brilho do jogo (cuidado com a corrente das fontes) |
| `LEDS_POR_TOQUE` | Quantos LEDs a cobra anda por apertada |
| `PASSO_MS` | Velocidade da cobra |
| `SENTIDO_COBRA` | Caminho da cobra (subindo pelas linhas, pelas colunas, etc.) |
| `POR_COLUNA` / `ZIGZAG` | Ordem física dos LEDs da matriz |
| `GIRAR_MALHA1` / `GIRAR_MALHA2` | Corrige matriz montada de cabeça para baixo |
| `TOM_MULT` | Deixa o buzzer mais agudo e mais alto |
| `TESTE_CORES` | Teste de cores ao ligar, para diagnóstico |

---

## 📂 Estrutura do repositório

```
cabo-de-guerra-leds/
├── firmware/
│   └── cabo_de_guerra_esp32/      # código final do jogo (ESP32)
├── testes/
│   ├── teste_rainbow_esp32/       # arco-íris nas duas matrizes
│   └── teste_apagar_esp32/        # mantém tudo apagado (diagnóstico)
├── historico/
│   └── versao_arduino_nano/       # primeira versão, no Arduino Nano
└── docs/
    ├── ligacoes.md                # esquema elétrico e pinos
    └── fotos/                     # fotos e vídeos do projeto
```

---

## 🧗 Dificuldades e aprendizados

O código do jogo em si saiu rápido. A maior parte do trabalho foi descobrir **por que as coisas não funcionavam**. Esta é a linha do tempo dos problemas que apareceram e de como foram resolvidos.

### 1. Começou no Arduino Nano
A primeira versão foi escrita para um **Arduino Nano**. Logo de cara apareceram limitações: com 512 LEDs, a memória do Nano (2 KB de RAM) fica no limite, porque cada LED ocupa 3 bytes. Também tivemos erros de compilação (placa errada selecionada na IDE) e de gravação (bootloader antigo dos Nanos genéricos).
> A versão do Nano está guardada em [`historico/`](historico/versao_arduino_nano) para mostrar a evolução.

### 2. Alimentação: o maior vilão
- **LEDs alimentados pelo USB:** as matrizes acendiam só em vermelho e amarelo e o jogo travava. O USB entrega uns 500 mA, mas as matrizes precisam de muito mais. Sem tensão suficiente, o verde e o azul (que precisam de mais tensão) não acendem, e o microcontrolador reinicia.
- **Terminal derretido 🔥:** com a fonte externa, as matrizes ficaram em **branco no brilho máximo** (cerca de 15 A) e um terminal de ligação esquentou até derreter. Aprendemos que a energia das matrizes precisa de **fio grosso**, direto da fonte, e nunca deve passar por jumpers ou conectores pequenos.
- **Fonte desarmando:** com fontes de 2A, a proteção desligava e religava em loop sempre que algo acendia demais. A solução foi **brilho baixo no código** e **uma fonte por matriz**.

### 3. Tudo branco: o problema do sinal
Mesmo com energia certa, as matrizes ficavam brancas. Branco total significa que o LED **não está entendendo os dados**. Investigamos GND comum, o pino de dados (DI × DOUT) e pinos do microcontrolador que ficam em 5V parados. Também testamos o protocolo WS2812 "na mão" e vimos que ele exige tempos de **frações de microssegundo**, impossíveis com `digitalWrite()`.

### 4. Migração para o ESP32
Trocamos o Nano por um **ESP32** e os testes passaram a funcionar. Junto vieram novos aprendizados:
- O **GPIO2** interfere no modo de gravação, e o upload dava erro de *checksum* até desligarmos as matrizes durante a gravação.
- O ESP32 envia dados em **3,3V** para LEDs de 5V, o que às vezes gerava **LEDs roxos** (leitura errada). Resolvemos no código reenviando a imagem a cada 40 ms, assim um LED que lê errado é corrigido quase na hora.

### 5. Mapeando a matriz
Cada fabricante numera os LEDs de um jeito (zigue-zague, linha por linha, coluna por coluna). Com o mapeamento errado, o **"GO" aparecia como "GG"**. O código ganhou uma função que converte coordenadas `(x, y)` para a posição física de cada LED, com opções para ajustar a ordem e a rotação de cada matriz.

### 6. Ajustando o jogo
O caminho das "cobras" passou por várias versões (por colunas, atravessando as matrizes, de lado a lado) até chegar ao formato final, em que cada cobra sobe pela sua matriz em direção ao adversário. Para garantir que **as cobras nunca se sobrepõem e nunca deixam buracos**, o caminho foi simulado para todos os placares possíveis antes de ir para o hardware.

### 7. Volume
O buzzer ficou mais baixo no ESP32 (3,3V contra 5V do Nano). Uma forma de compensar pelo código é tocar as notas perto da frequência de ressonância do buzzer (`TOM_MULT`).

---

## 🚀 Próximos passos

- [ ] Caixa definitiva para levar o jogo a eventos
- [ ] Conversor de nível 74AHCT125 para o sinal de dados
- [ ] Buzzer com transistor (ou alto-falante + amplificador) para mais volume
- [ ] Placar com melhor de 3 partidas

---

<p align="center">Feito com muitos LEDs, alguns fios derretidos e trabalho em equipe 💡</p>
