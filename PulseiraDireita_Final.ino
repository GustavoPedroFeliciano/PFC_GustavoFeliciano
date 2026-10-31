#include <WiFi.h>
#include <esp_now.h>

// ======================================================
// PINO DO MÓDULO VIBRACALL
// ======================================================

const int PINO_VIBRACALL = 4;


// ======================================================
// PADRÃO DE VIBRAÇÃO PULSANTE
// ======================================================

// 200 ms ligado / 200 ms desligado
const unsigned long TEMPO_PULSO = 200;


// ======================================================
// FAIL-SAFE
// ======================================================

// Se perder comunicação com o principal,
// a vibração é desligada após 2 segundos.
const unsigned long TEMPO_FAILSAFE = 2000;


// ======================================================
// ESTADOS RECEBIDOS
// ======================================================

enum EstadoVibracao {
  DESLIGADO = 0,
  CONTINUO  = 1,
  PULSANTE  = 2
};


// Deve ser igual à estrutura usada no S3
struct ComandoPulseira {
  uint8_t estado;
  uint32_t sequencia;
};


// Estado atual da pulseira
volatile uint8_t estadoAtual = DESLIGADO;

// Momento da última mensagem recebida
volatile unsigned long ultimaMensagem = 0;


// Controle do pulso
unsigned long ultimoPulso = 0;
bool motorLigado = false;


// ======================================================
// CALLBACK DO ESP-NOW
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
  Serial.print("MAC pulseira direita: ");
  Serial.println(WiFi.macAddress());

  if (esp_now_init() != ESP_OK) {

    Serial.println("Erro ao iniciar ESP-NOW.");

    while (true) {
      delay(1000);
    }
  }

  esp_now_register_recv_cb(OnDataRecv);

  Serial.println("Pulseira direita iniciada.");
}


// ======================================================
// LOOP
// ======================================================

void loop() {

  unsigned long agora = millis();


  // ----------------------------------------------------
  // FAIL-SAFE
  // ----------------------------------------------------

  if (
    ultimaMensagem != 0 &&
    agora - ultimaMensagem > TEMPO_FAILSAFE
  ) {

    estadoAtual = DESLIGADO;
  }


  // ----------------------------------------------------
  // VIBRAÇÃO DESLIGADA
  // ----------------------------------------------------

  if (estadoAtual == DESLIGADO) {

    digitalWrite(PINO_VIBRACALL, LOW);

    motorLigado = false;

    return;
  }


  // ----------------------------------------------------
  // VIBRAÇÃO CONTÍNUA
  // ----------------------------------------------------

  if (estadoAtual == CONTINUO) {

    digitalWrite(PINO_VIBRACALL, HIGH);

    motorLigado = true;

    return;
  }


  // ----------------------------------------------------
  // VIBRAÇÃO PULSANTE
  // ----------------------------------------------------

  if (estadoAtual == PULSANTE) {

    // Troca o estado a cada 200 ms sem usar delay()
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