#include "Matter.h"
#include "Arduino.h"
#include <app/server/OnboardingCodesUtil.h>
#include <credentials/examples/DeviceAttestationCredsExample.h>
using namespace chip;
using namespace chip::app::Clusters;
using namespace esp_matter;
using namespace esp_matter::endpoint;

/**
 * This program presents example Matter light device with OnOff cluster by
 * controlling LED with Matter and toggle button with two endpoints.
 *
 * If your ESP does not have buildin LED, please connect it to LED_PIN
 *
 * You can toggle light by both:
 *  - Matter (via CHIPTool or other Matter controller)
 *  - toggle button (by default attached to GPIO0 - reset button, with debouncing) 
 */

// Please configure your PINs
const int GPIO_2 = 2;
const int GPIO_5 = 5;

//
#define STANDARD_BRIGHTNESS 255
#define MATTER_BRIGHTNESS 254

// Debounce for toggle button
const int DEBOUNCE_DELAY = 5000;
int last_toggle;

// Endpoint and attribute ref that will be assigned to Matter device
uint16_t light_endpoint_id_1 = 0;
uint16_t light_endpoint_id_2 = 0;

// There is possibility to listen for various device events, related for example
// to setup process. Leaved as empty for simplicity.
static void on_device_event(const ChipDeviceEvent *event, intptr_t arg) {}
static esp_err_t on_identification(identification::callback_type_t type,
                                   uint16_t endpoint_id, uint8_t effect_id,
                                   uint8_t effect_variant, void *priv_data) {
  return ESP_OK;
}

// Listener on attribute update requests.
// In this example, when update is requested, path (endpoint, cluster and
// attribute) is checked if it matches light attribute. If yes, LED changes
// state to new one.
static esp_err_t on_attribute_update(attribute::callback_type_t type,
                                     uint16_t endpoint_id, uint32_t cluster_id,
                                     uint32_t attribute_id,
                                     esp_matter_attr_val_t *val,
                                     void *priv_data) {
  if (endpoint_id == light_endpoint_id_1) {
    if (type == attribute::PRE_UPDATE) {
      // We got an light on/off attribute update!
      if (cluster_id == OnOff::Id && attribute_id == OnOff::Attributes::OnOff::Id) {
        if (val->val.b == true) {
          digitalWrite(GPIO_5, true);
          delay(300);
          digitalWrite(GPIO_5, false);
        }
        else if (val->val.b == false) {
          digitalWrite(GPIO_2, true);
          delay(300);
          digitalWrite(GPIO_2, false);
        }
      }
    }
  }
  else if (endpoint_id == light_endpoint_id_2) {
    if (type == attribute::PRE_UPDATE) {
      if (cluster_id == LevelControl::Id && attribute_id == LevelControl::Attributes::CurrentLevel::Id) {
        int brt = REMAP_TO_RANGE(val->val.u8, MATTER_BRIGHTNESS, STANDARD_BRIGHTNESS);
        if (brt > 200) {
          digitalWrite(GPIO_5, true);
          delay(300);
          digitalWrite(GPIO_5, false);
        }
        else if (brt < 50) {
          digitalWrite(GPIO_2, true);
          delay(300);
          digitalWrite(GPIO_2, false);
        }
      }
    }
  }
  return ESP_OK;
}

void setup() {
  Serial.begin(115200);
  pinMode(GPIO_2, OUTPUT);
  pinMode(GPIO_5, OUTPUT);
  digitalWrite(GPIO_2, false);
  digitalWrite(GPIO_5, false);

  // Enable debug logging
  esp_log_level_set("*", ESP_LOG_DEBUG);

  // Setup Matter node
  node::config_t node_config;
  node_t *node = node::create(&node_config, on_attribute_update, on_identification);

  // Setup Plugin unit endpoint / cluster / attributes with default values
  color_temperature_light::config_t light_config;
  light_config.on_off.on_off = false;
  light_config.on_off.lighting.start_up_on_off = false;
  endpoint_t *endpoint_1 = color_temperature_light::create(node, &light_config,
                                                      ENDPOINT_FLAG_NONE, NULL);
  endpoint_t *endpoint_2 = color_temperature_light::create(node, &light_config,
                                                      ENDPOINT_FLAG_NONE, NULL);

  // Save generated endpoint id
  light_endpoint_id_1 = endpoint::get_id(endpoint_1);
  light_endpoint_id_2 = endpoint::get_id(endpoint_2);

  // Setup DAC (this is good place to also set custom commission data, passcodes etc.)
  esp_matter::set_custom_dac_provider(chip::Credentials::Examples::GetExampleDACProvider());

  // Start Matter device
  esp_matter::start(on_device_event);

  // Print codes needed to setup Matter device
  PrintOnboardingCodes(chip::RendezvousInformationFlags(chip::RendezvousInformationFlag::kBLE));
}

// When toggle light button is pressed (with debouncing),
// light attribute value is changed
void loop() {
  if ((millis() - last_toggle) > DEBOUNCE_DELAY) {
    last_toggle = millis();
    PrintOnboardingCodes(chip::RendezvousInformationFlags(chip::RendezvousInformationFlag::kBLE));
  }
}
