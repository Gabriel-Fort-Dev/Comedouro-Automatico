#include <Servo.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>


const uint8_t LARGURA_TELA = 128;
const uint8_t ALTURA_TELA = 64;
const uint8_t ENDERECO_OLED = 0x3C;


// PINOS
const byte PINO_TRIG = 2;
const byte PINO_ECHO = 5;
const byte PINO_BUZZER = 4;


const byte PINO_LED_VERMELHO = 11;
const byte PINO_LED_AMARELO = 10;
const byte PINO_LED_VERDE = 9;


const byte PINO_SERVO = 12;
const byte NENHUM_LED = 0;


// TEMPOS
const unsigned long TEMPO_ABERTURA_MS = 2000;
const unsigned long TEMPO_MIN_ENTRE_LIBERACOES_MS = 10000;
const unsigned long INTERVALO_VERIFICACAO_MS = 200;


// CALIBRACAO
const int DISTANCIA_CHEIA_CM = 10;
const int DISTANCIA_MEDIA_CM = 30;
const int DISTANCIA_VAZIA_CM = 70;


// SERVO
const int ANGULO_SERVO_FECHADO = 90;
const int ANGULO_SERVO_ABERTO = 120;


const unsigned int FREQUENCIA_ALARME_RESERVATORIO = 1000;


enum EstadoSistema {
  AGUARDANDO_CICLO,
  LIBERANDO_RACAO,
  RESERVATORIO_BAIXO
};


Adafruit_SSD1306 display(LARGURA_TELA, ALTURA_TELA, &Wire, -1);
Servo comporta;


EstadoSistema estadoAtual = AGUARDANDO_CICLO;


bool oledAtivo = false;
bool comportaAberta = false;


int distanciaAtualCm = -1;
int nivelReservatorio = -1;


unsigned long tempoInicioLiberacaoMs = 0;
unsigned long tempoFimUltimaLiberacaoMs = 0;
unsigned long tempoUltimaVerificacaoMs = 0;


// CONFIGURAR PINOS
void configurarPinos() {


  pinMode(PINO_TRIG, OUTPUT);
  pinMode(PINO_ECHO, INPUT);


  pinMode(PINO_LED_VERMELHO, OUTPUT);
  pinMode(PINO_LED_AMARELO, OUTPUT);
  pinMode(PINO_LED_VERDE, OUTPUT);


  pinMode(PINO_BUZZER, OUTPUT);
}


// CONTROLE DOS LEDS
void ligarLedExclusivo(byte pinoLed) {


  digitalWrite(PINO_LED_VERMELHO,
    (pinoLed == PINO_LED_VERMELHO) ? HIGH : LOW);


  digitalWrite(PINO_LED_AMARELO,
    (pinoLed == PINO_LED_AMARELO) ? HIGH : LOW);


  digitalWrite(PINO_LED_VERDE,
    (pinoLed == PINO_LED_VERDE) ? HIGH : LOW);
}


// TESTAR LEDS
void testarLedsInicializacao() {


  digitalWrite(PINO_LED_VERMELHO, HIGH);
  digitalWrite(PINO_LED_AMARELO, HIGH);
  digitalWrite(PINO_LED_VERDE, HIGH);


  delay(400);


  ligarLedExclusivo(NENHUM_LED);
}


// INICIALIZAR OLED
void inicializarOled() {


  if (display.begin(SSD1306_SWITCHCAPVCC, ENDERECO_OLED)) {


    oledAtivo = true;


    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);
    display.setCursor(0, 15);


    display.println(F("Comedouro automatico"));
    display.println(F("Inicializando..."));


    display.display();


  } else {


    Serial.println(F("OLED nao encontrado!"));
  }
}


// ABRIR COMPORTA
void abrirComporta() {


  comporta.write(ANGULO_SERVO_ABERTO);


  comportaAberta = true;
  tempoInicioLiberacaoMs = millis();


  Serial.println(F("COMPORTA ABERTA"));
}


// FECHAR COMPORTA
void fecharComporta() {


  comporta.write(ANGULO_SERVO_FECHADO);


  if (comportaAberta) {
    tempoFimUltimaLiberacaoMs = millis();
  }


  comportaAberta = false;


  Serial.println(F("COMPORTA FECHADA"));
}


// INICIALIZAR SERVO
void inicializarServo() {


  comporta.attach(PINO_SERVO);


  comporta.write(ANGULO_SERVO_FECHADO);


  comportaAberta = false;


  Serial.println(F("SERVO INICIALIZADO"));
  Serial.println(F("Posicao inicial: 90 graus"));


  delay(1000);
}


