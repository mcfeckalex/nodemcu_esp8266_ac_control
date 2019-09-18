// Get ESP8266 going with Arduino IDE
// - https://github.com/esp8266/Arduino#installing-with-boards-manager
// Required libraries (sketch -> include library -> manage libraries)
// - PubSubClient by Nick ‘O Leary
// - DHT sensor library by Adafruit

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <MitsubishiHeatpumpIR.h>

#define wifi_ssid       "HumleBo"
#define wifi_password   "HumleBo2014!"

#define mqtt_server     "192.168.1.111"
#define mqtt_port       1883
#define mqtt_user       "albe"
#define mqtt_password   "albe"

#define power_command_topic       "house/ac/power/set"
#define mode_command_topic        "house/ac/mode/set"
#define temperature_command_topic "house/ac/temperature/set"
#define fan_mode_command_topic    "house/ac/fan/set"
#define swing_mode_command_topic  "house/ac/swing/set"

#define INVALID_CMD       "OFF"

#define MODE_OFF          "off"
#define MODE_AUTO         "auto"
#define MODE_COOL         "cool"
#define MODE_HEAT         "heat"

#define FAN_AUTO          "auto"
#define FAN_QUIET         "quiet"
#define FAN_LOW           "low"
#define FAN_MEDIUM        "medium"
#define FAN_HIGH          "high"
#define FAN_XTRA_HIGH     "extra high"


#define PIN_IR 0


WiFiClient espClient;
PubSubClient client(espClient);
IRSenderBitBang irSender(PIN_IR);

MitsubishiHeatpumpIR  *heatpumpIR = new MitsubishiFDHeatpumpIR();

unsigned int powerModeCmd;
unsigned int operatingModeCmd;
unsigned int fanSpeedCmd;
unsigned int temperatureCmd;
unsigned int swingVCmd; 
unsigned int swingHCmd;


