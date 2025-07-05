#include <MicroLCD.h>

// Mapeia os LEDs
const int numLeds = 8;
int ledPins[numLeds] = { 2, 3, 4, 5, 6, 7, 8, 9 };
bool ledStates[numLeds] = { 0 };

// Mapeia o botão
const int potPin = A0;
const int buttonPin = 10;

// Display OLED
LCD_SSD1306 lcd; // Usa os pinos padrão (SDA/SCL)

// Buzzer
const int buzzerPin = 11; // Pino do buzzer

// Estados
bool blinkState = false;
unsigned long lastBlink = 0;
bool lastButtonState = LOW;
unsigned long buttonPressTime = 0;
bool buttonHeld = false;
int turingHead = 0;
bool turingAtiva = false;
unsigned long ultimoPasso = 0;
int modo = 1;
int seletor;
int potValue;

// Variáveis para controle do delay de escrita
unsigned long delayEscrita = 0;
bool aguardandoEscrita = false;

// Variável para controlar o pisca-pisca
bool piscaAtivo = true;

// Variável para controlar se está em transição
bool emTransicao = false;

// Delays
const unsigned long blinkInterval = 300;
unsigned long functionTimer = 2000;
const unsigned long delayAutomato = 2000; // Aumenta o delay para 2 segundos
const unsigned long delayMensagem = 500;   // Delay para as mensagens

// Variáveis para controlar a atualização do display
int lastModo = -1;
int lastSeletor = -1;
bool lastTuringAtiva = false;
bool lastLedStates[numLeds] = { 0 };

// Variáveis para controlar o pisca-pisca do cabeçote
unsigned long lastHeadBlink = 0;

void setup() {
  // Inicialização dos LEDs
  for (int i = 0; i < numLeds; i++) {
    pinMode(ledPins[i], OUTPUT);
  }

  pinMode(buttonPin, INPUT_PULLUP);  // define botão como entrada com pull-up
  pinMode(13, OUTPUT);               // LED da função
  lastButtonState = digitalRead(buttonPin);  // corrige o estado inicial do botão

  // Inicialização do buzzer
  pinMode(buzzerPin, OUTPUT);

  // Inicialização do display
  lcd.begin();
  lcd.clear();
  lcd.setFontSize(FONT_SIZE_SMALL);
  lcd.setCursor(4, 0);
  lcd.print("Sistema Iniciado");
  lcd.setCursor(4, 1);
  lcd.print("Aguardando...");
  delay(1000);

  Serial.begin(9600);
}

void loop() {
  potValue = analogRead(potPin);
  seletor = potValue * numLeds / 1024;

  bool buttonState = digitalRead(buttonPin);

  if (modo == 1) {
    if (buttonState == LOW && lastButtonState == HIGH) {
      buttonPressTime = millis();
      buttonHeld = true;
    }

    // Botão está sendo segurado por tempo suficiente
    if (buttonState == LOW && buttonHeld && (millis() - buttonPressTime >= functionTimer)) {
      buttonHeld = false;
      modo = 2;
      ligarLuz();
      tocarSomAutomatico(); // Toca o som ao entrar no modo automático
    }

    // Botão foi solto (detecção de clique curto)
    if (buttonState == HIGH && lastButtonState == LOW) {
      if ((millis() - buttonPressTime) < functionTimer) {
        ledStates[seletor] = !ledStates[seletor];
      }
      buttonHeld = false;
    }

    lastButtonState = buttonState;
  }
  
  executarAutomato();
  if (piscaAtivo) {
    manterLedsAtivado();
  } else {
    // Durante execução, mantém os LEDs fixos no estado atual
    for (int i = 0; i < numLeds; i++) {
      if (i != turingHead) {
        digitalWrite(ledPins[i], ledStates[i] ? HIGH : LOW);
      }
    }
  }
  
  // Só atualiza o display se não estiver em transição
  if (!emTransicao) {
    atualizarDisplay();
  }
}

