// Arquivo: balanca_IoT_FSM
// Autor: Luiz C M Oliveira
// Descricao: 
// - Utilizacao da rotina - arduinoJson versao 5.13.5 (só funciona com versao 5.*.*)   
// - Implementação da rotina para leitura dos parametros de funcionamento do BD
//    
// Atualização - 17.03.2020: 
//----------------------------------------------------------------------------------

#include "Particle.h"
#include <LiquidCrystal_I2C_Spark.h>
#include <HX711ADC.h>
#include <ArduinoJson.h>

//Display LCD
LiquidCrystal_I2C *lcd;

//HX711 Pinagem do condicionador de sinais
#define SCK A3
#define DT A2

HX711ADC scale(DT,SCK);


///**** Dados de operação da balança - POSTERIORMENTE DEVEM SER RECUPERADOS DO BD da aplicação

//fator de calibração ***** Importante para medidas corretas. Verificar conforme a balanca     
//float calibration_factor = 11210; //balanca 150 kg - balanca_iot2
float calibration_factor = 1132650; //balanca 1kg - GEPIC1

unsigned long PUBLISH_PERIOD_MS = 60*60000;  //periodo entre publicações 60 * 60s = 1hora
unsigned long FIRST_PUBLISH_MS = 5000;    //primeira publicacao com 5s

unsigned long PERIODO_MEDIDA_MS = 60*60000;  //periodo entre medidas 5s   //versao final ajustar este valor para um intervalo maior
unsigned long PRIMEIRA_MEDIDA = 500;     //primeira medida 0,5s

unsigned long PERIODO_ULTIMO_RESET_MS = 8*60*60000;  //periodo verificacao RESET - A cada 8 *60 *60000 = 8h 
unsigned long PRIMEIRO_RESET = 500;             //primeiro reset 0,5s
//********************************************************************************************

unsigned long lastPublish = FIRST_PUBLISH_MS - PUBLISH_PERIOD_MS;
unsigned long lastMed = PRIMEIRA_MEDIDA - PERIODO_MEDIDA_MS;
unsigned long lastRST = PRIMEIRO_RESET - PERIODO_ULTIMO_RESET_MS;

double medida = 0;
double medida_anterior = 0;
bool nova_medida = false;           //Flag que indica que houve alteração na medida
bool reset_da_balanca = false;      //Flag que indica que houve esvaziamento da balanca (RESET)
int startTime = 0;
int count = 0;                     

//Dados para identificação da balanca no BD
String deviceName;

String Data;
String Data_reset;  
String Data_load;

SYSTEM_THREAD(ENABLED);
SYSTEM_MODE(SEMI_AUTOMATIC);

//***IMPORTANTE PARA DEPURACAO **** - Ajuste as linhas abaixo conforme dispositivo em teste

//Se Utilizar conversor serial->USB FT232 para visualizar logs habilitar esta linha
//Serial1LogHandler logHandler(115200);

//Se nao, habilitar essa
SerialLogHandler logHandler;

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
int montar_json();                          // montagem do json para publicacao
int montar_json_reset();                    // montagem do json de reset da balanca
int atualiza_display(String valor);         // atualizaçao da medida no display
bool check_med(double med, double med_ant); // verificacao de nova medida
int salva_medida_EEPROM(double med);        // salva nova medida na eeprom
double le_medida_EEPROM();                  // le medida na eeprom
void deviceNameHandler(const char *topic, const char *data);        //cria nome do dispositivo para json
bool check_reset(double med, double med_ant); // verifica se houve reset
double get_med(int n_amostras);                           // Funcao para obter e condicionar as medidas da balanca
bool msg(int n_linhas, int tempo_MS, String linha1, String linha2); //Funcao para escrita no display

// Forward declarations
void getDataHandler(const char *topic, const char *data);

const unsigned long CHECK_PERIOD_MS = 60000;
const unsigned long FIRST_CHECK_MS = 5000;
const char *CHECK_EVENT_NAME = "load_variables";


// *** Rotina para tratar indicaçao pelo LED
STARTUP(RGB.mirrorTo(A4, A5, A7));
LEDStatus blinkBlue(RGB_COLOR_BLUE, LED_PATTERN_BLINK, LED_SPEED_NORMAL, LED_PRIORITY_IMPORTANT);
LEDStatus blinkGreen(RGB_COLOR_GREEN, LED_PATTERN_BLINK, LED_SPEED_NORMAL, LED_PRIORITY_IMPORTANT);

void setup() {
    Log.info("Executando void setup()...");
    Serial.begin(9600);
    medida_anterior = le_medida_EEPROM();
    init_display();
    msg(2,5000,"GEPIC","Balanca IoT1.0");
    init_scale();
    msg(1,0,"Conectando...","");
    Particle.connect();      // Conecta particle a nuvem
    Time.zone(-3.00);        // Ajusta horário conforme horário brasileiro - GMT-3h
    stateTime = millis();
    estado = ESTADO_INIT;   //Inicializa a balanca
    
}