void setup_wifi() {
  delay(2);
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
  Serial.println("WiFi connected!");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void setup_mqtt() {
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(mqtt_callback);
 
  while (!client.connected()) {
    Serial.println("Connecting to MQTT...");
 
    if (client.connect("ESP8266AC_control_client", mqtt_user, mqtt_password)) {
 
      Serial.println("...Connected");  
 
    } else {
 
      Serial.print("failed with state ");
      Serial.print(client.state());
      delay(2000);
 
    }
  }
 
  client.subscribe(power_command_topic);
  client.subscribe(mode_command_topic);
  client.subscribe(temperature_command_topic);
  client.subscribe(fan_mode_command_topic);
  client.subscribe(swing_mode_command_topic);
}

void setup_gpio() {
   // Set pir pin as input
  pinMode(PIN_IR, OUTPUT);

  pinMode(LED_BUILTIN, OUTPUT);

}

void setup_ir()
{
  powerModeCmd     = MITSUBISHI_AIRCON1_MODE_ON;
  operatingModeCmd = MITSUBISHI_AIRCON1_MODE_HEAT;
  fanSpeedCmd      = MITSUBISHI_AIRCON1_FAN2;
  temperatureCmd   = 20;
  swingVCmd        = MITSUBISHI_AIRCON1_VS_DOWN;
  swingHCmd        = MITSUBISHI_AIRCON1_HS_MIDDLE;
}

void setup() {
  setup_gpio();
  Serial.begin(115200);
  // Wait for serial to initialize.
  while (!Serial) { }
  Serial.println("Device Started");
  Serial.println("-------------------------------------");
  Serial.print("Running setup..");
  setup_wifi();
  setup_mqtt();
  setup_ir();
  toggle_led(100, 5);
}

int mqtt_callback(char* topic, byte* payload, unsigned int length) {
 
  char payload_string[length+1];

  bool valid_command;

  Serial.print("payload is: ");
  Serial.print(length);
  Serial.print(" bytes long");
  Serial.println("");

 valid_command =false;
  snprintf(payload_string, length+1, "%s", payload);
  
  if (strncmp(payload_string, INVALID_CMD, sizeof(INVALID_CMD)) == 0) {
    return -1;
  }

  if (strncmp(topic, mode_command_topic, sizeof(mode_command_topic)) == 0) {
    Serial.print("Mode set to:");
    Serial.println(payload_string);
    if (strncmp(payload_string, MODE_AUTO, sizeof(MODE_AUTO)) == 0) {
      operatingModeCmd = MITSUBISHI_AIRCON1_MODE_AUTO;
      powerModeCmd = MITSUBISHI_AIRCON1_MODE_ON;
      valid_command = true;
    } else if (strncmp(payload_string, MODE_COOL, sizeof(MODE_COOL)) == 0) {
      operatingModeCmd = MITSUBISHI_AIRCON1_MODE_COOL;
      powerModeCmd = MITSUBISHI_AIRCON1_MODE_ON;
      valid_command = true;
    } else if (strncmp(payload_string, MODE_HEAT, sizeof(MODE_HEAT)) == 0) {
      operatingModeCmd = MITSUBISHI_AIRCON1_MODE_HEAT;
      powerModeCmd = MITSUBISHI_AIRCON1_MODE_ON;
      valid_command = true;
    } else if (strncmp(payload_string, MODE_OFF, sizeof(MODE_OFF)) == 0) {
      powerModeCmd = MITSUBISHI_AIRCON1_MODE_OFF;
      valid_command = true;
    }
  }

  else if (strncmp(topic, fan_mode_command_topic, sizeof(fan_mode_command_topic)) == 0) {
    Serial.print("Fan set to:");
    Serial.println(payload_string);
    if (strncmp(payload_string, FAN_AUTO, sizeof(FAN_AUTO)) == 0) {
      fanSpeedCmd = MITSUBISHI_AIRCON1_FAN_AUTO;
      valid_command = true;
    } else if (strncmp(payload_string, FAN_LOW, sizeof(FAN_LOW)) == 0) {
      fanSpeedCmd = MITSUBISHI_AIRCON1_FAN1;
      valid_command = true;
    } else if (strncmp(payload_string, FAN_MEDIUM, sizeof(FAN_MEDIUM)) == 0) {
      fanSpeedCmd = MITSUBISHI_AIRCON1_FAN2;
      valid_command = true;
    } else if (strncmp(payload_string, FAN_HIGH, sizeof(FAN_HIGH)) == 0) {
      fanSpeedCmd = MITSUBISHI_AIRCON1_FAN3;
      valid_command = true;
    } else if (strncmp(payload_string, FAN_XTRA_HIGH, sizeof(FAN_XTRA_HIGH)) == 0) {
      fanSpeedCmd = MITSUBISHI_AIRCON1_FAN4;
      valid_command = true;
    }
  }

  
  else if (strncmp(topic, temperature_command_topic, sizeof(temperature_command_topic)) == 0) {
      Serial.println("Temperature set to:");
      Serial.println(atof(payload_string));
      temperatureCmd = atof(payload_string);
      valid_command = true;
  }
  Serial.println("Command valid:");
  Serial.println(valid_command);
  if (valid_command) {
    Serial.println("Valid command, sending! ");
    heatpumpIR->send(irSender,
       powerModeCmd, 
       operatingModeCmd, 
       fanSpeedCmd, 
       temperatureCmd, 
       swingVCmd, 
       swingHCmd);
    toggle_led(50,10);
  }
  return 0;
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

void loop() {  
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
}

void toggle_led(int ms, int times) {
  int times_left = 0;
  while (times_left < times) {
    digitalWrite(LED_BUILTIN, HIGH);   
    delay(ms);                      // Wait for a second
    digitalWrite(LED_BUILTIN, LOW);
    delay(ms); 
    times_left++;
  }
}
