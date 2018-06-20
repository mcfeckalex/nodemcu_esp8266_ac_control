// Get ESP8266 going with Arduino IDE
// - https://github.com/esp8266/Arduino#installing-with-boards-manager
// Required libraries (sketch -> include library -> manage libraries)
// - PubSubClient by Nick ‘O Leary
// - DHT sensor library by Adafruit

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>

#define wifi_ssid "HumleBo"
#define wifi_password "HumleBo2014!"

#define mqtt_server "192.168.1.111"
#define mqtt_user "albe"
#define mqtt_password "albe"

#define humidity_topic "home-assistant/forradet_ute_hum"
#define temperature_topic "home-assistant/forradet_ute_temp"
#define pir_topic "home-assistant/forradet/pir/state"

#define DHTTYPE DHT22
#define DHTPIN  4

#define PIR_PIN 5


WiFiClient espClient;
PubSubClient client(espClient);
DHT dht(DHTPIN, DHTTYPE, 11); // 11 works fine for ESP8266

void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(wifi_ssid);

  WiFi.begin(wifi_ssid, wifi_password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void setup() {
  Serial.begin(115200);
  // Wait for serial to initialize.
  while (!Serial) { }
  Serial.println("Device Started");
  Serial.println("-------------------------------------");
  Serial.print("Running setup..");
  // Set pir pin as input
  pinMode(PIR_PIN, INPUT);

  pinMode(LED_BUILTIN, OUTPUT);

  
  dht.begin();
  setup_wifi();
  client.setServer(mqtt_server, 1883);
}



void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    // If you do not want to use a username and password, change next line to
    if (client.connect("ESP8266Client")) {
    //if (client.connect("ESP8266Client", mqtt_user, mqtt_password)) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

bool checkBound(float newValue, float prevValue, float maxDiff) {
  return !isnan(newValue) &&
         (newValue < prevValue - maxDiff || newValue > prevValue + maxDiff);
}

long lastMsg = 0;
float temp = 0.0;
float hum = 0.0;
float diff = 1.0;
bool pir_high = false;
void loop() {
  
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  digitalRead(PIR_PIN);
  long now = millis();
  if (now - lastMsg > 1500) {
    lastMsg = now;
    
    float newTemp = dht.readTemperature();
    float newHum = dht.readHumidity();

    if (checkBound(newTemp, temp, diff)) {
      temp = newTemp;
      Serial.print("New temperature:");
      Serial.println(String(temp).c_str());
      client.publish(temperature_topic, String(temp).c_str(), true);
    }

    if (checkBound(newHum, hum, diff)) {
      hum = newHum;
      Serial.print("New humidity:");
      Serial.println(String(hum).c_str());
      client.publish(humidity_topic, String(hum).c_str(), true);
    }
    digitalWrite(LED_BUILTIN, LOW);   
    delay(250);                      // Wait for a second
    digitalWrite(LED_BUILTIN, HIGH);
    delay(250);
    digitalWrite(LED_BUILTIN, LOW);   
    delay(250);                      // Wait for a second
    digitalWrite(LED_BUILTIN, HIGH);
  }
  else if (digitalRead(PIR_PIN)) {
    
    if (!pir_high){
      pir_high =true;
      Serial.print("PIR triggered, publishing to HASSIO\r\n");
      client.publish(pir_topic, "on", true);
      digitalWrite(LED_BUILTIN, LOW);   
      delay(1000);                      // Wait for a second
      digitalWrite(LED_BUILTIN, HIGH);
      }
    
  }
  else if (!digitalRead(PIR_PIN))
  {
    if (pir_high){
      pir_high=false;
      Serial.print("PIR goes low, publishing to HASSIO\r\n");
      client.publish(pir_topic, "off", true);
      
    }
  }
}
