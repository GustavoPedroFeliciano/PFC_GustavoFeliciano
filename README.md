# Dispositivo Vestível para Auxílio à Locomoção de Pessoas com Deficiência Visual

Repositório desenvolvido como material complementar ao Trabalho de Conclusão de Curso **“Dispositivo Vestível para Auxílio à Locomoção de Pessoas com Deficiência Visual”**, do curso de Engenharia de Controle e Automação da Universidade Federal de Minas Gerais (UFMG).

O projeto consiste em um dispositivo vestível de baixo custo destinado à detecção de obstáculos em diferentes regiões do trajeto de pessoas com deficiência visual. O sistema utiliza sensores ultrassônicos, processamento embarcado, comunicação sem fio e feedback vibrotátil para fornecer informações relacionadas à posição aproximada dos obstáculos.

---

## Visão geral do sistema

O protótipo é composto por:

- um **módulo principal**, responsável pelo sensoriamento e processamento;
- uma **pulseira vibrotátil esquerda**;
- uma **pulseira vibrotátil direita**;
- uma **base externa de carregamento**.

A comunicação entre o módulo principal e as pulseiras é realizada por meio do protocolo **ESP-NOW**, sem necessidade de roteador ou conexão com a Internet.

### Arquitetura simplificada

```text
                    Módulo Principal
                       ESP32-S3
                           |
          +----------------+----------------+
          |                |                |
      HC-SR04 x4        Buzzer           ESP-NOW
                                            |
                              +-------------+-------------+
                              |                           |
                       Pulseira Esquerda           Pulseira Direita
                          ESP32-C3                    ESP32-C3
                              |                           |
                          Vibracall                   Vibracall
```

---

## Hardware utilizado

### Módulo principal

- ESP32-S3 Super Mini;
- 4 sensores ultrassônicos HC-SR04;
- buzzer ativo;
- bateria Li-Po de 3,7 V / 800 mAh;
- conversor elevador MT3608;
- capacitor eletrolítico de 1000 µF;
- capacitor cerâmico de 100 nF;
- divisores resistivos para adequação dos sinais ECHO;
- placa de circuito impresso desenvolvida para o protótipo.

### Pulseiras vibrotáteis

Cada pulseira utiliza:

- ESP32-C3 Super Mini;
- módulo motor de vibração;
- bateria Li-Po de 3,7 V / 800 mAh;
- conversor elevador MT3608;
- capacitor eletrolítico de 470 µF;
- capacitor cerâmico de 100 nF;
- placa de circuito impresso dedicada.

### Base de carregamento

A base de carregamento utiliza:

- módulo baseado no TP4056;
- alimentação externa de 5 V;
- interface dedicada para conexão com as unidades vestíveis.

> **Atenção:** os conectores USB Tipo C presentes nas unidades vestíveis são utilizados apenas como interface física para o carregamento desenvolvido neste projeto. Eles **não correspondem a entradas USB Tipo C convencionais de 5 V** e devem ser utilizados exclusivamente com a base de carregamento do protótipo.

---

## Distribuição dos GPIOs

### Módulo principal

| Dispositivo | TRIG | ECHO |
|---|---:|---:|
| Sensor superior direito | GPIO 11 | GPIO 10 |
| Sensor inferior direito | GPIO 9 | GPIO 8 |
| Sensor superior esquerdo | GPIO 4 | GPIO 5 |
| Sensor inferior esquerdo | GPIO 6 | GPIO 7 |
| Buzzer | GPIO 1 | — |

### Pulseiras

O módulo motor de vibração de cada pulseira é controlado pelo **GPIO 4** do ESP32-C3.

---

## Estratégia de detecção

Os sensores foram divididos entre as regiões superior e inferior dos lados esquerdo e direito do usuário.

Os sensores superiores são orientados predominantemente para a região frontal, enquanto os sensores inferiores são inclinados em direção à região inferior do trajeto.

Foram adotados os seguintes limiares:

| Posição do sensor | Alerta vibrotátil | Condição crítica |
|---|---:|---:|
| Superior | 140 cm | 70 cm |
| Inferior | 205 cm | 166 cm |

