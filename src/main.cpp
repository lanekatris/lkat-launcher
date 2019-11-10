#include "secrets.h"
#include "Adafruit_NeoTrellis.h"
#include <WiFi.h>
#include <AsyncMqttClient.h>

extern "C" {
  #include "freertos/FreeRTOS.h"
  #include "freertos/timers.h"
}

Adafruit_NeoTrellis trellis;
AsyncMqttClient mqttClient;

// Input a value 0 to 255 to get a color value.
// The colors are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  if(WheelPos < 85) {
   return trellis.pixels.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  } else if(WheelPos < 170) {
   WheelPos -= 85;
   return trellis.pixels.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  } else {
   WheelPos -= 170;
   return trellis.pixels.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  return 0;
}

//define a callback for key presses
TrellisCallback blink(keyEvent evt){
  // Check is the pad pressed?
  if (evt.bit.EDGE == SEESAW_KEYPAD_EDGE_RISING) {
    trellis.pixels.setPixelColor(evt.bit.NUM, Wheel(map(evt.bit.NUM, 0, trellis.pixels.numPixels(), 0, 255))); //on rising
    Serial.println("Rising");
  } else if (evt.bit.EDGE == SEESAW_KEYPAD_EDGE_FALLING) {
  // or is the pad released?
    trellis.pixels.setPixelColor(evt.bit.NUM, 0); //off falling
    Serial.println("Falling: " + evt.bit.NUM);
    Serial.println(evt.bit.NUM);

    // No reason to zero index, and there was an issue with 0 not being passed to mqtt
    String s = String(evt.bit.NUM + 1);
    char msg[10];
    s.toCharArray(msg, 10);

    mqttClient.publish(MQTT_TOPIC, 0, false, msg);
  }

  // Turn on/off the neopixels!
  trellis.pixels.show();

  return 0;
}

void onMqttConnect(bool sessionPresent) {
  Serial.println("Connected to MQTT");
}

void setup() {
  Serial.begin(115200);
  
  // LED Setup
  pinMode(BUILTIN_LED, OUTPUT);
  digitalWrite(BUILTIN_LED, HIGH);

  // Connect to wifi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  
  // Setup pubnub
  mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
  String clientId = String(PUBLISH_KEY) + "/" + SUBSCRIBE_KEY + "/" + MQTT_TOPIC;

  char clientIdArr[250];
  clientId.toCharArray(clientIdArr, 250);
  mqttClient.setClientId(clientIdArr);
  mqttClient.onConnect(onMqttConnect);
  mqttClient.connect();
  
  // Setup neotrellis
  if (!trellis.begin()) {
    Serial.println("Could not start trellis, check wiring?");
    while(1);
  } else {
    Serial.println("NeoPixel Trellis started");
  }

  //activate all keys and set callbacks
  for(int i=0; i<NEO_TRELLIS_NUM_KEYS; i++){
    trellis.activateKey(i, SEESAW_KEYPAD_EDGE_RISING);
    trellis.activateKey(i, SEESAW_KEYPAD_EDGE_FALLING);
    trellis.registerCallback(i, blink);
  }

  //do a little animation to show we're on
  for (uint16_t i=0; i<trellis.pixels.numPixels(); i++) {
    trellis.pixels.setPixelColor(i, Wheel(map(i, 0, trellis.pixels.numPixels(), 0, 255)));
    trellis.pixels.show();
    delay(50);
  }
  for (uint16_t i=0; i<trellis.pixels.numPixels(); i++) {
    trellis.pixels.setPixelColor(i, 0x000000);
    trellis.pixels.show();
    delay(50);
  }
}

void loop() {
  trellis.read();  // interrupt management does all the work! :)
  delay(20); //the trellis has a resolution of around 60hz
}
