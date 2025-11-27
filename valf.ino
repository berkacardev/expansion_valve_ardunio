#define COOLING_PIN 2
#define HEATING_PIN 3
#define BULB_TEMPERATURE_PIN A6
#define EVAPORATOR_TEMPERATURE_PIN A5
#define PRESSURE_TRANSMITTER_PIN A4

const float seriesResistor = 9900;
const float nominalResistance = 10000;
const float nominalTemperature = 18;
const float bCoefficient = 3950;
const int adcMax = 1023;
const float supplyVoltage = 5.0;
const float temperatureConfigurationCorrectionCoefficientForBulb = 3.0;
const float temperatureConfigurationCorrectionCoefficientForEvaporator = 3.0;

unsigned long previousPrintTime = 0;
const long printInterval = 100; 

unsigned long previousControlTime = 0;
const long controlInterval = 300; 

void setup() {
  Serial.begin(115200);
  pinMode(COOLING_PIN, OUTPUT);
  pinMode(HEATING_PIN, OUTPUT);
}

String incomingData = "";
float presentEvaporatorTemperature = 0.0;
float presentEvaporatorPressure = 0.0;

void loop() {
  readSerialDataNonBlocking();
  unsigned long currentMillis = millis();
  if (currentMillis - previousControlTime >= controlInterval) {
    previousControlTime = currentMillis;
    controlExpansionValveHeatingOrCoolingProcess(presentEvaporatorTemperature, presentEvaporatorPressure);
  }
  if (currentMillis - previousPrintTime >= printInterval) {
    previousPrintTime = currentMillis;
    printExpansionValveBulbTemperature();
  }
}

void readSerialDataNonBlocking() {
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n') {
      parseData(incomingData);
      incomingData = "";
    } else {
      incomingData += c;
    }
  }
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
  return saturationTemperature + 10;
}

void controlExpansionValveHeatingOrCoolingProcess(float presentEvaporatorTemperature, float presentEvaporatorPressureAsKiloPascal){
  float dT = 5; 
  float saturationTemperature = calculateSaturationTemperature(presentEvaporatorTemperature, presentEvaporatorPressureAsKiloPascal);
  float targetTemperature = getTargetTemperature(saturationTemperature);
  
  if(presentEvaporatorTemperature > targetTemperature + dT){
    makeHeatingProcess();
  }
  else if((targetTemperature + dT >= presentEvaporatorTemperature) && (presentEvaporatorTemperature > targetTemperature)){
    noHeatingCoolingProcess();
  }
  else if(presentEvaporatorTemperature <= targetTemperature){
    makeCoolingProcess();
  }
}

void parseData(String data) {
  int separatorIndex = data.indexOf(':');
  if (separatorIndex != -1) {
    String tempStr = data.substring(0, separatorIndex);
    String pressStr = data.substring(separatorIndex + 1);
    presentEvaporatorTemperature = tempStr.toFloat();
    presentEvaporatorPressure = pressStr.toFloat();
    Serial.println();
    Serial.println("---------------------------------------------------");
    Serial.print("Evaparatör Sıcaklık: ");
    Serial.print(presentEvaporatorTemperature);
    Serial.print(" Evaparatör Basınç: ");
    Serial.print(presentEvaporatorPressure);
  }
}

void makeCoolingProcess(){
  digitalWrite(HEATING_PIN, HIGH); 
  Serial.println();
  Serial.print("Soğutma işlemi yapılıyor");
  digitalWrite(COOLING_PIN, LOW); 
}

void makeHeatingProcess(){
  digitalWrite(COOLING_PIN, HIGH); 
  Serial.println();
  Serial.print("Isıtma işlemi yapılıyor");
  digitalWrite(HEATING_PIN, LOW); 
}

void noHeatingCoolingProcess(){
  digitalWrite(COOLING_PIN, HIGH); 
  digitalWrite(HEATING_PIN, HIGH); 
  Serial.println();
  Serial.print("Hedef sıcaklık içinde bir işlem yok");
}

float getExpansionValveBulbTemperature(){
  int adcValue = (analogRead(BULB_TEMPERATURE_PIN) - adcMax) * -1; 
  float voltage = adcValue * (supplyVoltage / adcMax);
  float resistance = (supplyVoltage * seriesResistor / voltage) - seriesResistor;
  float calculatingValue;
  float temperature;
  calculatingValue = resistance / nominalResistance;
  calculatingValue = log(calculatingValue);
  calculatingValue /= bCoefficient;
  calculatingValue += 1.0 / (nominalTemperature + 273.15); 
  calculatingValue = 1.0 / calculatingValue;
  calculatingValue -= 273.15; 
  temperature = calculatingValue + temperatureConfigurationCorrectionCoefficientForBulb;
  return temperature;
}

float getEvaporatorTemperature(){
  int adcValue = (analogRead(EVAPORATOR_TEMPERATURE_PIN) - adcMax) * -1; 
  float voltage = adcValue * (supplyVoltage / adcMax);
  float resistance = (supplyVoltage * seriesResistor / voltage) - seriesResistor;
  float calculatingValue;
  float temperature;
  calculatingValue = resistance / nominalResistance;
  calculatingValue = log(calculatingValue);
  calculatingValue /= bCoefficient;
  calculatingValue += 1.0 / (nominalTemperature + 273.15);
  calculatingValue = 1.0 / calculatingValue;
  calculatingValue -= 273.15;
  temperature = calculatingValue + temperatureConfigurationCorrectionCoefficientForEvaporator;
  return temperature;
}

float getEvaporatorPressure(){
  const float vMin = 1.0; 
  const float vMax = 5.0; 
  const float pressureMaxKPa = 1000.0;
  int adcValue = analogRead(PRESSURE_TRANSMITTER_PIN);
  float voltage = (adcValue * supplyVoltage) / adcMax;
  if (voltage < vMin){
    voltage = vMin;
  } 
  else if (voltage > vMax) {
    voltage = vMax;
  }
  float pressureKPa = (voltage - vMin) * (pressureMaxKPa / (vMax - vMin));
  return pressureKPa;
}

void printExpansionValveBulbTemperature(){
  float temperature = getExpansionValveBulbTemperature();
  Serial.println();
  Serial.print("Duyarga Sıcaklığı: ");
  Serial.print(temperature);
  Serial.println(" °C");
}