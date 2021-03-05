// Arquivo: balanca_IoT_FSM
// Autor: Luiz C M Oliveira
// Descricao: Versao inicial da balancaIoT modelada por FSM - máquina de estados.
//            - verifica conexao com a nuvem e informa em caso de problema
//            - verifica publicacao dos dados  e informa em caso de problema
//            - implementa o modo sleep
//            - implementa o log via monitor serial
//            
// Atualização - 04.03.2020: 
//

#include "Particle.h"
#include <LiquidCrystal_I2C_Spark.h>
#include <HX711ADC.h>

//Display LCD
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
bool nova_medida = false;           //Flag que indica que houve alteração na medida

int startTime = 0;

String Data;

SYSTEM_THREAD(ENABLED);
SYSTEM_MODE(SEMI_AUTOMATIC);

Serial1LogHandler logHandler(115200);
// SerialLogHandler logHandler;

const std::chrono::milliseconds connectMaxTime = 6min;
const std::chrono::seconds sleepTime = 1min;
const std::chrono::milliseconds publishMaxTime = 3min;

// These are the states in the finite state machine, handled in loop()
enum Estado {
    ESTADO_INIT = 0,
    ESTADO_EM_OPERACAO,
    ESTADO_PUBLICANDO,
    ESTADO_SLEEP,
    ESTADO_RESET_SCALE
};

// Global variables
Estado estado = ESTADO_INIT;
unsigned long stateTime;

// Sinais
bool sinal_conectado = false;

//*** Funções ***

// A funcao publishFuture é utilizada para descobrir quando a publicacão é completada, assincronamente
particle::Future<bool> publishFuture;

// inicializa display
int init_display();

// inicializa balança
int init_scale();

int config_display();

int msg_inicial();

int montar_json();

int atualiza_display(String valor);

bool check_med(double med, double med_ant);

int salva_medida_EEPROM(double med);

double le_medida_EEPROM();

// A buffer to hold the JSON data we publish.
char publishData[256];

void setup() {
    medida_anterior = le_medida_EEPROM();
    init_display();
    msg_inicial();
    init_scale();
    lcd->clear();
    lcd->setCursor(2,0);
    lcd->print("Conectando...");
    Particle.connect();
    Time.zone(-3.00);        // Ajusta horário conforme horário brasileiro - GMT-3h
    stateTime = millis();
    estado = ESTADO_RESET_SCALE;
}