void ligarLuz() {
  digitalWrite(13, HIGH);
  turingAtiva = true;
  turingHead = 0;
  piscaAtivo = false; // Desativa o pisca-pisca
  emTransicao = false; // Reset da transição
  // Reset das variáveis de controle
  aguardandoEscrita = false;
  delayEscrita = 0;

  // Mensagem de "Iniciando"
  lcd.clear();
  lcd.setFontSize(FONT_SIZE_SMALL);
  lcd.setCursor(4, 0);
  lcd.print("Iniciando Turing...");
  lcd.setCursor(4, 1);
  lcd.print("Aguarde...");
  delay(500);

  // Atualiza as variáveis de controle
  lastModo = modo;
  lastSeletor = seletor;
  lastTuringAtiva = turingAtiva;
  for (int i = 0; i < numLeds; i++) {
    lastLedStates[i] = ledStates[i];
  }
}

void manterLedsAtivado() {
  unsigned long now = millis();
  if (!(now - lastBlink > blinkInterval)) {
    return;
  }
  blinkState = !blinkState;
  lastBlink = now;

  for (int i = 0; i < numLeds; i++) {
    if (i == seletor) {
        digitalWrite(ledPins[i], blinkState ? HIGH : LOW);  // Sempre pisca no cursor
    } else {
      digitalWrite(ledPins[i], ledStates[i] ? HIGH : LOW);  // Fora do cursor, mostra o estado salvo
    }
  }
}

void executarAutomato() {
  if (!turingAtiva) return;

  unsigned long now = millis();

  // Verifica se é hora de piscar o LED do cabeçote
  if (now - lastHeadBlink >= blinkInterval) {
    blinkState = !blinkState;
    digitalWrite(ledPins[turingHead], blinkState ? HIGH : LOW);
    lastHeadBlink = now;
  }

  if (now - ultimoPasso >= delayAutomato) {
    lerFita();
    
    // Se o LED estava aceso, implementa a lógica de apagar/escrever
    if (ledStates[turingHead] == true) {
      if (!aguardandoEscrita) {
        apagarFita();
        delayEscrita = millis();
        aguardandoEscrita = true;
      } else if (millis() - delayEscrita >= 1000) {
        escreverFita();
        aguardandoEscrita = false;
      }
    } else {
      // Se não está na condição especial, pode mover normalmente
      if (!aguardandoEscrita) {
        mover();
      }
    }

    // Só atualiza ultimoPasso se não está aguardando escrita
    if (!aguardandoEscrita) {
      ultimoPasso = now;
    }
  }
}

void lerFita(){
  Serial.println("Lendo fita");
  emTransicao = true;
  lcd.clear();
  lcd.setFontSize(FONT_SIZE_MEDIUM);
  lcd.setCursor(30, 2);
  lcd.print("LENDO");
  tocarSomLendo(); // Toca o som ao ler a fita
  delay(delayMensagem);
  emTransicao = false;
}

void apagarFita(){
  Serial.println("Apagando na fita");
  emTransicao = true;
  lcd.clear();
  lcd.setFontSize(FONT_SIZE_MEDIUM);
  lcd.setCursor(15, 2);
  lcd.print("APAGANDO");
  tocarSomApagando(); // Toca o som ao apagar a fita
  delay(delayMensagem);
  emTransicao = false;
}

void escreverFita() {
  Serial.println("Escrevendo na fita");
  emTransicao = true;
  lcd.clear();
  lcd.setFontSize(FONT_SIZE_MEDIUM);
  lcd.setCursor(5, 2);
  lcd.print("ESCREVENDO");
  tocarSomEscrevendo(); // Toca o som ao escrever na fita
  delay(delayMensagem);
  ledStates[turingHead] = false;
  emTransicao = false;
}

