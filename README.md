# Dispositivo Vestível para Auxílio à Locomoção de Pessoas com Deficiência Visual

Repositório desenvolvido como material complementar ao Trabalho de Conclusão de Curso **“Dispositivo Vestível para Auxílio à Locomoção de Pessoas com Deficiência Visual”**, do curso de Engenharia de Controle e Automação da Universidade Federal de Minas Gerais (UFMG).

O projeto consiste em um dispositivo vestível de baixo custo destinado à detecção de obstáculos em diferentes regiões do trajeto de pessoas com deficiência visual. O sistema utiliza sensores ultrassônicos, processamento embarcado, comunicação sem fio e feedback vibrotátil para fornecer informações relacionadas à posição aproximada dos obstáculos.

---

## Visão geral do sistema

O protótipo é composto por três unidades vestíveis independentes:

- um **módulo principal**, responsável pelo sensoriamento e processamento;
- uma **pulseira vibrotátil esquerda**;
- uma **pulseira vibrotátil direita**.

Além dessas unidades, foi desenvolvida uma **base externa de carregamento** para recarga das baterias.

A comunicação entre o módulo principal e as pulseiras é realizada por meio do protocolo **ESP-NOW**, sem necessidade de roteador ou conexão com a Internet.

### Arquitetura simplificada

                 +----------------------+
                 |    Módulo Principal  |
                 |      ESP32-S3        |
                 +----------+-----------+
                            |
        +-------------------+-------------------+
        |                   |                   |
   HC-SR04 x4            Buzzer             ESP-NOW
                                                |
                              +-----------------+-----------------+
                              |                                   |
                    +---------+---------+               +---------+---------+
                    | Pulseira Esquerda |               | Pulseira Direita  |
                    |     ESP32-C3      |               |     ESP32-C3      |
                    +---------+---------+               +---------+---------+
                              |                                   |
                         Vibracall                           Vibracall

Hardware utilizado

Módulo principal

ESP32-S3 Super Mini;

4 sensores ultrassônicos HC-SR04;

buzzer ativo;

bateria Li-Po 3,7 V / 800 mAh;

conversor elevador MT3608;

capacitor eletrolítico de 1000 µF;

capacitor cerâmico de 100 nF;

divisores resistivos para adequação dos sinais ECHO;

placa de circuito impresso desenvolvida especificamente para o protótipo.

Pulseiras vibrotáteis

Cada pulseira utiliza:

ESP32-C3 Super Mini;

módulo motor de vibração;

bateria Li-Po 3,7 V / 800 mAh;

conversor elevador MT3608;

capacitor eletrolítico de 470 µF;

capacitor cerâmico de 100 nF;

placa de circuito impresso dedicada.

Base de carregamento

módulo carregador baseado no TP4056;

alimentação externa de 5 V;

interface dedicada para carregamento das três unidades do protótipo.

Atenção: os conectores USB Tipo C presentes nas unidades vestíveis são utilizados como interface física dedicada de carregamento e não devem ser tratados como entradas USB Tipo C convencionais de 5 V.

Distribuição dos sensores

Os quatro sensores ultrassônicos são organizados em quatro regiões:

Região

TRIG

ECHO

Superior direita

GPIO 11

GPIO 10

Inferior direita

GPIO 9

GPIO 8

Superior esquerda

GPIO 4

GPIO 5

Inferior esquerda

GPIO 6

GPIO 7

O buzzer do módulo principal é conectado ao GPIO 1.

Nas pulseiras, o módulo motor de vibração é controlado pelo GPIO 4 do ESP32-C3.

Estratégia de detecção

Os sensores superiores e inferiores possuem orientações diferentes no protótipo.

Foram definidos dois níveis principais de distância:

Posição do sensor

Alerta vibrotátil

Condição crítica

Superior

140 cm

70 cm

Inferior

205 cm

166 cm

Os valores dos sensores inferiores consideram a geometria de instalação dos sensores, posicionados aproximadamente a 1,50 m de altura.

Na versão final do software, não é realizada correção geométrica das distâncias durante a execução. A geometria foi utilizada apenas para determinar os limiares equivalentes dos sensores inferiores.

Tratamento das medições

Para melhorar a estabilidade das leituras ultrassônicas, o software utiliza:

faixa válida de medição entre 2 cm e 250 cm;

aquisição sequencial dos sensores;

intervalo de aproximadamente 40 ms entre leituras;

filtro de mediana de 3 amostras por sensor.

As medições filtradas são utilizadas diretamente pela lógica de detecção.

Estratégia de alertas

O sistema utiliza a posição e o padrão de vibração para representar a região na qual o obstáculo foi detectado.

Condição

Pulseira

Alerta

Obstáculo superior esquerdo

Esquerda

Vibração contínua

Obstáculo inferior esquerdo

Esquerda

Vibração pulsante

Obstáculo superior direito

Direita

Vibração contínua

Obstáculo inferior direito

Direita

Vibração pulsante

Superior + inferior no mesmo lado

Correspondente

Vibração contínua

Nenhum obstáculo

Correspondente

Desligado

O padrão pulsante utiliza aproximadamente:

200 ms ligado
200 ms desligado

Quando um obstáculo é detectado dentro da faixa crítica, o buzzer do módulo principal também é acionado.

Comunicação ESP-NOW

O ESP32-S3 atua como transmissor e os dois ESP32-C3 atuam como receptores.

Cada pulseira possui um endereço MAC próprio, permitindo que o módulo principal envie comandos independentemente para os lados esquerdo e direito.

Endereços utilizados no protótipo

Pulseira esquerda: B8:1F:3F:FC:38:14
Pulseira direita:  B8:1F:3F:FC:36:40