void loop() {
    switch(estado) {
        case ESTADO_INIT:
            // Wait for the connection to the Particle cloud to complete
            if (Particle.connected()) {
                Log.info("conectado a nuvem em %lu ms", millis() - stateTime);
                lcd->clear();
                lcd->setCursor(3,0);
                lcd->print("Conectado!");
                delay(2000);
                lcd->clear();
                config_display();        //display peso ok
                estado = ESTADO_EM_OPERACAO; 
            }
            else
            if (millis() - stateTime >= connectMaxTime.count()) {
                // Took too long to connect, go to sleep
                Log.info("falha de conexao, indo dormir");
                lcd->clear();
                lcd->setCursor(0,0);
                lcd->print("Falha na conexao!");
                delay(2000);
                estado = ESTADO_SLEEP;
            }
            break;

        case ESTADO_EM_OPERACAO:
           
             medida_anterior = le_medida_EEPROM();
             atualiza_display(String(medida_anterior,2));

             medida = (scale.get_units(10));
             if(medida<0){medida=0;}                        //condiciona medidas negativas
             
             nova_medida = check_med(medida, medida_anterior);
             
            if(nova_medida==true)
             {
                atualiza_display(String(medida,2));
                salva_medida_EEPROM(medida);
                estado = ESTADO_PUBLICANDO;
             }
             else
             {
                estado = ESTADO_EM_OPERACAO;
             }
             
            break;

        case ESTADO_PUBLICANDO: 
            
            montar_json();

            Log.info("about to publish %s", publishData);

            publishFuture = Particle.publish("medidas2", Data, PRIVATE | WITH_ACK);
            stateTime = millis();
                    
            // When checking the future, the isDone() indicates that the future has been resolved, 
            // basically this means that Particle.publish would have returned.
            if (publishFuture.isDone()) {
                // isSucceeded() is whether the publish succeeded or not, which is basically the
                // boolean return value from Particle.publish.
                if (publishFuture.isSucceeded()) {
                    Log.info("publicacao ok %s", publishData);
                    estado = ESTADO_EM_OPERACAO;
                    delay(1000); //teste - para evitar excesso de publicacoes na nuvem
                }
                else {
                    Log.info("falha na publicacao, medida perdida");
                    estado = ESTADO_SLEEP;
                }
            }
            
            break;

        case ESTADO_SLEEP:

            Log.info("going to sleep for %ld seconds", (long) sleepTime.count());

            {            
                // This is the equivalent to:
                // System.sleep(WKP, RISING, SLEEP_NETWORK_STANDBY);

                SystemSleepConfiguration config;
                config.mode(SystemSleepMode::STOP)
                    .gpio(WKP, RISING)
                    .duration(sleepTime)
                    .network(NETWORK_INTERFACE_CELLULAR);

                SystemSleepResult result = System.sleep(config);
            }

            Log.info("woke from sleep");

            estado = ESTADO_EM_OPERACAO; //correto voltar para o init
            stateTime = millis();
            break; 

        case ESTADO_RESET_SCALE:
            medida = 0;
            medida_anterior = 0;
            salva_medida_EEPROM(medida);
            lcd->clear();
            lcd->setCursor(3,0);
            lcd->print("Esvaziando");
            lcd->setCursor(5,1);
            lcd->print("a lixeira");
            delay(3000);
            estado = ESTADO_INIT;
         break;
    }
}

// **** Implementação das funções ****

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
    lcd->clear();
    lcd->setCursor(3 ,0 );
    lcd->print("Executando");
    lcd->setCursor(5 ,1 );
    lcd->print("tara...");
    // aguarda 3s
    startTime = millis();
          while(true){
            if(millis()-startTime >= 3000){
                break;
            }
        }
    lcd->clear();
    return 0;
}

int config_display()
{
    lcd->clear();
    lcd->setCursor(6 ,0 );
    lcd->print("Peso");
    lcd->setCursor(4 ,1 );
    lcd->print(String(medida_anterior,2));
    lcd->setCursor(10 ,1);
    lcd->print("kg");
    return 0;
}

int msg_inicial()
{
    lcd->clear();
    lcd->setCursor(5,0);
    lcd->print("GEPIC");
    lcd->setCursor(1,1);
    lcd->print("IoT Scale 1.0");
    // aguarda 5s
    startTime = millis();
          while(true){
            if(millis()-startTime >= 5000){
                break;
            }
        }
    return 0;
}

int montar_json(){
  
    String timeStamp = Time.timeStr();
    timeStamp = Time.format("%d/%m/%y %H:%M:%S");	

	String scaleID = System.deviceID();
	
    Data="{\"scaleID\":\""+ scaleID +"\",";              //iD_balanca
           Data+="\"medidaEm\":\""+String(timeStamp)+"\","; 	//Data da medicao
           Data+="\"peso\":\""+String(medida,2)+"\"}";			//valor da medida    
                        
 return 0;
	//Particle.publish("medidas", Data, PUBLIC);   
}

bool check_med(double med, double med_ant){
    int delta = 0.2; //variação detectável de peso 200g 

    // verifica se nova medida não foi pela compressão do lixo - aguarda 5s
    delay(5000);

    if((med > med_ant + delta)){// se houve aumento do peso - nova medida
        return true;
        }
        else{
            return false;
        }
}

int salva_medida_EEPROM(double med)
{
    int addr = 10;
    //double valor;
    EEPROM.put(addr, med);
    return 1;
}

double le_medida_EEPROM()
{
    int addr = 10;
    double medida;
    EEPROM.get(addr, medida);
    if(medida == 0xFFFF) {
    // EEPROM was empty -> initialize value
        medida = 0;
    }
    return medida;
}