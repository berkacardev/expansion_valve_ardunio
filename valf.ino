#define COOLING_PIN 2
#define HEATING_PIN 3
#define NTC_PIN A7

const float seriesResistor = 10000;
const float nominalResistance = 4700;
const float nominalTemperature = 25;
const float bCoefficient = 3950;
const int adcMax = 1023;
const float supplyVoltage = 5.0;


void setup() {
  Serial.begin(115200);
  pinMode(COOLING_PIN,OUTPUT);
  pinMode(HEATING_PIN,OUTPUT);
}

String incomingData = "";
float presentTemperature = 0.0;
float presentPressure = 0.0;

void loop() {
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n') {
      parseData(incomingData);
      incomingData = "";
    } else {
      incomingData += c;
    }
  }
  controlExpansionValveHeatingOrCoolingProcess(presentTemperature,presentPressure);
  //getEnvironmentTemperature();
  delay(1000);
}

float calculateSaturationTemperature(float temperature, float pressureAsKiloPascal){
  float saturationTemperature = -273.15;
  if(pressureAsKiloPascal>=30 && pressureAsKiloPascal<130){
    saturationTemperature = -1.483202e-07*pow(pressureAsKiloPascal, 4) + 6.050253e-05*pow(pressureAsKiloPascal, 3) - 1.004336e-02*pow(pressureAsKiloPascal, 2) + 1.007342*pressureAsKiloPascal - 72.35443;
  } 
  else if(pressureAsKiloPascal>=130 && pressureAsKiloPascal<415){
    saturationTemperature = -1.071269e-09*pow(pressureAsKiloPascal, 4) + 1.541994e-06*pow(pressureAsKiloPascal, 3) - 9.215127e-04*pow(pressureAsKiloPascal, 2) + 0.3447084*pressureAsKiloPascal - 52.78948;
  }
  Serial.println();
  Serial.print("Hesaplanan doyma sıcaklığı: ");
  Serial.print(saturationTemperature);
  return saturationTemperature;
}
float getTargetTemperature(float saturationTemperature){
  return saturationTemperature+10;
}
void controlExpansionValveHeatingOrCoolingProcess(float presentEvaporatorTemperature, float presentEvaporatorPressureAsKiloPascal){
  float dT = 5;
  float saturationTemperature = calculateSaturationTemperature(presentEvaporatorTemperature,presentEvaporatorPressureAsKiloPascal);
  float targetTemperature = getTargetTemperature(saturationTemperature);
  if(presentEvaporatorTemperature > targetTemperature+dT){
    makeHeatingProcess();
  }
  else if((targetTemperature + dT > presentEvaporatorTemperature) &&  (presentEvaporatorTemperature>targetTemperature)){
    noHeatingCoolingProcess();
  }
  else if(presentEvaporatorTemperature<targetTemperature){
    makeCoolingProcess();
  }
}

void parseData(String data) {
  int separatorIndex = data.indexOf(':');
  if (separatorIndex != -1) {
    String tempStr = data.substring(0, separatorIndex);
    String pressStr = data.substring(separatorIndex + 1);
    presentTemperature = tempStr.toFloat();
    presentPressure = pressStr.toFloat();
    Serial.println();
    Serial.println("---------------------------------------------------");
    Serial.print("Evaparatör Sıcaklık: ");
    Serial.print(presentTemperature);
    Serial.print(" Evaparatör Basınç: ");
    Serial.print(presentPressure);
  }
}

void makeCoolingProcess(){
  digitalWrite(HEATING_PIN,HIGH);
  Serial.println();
  Serial.print("Soğutma işlemi yapılıyor");
  digitalWrite(COOLING_PIN,LOW);
}
void makeHeatingProcess(){
  digitalWrite(COOLING_PIN,HIGH);
  Serial.println();
  Serial.print("Isıtma işlemi yapılıyor");
  digitalWrite(HEATING_PIN,LOW);
}
void noHeatingCoolingProcess(){
  digitalWrite(COOLING_PIN,HIGH);
  digitalWrite(HEATING_PIN,HIGH);
  Serial.println();
  Serial.print("Hedef sıcaklık içinde bir işlem yok");
}

void getEnvironmentTemperature(){
  int adcValue = analogRead(NTC_PIN);
  float voltage = adcValue * (supplyVoltage / adcMax);
  float resistance = (supplyVoltage * seriesResistor / voltage) - seriesResistor;
  float steinhart;
  steinhart = resistance / nominalResistance;     
  steinhart = log(steinhart);                     
  steinhart /= bCoefficient;                      
  steinhart += 1.0 / (nominalTemperature + 273.15);
  steinhart = 1.0 / steinhart;                    
  steinhart -= 273.15;                            
  Serial.println();
  Serial.print("Gerçek Ortam Sıcaklığı: ");
  Serial.print(steinhart);
  Serial.println(" °C");

}