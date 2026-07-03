#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
#define DHTTYPE DHT11
const char* ssid = "Wallacee Corporation";
const char* password = "";
// PC running Mosquitto + Node-RED
const char* mqtt_server = "10.153.223.113";
int temperature_pin=33;
DHT dht(temperature_pin,DHTTYPE);
#define TRIG_PIN 19
#define ECHO_PIN 18
#define BUZZER_PIN 14
#define SOIL_PIN 32
#define RELAY_PIN 4
WiFiClient espClient;
PubSubClient client(espClient);
bool alerted = false;
float temperature = 0;
float humidity = 0;
int moistureValue = 0;
int waterLevel = 0;
String pumpStatus = "OFF";
void callback(char* topic, byte* payload, unsigned int length)
{
  String msg = "";

  for (int i = 0; i < length; i++)
  {
    msg += (char)payload[i];
  }
  Serial.print("Message: ");
  Serial.println(msg);
  if (String(topic) == "smartfarm/pumpcontrol")
  {
    if (msg == "ON")
    {
      digitalWrite(RELAY_PIN, HIGH);
      pumpStatus = "ON";
    }
    if (msg == "OFF")
    {
      digitalWrite(RELAY_PIN, LOW);
      pumpStatus = "OFF";
    }
  }
}
void reconnect()
{
  while (!client.connected())
  {
    Serial.print("Connecting MQTT...");

    if (client.connect("ESP32_SmartFarm"))
    {
      Serial.println("Connected");

      client.subscribe("smartfarm/pumpcontrol");
    }
    else
    {
      Serial.print("Failed, rc=");
      Serial.println(client.state());

      delay(2000);
    }
  }
}
void setupWiFi()
{
  WiFi.begin(ssid, password);
  Serial.print("Connecting WiFi");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("WiFi Connected");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}
int getWaterLevel()
{
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  float duration = pulseIn(ECHO_PIN, HIGH);
  float distance = duration * 0.0343 / 2;
  int level = 19.21 - distance;
  if(level < 0)
    level = 0;
  return level;
}
void publishData()
{
  client.publish(
    "smartfarm/temperature",
    String(temperature).c_str());
  client.publish(
    "smartfarm/humidity",
    String(humidity).c_str());
  client.publish(
    "smartfarm/moisture",
    String(moistureValue).c_str());
  client.publish(
    "smartfarm/waterlevel",
    String(waterLevel).c_str());
  client.publish(
    "smartfarm/pump",
    pumpStatus.c_str());
  String json =
  "{"
  "\"temperature\":" + String(temperature) + "," +
  "\"humidity\":" + String(humidity) + "," +
  "\"moisture\":" + String(moistureValue) + "," +
  "\"waterlevel\":" + String(waterLevel) + "," +
  "\"pump\":\"" + pumpStatus + "\""
  "}";
  client.publish("smartfarm/data", json.c_str());
}
void setup()
{
  Serial.begin(115200);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);
  dht.begin();
  delay(2000);
  setupWiFi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}
void loop()
{
  if (!client.connected())
  {
    reconnect();
  }
  client.loop();
  waterLevel = getWaterLevel();
  Serial.print("Water Level: ");
  Serial.print(waterLevel);
  Serial.println(" cm");
  if (waterLevel >= 16 && !alerted)
  {
    tone(BUZZER_PIN, 2000);
    delay(1000);
    noTone(BUZZER_PIN);
    alerted = true;
  }
  if (waterLevel < 16)
  {
    alerted = false;
  }
  moistureValue = analogRead(SOIL_PIN);
  Serial.print("Soil Moisture: ");
  Serial.println(moistureValue);
  if (moistureValue >= 4000)
  {
    Serial.println("Soil Dry");
    digitalWrite(RELAY_PIN,LOW);
    pumpStatus = "OFF";
  }
  else if (moistureValue >= 2000)
  {
    Serial.println("Soil Moist");
    digitalWrite(RELAY_PIN,HIGH);
    pumpStatus = "ON";
  }
  else
  {
    Serial.println("Soil Wet");
    digitalWrite(RELAY_PIN, LOW);
    pumpStatus = "OFF";
  }
  humidity = dht.readHumidity();
  temperature = dht.readTemperature();
  Serial.print("Temperature: ");
  Serial.println(temperature);
  Serial.print("Humidity: ");
  Serial.println(humidity);
  publishData();
  delay(5000);
}