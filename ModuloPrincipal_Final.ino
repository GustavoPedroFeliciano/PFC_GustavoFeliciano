#include <WiFi.h>
#include <esp_now.h>

// ======================================================
// PINOS
// ======================================================

// Superior direito
const int TRIG_SD = 11;
const int ECHO_SD = 10;

// Inferior direito
const int TRIG_ID = 9;
const int ECHO_ID = 8;

// Superior esquerdo
const int TRIG_SE = 4;
const int ECHO_SE = 5;

// Inferior esquerdo
const int TRIG_IE = 6;
const int ECHO_IE = 7;

// Buzzer
const int PINO_BUZZER = 1;


// ======================================================
// LIMITES DE DETECÇÃO
// ======================================================

// Sensores superiores
const float DETECCAO_SUPERIOR = 140.0;  // cm
const float CRITICA_SUPERIOR  = 70.0;   // cm

// Sensores inferiores
// Valores compensados pela distância diagonal
const float DETECCAO_INFERIOR = 205.0;  // cm
const float CRITICA_INFERIOR  = 166.0;  // cm

// Faixa válida do HC-SR04
const float DISTANCIA_MINIMA = 2.0;
const float DISTANCIA_MAXIMA = 250.0;


// ======================================================
// ENDEREÇOS MAC
// ======================================================

uint8_t MAC_ESQUERDA[] = {
  0xB8, 0x1F, 0x3F, 0xFC, 0x38, 0x14
};

uint8_t MAC_DIREITA[] = {
  0xB8, 0x1F, 0x3F, 0xFC, 0x36, 0x40
};


// ======================================================
// ESTADOS DE VIBRAÇÃO
// ======================================================

enum EstadoVibracao {
  DESLIGADO = 0,
  CONTINUO  = 1,
  PULSANTE  = 2
};


// ======================================================
// MENSAGEM ESP-NOW
// ======================================================

struct ComandoPulseira {
  uint8_t estado;
  uint32_t sequencia;
};

uint32_t contadorSequencia = 0;


// ======================================================
// FILTRO DOS SENSORES
// ======================================================

struct FiltroSensor {
  float valores[3] = {-1.0, -1.0, -1.0};
  int indice = 0;
  int quantidade = 0;
};

FiltroSensor filtroSD;
FiltroSensor filtroID;
FiltroSensor filtroSE;
FiltroSensor filtroIE;


// ======================================================
// LEITURA DO HC-SR04
// ======================================================

float medirDistancia(int trig, int echo) {

  digitalWrite(trig, LOW);
  delayMicroseconds(2);

  digitalWrite(trig, HIGH);
  delayMicroseconds(10);
  digitalWrite(trig, LOW);

  // Timeout de 25 ms
  unsigned long duracao =
    pulseIn(echo, HIGH, 25000);

  if (duracao == 0) {
    return -1.0;
  }

  float distancia =
    (duracao * 0.0343) / 2.0;

  // Descarta valores fora da faixa útil
  if (
    distancia < DISTANCIA_MINIMA ||
    distancia > DISTANCIA_MAXIMA
  ) {
    return -1.0;
  }

  return distancia;
}


// ======================================================
// MEDIANA DE 3 LEITURAS
// ======================================================

float aplicarMediana(
  FiltroSensor &filtro,
  float novaLeitura
) {

  // Leitura inválida não entra no filtro
  if (novaLeitura < 0) {
    return -1.0;
  }

  filtro.valores[filtro.indice] =
    novaLeitura;

  filtro.indice =
    (filtro.indice + 1) % 3;

  if (filtro.quantidade < 3) {
    filtro.quantidade++;
  }

  // Ainda não há três valores
  if (filtro.quantidade < 3) {
    return novaLeitura;
  }

  float a = filtro.valores[0];
  float b = filtro.valores[1];
  float c = filtro.valores[2];

  // Retorna o valor intermediário
  if (
    (a >= b && a <= c) ||
    (a <= b && a >= c)
  ) {
    return a;
  }

  if (
    (b >= a && b <= c) ||
    (b <= a && b >= c)
  ) {
    return b;
  }

  return c;
}


// ======================================================
// DETECÇÃO DE OBSTÁCULO
// ======================================================

bool obstaculoDetectado(
  float distancia,
  float limite
) {

  if (distancia < 0) {
    return false;
  }

  return distancia <= limite;
}


// ======================================================
// DETECÇÃO DE PERIGO CRÍTICO
// ======================================================

bool perigoCritico(
  float distancia,
  float limite
) {

  if (distancia < 0) {
    return false;
  }

  return distancia <= limite;
}


// ======================================================
// PADRÃO DE VIBRAÇÃO
// ======================================================

EstadoVibracao definirEstado(
  bool superior,
  bool inferior
) {

  // Sensor superior tem prioridade
  if (superior) {
    return CONTINUO;
  }

  if (inferior) {
    return PULSANTE;
  }

  return DESLIGADO;
}


// ======================================================
// ENVIO ESP-NOW
// ======================================================

void enviarComando(
  uint8_t *mac,
  EstadoVibracao estado
) {

  ComandoPulseira comando;

  comando.estado = estado;
  comando.sequencia = contadorSequencia;

  esp_now_send(
    mac,
    (uint8_t *)&comando,
    sizeof(comando)
  );
}


// ======================================================
// CONFIGURAÇÃO
// ======================================================

