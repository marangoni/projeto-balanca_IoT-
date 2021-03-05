/******************************************************/
//       THIS IS A GENERATED FILE - DO NOT EDIT       //
/******************************************************/

#line 1 "c:/dev/balancaIoT/balancaIoT_FSM/src/balancaIoT_FSM.ino"
// Arquivo: balanca_IoT_FSM
// Autor: Luiz C M Oliveira
// Descricao: Versao inicial da balancaIoT modelada por FSM.
//            - verifica conexao com a nuvem e informa em caso de problema
//            - verifica publicacao dos dados  e informa em caso de problema
//            - implementa o modo sleep
//            - implementa o log via monitor serial
//            
// Atualização - 26.12.2020: 
//

#include "Particle.h"
#include <LiquidCrystal_I2C_Spark.h>
#include <HX711ADC.h>

//Display LCD
void setup();
void loop();
int atualiza_display(String valor);
#line 17 "c:/dev/balancaIoT/balancaIoT_FSM/src/balancaIoT_FSM.ino"
LiquidCrystal_I2C *lcd;

//HX711 Pinagem do condicionador de sinais
#define SCK A3
#define DT A2

HX711ADC scale(DT,SCK);

//fator de calibração     
float calibration_factor = 17130;

double medida = 0;
double medida_anterior = 0;
bool atualizar_Medida = true;
bool timerFlag = true;              //Flag indica que nova medida pode ser feita
bool pubFlag = true;                //Flag que indica que foi feita uma publicaçao

int startTime = 0;

SYSTEM_THREAD(ENABLED);
SYSTEM_MODE(SEMI_AUTOMATIC);

// Depuracao utilizando o conversor serial para USB FT232 ligado ao particle photon e ao PC
// para que não ocorra a perda da conexão e logs quando a balanca entrar no modo sleep.
// Se não tiver um modulo Ft232 disponivel simplemente comente a linha Serial1LogHandler e
// descomente a SerialLogHandler.
Serial1LogHandler logHandler(9600);
//SerialLogHandler logHandler;

// Tempo máximo de espera pela conexão à nuvem. Deve ser de, pelo menos, 5 min.
const std::chrono::milliseconds connectMaxTime = 6min;

// Tempo em modo sleep
const std::chrono::seconds sleepTime = 1min;

// Tempo máximo para que a publicação do evento - Particle.publish seja completa.Tempo normal típico 20s 
// podendo chegar a 5min, se houver problemas de comunicação. Após este tempo a medida será perdida.
const std::chrono::milliseconds publishMaxTime = 3min;

// Estados da FSM manipulados no loop()
enum Estado {
    ESTADO_INIT = 0,
    ESTADO_CONECTADO,
    ESTADO_DESCONECTADO,
    ESTADO_EM_OPERACAO,
    ESTADO_CHECK_SENSOR,
    ESTADO_PUBLICA,
    ESTADO_PUBLICA_OK,
    ESTADO_AGUARDANDO_CONEXAO,
    ESTADO_MSG,
    ESTADO_SLEEP
};

// Variáveis Globais
Estado estado = ESTADO_INIT;
unsigned long stateTime;

// Sinais
bool sinal_conectado = false;


//*** Funções ***
// Funcao para tara da balanca pela nuvem
int tareScale(String command);

// A funcao publishFuture é utilizada para descobrir quando a publicacão é completada, assincronamente
particle::Future<bool> publishFuture;

// Cria uma função para fazer a tara da balança via pela particle Cloud
bool success = Particle.function("tareScale", tareScale);        

// inicializa display
int init_display();

// inicializa balança
int init_scale();

int config_display();

int msg_inicial();

// A buffer to hold the JSON data we publish.
char publishData[256];

void setup() {
    Particle.connect();
    stateTime = millis();

    Serial.begin(9600);     // inicializa depuração via monitor serial                          
        
    init_display();     // inicializa LCD
    
    init_scale();       // inicializa balanca

    // Funções na nuvem
    Time.zone(-3.00);                                 // Ajusta horário conforme horário brasileiro - GMT-3h
          
    
    
  
    lcd->clear();
    lcd->setCursor(6 ,0 );
    lcd->print("Peso");
    lcd->setCursor(4 ,1 );
    lcd->print(String(medida,2));
    lcd->setCursor(10 ,1);
    lcd->print("kg");
}

