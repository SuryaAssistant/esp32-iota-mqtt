//=============================================================================
// Upload Temperature and Humidity data from DHT11 to 
// IOTA Tangle using ESP32
//
// Developer : SuryaAssistant / Fandi Adinata
//             (https://github.com/SuryaAssistant)
// License   : MIT License
//=============================================================================


//=============================================================================
// Library
//=============================================================================
// REQUIRES the following Arduino libraries:
// - DHT Sensor Library: https://github.com/adafruit/DHT-sensor-library
// - Adafruit Unified Sensor Lib: https://github.com/adafruit/Adafruit_Sensor
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>

#include <WiFi.h>
#include <PubSubClient.h>

//=============================================================================
// WiFi Credential
//=============================================================================
const char* SSID = "your_wifi_ssid";
const char* PASS = "your_wifi_pass";

//=============================================================================
// DHT11
//=============================================================================
#define DHTPIN 15
#define DHTTYPE    DHT11
DHT_Unified dht(DHTPIN, DHTTYPE);

float temperature;
float humidity;

//=============================================================================
// MQTT Configuration
//=============================================================================
const char *mqtt_broker = "test.mosquitto.org";
const char *topic = "surya_gateway/submit";
const char *returnTopic = "esp32dht11";
const char *subscribeTopic = "surya_gateway/esp32dht11";
const int mqtt_port = 1883;

WiFiClient espClient;
PubSubClient client(espClient);

uint32_t interval = 300000;               // 5 minute interval
uint32_t lastCheck = 0;
#define LED 2


void setup() {
  Serial.begin(115200);

  // initialize DHT
  dht.begin();
  sensor_t sensor;
  dht.temperature().getSensor(&sensor);

  // start WiFi Connection
  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID, PASS);
  pinMode(LED, OUTPUT);

  // wait to connect
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(250);
    digitalWrite(LED, HIGH);
    delay(250);
    digitalWrite(LED, LOW);
    delay(250);
    digitalWrite(LED, HIGH);
    delay(250);
    digitalWrite(LED, LOW);
  }

  // connect MQTT
  client.setServer(mqtt_broker, mqtt_port);
  client.setCallback(callback);
  connectMQTT();

  // config NTP for timestamp
  configTime(7*3600, 0, "pool.ntp.org");
}

void loop() {
  // Auto reconnect Wifi
  if(WiFi.status() != WL_CONNECTED){
    WiFi.disconnect();
    WiFi.reconnect();
  }


  // if mqtt disconnected, reconnect
  // see function at bottom
  connectMQTT();

  // Read sensor every interval and upload
  if(millis() - lastCheck >= interval){
    lastCheck = millis();

    sensors_event_t event;
    dht.temperature().getEvent(&event);
    temperature = (float)event.temperature;
    dht.humidity().getEvent(&event);
    humidity = (float)event.relative_humidity;

    Serial.print(temperature);
    Serial.print("--");
    Serial.println(humidity);

    // get timestamp
    time_t now = time(nullptr);
    while (now < 24 * 3600)
    {
      Serial.print(".");
      delay(100);
      now = time(nullptr);
    }

    String client_id = "esp32-client-";
    client_id += String(WiFi.macAddress());

    // manually construct JSON format
    String mqtt_data = "{'node':'";
    mqtt_data += client_id;
    mqtt_data += "',";
    mqtt_data += "'nodeTimestamp':";
    mqtt_data += String(now);
    mqtt_data += ",";
    mqtt_data += "'temperature':";
    mqtt_data += String(temperature,2);
    mqtt_data += ",";
    mqtt_data += "'humidity':";
    mqtt_data += String(humidity,2);
    mqtt_data += "}/";
    mqtt_data += returnTopic;
    Serial.println(mqtt_data);
    client.publish(topic, mqtt_data.c_str(), false);
  }

  // loop mqtt
  client.loop();
}



void connectMQTT(){
  if (!client.connected()) {
    String client_id = "esp32-client-";
    client_id += String(WiFi.macAddress());
    Serial.printf("The client %s connects to the public mqtt broker\n", client_id.c_str());
    Serial.println("");
    if (client.connect(client_id.c_str())) {
      Serial.println("Connected to test.mosquitto.org");
    }
    else {
      Serial.print("Connect failed with state ");
      Serial.print(client.state());
      Serial.println(". Try again in 5 second");
      delay(5000);
    }
  }

  // subscribe for return IOTA address
  client.subscribe(subscribeTopic);
}


void callback(char *topic, byte *payload, unsigned int length) {
  String newmqtt;
  for (uint8_t i = 0; i < length; i++) {
    newmqtt += (char) payload[i];
  }

  Serial.print("New IOTA Msg ID : ");
  Serial.println(newmqtt);
}
