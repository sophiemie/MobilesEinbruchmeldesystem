/**
 * Mobiles Einbruchmeldesystem
 * Team 2 - Network System AG
 * 
 * Sophie Miessner 5046830
 * Reda Khalife 5034226 
 * Maximilian Altrock 383459
 * Kelly Mbitketchie Koudjo 5136175
 * 
 * Organisation und Management von Softwareprojekten
 * Prof. Dr.-Ing. Jasminka Matevska
 * Maik Purrmann
 */

 // Header-Datein
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>

// Vordefinierte Werte
#define BAUDRATE 115200
//String ssid = "SomoWLAN";
//String password = "g7MHda9FaFrNAJdL";

String ssid = "SomoWLAN";
String password = "g7MHda9FaFrNAJdL";

String newWlan = "";
String newPasswort = "";

String CHAT_ID = "688408069";
// String CHAT_ID = "-718357539";
String systemName = "Einbruchmeldesystem";
bool benachrichtigungAn = true;

// Telegram BOT
#define BOTtoken "5367355855:AAEssNRSR9xg7cvffE369tOACSxHqqMS-zM"

// Timer-Variablen
unsigned long millisOld;
bool tick_ms;
bool tick_s;
unsigned int tickCount;
#define BEWEGUNGSERKENNUNG_DELAY 5
#define NACHRICHTVERSENDUNG_DELAY 5 
unsigned int bewegungDelayTimer_s;
unsigned int sendeNachrichtTimer_s;
int botRequestDelay = 1000;
unsigned long lastTimeBotRan;

X509List cert(TELEGRAM_CERTIFICATE_ROOT);
WiFiClientSecure client;
UniversalTelegramBot bot(BOTtoken, client);

String botAusgabe;

// Eingabe- Ausgabevariablen
const int bewegungsSensor = 4; // Pin 4
const int ledPin = 2;
bool ledState = LOW;

/**
 * Neu erhaltene Nachrichten behandeln
 */
void handleNewMessages(int numNewMessages)
{
  Serial.println("handleNewMessages");
  Serial.println(String(numNewMessages));

  for (int i=0; i<numNewMessages; i++) 
  {  
    String chat_id = String(bot.messages[i].chat_id); // Chat id of the requester
    if (chat_id != CHAT_ID)
    {
      bot.sendMessage(chat_id, "Unauthorized user", "");
      continue;
    }
    
    String text = bot.messages[i].text;
    Serial.println(text); // Print the received message

    checkBefehlStart(text, bot.messages[i].from_name);
    checkBefehlState(text);
    checkBefehlChangeName(text);
    checkBefehlMessageOnOff(text);
    checkBefehlChangeChatId(text);
  }
}

void checkBefehlChangeChatId(String text)
{
  if (text.startsWith("/change_chatid"))
  {
    Serial.println("Empfänger (Chat-ID) ändern");
    text.remove(0,15); // Entfernde den Befehl vom Text um neue ID rauszufiltern
    Serial.println(text);
    if (text != "") CHAT_ID = text;
  }  
}

void checkBefehlMessageOnOff(String text)
{
  if (text == "/message_on" or text == "/message_on@einbrechermelde_bot")
  {
    benachrichtigungAn = true;
    botAusgabe = "Benachrichtigungen wurden aktiviert.";
    bot.sendMessage(CHAT_ID, botAusgabe, "");
  }
  else if (text == "/message_off" or text == "/message_off@einbrechermelde_bot")
  {
    benachrichtigungAn = false;
    botAusgabe = "Benachrichtigungen wurden deaktiviert.";
    bot.sendMessage(CHAT_ID, botAusgabe, "");
  }  
}

void checkBefehlChangeName(String neuerName)
{
   if (neuerName.startsWith("/change_name"))
    {
      Serial.println("Name ändern");
      neuerName.remove(0,12); // Entfernde den Befehl vom Text um neuen Namen rauszufiltern
      if (neuerName != "") systemName = neuerName;
    }  
}

