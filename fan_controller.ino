#include <WiFiManager.h>
#include <ezButton.h>
#include "secrets.h"
#include <WiFiClientSecure.h>
#include <ESP32Time.h>

// =======================================================================
// --- Mapeamento de Hardware ---
#define  blinkLed   33
#define  outRelay   25
#define  button     21


// =======================================================================
// --- Variáveis Globais ---
static bool isRunning = true;
unsigned long timeStart = 0;

byte lastButtonState = LOW;
byte ledState = LOW;
unsigned long debounceDuration = 50; // millis
unsigned long lastTimeButtonStateChanged = 0;

const int startHour = 2; // Horário de início (3 AM)
const int endHour = 3;   // Horário de término (4 AM)
unsigned long milisseconds = 1000;
unsigned long minInSeconds = 60;
unsigned long desiredMins = 60;
unsigned long updateTimeDuration = desiredMins * minInSeconds * milisseconds; // millis

unsigned long lastTimeManageChanged = 0;


const char* NTP_SERVER = "br.pool.ntp.org";
int timezoneOffset = -3; 
int timezoneOffsetSec = timezoneOffset * 3600;

struct tm timeinfo;


// =======================================================================
// --- Protótipo das Funções ---
void blink(int time_interval, int led_pin);
void handleFanButton();
void NTPConnect();
void updateRTC();
void manageTime();


ezButton fanButton(button);
WiFiClientSecure net = WiFiClientSecure();
ESP32Time rtc(0);

// =======================================================================
// --- Configurações Iniciais ---
void setup() {
  Serial.begin(9600);
  delay(10);
  
  pinMode(blinkLed, OUTPUT);
  pinMode(outRelay, OUTPUT);

  digitalWrite(blinkLed, LOW);
  digitalWrite(outRelay, LOW);

  fanButton.setDebounceTime(debounceDuration);

  wifiConnection();
  NTPConnect();
}

void loop() {
  // Serial.printf("isRunning: %d\n", isRunning);
  if (millis() - timeStart > 500 && isRunning) {
    blink(2000, blinkLed);
    handleFanButton();
    manageTime();
  }
}


// =======================================================================
// --- Desenvolvimento das Funções ---
void blink(int time_interval, int led_pin)
{
  static int time_save = 0;

  static bool led_state = false;

  unsigned long time = millis();
  
  if((time - time_save) >= time_interval)
  {
    digitalWrite(led_pin, !led_state);
    led_state = !led_state;
    
    time_save = millis();
  }
}

void handleFanButton() {
  fanButton.loop();

  static bool led_state = false;

  if (millis() - lastTimeButtonStateChanged > debounceDuration) {
    byte buttonState = fanButton.getState();
    if (buttonState != lastButtonState) {
      lastTimeButtonStateChanged = millis();
      lastButtonState = buttonState;
      if (buttonState == LOW) {
        ledState = (ledState == HIGH) ? LOW : HIGH;
        digitalWrite(outRelay, ledState);
      }
    }
  }
}

void wifiConnection()
{
  WiFiManager wm;

  bool res = wm.autoConnect(AP_SSID, AP_PWD);

  if(res) Serial.println("Successfully connected! :)");
  else    Serial.println("Failed to connect");
}

void NTPConnect() {
  configTime(timezoneOffsetSec, 0, NTP_SERVER);

  while (time(NULL) <= 100000) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nConectado ao NTP Server");
}

// garante que o RTC do ESP32 seja mantido sincronizado com o tempo local obtido do sistema operacional subjacente (sistema FreeRTOS) através da estrutura timeinfo.
// Isso é importante para manter a precisão do RTC do ESP32, especialmente se você estiver executando tarefas sensíveis ao tempo ou precisar registrar eventos com base no tempo real
void updateRTC() { //atualiza o RTC (Real-Time Clock) com o tempo obtido a partir da estrutura de tempo 'timeinfo. 
  if (getLocalTime(&timeinfo)){  //obter o tempo atual da máquina em que o código está sendo executado e armazená-lo na estrutura timeinfo
    rtc.setTimeStruct(timeinfo); //Uma vez que timeinfo contém o tempo atual em um formato legível, esta linha de código usa esse valor para atualizar o RTC do dispositivo ESP32
  }
}

void manageTime() {
  
  if (millis() - lastTimeManageChanged > updateTimeDuration) {
    
    String currentAmPmTime = rtc.getAmPm();
    time_t currentHourTime = rtc.getHour();
    
    if (currentHourTime >= startHour && currentHourTime <= endHour && currentAmPmTime == "AM") {
      getCurrentTime();
      ledState = LOW;
      digitalWrite(outRelay, ledState);
      
      updateRTC(); //chamar periodicamente para garantir que o RTC esteja sempre atualizado com o tempo correto.
    }

    lastTimeManageChanged = millis();
  }
  
  /* time_t currentEpochTime = rtc.getEpoch();
  
    Serial.printf("currentEpochTime: %lu\n", currentEpochTime);
    Serial.printf("currentAmPmTime: %s\n", currentAmPmTime.c_str());
    Serial.printf("currentHourTime: %02d\n", currentHourTime);*/
}

void getCurrentTime() {
  String formattedTime = rtc.getTime("%d/%m/%Y %H:%M:%S");
  Serial.printf("Hora atual: %s\n", formattedTime.c_str()); 
}