Os valores dos sensores inferiores foram definidos considerando a geometria de instalação, com altura aproximada de 1,50 m.

Na versão final do software, **não é realizada correção geométrica das distâncias durante a execução**. A geometria foi utilizada apenas para definir os limiares equivalentes dos sensores inferiores.

---

## Tratamento das medições

Para aumentar a estabilidade das leituras dos sensores ultrassônicos, foram utilizadas as seguintes estratégias:

- faixa válida de medição entre **2 cm e 250 cm**;
- leitura sequencial dos quatro sensores;
- intervalo de aproximadamente **40 ms** entre acionamentos;
- filtro de mediana de **3 amostras** por sensor.

As distâncias filtradas são utilizadas diretamente pela lógica de detecção de obstáculos.

---

## Estratégia de alertas

A posição e o padrão da vibração são utilizados para indicar a região aproximada do obstáculo.

| Condição detectada | Pulseira | Padrão |
|---|---|---|
| Obstáculo superior esquerdo | Esquerda | Contínuo |
| Obstáculo inferior esquerdo | Esquerda | Pulsante |
| Obstáculo superior direito | Direita | Contínuo |
| Obstáculo inferior direito | Direita | Pulsante |
| Superior e inferior no mesmo lado | Correspondente | Contínuo |
| Nenhum obstáculo | Correspondente | Desligado |

O padrão pulsante utiliza aproximadamente **200 ms ligado e 200 ms desligado**.

Quando qualquer sensor identifica um obstáculo dentro da faixa crítica, o **buzzer do módulo principal** também é acionado.

---

## Comunicação ESP-NOW

O ESP32-S3 do módulo principal atua como transmissor, enquanto os ESP32-C3 das pulseiras atuam como receptores.

Cada pulseira possui um endereço MAC próprio, permitindo o envio independente dos comandos.

### Endereços MAC utilizados

| Unidade | Endereço MAC |
|---|---|
| Pulseira esquerda | `B8:1F:3F:FC:38:14` |
| Pulseira direita | `B8:1F:3F:FC:36:40` |

A estrutura utilizada para transmissão dos comandos é:

```cpp
struct DataMessage {
    uint8_t estado;
    uint32_t sequencia;
};
```

Os estados utilizados são:

| Estado | Valor |
|---|---:|
| Desligado | `0` |
| Contínuo | `1` |
| Pulsante | `2` |

O campo `sequencia` é utilizado para acompanhar a ordem das mensagens durante os testes de comunicação.

---

## Segurança em caso de perda de comunicação

Cada pulseira registra o instante da última mensagem válida recebida.

Caso nenhuma nova mensagem seja recebida durante aproximadamente **2000 ms**, o motor de vibração é automaticamente desligado.

Esse mecanismo evita que um comando antigo permaneça ativo após uma interrupção da comunicação.

O funcionamento normal é retomado após o recebimento de uma nova mensagem válida.

---

## Estrutura do repositório

```text
/
├── ModuloPrincipal/
│   └── ModuloPrincipal_Final.ino
│
├── PulseiraEsquerda/
│   └── PulseiraEsquerda_Final.ino
│
├── PulseiraDireita/
│   └── PulseiraDireita_Final.ino
│
├── Utilitarios/
│   └── MACadress.ino
│
└── README.md
```

### Módulo principal

O arquivo `ModuloPrincipal_Final.ino` é responsável por:

- leitura dos quatro sensores HC-SR04;
- validação das distâncias;
- aplicação do filtro de mediana;
- classificação das condições de detecção;
- definição dos estados das pulseiras;
- acionamento do buzzer;
- transmissão dos comandos por ESP-NOW.

### Pulseira esquerda

O arquivo `PulseiraEsquerda_Final.ino` realiza:

- recepção dos comandos ESP-NOW;
- controle do módulo de vibração;
- geração dos padrões contínuo e pulsante;
- tratamento do timeout de comunicação.

### Pulseira direita

O arquivo `PulseiraDireita_Final.ino` utiliza a mesma lógica de funcionamento, sendo destinado à unidade do lado direito.

### Utilitário de endereço MAC