void checkBefehlStart(String text, String benutzerName)
{
  if (text == "/start" or text == "/start@einbrechermelde_bot") 
  {
    String welcome = "Willkommen, " + benutzerName + ".\n";
    welcome += "Es können folgende Befehle verwendet werden:\n\n";
    welcome += "/state - Zeigt die aktuellen Einstellungen und weitere Informationen zum Einbruchmeldesystem. \n";
    welcome += "/message_on - Schalte die Benachrichtigungen ein. \n";
    welcome += "/message_off - Schalte die Benachrichtigungen aus. \n";
    welcome += "/change_name NAME - Ändere den Namen vom Einbruchmeldesystem. \n";
    welcome += "/change_chatid CHATID - Wähle einen neuen Empfänger mit der Chat-ID von einer Person oder Gruppe. \n";
    welcome += "(Um die Chat-ID rauszufinden muss der Bot @myidbot verwendet werden.) \n";
    bot.sendMessage(CHAT_ID, welcome, "");
  }  
}

void checkBefehlState(String text)
{
  if (text == "/state" or text == "/state@einbrechermelde_bot") 
  {
    botAusgabe = "Name: " + systemName + "\n";      
  
    if (CHAT_ID.startsWith("-")) botAusgabe += "Aktueller Empfänger: " + CHAT_ID + " (Gruppe) \n";
    else botAusgabe += "Aktueller Empfänger: " + CHAT_ID + "\n";
    
    if (benachrichtigungAn) botAusgabe += "Benachrichtigungen: aktiviert \n";
    else botAusgabe += "Benachrichtigungen: deaktiviert \n";
  
    bot.sendMessage(CHAT_ID, botAusgabe, "");
  }  
}

/**
 * Initialisierungen
 */
void setup() 
{
  Serial.begin(BAUDRATE);
  configTime(0, 0, "pool.ntp.org"); // Zeiteinstellungen
  client.setTrustAnchors(&cert); // Telegram Zertifizierung
  initInputOutput();
  initWiFi();
}

void initInputOutput()
{
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, ledState);  
  pinMode(bewegungsSensor, INPUT);
}

void initWiFi()
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) 
  {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }
  Serial.println(WiFi.localIP());  
}

void checkTimer()
{
  // Aktuelle Laufzeit entspricht letzten Laufzeit (keine Millisekunde)
  if (millis() == millisOld) tick_ms = false;
  else 
  {
    tick_ms = true; // Eine Millisekunde vergangen
    tickCount++;
    millisOld = millis(); // letzte Laufzeit aktualisieren
  }

  // Jede Sekunde ein Tick setzen
  if (tickCount >= (1000))
  {
    tick_s = true; // Jede Sekunde
    tickCount = 0;
  }
  else tick_s = false;

  if (tick_s)
  {
    if (bewegungDelayTimer_s < BEWEGUNGSERKENNUNG_DELAY) bewegungDelayTimer_s++;
    if (sendeNachrichtTimer_s < NACHRICHTVERSENDUNG_DELAY) sendeNachrichtTimer_s++;
  }
}

void checkBewegungssensor()
{
  // Wenn aus dem Bewegungssensor eine Bewegung gemeldet und der Timer bereit ist
  if (digitalRead(bewegungsSensor) && bewegungDelayTimer_s >= BEWEGUNGSERKENNUNG_DELAY)
  {
    bewegungDelayTimer_s = 0;
    Serial.println(systemName + ": Bewegung erkannt");

    if (benachrichtigungAn) sendeNachricht(systemName + ": Bewegung erkannt");
  }  
}

void sendeNachricht(String nachricht)
{
  if (sendeNachrichtTimer_s >= NACHRICHTVERSENDUNG_DELAY)
  {
    bot.sendMessage(CHAT_ID, nachricht, "");
    Serial.println("Nachricht versendet");
    sendeNachrichtTimer_s = 0;
  }   
}

void checkBot()
{
  if (millis() > lastTimeBotRan + botRequestDelay)  
  {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    while(numNewMessages) 
    {
      Serial.println("got response");
      handleNewMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }
    lastTimeBotRan = millis();
  }  
}

void changeWiFi()
{
  String eingelesenerText = Serial.readString();

  if (eingelesenerText.startsWith("wlan="))
  {
    eingelesenerText.remove(0,5);
    newWlan = eingelesenerText;
    Serial.println(newWlan);
  }
  else if (eingelesenerText.startsWith("passwort="))
  {
    eingelesenerText.remove(0,9);
    newPasswort = eingelesenerText;
    Serial.println(newPasswort);
  }

  if (newWlan != "" and newPasswort != "")
  {
    ssid = newWlan;
    password = newPasswort;
  }
}

/**
 * Main-Methode
 */
void loop() 
{
  checkBot();  
  checkTimer();
  checkBewegungssensor();
}
