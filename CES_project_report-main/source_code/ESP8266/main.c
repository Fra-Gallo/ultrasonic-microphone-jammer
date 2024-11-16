#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Arduino.h>
#include <arduino_homekit_server.h>
#include <wifi_info.h>
const char *mqtt_server = "broker.hivemq.com";
WiFiClient espClient;
PubSubClient client(espClient);
#define PIN_LED 0

void callback(char *topic, byte *payload, unsigned int length)
{
    Serial.print("Message received on topic: ");
    Serial.println(topic);

    String payloadStr;
    for (unsigned int i = 0; i < length; i++)
    {
        payloadStr += (char)payload[i];
    }

    if (strcmp(topic, "CosFraGia/JammerProject") == 0)
    {
        if (payloadStr == "Accendi")
        {
            digitalWrite(PIN_LED, HIGH);            // Turn on
            cha_switch_on.value.bool_value = false; // Update HomeKit state
        }
        else if (payloadStr == "Spegni")
        {
            digitalWrite(PIN_LED, LOW);            // Turn off
            cha_switch_on.value.bool_value = true; // Update HomeKit state
        }
        homekit_characteristic_notify(&cha_switch_on, cha_switch_on.value); // Notify HomeKit
    }
}

void reconnect()
{
    while (!client.connected())
    {
        if (client.connect("ArduinoClient"))
        {
            client.subscribe("CosFraGia/JammerProject");
        }
        else
        {
            delay(5000);
        }
    }
}

void setup_wifi()
{
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
    }
    Serial.println("WiFi connected");
    Serial.println("IP Address: ");
    Serial.println(WiFi.localIP());
}

void setup()
{
    Serial.begin(115200);
    pinMode(PIN_LED, OUTPUT);
    wifi_connect();
    my_homekit_setup();
    client.setServer(mqtt_server, 1883);
    client.setCallback(callback);
}

void loop()
{
    my_homekit_loop();
    if (!client.connected())
    {
        reconnect();
    }
    client.loop();
    delay(10);
}

//==============================
// HomeKit setup and loop
//==============================

// access your HomeKit characteristics defined in my_accessory.c
extern "C" homekit_server_config_t config;
static uint32_t next_heap_millis = 0;

#define PIN_SWITCH 0

// Called when the switch value is changed by iOS Home APP
void cha_switch_on_setter(const homekit_value_t value)
{
    bool on = value.bool_value;
    cha_switch_on.value.bool_value = on; // sync the value
    LOG_D("Switch: %s", on ? "ON" : "OFF");
    digitalWrite(PIN_SWITCH, on ? LOW : HIGH);
}

void my_homekit_setup()
{
    pinMode(PIN_SWITCH, OUTPUT);
    digitalWrite(PIN_SWITCH, HIGH);

    // Add the .setter function to get the switch-event sent from iOS Home APP.
    // The .setter should be added before arduino_homekit_setup.
    // HomeKit sever uses the .setter_ex internally, see homekit_accessories_init function.
    // Maybe this is a legacy design issue in the original esp-homekit library,
    // and I have no reason to modify this "feature".
    cha_switch_on.setter = cha_switch_on_setter;
    arduino_homekit_setup(&config);

    // report the switch value to HomeKit if it is changed (e.g. by a physical button)
    // bool switch_is_on = true/false;
    // cha_switch_on.value.bool_value = switch_is_on;
    // homekit_characteristic_notify(&cha_switch_on, cha_switch_on.value);
}

void my_homekit_loop()
{
    arduino_homekit_loop();

    const uint32_t t = millis();
    if (t > next_heap_millis)
    {
        // show heap info every 5 seconds
        next_heap_millis = t + 5 * 1000;
        LOG_D("Free heap: %d, HomeKit clients: %d",
              ESP.getFreeHeap(), arduino_homekit_connected_clients_count());
    }
}
