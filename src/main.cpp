#include <Arduino.h>
#include <RotaryEncoder.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "wlan.h"
#include "RTClib.h"
#include "webserver.h"

RTC_DS3231 rtc;

#define PIN_A 32
#define PIN_B 35
#define PIN_T1 2      // Schnellzugriff Programm 1
#define PIN_T2 4      // Schnellzugriff Programm 8
#define PIN_T3 33     // Encoder-Taster
#define PIN_BUZZER 13 // Buzzer-Pin

RotaryEncoder encoder(PIN_A, PIN_B, RotaryEncoder::LatchMode::FOUR3);

#define MENU_ANZ 11 // 10 Menüeinträge
char *menu[] =
    {
        "Eier weich",
        "Eier mittelweich",
        "Eier hart kochen",
        "kurzer Nap",
        "Teestunde",
        "Nudeln al dente",
        "Reis kochen",
        "Videospiele",
        "Teig ruhen",
        "Yogaübungen",
        "WLAN"};

unsigned long subMenuTimes[] =
    {
        4,           // (3 Minuten für "Eier weich)=kurz zum Testen"
        5 * 60,      // 5 Minuten für "Eier mittelweich"
        7 * 60,      // 7 Minuten für "Eier hart kochen"
        20 * 60,     // 20 Minuten für "kurzer Nap"
        10 * 60,     // 10 Minuten für "Teestunde"
        8 * 60,      // 8 Minuten für "Nudeln al dente"
        15 * 60,     // 15 Minuten für "Reis kochen"
        2 * 60 * 60, // 2 Stunden für "Videospiele"
        30 * 60,     // 30 Minuten für "Teig ruhen"
        45 * 60,     // 45 Minuten für "Yogaübungen"
};

#define SCREEN_WIDTH 128 // OLED Display Breite in Pixel
#define SCREEN_HEIGHT 64 // OLED Display Höhe in Pixel
#define OLED_RESET -1    // Reset-Pin # (oder -1, wenn der Arduino-Reset-Pin verwendet wird)

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

bool countDownActive = false;
unsigned long countDownStartTime;
unsigned long countDownDuration;
char currentMenuItem[20]; // Enthält den Namen des aktuellen Menüpunkts

bool isEndAnimationActive = false; // Flag zur Steuerung der Endanimation
unsigned long animationStartTime;  // Zeitpunkt, zu dem die Endanimation gestartet wurde

unsigned long buzzerDuration = 400; // Dauer des Buzzer-Signals in Millisekunden (1 Sekunde in diesem Beispiel)

bool isSettingTime = false;    // Flag zur Steuerung der Zeiteinstellung
unsigned long setTimeDuration; // Die eingestellte Dauer über den Rotary Encoder
unsigned long lastEncoderPosition = 0;

void showSubMenu(int selectedMenu)
{
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);

  // Zeige das ausgewählte Menü und sein Untermenü
  display.println(menu[selectedMenu]);
  unsigned long time = subMenuTimes[selectedMenu];
  unsigned long hours = time / 3600;
  unsigned long minutes = (time % 3600) / 60;
  unsigned long seconds = time % 60;
  display.print(hours);
  display.print(":");
  if (minutes < 10)
  {
    display.print("0");
  }
  display.print(minutes);
  display.print(":");
  if (seconds < 10)
  {
    display.print("0");
  }
  display.println(seconds);
  display.display();
}

void updateCountDown()
{
  if (countDownActive)
  {
    unsigned long elapsedTime = millis() - countDownStartTime;
    unsigned long remainingTime = countDownDuration > elapsedTime ? countDownDuration - elapsedTime : 0;

    if (remainingTime <= 0)
    {
      isEndAnimationActive = true; // Starte die Endanimation, wenn der Countdown abgelaufen ist
      animationStartTime = millis();
      countDownActive = false;
      digitalWrite(PIN_BUZZER, HIGH); // Buzzer einschalten
    }
    else
    {
      unsigned long hours = remainingTime / 3600000;
      remainingTime %= 3600000;
      unsigned long minutes = remainingTime / 60000;
      remainingTime %= 60000;
      unsigned long seconds = remainingTime / 1000;

      display.clearDisplay();
      display.setTextSize(1);
      display.setTextColor(SSD1306_WHITE);
      display.setCursor(0, 0);
      display.print(currentMenuItem); // Zeige den Namen des ausgewählten Menüpunkts an
      display.print(" \n ");
      display.print(hours);
      display.print(":");
      if (minutes < 10)
      {
        display.print("0");
      }
      display.print(minutes);
      display.print(":");
      if (seconds < 10)
      {
        display.print("0");
      }
      display.println(seconds);
      display.display();
    }
  }
}

void showDateTime() // Zeige das aktuelle Datum und die Uhrzeit auf dem OLED-Display
{
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);

  DateTime now = rtc.now();
  String dateStr = now.timestamp(DateTime::TIMESTAMP_DATE);
  String timeStr = now.timestamp(DateTime::TIMESTAMP_TIME);

  int16_t xDate = (SCREEN_WIDTH - 12 * dateStr.length()) / 2;
  int16_t xTime = (SCREEN_WIDTH - 12 * timeStr.length()) / 2;

  display.setCursor(xDate, SCREEN_HEIGHT / 2 - 14);
  display.println(dateStr);

  display.setCursor(xTime, SCREEN_HEIGHT / 2 + 4);
  display.println(timeStr);

  display.display();
}