void mover(){
  Serial.println("Movendo");
  emTransicao = true;
  lcd.clear();
  lcd.setFontSize(FONT_SIZE_MEDIUM);
  lcd.setCursor(20, 2);
  lcd.print("MOVENDO");
  tocarSomMovendo(); // Toca o som ao mover a cabeça
  delay(delayMensagem);

  turingHead++;
  if (turingHead >= numLeds) {
    turingAtiva = false;  // Parou ao final da fita
    turingHead = 0;
    modo = 1;
    digitalWrite(13, LOW);  // Apaga o LED indicador
    piscaAtivo = true; // Reativa o pisca-pisca
    tocarSomManual(); // Toca o som ao voltar para o modo manual

    // Mensagem de "Fim da fita"
    lcd.clear();
    lcd.setFontSize(FONT_SIZE_SMALL);
    lcd.setCursor(4, 0);
    lcd.print("Fim da fita!");
    lcd.setCursor(4, 1);
    lcd.print("Voltando ao manual");
    delay(2000);
  }
  emTransicao = false;
}

void atualizarDisplay() {
  bool displayAtualizado = false;

  if (modo != lastModo || seletor != lastSeletor || turingAtiva != lastTuringAtiva) {
    displayAtualizado = true;
  } else {
    for (int i = 0; i < numLeds; i++) {
      if (ledStates[i] != lastLedStates[i]) {
        displayAtualizado = true;
        break;
      }
    }
  }

  if (displayAtualizado) {
    if (!turingAtiva) {
      String modoTexto = "Modo: " + String(modo == 1 ? "Manual" : "Auto");
      String posicaoTexto = "Posicao: " + String(seletor);
      String fitaTexto = "Fita: ";
      for (int i = 0; i < numLeds; i++) {
        fitaTexto += (ledStates[i] ? "1" : "0");
      }
      String instrucaoTexto = "Segure botao p/ auto";

      lcd.clear();
      lcd.setFontSize(FONT_SIZE_SMALL);
      lcd.setCursor(4, 0);
      lcd.print(modoTexto);
      lcd.setCursor(4, 1);
      lcd.print(posicaoTexto);
      lcd.setCursor(4, 2);
      lcd.print(fitaTexto);
      lcd.setCursor(4, 3);
      lcd.print(instrucaoTexto);
    } else {
      // Durante execução, mostra informações adicionais
      String cabecaTexto = "Cabeca: " + String(turingHead);
      String fitaTexto = "Fita: ";
      for (int i = 0; i < numLeds; i++) {
        if (i == turingHead) {
          fitaTexto += "[" + String(ledStates[i] ? "1" : "0") + "]";
        } else {
          fitaTexto += (ledStates[i] ? "1" : "0");
        }
      }

      lcd.clear();
      lcd.setFontSize(FONT_SIZE_SMALL);
      lcd.setCursor(4, 1);
      lcd.print(cabecaTexto);
      lcd.setCursor(4, 2);
      lcd.print(fitaTexto);
    }

    // Atualiza as variáveis de controle
    lastModo = modo;
    lastSeletor = seletor;
    lastTuringAtiva = turingAtiva;
    for (int i = 0; i < numLeds; i++) {
      lastLedStates[i] = ledStates[i];
    }
  }
}

// Função para tocar o som do modo manual
void tocarSomManual() {
  tone(buzzerPin, 440, 500); // Toca um tom de 440Hz por 500ms
  delay(500);
}

// Função para tocar o som do modo automático
void tocarSomAutomatico() {
  tone(buzzerPin, 880, 500); // Toca um tom de 880Hz por 500ms
  delay(500);
}

// Funções para tocar os sons dos estados
void tocarSomLendo() {
  tone(buzzerPin, 500, 200); // Frequência e duração para "Lendo"
}

void tocarSomApagando() {
  tone(buzzerPin, 600, 200); // Frequência e duração para "Apagando"
}

void tocarSomEscrevendo() {
  tone(buzzerPin, 700, 200); // Frequência e duração para "Escrevendo"
}

void tocarSomMovendo() {
  tone(buzzerPin, 800, 200); // Frequência e duração para "Movendo"
}
