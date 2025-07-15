// Sensor de calidad de agua, compensación por temperatura y calibración de valor K automáticamente.

// Librerías
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <WiFi.h>
#include <Preferences.h>
#include <Adafruit_ADS1X15.h>

// Definiciones y constantes
#define OFFSET 0.05
#define TdsSensorPin 36
#define VREF 6.144
#define SCOUNT 30

// WiFi
const char* ssid     = "EMA.AGUA";
const char* password = "EMA.AGUA";
WiFiServer server(80);
IPAddress IP = WiFi.softAPIP();

// Variables globales
String header;
String readString;
int analogBuffer[SCOUNT];
int analogBufferTemp[SCOUNT];
int analogBufferIndex = 0;
int copyIndex = 0;
float kValue = 1.0;
float averageVoltage = 0.0;
float tdsValue = 0.0;
float tdsValueTemp = 0.0;
float temperature = 25;
float tdsT;

// LCD
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Sensor de temperatura
const int oneWireBus = 4;
OneWire oneWire(oneWireBus);
DallasTemperature sensors(&oneWire);

// Memoria flash
Preferences variablesPermanentes;

// ADC externo
Adafruit_ADS1115 ads;

// Función para filtro de media
int getMedianNum(int bArray[], int iFilterLen) {
  int bTab[iFilterLen];
  for (byte i = 0; i < iFilterLen; i++)
    bTab[i] = bArray[i];
  int i, j, bTemp;
  for (j = 0; j < iFilterLen - 1; j++) {
    for (i = 0; i < iFilterLen - j - 1; i++) {
      if (bTab[i] > bTab[i + 1]) {
        bTemp = bTab[i];
        bTab[i] = bTab[i + 1];
        bTab[i + 1] = bTemp;
      }
    }
  }
  if ((iFilterLen & 1) > 0) {
    bTemp = bTab[(iFilterLen - 1) / 2];
  } else {
    bTemp = (bTab[iFilterLen / 2] + bTab[iFilterLen / 2 - 1]) / 2;
  }
  return bTemp;
}

void setup() {
  // Inicializar memoria
  variablesPermanentes.begin("vars", false);
  if (variablesPermanentes.isKey("kvalueP")) {
    kValue = variablesPermanentes.getFloat("kvalueP");
  }

  // LCD
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("  Bienvenid@s");
  lcd.setCursor(0, 1);
  lcd.print("EMA cond. elect.");

  Serial.begin(115200);
  delay(6000);

  // Sensores
  pinMode(TdsSensorPin, INPUT);
  sensors.begin();

  // WiFi
  Serial.print("Setting AP (Access Point)…");
  WiFi.softAP(ssid, password);
  IP = WiFi.softAPIP();
  server.begin();

  // ADC externo
  if (!ads.begin(0x48)) {
    Serial.println("Failed to initialize ADS.");
    while (1);
  }
}

void handleClient() {
  WiFiClient client = server.available();
  if (!client) return;

  Serial.println("New Client.");
  String currentLine = "";
  while (client.connected()) {
    if (client.available()) {
      char c = client.read();
      if (readString.length() < 15) {
        readString += c;
      }
      header += c;
      if (c == '\n') {
        if (currentLine.length() == 0) {
          // Responder con la página web
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/html");
          client.println();
          client.println("<HTML>");
          client.println("<HEAD><TITLE>Calidad de Agua</TITLE></HEAD>");
          client.println("<BODY>");
          client.println("<H1>EMA calidad de agua MAC Caqueta PPM-TERRAE</H1>");
          client.println("<H2>Calibracion de parametro K</H2>");
          client.println("<p>Valor actual de K: " + String(kValue) + "</p>");
          client.println("<FORM ACTION='/' method=get >");
          client.println("Ingrese el valor conocido de conductividad electrica: <INPUT TYPE=TEXT NAME='LED' VALUE='' SIZE='25' MAXLENGTH='50'>");
          client.println("<INPUT TYPE=SUBMIT NAME='submit' VALUE='CALCULAR'>");
          client.println("</FORM>");
          client.println("</BODY></HTML>");
          client.println();
          break;
        } else {
          currentLine = "";
        }
      } else if (c != '\r') {
        currentLine += c;
      }
    }
  }
  header = "";
  client.stop();
  Serial.println("Client disconnected.");
  Serial.println(readString);

  // Procesar valor K si se envió desde la web
  if (readString.indexOf("GET /?LED") >= 0) {
    int i;
    char delimiter[] = "=&";
    char *p;
    char string[128];
    String test = readString;
    String words[3];
    test.toCharArray(string, sizeof(string));
    i = 0;
    p = strtok(string, delimiter);
    while (p && i < 3) {
      words[i] = p;
      p = strtok(NULL, delimiter);
      ++i;
    }
    float tdsTempo = words[1].toFloat();
    if (tdsValueTemp == 0) tdsValueTemp = 1;
    kValue = tdsTempo / tdsValueTemp;
    variablesPermanentes.putFloat("kvalueP", kValue);
  }
  readString = "";
}

void readSensors() {
  // Leer temperatura
  sensors.requestTemperatures();
  float temperatureC = sensors.getTempCByIndex(0);
  if (temperatureC < 0) temperatureC = 25.0;

  // Leer ADC
  static unsigned long analogSampleTimepoint = millis();
  if (millis() - analogSampleTimepoint > 500U) {
    analogSampleTimepoint = millis();
    analogBuffer[analogBufferIndex] = ads.readADC_SingleEnded(0) - 70;
    if (analogBuffer[analogBufferIndex] < 0) analogBuffer[analogBufferIndex] = 0;
    Serial.print("anval:");
    Serial.print(analogBuffer[analogBufferIndex]);
    analogBufferIndex++;
    if (analogBufferIndex == SCOUNT) {
      analogBufferIndex = 0;
    }
  }

  // Calcular TDS
  static unsigned long printTimepoint = millis();
  if (millis() - printTimepoint > 200U) {
    printTimepoint = millis();
    for (copyIndex = 0; copyIndex < SCOUNT; copyIndex++) {
      analogBufferTemp[copyIndex] = analogBuffer[copyIndex];
      averageVoltage = getMedianNum(analogBufferTemp, SCOUNT) * (float)VREF / 65535.0;
      float compensationCoefficient = 1.0 + 0.02 * (temperatureC - 25.0);
      float compensationVoltage = averageVoltage / compensationCoefficient;
      tdsValue = (133.42 * pow(compensationVoltage, 3) - 255.86 * pow(compensationVoltage, 2) + 857.39 * compensationVoltage) * 0.5 * kValue;
      tdsValueTemp = (133.42 * pow(compensationVoltage, 3) - 255.86 * pow(compensationVoltage, 2) + 857.39 * compensationVoltage) * 0.5;
      if (tdsValue > 0) {
        tdsT = tdsValue;
      } else {
        tdsT = tdsT / 2;
      }
      tdsValue = tdsT;
      Serial.print("TDS Value:");
      Serial.print(tdsValue);
      Serial.println("uS/cm");
    }
  }

  // Mostrar en LCD
  lcd.setCursor(0, 0);
  lcd.print("ip: ");
  lcd.print(IP);
  lcd.setCursor(0, 1);
  lcd.print("CE:");
  lcd.print(tdsValue, 0);
  lcd.print("uS, T:");
  lcd.print(temperatureC, 0);
  lcd.print((char)223);
  lcd.print("C    ");
}

void loop() {
  handleClient();
  readSensors();
}
