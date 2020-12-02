// Arquivo: balanca_IoT_V6_Firebase
// Autor: Luiz C M Oliveira
// Descricao: Programa funcional da balança_IoT. Realiza a pesagem, exibe o valor e publica na nuvem. 
//            Funcionalidade de função remota para tara da balança. 
//            Envia medição para o RealTime database do google Firebase
// Atualização - 02.12.2020: 
// - Corrigido o envio de informação de timeStamp para o webhook;
// - Formatação do timestamp corrigida com a função time.format();
// - Adicionado bloqueio de envio de webhook em duplicata;
// - Ajuste das temporizações de medição para evitar envio de informações de "transição" de peso

#include <LiquidCrystal_I2C_Spark.h>
#include <HX711ADC.h>

#define DEBUG_PRINT(...) { Particle.publish( "DEBUG", String::format(__VA_ARGS__) ); }

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

int tareScale(String command);

const char * scaleID = "5001";

int startTime = 0;

void setup(){
  
  // Depuração via monitor serial
  Serial.begin(9600);                               
         
  scale.begin();                            // inicializa balanca
  scale.set_scale(calibration_factor);      // ajusta fator de escala
  
  // inicializa LCD
  lcd = new LiquidCrystal_I2C(0x27, 16, 2);
  lcd->init();
  lcd->backlight();
  lcd->clear();
  
  // Funções na nuvem
  Time.zone(-3.00);                                 // Ajusta horário conforme horário brasileiro - GMT-3h
  Particle.function("tareScale", tareScale);        // Cria uma função para fazer a tara da balança via pela particle Cloud
  
  //Serial.println(scale.get_units());
  scale.tare();
  DEBUG_PRINT("Tara da Balanca...");
  lcd->setCursor(0 ,0 );
  lcd->print("Tara da balanca...");
  
  // aguarda 3s
  startTime = millis();
        while(true){
        if(millis()-startTime >= 3000){
      break;
        }
    }
  
  lcd->clear();  
  lcd->setCursor(1 ,0 );
  lcd->print("Balanca zerada");
  
  // aguarda 3s
  startTime = millis();
        while(true){
        if(millis()-startTime >= 3000){
      break;
        }
    }
  
  lcd->clear();
  lcd->setCursor(6 ,0 );
  lcd->print("Peso");
  lcd->setCursor(4 ,1 );
  lcd->print(String(medida,2));
  lcd->setCursor(10 ,1);
  lcd->print("kg");
 }

void loop(){
  scale.power_up();
  //Verifica se houve mudança no valor medido
    medida = (scale.get_units(10));
    
    // Evita envio de medidas negativas - verificar fator de calibração para tentar resolver esta questão
    if(medida<0){medida=0;}
    
    if((medida > medida_anterior+0.2 || medida < medida_anterior - 0.2 ) && (timerFlag == true)){
    // Se houve alteração da medição em +-0.2 kg e o passou o tempo entre medidas => nova medida!
        Particle.publish("weightChange");
        atualizar_Medida = true;
  }
   
    if (atualizar_Medida == true){
        medida_anterior = medida;
        //DEBUG_PRINT(String(medida,2));
        atualiza_display(String(medida,2));
        atualizar_Medida = false;
        
        //Evita publicação na nuvem em  duplicata
        if (pubFlag == true){
                publicar_na_nuvem();
                //DEBUG_PRINT("Publicou na nuvem");
                pubFlag = false;
        }
    }
  
   scale.power_down();
   
   // Repete o loop a cada 1s
    int startTime = millis();
        while(true){
        if(millis()-startTime >= 1000){
            timerFlag = true;
            pubFlag = true;
            break;
        }
        timerFlag = false;
    }
}


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

int atualiza_display(String valor)
{
    lcd->clear();
    lcd->setCursor(6 ,0 );
    lcd->print("Peso");
    lcd->setCursor(4 ,1 );
    lcd->print(valor);
    lcd->setCursor(10 ,1);
    lcd->print("kg");
}

int publicar_na_nuvem(){
    // Cria o JSON para envio
	String timeStamp = Time.timeStr();
    timeStamp = Time.format("%d/%m/%y %H:%M:%S");	
	
	String Data="{\"scaleID\":\""+String(scaleID)+"\",";        //iD_balanca
	        Data+="\"medidaEm\":\""+String(timeStamp)+"\","; 	//Data da medicao
		    Data+="\"peso\":\""+String(medida,2)+"\"}";			//valor da medida    
	        
	Particle.publish("medidas", Data, PUBLIC);      //publica medida na nuvem
	
}