#include <WiFi.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"

/************************* WiFi Access Point ***************************/
#define WIFI_SSID       "Wokwi-GUEST"
#define WIFI_PASS       ""

/************************* Adafruit.io Setup ***************************/
#define AIO_SERVER      "io.adafruit.com"
#define AIO_SERVERPORT  1883
#define IO_USERNAME     "iandionisio"

#define TRIG_PIN 13
#define ECHO_PIN 12
#define LED_PIN  33
#define BUZZ_PIN 26

bool alarmeAtivado = false;


// Create an ESP8266 WiFiClient class to connect to the MQTT server.
WiFiClient client;

// Setup the MQTT client class by passing in the WiFi client and MQTT server and login details.
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, IO_USERNAME, IO_KEY);

/****************************** Feeds ***************************************/
Adafruit_MQTT_Subscribe onoffbtn = Adafruit_MQTT_Subscribe(&mqtt, IO_USERNAME "/feeds/status_alarme");
Adafruit_MQTT_Publish   pubDistancia = Adafruit_MQTT_Publish(&mqtt, IO_USERNAME "/feeds/movimento");

void MQTT_connect() {
  int8_t ret;
  if (mqtt.connected()) return;

  Serial.print("Conectando ao MQTT... ");
  uint8_t retries = 3;
  while ((ret = mqtt.connect()) != 0) {
    Serial.println(mqtt.connectErrorString(ret));
    Serial.print("Tentando novamente em 10s...");
    mqtt.disconnect();
    delay(10000);
    retries--;
    if (retries == 0) {
      while (1);
    }
  }
  Serial.println("Conectado ao MQTT!");
  mqtt.subscribe(&onoffbtn);
}

void connectWiFi() {
  Serial.print("Conectando ao Wifi...");
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi conectado.");
  Serial.println("IP: "); Serial.println(WiFi.localIP());
}

void onoffcallback(char *data, uint16_t len) {
  Serial.print("Callback recebido. Valor do botão: ");
  Serial.println(data);
  if (strcmp(data, "ON") == 0) {
    Serial.println("Ligando o alarme");
    alarmeAtivado = true;
  } 
  if(strcmp(data, "OFF") == 0) {
    Serial.println("Desligando o alarme");
    alarmeAtivado = false;
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZ_PIN, OUTPUT);

  connectWiFi();
  MQTT_connect();

  onoffbtn.setCallback(onoffcallback);
  mqtt.subscribe(&onoffbtn);

  mqtt.processPackets(1000);

  Serial.println("Estado inicial do alarme: ");
  Serial.println("Alarme desativado por padrão."); // por padão é OFF (sem comandos ON/OFF)
}

float medirDistanciaCM() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duracao = pulseIn(ECHO_PIN, HIGH);
  float distancia = duracao * 0.034 / 2;
  return distancia;
}

void loop() {
  MQTT_connect();

  if (!mqtt.ping()) {
    mqtt.disconnect();
  }

  Adafruit_MQTT_Subscribe *subscription;
  while ((subscription = mqtt.readSubscription(1000))) {
    if (subscription == &onoffbtn) {
      String v = (char*)subscription->lastread;
      Serial.print("Alarme recebido: ");
      Serial.println(v);
      alarmeAtivado = (v == "ON");
    }
  }

  float distancia = medirDistanciaCM();
  Serial.print("Distância medida: ");
  Serial.print(distancia);
  Serial.println(" cm");

 if (alarmeAtivado) {
  // Se houver movimento na proximidade de 100cm
  if (distancia < 100.0) {
    digitalWrite(LED_PIN, HIGH);
    
    tone(BUZZ_PIN, 1000);
    delay(200);
    noTone(BUZZ_PIN);
    delay(400);           

    Serial.println("Alarme ativado por movimento.");
  } else {
    digitalWrite(LED_PIN, LOW);
    noTone(BUZZ_PIN);
    Serial.println("Nenhum movimento detectado.");
  }
} else {
  digitalWrite(LED_PIN, LOW);
  noTone(BUZZ_PIN);
  Serial.println("Alarme desligado pelo usuário OU por padrão.");
}

  pubDistancia.publish(String(distancia).c_str());

  delay(1000);
}