// LEITURA DO SENSOR
int lerNivelReservatorio() {


  digitalWrite(PINO_TRIG, LOW);
  delayMicroseconds(2);


  digitalWrite(PINO_TRIG, HIGH);
  delayMicroseconds(10);


  digitalWrite(PINO_TRIG, LOW);


  long duracao = pulseIn(PINO_ECHO, HIGH, 25000UL);


  if (duracao == 0) {


    distanciaAtualCm = -1;
    return -1;
  }


  distanciaAtualCm = (int)(duracao * 0.034 / 2.0);


  if (distanciaAtualCm <= DISTANCIA_CHEIA_CM) {
    return 100;
  }


  if (distanciaAtualCm <= DISTANCIA_MEDIA_CM) {
    return map(distanciaAtualCm, 10, 30, 100, 50);
  }


  if (distanciaAtualCm < DISTANCIA_VAZIA_CM) {
    return map(distanciaAtualCm, 30, 70, 50, 0);
  }


  return 0;
}


// GERENCIAR LEDS E ALARME
void gerenciarSemaforoEAlarme() {


  nivelReservatorio = lerNivelReservatorio();


  Serial.print(F("Distancia: "));
  Serial.print(distanciaAtualCm);
  Serial.print(F(" cm | Nivel: "));
  Serial.println(nivelReservatorio);


  if (distanciaAtualCm < 0) {


    ligarLedExclusivo(NENHUM_LED);
    noTone(PINO_BUZZER);


    return;
  }


  if (distanciaAtualCm <= DISTANCIA_CHEIA_CM) {


    ligarLedExclusivo(PINO_LED_VERDE);
    noTone(PINO_BUZZER);


  } else if (distanciaAtualCm < DISTANCIA_VAZIA_CM) {


    ligarLedExclusivo(PINO_LED_AMARELO);
    noTone(PINO_BUZZER);


  } else {


    ligarLedExclusivo(PINO_LED_VERMELHO);
    tone(PINO_BUZZER, FREQUENCIA_ALARME_RESERVATORIO);
  }
}


// ATUALIZAR OLED
void atualizarOled() {


  if (!oledAtivo) {
    return;
  }


  display.clearDisplay();


  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);


  display.setCursor(0, 0);
  display.print(F("Nivel: "));


  if (nivelReservatorio >= 0) {


    display.print(nivelReservatorio);
    display.print(F("%"));


  } else {


    display.print(F("Sem leitura"));
  }


  display.setCursor(0, 20);


  if (distanciaAtualCm < 0) {


    display.print(F("Erro no sensor"));


  } else if (distanciaAtualCm >= DISTANCIA_VAZIA_CM) {


    display.print(F("Sem racao!"));


  } else if (distanciaAtualCm <= DISTANCIA_CHEIA_CM) {


    display.print(F("Dispenser Cheio"));


  } else if (distanciaAtualCm <= DISTANCIA_MEDIA_CM) {


    display.print(F("Nivel Medio"));


  } else {


    display.print(F("Nivel Baixo"));
  }


  display.setCursor(0, 45);


  if (comportaAberta) {


    display.print(F("Liberando racao..."));


  } else {


    display.print(F("Comporta fechada"));
  }


  display.display();
}


// CONTROLAR COMPORTA
void processarComporta(unsigned long agoraMs) {


  // BLOQUEIO DE SEGURANCA
  if (distanciaAtualCm < 0 ||
      distanciaAtualCm >= DISTANCIA_VAZIA_CM ||
      nivelReservatorio <= 0) {


    if (comportaAberta) {
      fecharComporta();
    }


    estadoAtual = RESERVATORIO_BAIXO;


    return;
  }


  // SE ESTIVER ABERTA
  if (comportaAberta) {


    estadoAtual = LIBERANDO_RACAO;


    if (agoraMs - tempoInicioLiberacaoMs >= TEMPO_ABERTURA_MS) {


      fecharComporta();


      estadoAtual = AGUARDANDO_CICLO;
    }


    return;
  }


  // INTERVALO ENTRE LIBERACOES
  bool intervaloOk =
    (tempoFimUltimaLiberacaoMs == 0) ||
    (agoraMs - tempoFimUltimaLiberacaoMs >= TEMPO_MIN_ENTRE_LIBERACOES_MS);


  if (intervaloOk && nivelReservatorio > 10) {


    abrirComporta();


    estadoAtual = LIBERANDO_RACAO;


  } else {


    estadoAtual = AGUARDANDO_CICLO;
  }
}


// SETUP
void setup() {


  Serial.begin(9600);


  configurarPinos();


  testarLedsInicializacao();


  inicializarOled();


  inicializarServo();
}


// LOOP
void loop() {


  unsigned long agoraMs = millis();


  if (agoraMs - tempoUltimaVerificacaoMs >= INTERVALO_VERIFICACAO_MS) {


    tempoUltimaVerificacaoMs = agoraMs;


    gerenciarSemaforoEAlarme();


    processarComporta(agoraMs);


    atualizarOled();
  }
}
