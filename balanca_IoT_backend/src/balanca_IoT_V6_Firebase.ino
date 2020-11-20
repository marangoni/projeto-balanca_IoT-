//Atualizacao do firmware da balancaIoT para adequacao ao BD do FIREBASE
//Autor: Luiz C M Oliveira
// 20.11.2020



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

// Integração com o firebase
// definição do nome do evento, local, apelido e material

//const char * eventName2 = "medidas";
//const char * proprietario = "Jeca Teste";
//const char * local = "Rua San Paul, 38";
//const char * apelido = "Escola de Artes Suzanus";
//const char * material = "papel";

const char * scaleID = "5001";

void setup(){
  
  Serial.begin(9600);                       // monitor serial 9600 Bps
  Particle.variable("medida", medida);
  Particle.function("tareScale", tareScale);
  scale.begin();                            // inicializa balanca
  scale.set_scale(calibration_factor);      // ajusta fator de escala
  
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
    // Se houve alteração da medição em +-0.2 kg => nova medida!
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

int tareScale(String command)
{
  scale.tare();
  delay(100);
  Serial.println("Tara da balanca pela nuvem!!");
  DEBUG_PRINT("Tara da balanca");
   lcd->clear();
   lcd->setCursor(0 ,0 );
   lcd->print("Tara da balanca");
   lcd->setCursor(0 ,1 );
   lcd->print(" pela nuvem!");
   delay(2000);
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
	
	//timeStamp = Time.timeStr();
	
	// OBS: inclusao do timestamp NÃO está funcionando - problemas na formatação do JSON - VERIFICAR!!!!
	//      inclusao do identificador da balanca - scaleID - nao funciona adequadamente - se scaleID = 0001, por exemplo, o
	//      dado não é enviado e todos os campos do JSON ficam vazios. VERIFICAR !!!!
	
	
	char * timeStamp = "2020";
	//valor de teste
	
    // formatacao do JSON para envio
	char Data[256];
	snprintf(Data, sizeof(Data), "{\"scaleID\":%s,\"medidaEm\":%s,\"peso\":%.2f}", scaleID, timeStamp, medida);
	
	
    	//Publicação na nuvem do JSON
		Particle.publish("state", "Nova_medida");
		//Particle.publish(eventName, Data, PUBLIC);      //publica na nuvem
		Particle.publish("medidas", Data, PUBLIC);      //publica na nuvem
}
    