O arquivo `MACadress.ino` foi utilizado para consultar o endereço MAC dos microcontroladores durante a configuração da comunicação ESP-NOW.

---

## Ambiente de desenvolvimento

Os códigos foram desenvolvidos utilizando:

- Arduino IDE;
- suporte à família ESP32;
- ESP32-S3 Super Mini;
- ESP32-C3 Super Mini.

A comunicação entre os dispositivos utiliza os recursos de Wi-Fi e ESP-NOW disponíveis nos microcontroladores ESP32.

---

## Utilização

### 1. Identificação dos endereços MAC

Caso sejam utilizados outros microcontroladores, os endereços MAC das pulseiras devem ser identificados utilizando o código:

`Utilitarios/MACadress.ino`

Os novos endereços devem ser inseridos no código do módulo principal.

### 2. Programação das pulseiras

Carregar os respectivos arquivos nos ESP32-C3:

- `PulseiraEsquerda/PulseiraEsquerda_Final.ino`
- `PulseiraDireita/PulseiraDireita_Final.ino`

### 3. Programação do módulo principal

Carregar no ESP32-S3:

`ModuloPrincipal/ModuloPrincipal_Final.ino`

### 4. Operação

Após a inicialização:

1. o módulo principal realiza a leitura dos sensores;
2. as medições são filtradas;
3. as condições de detecção são classificadas;
4. os estados das pulseiras são definidos;
5. os comandos são enviados via ESP-NOW;
6. as pulseiras geram os padrões de vibração correspondentes;
7. condições críticas também provocam o acionamento do buzzer.

---

## Alimentação

Cada uma das três unidades vestíveis utiliza uma bateria independente com:

- tensão nominal de **3,7 V**;
- capacidade de **800 mAh**;
- configuração de **uma célula (1S)**.

A tensão da bateria é elevada para aproximadamente 5 V por meio do conversor MT3608.

A recarga é realizada individualmente por meio da base externa desenvolvida para o projeto.

Durante o carregamento, a respectiva unidade deve permanecer desligada.

---

## Placas de circuito impresso

Foram desenvolvidas placas específicas para:

- módulo principal;
- pulseiras vibrotáteis;
- base de carregamento.

Os esquemáticos e layouts foram elaborados no **EasyEDA**.

As placas foram fabricadas artesanalmente por meio de transferência térmica, corrosão química, furação e soldagem.

As pulseiras esquerda e direita utilizam o mesmo projeto de placa.

---

## Estrutura mecânica

Os invólucros foram modelados no **SolidWorks** e fabricados por impressão 3D em **PETG**.

Foram desenvolvidos:

- invólucro do módulo principal;
- tampa do módulo principal;
- invólucro das pulseiras;
- tampas das pulseiras;
- invólucro da base de carregamento;
- tampa da base de carregamento;
- suportes auxiliares dos sensores.

O módulo principal é fixado a uma cinta de postura comercial utilizada como suporte vestível.

---

## Limitações e finalidade do projeto

Este projeto corresponde a um **protótipo acadêmico experimental** desenvolvido para avaliar a viabilidade funcional da arquitetura proposta.

O dispositivo não constitui um produto médico certificado e não deve ser considerado substituto para recursos tradicionais de orientação e mobilidade, como a bengala branca.

Os resultados, limitações e possibilidades de melhoria são discutidos no Trabalho de Conclusão de Curso associado a este repositório.

---

## Versão associada ao TCC

A versão do código correspondente à implementação apresentada no Trabalho de Conclusão de Curso está registrada no commit:

bd11a36791bfb6d69456bc6ffdbd0aebd00a61dc

Esse commit constitui a referência da versão do software utilizada nos testes e resultados apresentados na monografia.

---

## Autor

**Gustavo Pedro Feliciano**  
Engenharia de Controle e Automação  
Universidade Federal de Minas Gerais — UFMG

---

## Trabalho acadêmico

**Título:** Dispositivo Vestível para Auxílio à Locomoção de Pessoas com Deficiência Visual  
**Instituição:** Universidade Federal de Minas Gerais — UFMG  
**Curso:** Engenharia de Controle e Automação  
**Ano:** 2026
