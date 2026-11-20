#include <Servo.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ======================== CONFIGURACOES ========================

// Display OLED
const uint8_t LARGURA_TELA = 128;
const uint8_t ALTURA_TELA = 64;
const uint8_t ENDERECO_OLED = 0x3C;

// Pinos
const byte PINO_TRIG = 4;
const byte PINO_ECHO = 5;
const byte PINO_BUZZER = 6;
const byte PINO_LED_VERMELHO = 7;
const byte PINO_LED_AMARELO = 8;
const byte PINO_LED_VERDE = 9;
const byte PINO_SERVO = 12;

// Temporizacao
const unsigned long TEMPO_ABERTURA_MS = 2000;
const unsigned long TEMPO_MIN_ENTRE_LIBERACOES_MS = 10000;
const unsigned long INTERVALO_VERIFICACAO_MS = 200;

// Distancias de referencia
const int DISTANCIA_CHEIA_CM = 3;
const int DISTANCIA_VAZIA_CM = 22;

// Servo motor
const int ANGULO_SERVO_FECHADO = 90;
const int ANGULO_SERVO_ABERTO = 135;

// Buzzer
const unsigned int FREQUENCIA_ALARME_RESERVATORIO = 1000;

// Niveis de estoque
const int NIVEL_MINIMO_LIBERACAO = 10;
const int NIVEL_ALERTA_BAIXO = 60;
const int NIVEL_ALERTA_INTERMEDIARIO = 61;
const int NIVEL_RESERVATORIO_ADEQUADO = 80;

// ======================== ESTADOS ==============================

enum EstadoSistema {
    AGUARDANDO_CICLO,
    LIBERANDO_RACAO,
    RESERVATORIO_BAIXO,
    ERRO_SENSOR
};

// ======================== OBJETOS ==============================

Adafruit_SSD1306 display(
    LARGURA_TELA,
    ALTURA_TELA,
    &Wire,
    -1
);

Servo comporta;

// ======================== VARIAVEIS ============================

EstadoSistema estadoAtual = AGUARDANDO_CICLO;

bool oledAtivo = false;
bool comportaAberta = false;

int distanciaAtualCm = -1;
int nivelReservatorio = -1;

unsigned long tempoInicioLiberacaoMs = 0;
unsigned long tempoFimUltimaLiberacaoMs = 0;
unsigned long tempoUltimaVerificacaoMs = 0;

// ======================== PINOS E LEDS =========================

void configurarPinos() {
    pinMode(PINO_TRIG, OUTPUT);
    pinMode(PINO_ECHO, INPUT);

    pinMode(PINO_LED_VERMELHO, OUTPUT);
    pinMode(PINO_LED_AMARELO, OUTPUT);
    pinMode(PINO_LED_VERDE, OUTPUT);

    pinMode(PINO_BUZZER, OUTPUT);
}

void ligarLedExclusivo(byte pinoLed) {
    digitalWrite(
        PINO_LED_VERMELHO,
        (pinoLed == PINO_LED_VERMELHO) ? HIGH : LOW
    );

    digitalWrite(
        PINO_LED_AMARELO,
        (pinoLed == PINO_LED_AMARELO) ? HIGH : LOW
    );

    digitalWrite(
        PINO_LED_VERDE,
        (pinoLed == PINO_LED_VERDE) ? HIGH : LOW
    );
}

void testarLedsInicializacao() {
    digitalWrite(PINO_LED_VERMELHO, HIGH);
    digitalWrite(PINO_LED_AMARELO, HIGH);
    digitalWrite(PINO_LED_VERDE, HIGH);

    delay(400);

    ligarLedExclusivo(0);
}

// ======================== DISPLAY OLED ==========================

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

    display.setCursor(0, 22);

    if (nivelReservatorio >= NIVEL_RESERVATORIO_ADEQUADO) {
        display.print(F("Dispenser Cheio"));
    } else if (
        nivelReservatorio >= NIVEL_ALERTA_INTERMEDIARIO
    ) {
        display.print(F("Atencao para repor"));
    } else if (nivelReservatorio >= 0) {
        display.print(F("Dispenser Vazio"));
    } else {
        display.print(F("Erro no sensor"));
    }

    display.setCursor(0, 48);

    if (comportaAberta) {
        display.print(F("Liberando racao..."));
    } else {
        display.print(F("Comporta fechada"));
    }

    display.display();
}

