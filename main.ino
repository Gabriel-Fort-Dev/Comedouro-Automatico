#include <Servo.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>


// OLED
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


// CALIBRACAO DO RESERVATORIO
const float ALTURA_RESERVATORIO_CM = 31.0;
const int LIMITE_NIVEL_BAIXO = 48;


// SERVO
const int ANGULO_SERVO_FECHADO = 90;
const int ANGULO_SERVO_ABERTO = 130;


// TEMPOS
const unsigned long TEMPO_ABERTURA_MS = 2000;
const unsigned long TEMPO_MIN_ENTRE_LIBERACOES_MS = 10000;
const unsigned long INTERVALO_VERIFICACAO_MS = 200;


// ALARME
const unsigned int FREQUENCIA_ALARME_RESERVATORIO = 1000;


// ESTADOS
enum EstadoSistema {
  AGUARDANDO_CICLO,
  LIBERANDO_RACAO,
  RESERVATORIO_BAIXO
};


// OBJETOS
Adafruit_SSD1306 display(LARGURA_TELA, ALTURA_TELA, &Wire, -1);
Servo comporta;


// VARIAVEIS
EstadoSistema estadoAtual = AGUARDANDO_CICLO;


bool oledAtivo = false;
bool comportaAberta = false;


float distanciaAtualCm = -1.0;
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


  digitalWrite(PINO_TRIG, LOW);
}


// CONTROLE DOS LEDS
void ligarLedExclusivo(byte pinoLed) {
  digitalWrite(PINO_LED_VERMELHO,
    pinoLed == PINO_LED_VERMELHO ? HIGH : LOW);


  digitalWrite(PINO_LED_AMARELO,
    pinoLed == PINO_LED_AMARELO ? HIGH : LOW);


  digitalWrite(PINO_LED_VERDE,
    pinoLed == PINO_LED_VERDE ? HIGH : LOW);
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


// LEITURA E CALCULO DO RESERVATORIO
int lerNivelReservatorio() {
  digitalWrite(PINO_TRIG, LOW);
  delayMicroseconds(2);


  digitalWrite(PINO_TRIG, HIGH);
  delayMicroseconds(10);


  digitalWrite(PINO_TRIG, LOW);


  long duracao = pulseIn(PINO_ECHO, HIGH, 25000UL);


  if (duracao == 0) {
    distanciaAtualCm = -1.0;
    return -1;
  }


  float distancia = duracao * 0.0343 / 2.0;


  if (distancia <= 0 || distancia > 400) {
    distanciaAtualCm = -1.0;
    return -1;
  }


  distanciaAtualCm = distancia;


  if (distancia >= ALTURA_RESERVATORIO_CM) {
    return 0;
  }


  if (distancia <= 0) {
    return 100;
  }


  float porcentagem =
    ((ALTURA_RESERVATORIO_CM - distancia) /
    ALTURA_RESERVATORIO_CM) * 100.0;


  return constrain((int)(porcentagem + 0.5), 0, 100);
}


// GERENCIAR LEDS E ALARME
void gerenciarSemaforoEAlarme() {
  nivelReservatorio = lerNivelReservatorio();


  Serial.print(F("Distancia: "));


  if (distanciaAtualCm < 0) {
    Serial.print(F("Sem leitura"));
  } else {
    Serial.print(distanciaAtualCm, 1);
    Serial.print(F(" cm"));
  }


  Serial.print(F(" | Nivel: "));


  if (nivelReservatorio < 0) {
    Serial.println(F("Sem leitura"));
  } else {
    Serial.print(nivelReservatorio);
    Serial.println(F("%"));
  }


  if (nivelReservatorio < 0) {
    ligarLedExclusivo(NENHUM_LED);
    noTone(PINO_BUZZER);
    return;
  }


  if (nivelReservatorio == 100) {
    ligarLedExclusivo(PINO_LED_VERDE);
    noTone(PINO_BUZZER);


  } else if (nivelReservatorio > 0) {
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


  if (nivelReservatorio < 0) {
    display.print(F("Erro no sensor"));


  } else if (nivelReservatorio == 0) {
    display.print(F("Sem racao!"));


  } else if (nivelReservatorio < LIMITE_NIVEL_BAIXO) {
    display.print(F("Nivel Baixo"));


  } else if (nivelReservatorio == 100) {
    display.print(F("Dispenser Cheio"));


  } else {
    display.print(F("Nivel Medio"));
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


  // BLOQUEIO APENAS SE O SENSOR FALHAR OU O RESERVATORIO ESTIVER VAZIO
  if (nivelReservatorio < 0 || nivelReservatorio == 0) {
    if (comportaAberta) {
      fecharComporta();
    }


    estadoAtual = RESERVATORIO_BAIXO;
    return;
  }


  // SE A COMPORTA ESTIVER ABERTA
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


  // LIBERAR RACAO MESMO COM NIVEL BAIXO
  if (intervaloOk) {
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


  ligarLedExclusivo(NENHUM_LED);
  noTone(PINO_BUZZER);


  Serial.println(F("Sistema iniciado."));
  Serial.println(F("Aguardando leitura do sensor..."));
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