void loop() {
    //blinkBlue.setActive(false);
    //blinkGreen.setActive(false);
    
    //gera evento atualizacao parametros
    // Melhorias: Deve ser colocado dentro de um estado INIT ou inicialização
    //          : Ainda gera confusão se houver mais de uma balança, verificar
     
    if (millis() - lastPublish >= PUBLISH_PERIOD_MS) {
	 	    lastPublish = millis();
         	Data_load ="{\"n\":\"" + String(deviceName.c_str())+"\"}";  //nome do dispositivo
            Particle.publish(CHECK_EVENT_NAME, Data_load, PRIVATE);  //obtem parametros do dispositivo da nuvem
	        Log.info(String(calibration_factor));
            scale.set_scale(calibration_factor); //atualiza fator de calibração da balanca (verificar depois)
            Log.info(String(PERIODO_MEDIDA_MS)); 
         }
        
  
    switch(estado) {
           
        case ESTADO_INIT:
            Log.info("Estado init");
            // Espera que a conexao a nuvem do particle seja resolvida
            if (Particle.connected()) {
                Log.info("conectado a nuvem em %lu ms", millis() - stateTime);
                Particle.subscribe("spark/", deviceNameHandler);    
	            Particle.publish("spark/device/name"); //obtem nome do dispositivo na nuvem
                msg(1,2000,"Conectado!","");
                config_display();                   //display peso ok
                Log.info("Obtendo parametros de funcionamento do DB...");
                Particle.subscribe("hook-response/load_variables", getDataHandler, MY_DEVICES);
                estado = ESTADO_EM_OPERACAO;
                Log.info("Indo para estado_em_operacao"); 
            }
            else
            if (millis() - stateTime >= connectMaxTime.count()) {
                // Se demorar muito para conectar exibe mensagem de falha e vai para hibernação (SLEEP)
                Log.info("falha de conexao, indo dormir");
                msg(1,2000,"Falha na conexao","");
                estado = ESTADO_SLEEP;
                Log.info("Indo para estado_sleep"); 
            }
            break;

        case ESTADO_EM_OPERACAO:
             //blinkBlue.setActive(true);
             //delay(3000);
                          
             Log.info("ESTADO_EM_OPERACAO");            
             medida_anterior = le_medida_EEPROM();
             atualiza_display(String(medida_anterior,2));

             // Há como melhorar este código... esta bloqueando o programa o tempo todo 
             // para verificar nova_medida (PROXIMAS MELHORIAS)

              medida = get_med(20);
            
             if(medida<0){medida=0;}                           //condiciona medidas negativas
             
            Log.info("medida " + String(medida));
            Log.info("medida anterior" + String(medida_anterior));
             
             
             if (millis() - lastMed >= PERIODO_MEDIDA_MS) {   //a cada 5s verifica se é nova medida ou nao e se zerou ou nao
		            lastMed = millis();
		            Log.info("Verifica se eh nova medida ou nao");
                    nova_medida = check_med(medida, medida_anterior);
                    if(medida ==0){ //Se medida for zero, repita 3x, se continuar zero - atualize eeprom
                        count++;
                        if (count ==3){
                            Log.info("Medida zerada na EEPROM");
                            salva_medida_EEPROM(medida);
                            count = 0; 
                        }
                    }
             }      

             if (millis() - lastRST >= PERIODO_ULTIMO_RESET_MS) {   //a cada 60s verifica se houve reset ou nao
                    lastRST = millis();
		            Log.info("Verificando se houve reset...");
                    reset_da_balanca = check_reset(medida, medida_anterior);    //sinaliza se houve ou nao reset
            }     
            
            if((nova_medida==true) && (medida!=0)) //somente publica se a medida for nova e DIFERENTE DE ZERO
             {
                Log.info("Nova medida detectada...");  
                atualiza_display(String(medida,2));
                salva_medida_EEPROM(medida);
                estado = ESTADO_PUBLICA_EVENTO;
                Log.info("Indo para estado_publica_evento"); 
             }
             else if(reset_da_balanca == true)  //se ocorreu o RESET (esvaziamento) da lixeira, vá para o estado reset
             {
                reset_da_balanca = false;       //consome o flag
                Log.info("Reset da balanca");
                estado = ESTADO_RESET_SCALE;
             }
             else                               //se nada ocorreu, continua em operacao
             {
                estado = ESTADO_EM_OPERACAO;
                Log.info("Fica no estado em operação"); 
             }
  
        break;

        case ESTADO_PUBLICA_EVENTO: 
            Log.info("ESTADO_PUBLICA_EVENTO"); 
            
            montar_json();
            Log.info(Data);

            if (millis() - lastPublish >= PUBLISH_PERIOD_MS) {
		        lastPublish = millis();
		        if (deviceName.length() > 0) {
                    
                    publishFuture = Particle.publish("nova_medida", Data, PRIVATE | WITH_ACK);
                    Log.info("publicando...", Data);
                }
                stateTime = millis();
            }
            
            if (publishFuture.isDone()) {               //Se houve resposta ao Particle.publish executa abaixo, se não, sai
                Log.info("Retorno da funcao publish");
                
                if (publishFuture.isSucceeded()) {      //Se a publicacao foi bem sucedida
                    Log.info("publicacao ok");
                    //blinkGreen.setActive(true);
                    estado = ESTADO_EM_OPERACAO;
                    Log.info("Indo para estado_em_operacao"); 
                }
                else {                                  //se a publicação nao foi bem sucedida
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
            Log.info("ESTADO_RESET_SCALE"); 
            medida = 0;
            medida_anterior = 0;
            salva_medida_EEPROM(medida);
            msg(2,3000,"Esvaziando","lixeira");
            
            //montajson e publica evento reset           
            montar_json_reset();    //rotina deve ser verificada pois apaga os demais campos da tabela status
            
            if (millis() - lastPublish >= PUBLISH_PERIOD_MS) {
		         lastPublish = millis();
		         if (deviceName.length() > 0) {
                     publishFuture = Particle.publish("novo_reset", Data_reset, PRIVATE | WITH_ACK);
                     Log.info("publicando evento reset...", Data_reset);
                 }
            }
            
            estado = ESTADO_EM_OPERACAO;
            Log.info("Indo para estado_em_operacao"); 
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
    msg(2,3000,"Executando","tara...");
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

int montar_json(){
  
    String timeStamp = Time.timeStr();
    timeStamp = Time.format("%d/%m/%y %H:%M:%S");	

	String scaleID = System.deviceID();
	
    Data="{\"scaleID\":\""+ scaleID +"\",";                     //iD_balanca
    Data+="\"medidaEm\":\""+String(timeStamp)+"\","; 	//Data da medicao
    Data+="\"peso\":\""+String(medida,2)+"\",";         //valor da medida
    Data+="\"n\":\"" + String(deviceName.c_str())+"\"}";			//nome do dispositivo    
    
 return 0;
	
}

int montar_json_reset(){
    // Monta o json com a informacao de data do último reset para publicacao no evento - novo_reset
    // Nesta versao está sobreescrevendo os campos do BD - verificar webhook com o PUT

    String timeStamp = Time.timeStr();
    timeStamp = Time.format("%d/%m/%y %H:%M:%S");
  
    Data_reset="{\"ultimoReset\":\""+ String(timeStamp) +"\",";              //ultimoReset
    Data_reset+="\"n\":\"" + String(deviceName.c_str())+"\"}";               //nome do dispositivo
            
 return 0;
	  
}

bool check_med(double med, double med_ant){
    int delta = 0.2; //variação detectável de peso 200g 

    if((med > med_ant + delta)||(med==0)){// se houve aumento do peso - nova medida OU medida zerou
        return true;
        }
        else{
            return false;
        }
}

bool check_reset(double med, double med_ant){
Log.info("check_reset");
if(med<0.001){med=0;}
    if(med==0 && med_ant==0){
        delay(3000);
        //med = get_med(); //Após 3s verifica balanca novamente
        med = get_med(20);
        if(med<0.001){med=0;}
        Log.info("check_reset");
        Log.info(String(med,2));
        if(med==0){
            Log.info("reset detectado na check_reset");
            return true; //se após 5s a medida ainda for 0. RESET
        }
        else{
            Log.info("reset não detectado na check_reset");
            return false; //se não. Não é reset
        }
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

void deviceNameHandler(const char *topic, const char *data) {
	deviceName = data;
}

double get_med(int no_amostras){
    double med = 0;
    double resultado = 0;
    int count=0;
        
    while(count < no_amostras){
        med = med + scale.get_units(3);
        count++;
        delay(10); //tempo entre medidas 
     }
    resultado = med/no_amostras;      //Calcula media das medidas
    return resultado;
}

bool msg(int n_linhas, int tempo_MS, String linha1, String linha2){

    int size1 = 0;
    int size2 = 0;
    int pos1 =0;
    int pos2 =0;

    size1 = linha1.length();
    size2 = linha2.length();
    pos1 = (16 - size1)/2;
    pos2 = (16 - size2)/2;

    lcd->clear();
    switch(n_linhas) {
        case 1:
        lcd->setCursor(pos1,0);
        lcd->print(linha1);
        break;

        case 2:
        lcd->setCursor(pos1,0);
        lcd->print(linha1);
        lcd->setCursor(pos2,1);
        lcd->print(linha2);
        break;
    }
    if(tempo_MS!=0){//se nao for especificado tempo, nao apaga o display
        delay(tempo_MS);
        lcd->clear();
        return true;
    }
    return true;
}

void getDataHandler(const char *topic, const char *data) {
	StaticJsonBuffer<256> jsonBuffer;
	char *mutableCopy = strdup(data);
	JsonObject& root = jsonBuffer.parseObject(mutableCopy);
	free(mutableCopy);

	// Serial.printlnf("data: %s", data);

	// Because of the way the webhooks work, all data, including numbers, are represented as
	// strings, so we need to convert them back to their native data type here
    calibration_factor = atoi(root["fatorCalibracao"]);
	PERIODO_MEDIDA_MS = atoi(root["intervaloMedidasMS"]);
    PUBLISH_PERIOD_MS = atoi(root["intervaloPublish"]);
    PERIODO_ULTIMO_RESET_MS = atoi(root["intervaloResetMS"]);
  
}