void loop() {
    switch(estado) {
        case ESTADO_INIT:
            // Tara da balanca
             scale.tare();
            
            // Aguarda conexao do Particle a nuvem 
            if (Particle.connected()) {
                // saida Log.info no monitor serial - utilizar putty ou outro para verificar
                Log.info("conectado a nuvem em %lu ms", millis() - stateTime);
                estado = ESTADO_EM_OPERACAO; 
            }
            else
            if (millis() - stateTime >= connectMaxTime.count()) {
                // Se demorar mais que o tempo máximo previsto, entre em modo SLEEP
                Log.info("falha de conexao, entrando no modo SLEEP");
                estado = ESTADO_SLEEP;
            }
            break;

        case ESTADO_EM_OPERACAO:
            //MSG("Conectado a rede wifi"); 
                        
            // // Se mostrou no display - proximo estado
            // if (sinal_msg_ok==true) {
            //     estado = ESTADO_CHECK_SENSOR; 
            // }
            // else { // se nao mostrou aguarda
            //     estado = ESTADO_CONECTADO;
            // }
            break;
        
        case ESTADO_PUBLICA: 
            // {
            //     // This is just a placeholder for code that you're write for your actual situation
            //     // Simulacao de medida realizada.
            //     int a0 = analogRead(A0);

            //     // Cria um JSON simples com o valor de A0
            //     snprintf(publishData, sizeof(publishData), "{\"a0\":%d}", a0);
            // }
            
            // Log.info("leitura do sensor %s", publishData);

            // publishFuture = Particle.publish("sensorTest", publishData, PRIVATE | WITH_ACK);
            // state = STATE_PUBLISH_WAIT;
            // stateTime = millis();
        break;     
        
        case ESTADO_SLEEP:

            // Log.info("entrando no modo SLEEP por %ld seconds", (long) sleepTime.count());

            // {            
            //       SystemSleepResult result = System.sleep(SLEEP_MODE_DEEP, sleepTime);
            // }

            // Log.info("acordando do modo sleep");

            // state = STATE_WAIT_CONNECTED;
            // stateTime = millis();
            break; 
    }
}


// **** Implementação das funções ****

// Funcao na nuvem para executar a tara da balança
int tareScale(String command)
{
  scale.tare();
  delay(100);
  //Serial.println("Tara da balanca pela nuvem!!");
  //DEBUG_PRINT("Tara da balanca");
   lcd->clear();
   lcd->setCursor(0 ,0 );
   lcd->print("Tara da balanca");
   lcd->setCursor(0 ,1 );
   lcd->print(" pela nuvem!");
   
    int startTime = millis();
        while(true){
        if(millis()-startTime >= 200){
      break;
        }
    }
   atualiza_display(String(medida,2));
 return 1;
}

// funcao atualiza medida no display
int atualiza_display(String valor)
{
    lcd->clear();
    lcd->setCursor(6 ,0 );
    lcd->print("Peso");
    lcd->setCursor(4 ,1 );
    lcd->print(valor);
    lcd->setCursor(10 ,1);
    lcd->print("kg");

    return 0;
}

int init_display()
{
    lcd = new LiquidCrystal_I2C(0x27, 16, 2);
    lcd->init();
    lcd->backlight();
    lcd->clear();

    return 0;
}

int init_scale()
{
    scale.begin();                            // inicializa balanca
    scale.set_scale(calibration_factor);      // ajusta fator de escala
    //Executa a tara da balanca
    scale.tare();
    lcd->setCursor(0 ,0 );
    lcd->print("Tara da balanca...");
    // aguarda 3s
    startTime = millis();
          while(true){
            if(millis()-startTime >= 3000){
                break;
            }
        }
    return 0;
}

int config_display()
{
    lcd->clear();
    lcd->setCursor(6 ,0 );
    lcd->print("Peso");
    lcd->setCursor(4 ,1 );
    lcd->print(String(medida,2));
    lcd->setCursor(10 ,1);
    lcd->print("kg");
    return 0;
}

int msg_inicial()
{
    lcd->setCursor(1,0);
    lcd->print("GEPIC");
    lcd->setCursor(5,0);
    lcd->print("IoT Scale 1.0");
    delay(5000);
    config_display();
    return 0;
}