A mensagem enviada entre os dispositivos possui a estrutura:

struct DataMessage {
    uint8_t estado;
    uint32_t sequencia;
};

Os estados utilizados são:

DESLIGADO = 0
CONTINUO  = 1
PULSANTE  = 2

O campo sequencia é utilizado para acompanhar a ordem das mensagens durante os testes de comunicação.

Segurança em caso de perda de comunicação

As pulseiras possuem um mecanismo de segurança para evitar que um comando antigo permaneça acionando o motor caso a comunicação seja interrompida.

Se nenhuma mensagem válida for recebida por aproximadamente:

2000 ms

o motor de vibração é automaticamente desligado.

O funcionamento normal é restabelecido quando uma nova mensagem válida é recebida.

Estrutura do repositório

A organização sugerida para os arquivos é:

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

ModuloPrincipal_Final.ino

Responsável por:

leitura dos quatro sensores HC-SR04;

validação das distâncias;

filtro de mediana;

classificação dos obstáculos;

definição dos estados das pulseiras;

acionamento do buzzer;

transmissão dos comandos por ESP-NOW.

PulseiraEsquerda_Final.ino

Responsável por:

recepção dos comandos destinados à pulseira esquerda;

controle do motor de vibração;

geração dos estados contínuo e pulsante;

implementação do mecanismo de timeout.

PulseiraDireita_Final.ino

Possui a mesma lógica da pulseira esquerda, utilizando o endereço correspondente à unidade direita.

MACadress.ino

Código auxiliar utilizado para consultar o endereço MAC dos microcontroladores ESP32 durante a configuração da comunicação ESP-NOW.

Ambiente de desenvolvimento

Os códigos foram desenvolvidos utilizando:

Arduino IDE;

suporte para microcontroladores ESP32;

ESP32-S3 Super Mini;

ESP32-C3 Super Mini.

As principais funcionalidades de comunicação utilizam os recursos de Wi-Fi e ESP-NOW disponibilizados pelo ecossistema ESP32.

Execução

1. Identificação dos endereços MAC

Antes da configuração do sistema, o endereço MAC de cada pulseira pode ser obtido utilizando:

Utilitarios/MACadress.ino

Os endereços obtidos devem ser configurados no código do módulo principal.

2. Gravação das pulseiras

Carregar os respectivos códigos:

PulseiraEsquerda/PulseiraEsquerda_Final.ino
PulseiraDireita/PulseiraDireita_Final.ino

nos ESP32-C3 correspondentes.

3. Gravação do módulo principal

Carregar:

ModuloPrincipal/ModuloPrincipal_Final.ino

no ESP32-S3.

4. Inicialização

Após a alimentação dos três módulos:

o módulo principal inicia a aquisição dos sensores;

as distâncias são filtradas e classificadas;

os estados das pulseiras são determinados;

os comandos são transmitidos via ESP-NOW;

cada pulseira gera o padrão de vibração correspondente;

condições críticas também provocam o acionamento do buzzer.

Alimentação

Cada unidade vestível possui bateria independente de:

3,7 V
800 mAh
1 célula (1S)

A tensão é elevada para aproximadamente 5 V por um conversor MT3608.

O carregamento das três baterias é realizado individualmente por meio da base externa desenvolvida para o projeto.

Durante o carregamento, recomenda-se manter a respectiva unidade desligada.

Placas de circuito impresso

Foram desenvolvidas placas específicas para:

módulo principal;

pulseiras vibrotáteis;

base de carregamento.

Os esquemáticos e layouts foram elaborados no EasyEDA e as placas foram fabricadas artesanalmente por transferência térmica, corrosão química, furação e soldagem.

As duas pulseiras utilizam o mesmo projeto de placa.

Estrutura mecânica

Os invólucros do protótipo foram modelados no SolidWorks e fabricados por impressão 3D em PETG.

Foram desenvolvidos:

invólucro do módulo principal;

tampa do módulo principal;

invólucro das pulseiras;

tampas das pulseiras;

invólucro da base de carregamento;

tampa da base de carregamento;

suportes auxiliares para os sensores.

O módulo principal é fixado a uma cinta de postura comercial utilizada como suporte vestível.

Observações

Este projeto corresponde a um protótipo acadêmico experimental desenvolvido para avaliação da viabilidade funcional da arquitetura proposta.

O dispositivo não deve ser considerado um substituto para recursos tradicionais de orientação e mobilidade, como a bengala branca, nem um produto médico ou comercial certificado.

Os resultados, limitações e possibilidades de melhoria são discutidos detalhadamente no Trabalho de Conclusão de Curso associado a este repositório.

Versão associada ao TCC

A versão do código correspondente à implementação apresentada na monografia deve ser identificada por meio da seguinte tag/release:

v1.0-tcc

Essa versão deve permanecer inalterada para garantir a rastreabilidade entre o código disponibilizado neste repositório e os resultados documentados no trabalho.

Autor

Gustavo Pedro Feliciano
Engenharia de Controle e Automação
Universidade Federal de Minas Gerais — UFMG

Trabalho acadêmico

Título: Dispositivo Vestível para Auxílio à Locomoção de Pessoas com Deficiência Visual

Instituição: Universidade Federal de Minas Gerais — UFMG
Curso: Engenharia de Controle e Automação
Ano: 2026


Eu manteria essa versão relativamente completa porque, ao retirar os códigos da monografia, o **GitHub passa a ser a documentação principal da implementação**. No Apêndice A, aí sim deixamos apenas uma página com o link/QR Code do repositório e a indicação da release `v1.0-tcc`.
