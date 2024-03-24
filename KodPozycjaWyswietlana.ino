#include <AccelStepper.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Adafruit_Sensor.h>
#include "DHT.h"

const int steps_per_rev = 4098;  
#define IN1 D6
#define IN2 D3
#define IN3 D0
#define IN4 D5
#define czujnik D7
//#define GORA (1)
//#define DOL (0)
#define STOI (4)
#define JEDZ_GORA (2)
#define JEDZ_DOL (3)
#define DO_GORY (4*4098)
#define W_DOL (0)
#define DHTTYPE DHT11
#define DHTPin D4


DHT dht(DHTPin, DHTTYPE);

//const char* ssid = "BeskidMedia_7410";
//const char* password = "VBLN8SXg";
const char* ssid = "MW40V_8539";
const char* password = "02893406";
const char* mqtt_server = "broker.hivemq.com";

AccelStepper motor(AccelStepper::HALF4WIRE, IN1, IN3, IN2, IN4);

WiFiClient espClient;
PubSubClient client(espClient);

unsigned long lastMsg = 0;
unsigned long ostatnia = 0;
#define MSG_BUFFER_SIZE  (70)
char msg[MSG_BUFFER_SIZE];
char informacja[5];
char wiadomosc[MSG_BUFFER_SIZE];
int stan; 
int licznik = 0;
int aktualnaPozycja = 0;
int temp = 0 ;
int wilgotnosc=0;

//funkcja do łaczenia z wifi
void setup_wifi() {

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }

  //randomSeed(micros()); // wyrzucic

  //Serial.println("WiFi connected"); 
}


void callback(char* topic, byte* payload, unsigned int length) {

  for (int i = 0; i < 5; i++)
  {
     informacja[i]='\0';
  }
  for (int i = 0; i < length; i++) {

    informacja[i]=(char)payload[i];
  }
  //Serial.println(informacja);
 
}

void reconnect() {
 
  while (!client.connected()) {
   // Serial.print("Attempting MQTT connection...");
    // Create a random client ID
   // String clientId = "ESP8266Client-"; // inne id a nie randomowe 
    //clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect("NataliaiAnnaKlientESP")) {
      Serial.println("connected");
      client.subscribe("roletkaProjekt");
    } 
    else {
      //Serial.print("failed, rc=");
      //Serial.print(client.state()); // usunac to z raportu it 
     // Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000); //nw czy tu nie zmienić na wiecej czasu
    }

  }
}

void setup() {

  pinMode(czujnik, INPUT);
  Serial.begin(115200);

  delay(10);
  dht.begin();
 
  motor.setCurrentPosition(0);
  motor.setMaxSpeed(1000);
  motor.setAcceleration(200);
  motor.setSpeed(600); 
  
  while(digitalRead(czujnik)==LOW){
    motor.move(100); 
    motor.run();
    yield();
  }
   
  motor.setCurrentPosition(0);

  stan = STOI;
  
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void loop() {

    if (!client.connected()) {
      reconnect();
    }
  client.loop();

  Serial.println(dht.readTemperature());
  
  if(stan != JEDZ_DOL && dht.readTemperature()>30){
    stan = JEDZ_DOL;
    motor.moveTo(W_DOL); 
  }

  if(stan != JEDZ_GORA && dht.readTemperature()<20){
    stan = JEDZ_GORA;
    motor.moveTo(DO_GORY); 
  }

  if(informacja[0]=='o' && informacja[1]=='n')
  {
        if(stan != JEDZ_GORA){
          stan = JEDZ_GORA; // stan poruszania się do gory
          motor.moveTo(DO_GORY); 
        }
       // else{
     ///     snprintf (msg, MSG_BUFFER_SIZE,"Roller is already going up");
       // }
        
        for (int i = 0; i < 5; i++)
        {
           informacja[i]='\0';
        }
    
  }
    
  if(informacja[0]=='o' && informacja[1]=='f' && informacja[2]=='f')
  {
          if(stan != JEDZ_DOL){
            stan = JEDZ_DOL; // stan poruszania się
            motor.moveTo(W_DOL); 
          }
            for (int i = 0; i < 5; i++)
            {
               informacja[i]='\0';
            }
  }

  if(informacja[0]=='s' && informacja[1]=='t' && informacja[2]=='o' && informacja[3]=='p'){
    
            if(stan != STOI)
            {
              motor.stop();
              stan = STOI;
              aktualnaPozycja=motor.currentPosition()*100/(4*4098);
              snprintf (msg, MSG_BUFFER_SIZE,"Roller stopped, degree of exposure %ld percent", aktualnaPozycja);
            }
            for (int i = 0; i < 5; i++)
            {
               informacja[i]='\0';
            }
  }

  if(stan == JEDZ_GORA){
    jazda_gora();
  }
  if(stan == JEDZ_DOL){
    jazda_dol();
  }

  unsigned long now = millis();
  
  if (now - lastMsg > 1000) {
    
    lastMsg = now;
    client.publish("roletkaprojekcik", msg);
  }

   unsigned long teraz = millis();
  
  if (teraz - ostatnia > 5000) {
    
    ostatnia = teraz;
    temp = dht.readTemperature();
    wilgotnosc=dht.readHumidity();
    snprintf (wiadomosc, MSG_BUFFER_SIZE,"Temperature: %ld C, Humidity: %ld percent RH", temp, wilgotnosc);
    client.publish("czujnikiPomiary", wiadomosc);
  }
}



void jazda_dol(){
  
      motor.run();
      snprintf (msg, MSG_BUFFER_SIZE,"Roller is going down");
      
      if(motor.distanceToGo()==0){
        stan = STOI; 
        aktualnaPozycja=motor.currentPosition()*100/(4*4098);
        snprintf (msg, MSG_BUFFER_SIZE,"Roller's rolled down, degree of exposure %ld percent", aktualnaPozycja);
      }
}

void jazda_gora(){
  
    motor.run();
    snprintf (msg, MSG_BUFFER_SIZE,"Roller is going up");
    
    if(motor.distanceToGo()==0){
      stan = STOI; //rolete zwinieta
      aktualnaPozycja=motor.currentPosition()*100/(4*4098);
      snprintf (msg, MSG_BUFFER_SIZE,"Roller's rolled up, degree of exposure %ld percent", aktualnaPozycja);
    }
}
