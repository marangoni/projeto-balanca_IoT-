// Arquivo inicial balanca_IoT_V2_DEBUG
// Autor: Luiz C M Oliveira
// Teste da função debug para exibição do erro como evento no console do Particle.io
// Outubro de 2020


// This #include statement was automatically added by the Particle IDE.
#include <LiquidCrystal_I2C_Spark.h>
#include <HX711ADC.h>

#define DEBUG_PRINT(...) { Particle.publish( "DEBUG", String::format(__VA_ARGS__) ); }

//Display LCD
LiquidCrystal_I2C *lcd;

//HX711 Pin Hookup
#define SCK A3
#define DT A2

//Initialize scale
HX711ADC scale(DT,SCK);

int leitura_A2 = 0;
int leitura_A3 = 0;



void setup(){
  scale.begin();
  lcd = new LiquidCrystal_I2C(0x27, 16, 2);
  lcd->init();
  lcd->backlight();
  lcd->clear();
  Time.zone(-3.00);

  //scale.set_scale(107.32);//Calibrates scale (See calibration instructions)

  //Serial.println(scale.get_units());
  //scale.tare();
}

void loop(){
    
    long reading = scale.read();
    
    leitura_A3 = analogRead(A3);
    leitura_A2 = analogRead(A2);
    int dif = leitura_A3 - leitura_A2;
    DEBUG_PRINT(String(reading));
    //DEBUG_PRINT(String(leitura_A3));
    delay(1000); 
}