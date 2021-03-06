// Arquivo: balanca_IoT_FSM
// Autor: Luiz C M Oliveira
// Descricao: Versao funcional da balancaIoT modelada por FSM - máquina de estados.
//            - Modelada conforme FSM (parcial - precisa ainda ser melhorado)
//            - Salva e recupera valores medidos da eeprom (se desligar guarda os valores)
//            - Rotina de verificação de novas medidas ou ruido implementada -check_medida
//            - Rotinas de exibicao de dados no display aprimoradas (precisam melhorias - retirar delay)
//            - implementa Estado de reset da lixeira (está inicialmente ativada - lixeira inicia zerada)
//            - Ajustada temporização para uma unica publicação na nuvem e base de dados - ESTADO_PUBLICA_EVENTO
//
// Atualização - 05.03.2020: 
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

const unsigned long PUBLISH_PERIOD_MS = 60000;  //periodo entre publicações 60s
const unsigned long FIRST_PUBLISH_MS = 5000;    //primeira publicacao com 5s
unsigned long lastPublish = FIRST_PUBLISH_MS - PUBLISH_PERIOD_MS;

double medida = 0;
double medida_anterior = 0;
bool nova_medida = false;           //Flag que indica que houve alteração na medida
int startTime = 0;

String Data;

SYSTEM_THREAD(ENABLED);
SYSTEM_MODE(SEMI_AUTOMATIC);

//Utilizar conversor serial->USB FT232 para visualizar logs
Serial1LogHandler logHandler(115200);

const std::chrono::milliseconds connectMaxTime = 6min;
const std::chrono::seconds sleepTime = 1min;
const std::chrono::milliseconds publishMaxTime = 3min;

// These are the states in the finite state machine, handled in loop()
enum Estado {
    ESTADO_INIT = 0,
    ESTADO_EM_OPERACAO,
    ESTADO_PUBLICA_EVENTO,
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
int init_display();                         // inicializa display
int init_scale();                           // inicializa balança
int config_display();                       // configuraçao do display LCD - peso e kg
int msg_inicial();                          // abertura do programa 
int montar_json();                          // montagem do json para publicacao
int atualiza_display(String valor);         // atualizaçao da medida no display
bool check_med(double med, double med_ant); // verificacao de nova medida
int salva_medida_EEPROM(double med);        // salva nova medida na eeprom
double le_medida_EEPROM();                  // le medida na eeprom

void setup() {
    Log.info("Executando void setup()...");
    Serial.begin(9600);
    medida_anterior = le_medida_EEPROM();
    init_display();
    msg_inicial();
    init_scale();
    lcd->clear();
    lcd->setCursor(2,0);
    lcd->print("Conectando...");
    Particle.connect();      // Conecta particle a nuvem
    Time.zone(-3.00);        // Ajusta horário conforme horário brasileiro - GMT-3h
    stateTime = millis();
    estado = ESTADO_RESET_SCALE;  // NESSA versao a balanca deve iniciar zerada, na versao final reset por evento
}

void loop() {
    switch(estado) {
        case ESTADO_INIT:
            Log.info("Estado init");
            // Espera que a conexao a nuvem do particle seja resolvida
            if (Particle.connected()) {
                Log.info("conectado a nuvem em %lu ms", millis() - stateTime);
                lcd->clear();
                lcd->setCursor(3,0);
                lcd->print("Conectado!");
                delay(2000);                        //remover para não "bloquear o codigo"
                lcd->clear();
                config_display();                   //display peso ok
                estado = ESTADO_EM_OPERACAO;
                Log.info("Indo para estado_em_operacao"); 
            }
            else
            if (millis() - stateTime >= connectMaxTime.count()) {
                // Se demorar muito para conectar exibe mensagem de falha e vai para hibernação (SLEEP)
                Log.info("falha de conexao, indo dormir");
                lcd->clear();
                lcd->setCursor(0,0);
                lcd->print("Falha na conexao!");
                delay(2000);    //remover para não "bloquear o codigo"
                estado = ESTADO_SLEEP;
                Log.info("Indo para estado_sleep"); 
            }
            break;

        case ESTADO_EM_OPERACAO:
             Log.info("ESTADO_EM_OPERACAO");            
             medida_anterior = le_medida_EEPROM();
             atualiza_display(String(medida_anterior,2));

             // Há como melhorar este código... esta bloqueando o programa o tempo todo 
             // para verificar nova_medida (PROXIMAS MELHORIAS)

             medida = (scale.get_units(10));
             if(medida<0){medida=0;}                           //condiciona medidas negativas
             
             nova_medida = check_med(medida, medida_anterior); //verifica se é nova medida ou ruido
             
            if(nova_medida==true)
             {
                Log.info("Nova medida detectada...");  
                atualiza_display(String(medida,2));
                salva_medida_EEPROM(medida);
                estado = ESTADO_PUBLICA_EVENTO;
                Log.info("Indo para estado_em_publicando"); 
             }
             else
             {
                estado = ESTADO_EM_OPERACAO;
                Log.info("Fica no estado em operação"); 
             }
             
            break;

        case ESTADO_PUBLICA_EVENTO: 
            Log.info("ESTADO_PUBLICA_EVENTO"); 
            
            montar_json();

            if (millis() - lastPublish >= PUBLISH_PERIOD_MS) {
		        lastPublish = millis();
		        publishFuture = Particle.publish("nova_medida", Data, PRIVATE | WITH_ACK);
                Log.info("publicando...", Data);
                stateTime = millis();
            }
            // When checking the future, the isDone() indicates that the future has been resolved, 
            // basically this means that Particle.publish would have returned.
            if (publishFuture.isDone()) {
                Log.info("Retorno da funcao publish");
                // isSucceeded() is whether the publish succeeded or not, which is basically the
                // boolean return value from Particle.publish.
                if (publishFuture.isSucceeded()) {
                    Log.info("publicacao ok");
                    //estado = ESTADO_AGUARDE_1S;
                    estado = ESTADO_EM_OPERACAO;
                    Log.info("Indo para estado_em_operacao"); 
                }
                else {
                    Log.info("falha na publicacao, medida perdida");
                    estado = ESTADO_SLEEP;
                    Log.info("Indo para estado_SLEEP"); 
                }
            }
        break;
            
        case ESTADO_SLEEP:
            Log.info("ESTADO_SLEEP"); 
            Log.info("Hibernacao por %ld segundos", (long) sleepTime.count());
            // Estado ainda não está funcionando - deve ser configurado para o photon
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
            Log.info("Indo para estado_em_operacao"); 
            stateTime = millis();
            break; 

        case ESTADO_RESET_SCALE:
            Log.info("EStADO_RESET_SCALE"); 
            medida = 0;
            medida_anterior = 0;
            salva_medida_EEPROM(medida);
            lcd->clear();
            lcd->setCursor(3,0);
            lcd->print("Esvaziando");
            lcd->setCursor(4,1);
            lcd->print("lixeira");
            delay(3000);        //remover para não bloquear o programa
            estado = ESTADO_INIT;
            Log.info("Indo para estado_init"); 
         break;

        //  case ESTADO_AGUARDE_1S:
        //  // nao utilizado
        //  Log.info("ESTADO_AGUARDE_1S"); 
        //     //stateTime = millis();
        //     if(millis()-stateTime>3000){ 
        //         Log.info("Indo para estado_em_operacao"); 
        //         estado = ESTADO_EM_OPERACAO;
        //     }
        //     else{
        //         Log.info("Indo para estado_aguarde_1s"); 
        //         estado = ESTADO_AGUARDE_1S;
        //     }

        //  break;
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