// ======================== SERVO MOTOR ===========================

void abrirComporta() {
    comporta.write(ANGULO_SERVO_ABERTO);

    comportaAberta = true;
    tempoInicioLiberacaoMs = millis();
}

void fecharComporta() {
    comporta.write(ANGULO_SERVO_FECHADO);

    if (comportaAberta) {
        tempoFimUltimaLiberacaoMs = millis();
    }

    comportaAberta = false;
}

void inicializarServo() {
    comporta.attach(PINO_SERVO);
    fecharComporta();
}

// ======================== SENSOR ULTRASSONICO ===================

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

    distanciaAtualCm = duracao * 0.034 / 2;

    int porcentagem = map(
        distanciaAtualCm,
        DISTANCIA_VAZIA_CM,
        DISTANCIA_CHEIA_CM,
        0,
        100
    );

    return constrain(porcentagem, 0, 100);
}

// ======================== INDICADORES ===========================

void gerenciarSemaforoEAlarme() {
    nivelReservatorio = lerNivelReservatorio();

    if (nivelReservatorio < 0) {
        ligarLedExclusivo(0);
        noTone(PINO_BUZZER);

        estadoAtual = ERRO_SENSOR;
        return;
    }

    if (nivelReservatorio >= NIVEL_RESERVATORIO_ADEQUADO) {
        ligarLedExclusivo(PINO_LED_VERDE);
        noTone(PINO_BUZZER);

    } else if (
        nivelReservatorio >= NIVEL_ALERTA_INTERMEDIARIO
    ) {
        ligarLedExclusivo(PINO_LED_AMARELO);
        noTone(PINO_BUZZER);

    } else {
        ligarLedExclusivo(PINO_LED_VERMELHO);
        tone(PINO_BUZZER, FREQUENCIA_ALARME_RESERVATORIO);
    }
}

// ======================== CONTROLE DA LIBERACAO ================

void processarComporta(unsigned long agoraMs) {
    if (nivelReservatorio < 0) {
        fecharComporta();
        estadoAtual = ERRO_SENSOR;
        return;
    }

    if (comportaAberta) {
        estadoAtual = LIBERANDO_RACAO;

        if (
            agoraMs - tempoInicioLiberacaoMs >=
            TEMPO_ABERTURA_MS
        ) {
            fecharComporta();
        }

        return;
    }

    bool intervaloOk =
        (tempoFimUltimaLiberacaoMs == 0) ||
        (
            agoraMs - tempoFimUltimaLiberacaoMs >=
            TEMPO_MIN_ENTRE_LIBERACOES_MS
        );

    bool nivelSuficiente =
        nivelReservatorio > NIVEL_MINIMO_LIBERACAO;

    if (intervaloOk && nivelSuficiente) {
        abrirComporta();
        estadoAtual = LIBERANDO_RACAO;

    } else {
        estadoAtual =
            (nivelReservatorio <= NIVEL_ALERTA_BAIXO)
            ? RESERVATORIO_BAIXO
            : AGUARDANDO_CICLO;
    }
}

// ======================== INICIALIZACAO =========================

void setup() {
    Serial.begin(9600);

    configurarPinos();
    testarLedsInicializacao();
    inicializarOled();
    inicializarServo();
}

// ======================== LOOP PRINCIPAL ========================

void loop() {
    unsigned long agoraMs = millis();

    if (
        agoraMs - tempoUltimaVerificacaoMs >=
        INTERVALO_VERIFICACAO_MS
    ) {
        tempoUltimaVerificacaoMs = agoraMs;

        gerenciarSemaforoEAlarme();
        processarComporta(agoraMs);
        atualizarOled();
    }
}
