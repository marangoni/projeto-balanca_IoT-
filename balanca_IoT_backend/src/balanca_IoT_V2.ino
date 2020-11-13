// Arquivo inicial balanca_IoT
// Autor: Luiz C M Oliveira
// Faz a pesagem e exibe no display LCD
// Outubro de 2020


#include <LiquidCrystal_I2C_Spark.h>
#include <HX711ADC.h>

//Display LCD
LiquidCrystal_I2C *lcd;

//HX711 Pin Hookup
#define SCK A3
#define DT A2

//Initialize scale
HX711ADC scale(DT,SCK);

double prevScaleValue;
double scaleValue;
double fullWeight = 100;
double lockWeight;
bool locked = false;
int lockedInt = 0;

bool updatePrev = true;
double seenWeight;
int numTimesSeen;

int tareScale(String command);
double absValue(double value);
int setFullWeight(String command);
int lockScale(String command);

void setup(){
  Serial.begin(9600);
  Particle.variable("scaleValue", scaleValue);
  Particle.variable("lockedInt", lockedInt);
  Particle.function("setFull", setFullWeight);
  Particle.function("tareScale", tareScale);
  Particle.function("lockScale", lockScale);
  
  lcd = new LiquidCrystal_I2C(0x27, 16, 2);
  lcd->init();
  lcd->backlight();
  lcd->clear();
  Time.zone(-3.00);

  //display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  //display.display();
  
   lcd->clear();
   lcd->setCursor(0 ,0 );
   lcd->print("   Balanca IoT   ");
   lcd->setCursor(0 ,1 );
   lcd->print("   Bem-vindo!  ");
   delay(5000);
   lcd->clear();
   lcd->setCursor(0 ,0 );
   lcd->print("Suba na balanca");

  scale.set_scale(107.32);//Calibrates scale (See calibration instructions)

  Serial.println(scale.get_units());
  scale.tare();
}

void loop(){
  scale.power_up();

  //Check for a change in weight and send event
  if(scaleValue > prevScaleValue+2 || scaleValue < prevScaleValue - 2){
    Particle.publish("weightChange");
  }

  if(locked && (scaleValue > prevScaleValue +2 || scaleValue < prevScaleValue -2)){
    updatePrev = false;
    numTimesSeen++;
    if(numTimesSeen >=2){
      updatePrev = true;
      numTimesSeen = 0;
      Particle.publish("lockWtChange");
    }
  }

  //Get new Readings
  if(updatePrev){
    prevScaleValue = scaleValue;
  }
  scaleValue = scale.get_units(5);
  scaleValue = absValue(scaleValue);

  //Update Display
  //display.clearDisplay();
  //display.setTextSize(2);
  //display.setTextColor(WHITE);
  //display.printf("Wt:%2d g", (int)scaleValue);
  //display.setCursor(0,0);
  //display.setTextSize(1);
  //if(locked){
  //display.printf("\n\nFull: %2d g", (int)fullWeight);
  //  display.printf("\n\n\nLocked at %2d g", (int)lockWeight);
  //}
  //display.display();


  //Serial.print(scaleValue);Serial.println("g");

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
  Serial.println("Scale Teared From Cloud!!");
  return 1;
}

int setFullWeight(String command){
  fullWeight = scaleValue;
  return fullWeight;
}

int lockScale(String command){
  locked = !locked;
  lockWeight = scaleValue;

  if(locked){
    lockedInt = 1;
    return lockWeight;
  }else{
    lockedInt = 0;
    return -1;
  }
}

//Not really working right now
double absValue(double value){
  double newValue = 0;
  if(value < 0){
    newValue = -1*value;
    return newValue;
  }
  return value;
}