void setup()
{
  pinMode(PIN_T3, INPUT_PULLUP);
  pinMode(PIN_BUZZER, OUTPUT); // Buzzer-Pin als Ausgang festlegen
  Serial.begin(115200);
  Serial.println("Einfaches Beispiel zum Rotary Encoder");

  connectToWiFi(); // Verbindung zum WLAN herstellen

  startWebserver(menu, subMenuTimes, 10); // Startet Webserver

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
  {
    Serial.println(F("SSD1306-Zuweisung fehlgeschlagen"));
    for (;;)
      ;
  }

  if (!rtc.begin())
  {
    Serial.println("RTC nicht gefunden");
    while (1)
      ;
  }
  rtc.adjust(DateTime(__DATE__, __TIME__));
  showDateTime();
}

static long pos = 0;
int t1Counter = 0;
int t2Counter = 0;
int t3Counter = 0;
int t4Counter = 0; // Zähler für T4

void showEndAnimation()
{
  unsigned long currentTime = millis();    // Dieser Code wird ausgeführt, wenn der Timer abgelaufen ist
  unsigned long animationDuration = 10000; // Dauer der Animation

  if (currentTime - animationStartTime <= animationDuration)
  {
    // Zeige den Text "Ende" nach oben bewegend
    int yPos = (SCREEN_HEIGHT - 8) - ((currentTime - animationStartTime) * SCREEN_HEIGHT / animationDuration);
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor((SCREEN_WIDTH - 32) / 2, yPos);
    display.println("Ende");
    display.display();
  }
  else
  {
    isEndAnimationActive = false;   // Beende die Animation
    showDateTime();                 // Zurück zum Hauptmenü
    digitalWrite(PIN_BUZZER, HIGH); // Buzzer ausschalten
  }
}

void loop()
{
  encoder.tick();
  int newPos = encoder.getPosition();

  if (pos != newPos)
  {
    if (isSettingTime)
    {
      setTimeDuration += newPos - lastEncoderPosition;
      if (setTimeDuration < 0)
        setTimeDuration = 0;
      lastEncoderPosition = newPos;

      display.clearDisplay();
      display.setTextSize(1);
      display.setTextColor(SSD1306_WHITE);
      display.setCursor(0, 0);
      display.print("Einstellen: ");
      unsigned long hours = setTimeDuration / 3600;
      unsigned long minutes = (setTimeDuration % 3600) / 60;
      unsigned long seconds = setTimeDuration % 60;
      display.print(hours);
      display.print(":");
      if (minutes < 10)
      {
        display.print("0");
      }
      display.print(minutes);
      display.print(":");
      if (seconds < 10)
      {
        display.print("0");
      }
      display.println(seconds);
      display.display();
    }
    else
    {
      display.clearDisplay();
      display.setTextSize(1);
      display.setTextColor(SSD1306_WHITE);
      display.setCursor(0, 0);

      int displayStart = newPos % MENU_ANZ;
      if (displayStart < 0)
      {
        displayStart += MENU_ANZ;
      }
      for (int i = 0; i < MENU_ANZ; ++i)
      {
        int index = (displayStart + i) % MENU_ANZ;
        if (i == 0)
        {
          display.print("* ");
          strcpy(currentMenuItem, menu[index]); // Speichere den Namen des aktuellen Menüpunkts
        }
        else
        {
          display.print("  ");
        }
        display.println(menu[index]);
      }

      display.display();
      pos = newPos;
    }
  }

  if (digitalRead(PIN_T3) == LOW)
  {
    t3Counter++;
    delay(200); // Entprellung

    if (t3Counter == 2)
    {
      t3Counter = 0;                  // Zurück zum Hauptmenü
      countDownActive = false;        // Stoppe den Countdown
      isEndAnimationActive = false;   // Stoppe die Endanimation
      isSettingTime = false;          // Beende die Zeiteinstellung
      showDateTime();                 // Zeige das Hauptmenü (Datum/Uhrzeit)
      digitalWrite(PIN_BUZZER, HIGH); // Buzzer ausschalten
    }
  }

  if (digitalRead(PIN_T1) == HIGH)
  {
    t1Counter++;
    delay(200); // Entprellung

    if (t1Counter == 2)
    {
      // Auswahl von dem aktuellen Eintrag aus dem Menü
      t1Counter = 0;
      showSubMenu(0); // Zeige das Untermenü für den aktuellen Eintrag aus dem Menü
      countDownActive = true;
      countDownStartTime = millis();
      countDownDuration = subMenuTimes[0] * 1000;
      strcpy(currentMenuItem, menu[0]); // Speichere den Namen des Menüpunkts
    }
  }

  if (digitalRead(PIN_T2) == HIGH)
  {
    t2Counter++;
    delay(200); // Entprellung

    if (t2Counter == 2)
    {
      // Auswahl von "Teig ruhen" aus dem Menü
      t2Counter = 0;
      showSubMenu(8); // Zeige das Untermenü für "Teig ruhen"
      countDownActive = true;
      countDownStartTime = millis();
      countDownDuration = subMenuTimes[8] * 1000; // Starte den Countdown für "Teig ruhen"
      strcpy(currentMenuItem, menu[8]);           // Speichere den Namen des Menüpunkts
    }
  }

  if (isEndAnimationActive)
  {
    showEndAnimation(); // Starte die Endanimation
  }
  else
  {
    updateCountDown();
  }
}