void setup() {

  Serial.begin(115200);
  delay(1000);

  // Sensores
  pinMode(TRIG_SD, OUTPUT);
  pinMode(ECHO_SD, INPUT);

  pinMode(TRIG_ID, OUTPUT);
  pinMode(ECHO_ID, INPUT);

  pinMode(TRIG_SE, OUTPUT);
  pinMode(ECHO_SE, INPUT);

  pinMode(TRIG_IE, OUTPUT);
  pinMode(ECHO_IE, INPUT);

  // Mantém TRIGs inicialmente baixos
  digitalWrite(TRIG_SD, LOW);
  digitalWrite(TRIG_ID, LOW);
  digitalWrite(TRIG_SE, LOW);
  digitalWrite(TRIG_IE, LOW);

  // Buzzer desligado
  pinMode(PINO_BUZZER, OUTPUT);
  digitalWrite(PINO_BUZZER, LOW);

  // Wi-Fi
  WiFi.mode(WIFI_STA);

  // Inicializa ESP-NOW
  if (esp_now_init() != ESP_OK) {

    Serial.println(
      "Erro ao iniciar ESP-NOW."
    );

    while (true) {
      delay(1000);
    }
  }

  // Pulseira esquerda
  esp_now_peer_info_t peerEsquerda = {};

  memcpy(
    peerEsquerda.peer_addr,
    MAC_ESQUERDA,
    6
  );

  peerEsquerda.channel = 0;
  peerEsquerda.encrypt = false;

  if (
    esp_now_add_peer(&peerEsquerda)
    != ESP_OK
  ) {
    Serial.println(
      "Erro ao adicionar pulseira esquerda."
    );
  }

  // Pulseira direita
  esp_now_peer_info_t peerDireita = {};

  memcpy(
    peerDireita.peer_addr,
    MAC_DIREITA,
    6
  );

  peerDireita.channel = 0;
  peerDireita.encrypt = false;

  if (
    esp_now_add_peer(&peerDireita)
    != ESP_OK
  ) {
    Serial.println(
      "Erro ao adicionar pulseira direita."
    );
  }

  Serial.println();
  Serial.println(
    "Sistema principal iniciado."
  );
}


// ======================================================
// LOOP PRINCIPAL
// ======================================================

void loop() {

  // --------------------------------------------------
  // LEITURA DOS SENSORES
  // --------------------------------------------------

  float distanciaSD =
    aplicarMediana(
      filtroSD,
      medirDistancia(TRIG_SD, ECHO_SD)
    );

  // Reduz interferência entre sensores
  delay(40);


  float distanciaID =
    aplicarMediana(
      filtroID,
      medirDistancia(TRIG_ID, ECHO_ID)
    );

  delay(40);


  float distanciaSE =
    aplicarMediana(
      filtroSE,
      medirDistancia(TRIG_SE, ECHO_SE)
    );

  delay(40);


  float distanciaIE =
    aplicarMediana(
      filtroIE,
      medirDistancia(TRIG_IE, ECHO_IE)
    );

  delay(40);


  // --------------------------------------------------
  // DETECÇÃO
  // --------------------------------------------------

  bool superiorDireito =
    obstaculoDetectado(
      distanciaSD,
      DETECCAO_SUPERIOR
    );

  bool inferiorDireito =
    obstaculoDetectado(
      distanciaID,
      DETECCAO_INFERIOR
    );

  bool superiorEsquerdo =
    obstaculoDetectado(
      distanciaSE,
      DETECCAO_SUPERIOR
    );

  bool inferiorEsquerdo =
    obstaculoDetectado(
      distanciaIE,
      DETECCAO_INFERIOR
    );


  // --------------------------------------------------
  // ESTADO DAS PULSEIRAS
  // --------------------------------------------------

  EstadoVibracao estadoDireito =
    definirEstado(
      superiorDireito,
      inferiorDireito
    );

  EstadoVibracao estadoEsquerdo =
    definirEstado(
      superiorEsquerdo,
      inferiorEsquerdo
    );


  // --------------------------------------------------
  // PERIGO CRÍTICO
  // --------------------------------------------------

  bool perigo =
    perigoCritico(
      distanciaSD,
      CRITICA_SUPERIOR
    )
    ||
    perigoCritico(
      distanciaID,
      CRITICA_INFERIOR
    )
    ||
    perigoCritico(
      distanciaSE,
      CRITICA_SUPERIOR
    )
    ||
    perigoCritico(
      distanciaIE,
      CRITICA_INFERIOR
    );


  // Buzzer ativo em HIGH
  digitalWrite(
    PINO_BUZZER,
    perigo ? HIGH : LOW
  );


  // --------------------------------------------------
  // ENVIO PARA AS PULSEIRAS
  // --------------------------------------------------

  contadorSequencia++;

  enviarComando(
    MAC_DIREITA,
    estadoDireito
  );

  enviarComando(
    MAC_ESQUERDA,
    estadoEsquerdo
  );


  // --------------------------------------------------
  // MONITOR SERIAL
  // --------------------------------------------------

  Serial.println(
    "================================"
  );

  Serial.print(
    "Superior direito: "
  );
  Serial.print(distanciaSD);
  Serial.println(" cm");


  Serial.print(
    "Inferior direito: "
  );
  Serial.print(distanciaID);
  Serial.println(" cm");


  Serial.print(
    "Superior esquerdo: "
  );
  Serial.print(distanciaSE);
  Serial.println(" cm");


  Serial.print(
    "Inferior esquerdo: "
  );
  Serial.print(distanciaIE);
  Serial.println(" cm");


  Serial.print(
    "Estado direito: "
  );
  Serial.println(estadoDireito);


  Serial.print(
    "Estado esquerdo: "
  );
  Serial.println(estadoEsquerdo);


  Serial.print("Buzzer: ");
  Serial.println(
    perigo ? "ATIVO" : "DESLIGADO"
  );

  Serial.println();
}