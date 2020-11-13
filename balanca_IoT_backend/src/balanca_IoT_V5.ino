// Arquivo inicial balanca_IoT_V4_TESTE
// Autor: Luiz C M Oliveira
// Programa funcional da balança_IoT. Realiza a pesagem, exibe o valor e publica na nuvem. 
// Funcionalidade de função remota para tara da balança
// Outubro de 2020


// This #include statement was automatically added by the Particle IDE.
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

int tareScale(String command);

// Informacao do canal - ThingSpeak
unsigned long myChannelNumber =1190170; //Canal balanca IoT
const char * myWriteAPIKey = "RGRHPJRPMVESHB91";
const char * eventName = "balanca_IoT";


void setup(){
  
  Serial.begin(9600);                       // monitor serial 9600 Bps
  Particle.variable("medida", medida);
  Particle.function("tareScale", tareScale);
  scale.begin();                        // inicializa balanca
  scale.set_scale(calibration_factor);  // ajusta fator de escala
  
  // inicializa LCD
  lcd = new LiquidCrystal_I2C(0x27, 16, 2);
  lcd->init();
  lcd->backlight();
  lcd->clear();
  Time.zone(-3.00);

 
  //Serial.println(scale.get_units());
  scale.tare();
  DEBUG_PRINT("Balanca zerada");
  lcd->setCursor(1 ,0 );
  lcd->print("Balanca zerada");
  delay(5000);
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
    if(medida<0){medida=0;}
  
    if(medida > medida_anterior+0.2 || medida < medida_anterior - 0.2 ){
        Particle.publish("weightChange");
        atualizar_Medida = true;
  }
   
    if (atualizar_Medida == true){
        medida_anterior = medida;
        DEBUG_PRINT(String(medida,2));
        atualiza_display(String(medida,2));
        atualizar_Medida = false;
        publicar_na_nuvem();
    }
  
   scale.power_down();
    //delay(500);
    int startTime = millis();
        while(true){
        if(millis()-startTime >= 500){
      break;
        }
    }
}

int tareScale(String command){
  scale.tare();
  delay(100);
  Serial.println("Tara da balanca pela nuvem!!");
  DEBUG_PRINT("Tara da balanca");
   lcd->clear();
   lcd->setCursor(0 ,0 );
   lcd->print("Tara da balanca");
   lcd->setCursor(0 ,1 );
   lcd->print(" pela nuvem!");
   delay(5000);
 return 1;
}

int atualiza_display(String valor){
    lcd->clear();
    lcd->setCursor(6 ,0 );
    lcd->print("Peso");
    lcd->setCursor(4 ,1 );
    lcd->print(valor);
    lcd->setCursor(10 ,1);
    lcd->print("kg");
}

int publicar_na_nuvem(){
    // Integração ThingSpeak
        // Cria o JSON para envio
		String timeStamp = Time.timeStr();
		String Data="{\"Data_med\":\""+String(timeStamp)+"\","; //timestamp
	    Data+="\"medida\":\""+String(medida,2)+"\","; 			//medida
		Data+="\"key\":\""+String(myWriteAPIKey)+"\"}";			//api_key thingspeak

		//Publicação na nuvem do JSON
		Particle.publish("state", "Nova_medida");
		Particle.publish(eventName, Data, PUBLIC);      //publica na nuvem
}