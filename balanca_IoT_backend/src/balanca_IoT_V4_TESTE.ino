// Arquivo inicial balanca_IoT_V4_TESTE
// Autor: Luiz C M Oliveira
// Teste da balanca com o fator de calibração definido manualmente. Transmissão serial da medida
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
//float calibration_factor = 11130;     // fator de calibração para teste inicial
float calibration_factor = 17130;

double massa = 0;

void setup(){
  Particle.variable("massa", massa);
 
  Serial.begin(9600);                   // monitor serial 9600 Bps
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
}

void loop(){
    massa = (scale.get_units(5));
    if(massa<0){massa=0;}
    
    DEBUG_PRINT(String(massa,2));//String(massa));
    lcd->clear();
    lcd->setCursor(6 ,0 );
    lcd->print("Peso");
    lcd->setCursor(4 ,1 );
    lcd->print(String(massa,2));
    lcd->setCursor(10 ,1);
    lcd->print("kg");
    delay(1000); 
}