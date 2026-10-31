#include <WiFi.h>
#include <esp_now.h>

// ======================================================
// PINO DO MÓDULO VIBRACALL
// ======================================================

const int PINO_VIBRACALL = 4;


// ======================================================
// PADRÃO DE VIBRAÇÃO PULSANTE
// ======================================================

const unsigned long TEMPO_PULSO = 200;


// ======================================================
// FAIL-SAFE
// ======================================================

const unsigned long TEMPO_FAILSAFE = 2000;


// ======================================================
// ESTADOS POSSÍVEIS
// ======================================================

enum EstadoVibracao {
  DESLIGADO = 0,
  CONTINUO  = 1,
  PULSANTE  = 2
};


struct ComandoPulseira {
  uint8_t estado;
  uint32_t sequencia;
};


volatile uint8_t estadoAtual = DESLIGADO;

volatile unsigned long ultimaMensagem = 0;

unsigned long ultimoPulso = 0;

bool motorLigado = false;


// ======================================================
// RECEPÇÃO ESP-NOW
// ======================================================

void OnDataRecv(
  const esp_now_recv_info_t *info,
  const uint8_t *data,
  int len
) {

  if (len != sizeof(ComandoPulseira)) {
    Serial.print("Pacote com tamanho incorreto: ");
    Serial.println(len);
    return;
  }

  ComandoPulseira comando;

  memcpy(
    &comando,
    data,
    sizeof(comando)
  );

  estadoAtual = comando.estado;
  ultimaMensagem = millis();

  // DEBUG TEMPORÁRIO
  Serial.print("Pacote recebido | Estado: ");
  Serial.print(comando.estado);

  Serial.print(" | Sequencia: ");
  Serial.println(comando.sequencia);
}



// ======================================================
// CONFIGURAÇÃO
// ======================================================

void setup() {

  Serial.begin(115200);
  delay(1000);

  pinMode(PINO_VIBRACALL, OUTPUT);

  digitalWrite(PINO_VIBRACALL, LOW);

  WiFi.mode(WIFI_STA);

  Serial.println();

  Serial.print("MAC pulseira esquerda: ");

  Serial.println(WiFi.macAddress());


  if (esp_now_init() != ESP_OK) {

    Serial.println("Erro ao iniciar ESP-NOW.");

    while (true) {
      delay(1000);
    }
  }

  esp_now_register_recv_cb(OnDataRecv);

  Serial.println("Pulseira esquerda iniciada.");
}


// ======================================================
// LOOP
// ======================================================

void loop() {

  unsigned long agora = millis();


  // ----------------------------------------------------
  // Proteção contra perda de comunicação
  // ----------------------------------------------------

  if (
    ultimaMensagem != 0 &&
    agora - ultimaMensagem > TEMPO_FAILSAFE
  ) {

    estadoAtual = DESLIGADO;
  }


  // ----------------------------------------------------
  // Motor desligado
  // ----------------------------------------------------

  if (estadoAtual == DESLIGADO) {

    digitalWrite(PINO_VIBRACALL, LOW);

    motorLigado = false;

    return;
  }


  // ----------------------------------------------------
  // Vibração contínua
  // ----------------------------------------------------

  if (estadoAtual == CONTINUO) {

    digitalWrite(PINO_VIBRACALL, HIGH);

    motorLigado = true;

    return;
  }


  // ----------------------------------------------------
  // Vibração pulsante
  // ----------------------------------------------------

  if (estadoAtual == PULSANTE) {

    if (agora - ultimoPulso >= TEMPO_PULSO) {

      ultimoPulso = agora;

      motorLigado = !motorLigado;

      digitalWrite(
        PINO_VIBRACALL,
        motorLigado ? HIGH : LOW
      );
    }
  }
}