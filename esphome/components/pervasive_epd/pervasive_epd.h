#pragma once

#include "esphome/core/component.h"
#include "esphome/components/spi/spi.h"
#include "esphome/components/display/display_buffer.h"
#include "esphome/core/preferences.h"
//#include "../../../esphome/core/component.h"
//#include "../../../esphome/components/spi/spi.h"
//#include "../../../esphome/components/display/display_buffer.h"

// #define USE_CUSTOM_SPI

namespace esphome {
namespace pervasive_epd {

enum Pervasive_EPD_Model {
  EPD_150_KS_0J = 0,
  EPD_152_KS_0J,
  EPD_154_KS_0C,
  EPD_206_KS_0E,
  EPD_213_KS_0E,
  EPD_266_KS_0C,
  EPD_271_KS_09,
  EPD_271_KS_0C,
  EPD_290_KS_0F,
  EPD_370_KS_0C,
  EPD_417_KS_0D,
  EPD_437_KS_0C,
};

class TemplateTextSaverBase {
 public:
  virtual bool save(const std::string &value) { return true; }

  virtual void setup(uint32_t id, std::string &value) {}
};

class COGDataSaver {
 public:
  bool save(uint8_t *data) {
    if (data[0] == prev_0 && data[1] == prev_1)
      return false;
    uint8_t temp[2];
    memcpy(temp, data, 2);
    this->pref_.save(&temp);
    this->prev_0 = data[0];
    this->prev_1 = data[1];
    return true;
  }
  // Make the preference object.  Fill the provided location with the saved data
  // If it is available, else leave it alone
  bool load(uint32_t id, uint8_t *COG_data) {
    this->pref_ = global_preferences->make_preference<uint8_t[2]>(id);

    char temp[2];
    bool hasdata = this->pref_.load(&temp);
    if (hasdata) {
      prev_0 = temp[0];
      prev_1 = temp[1];
      COG_data[0] = prev_0;
      COG_data[1] = prev_1;
      return true;
    }
    return false;
  }

 protected:
  ESPPreferenceObject pref_;
  uint8_t prev_0;
  uint8_t prev_1;
};

class Pervasive_EPD : public display::DisplayBuffer,
                      public spi::SPIDevice<spi::BIT_ORDER_MSB_FIRST, spi::CLOCK_POLARITY_LOW, spi::CLOCK_PHASE_LEADING,
                                            spi::DATA_RATE_4MHZ> {
 public:
  Pervasive_EPD(Pervasive_EPD_Model model);

  void set_dc_pin(GPIOPin *dc_pin) { dc_pin_ = dc_pin; }
  // void set_cs_pin(GPIOPin *cs_pin) { cs_pin_ = cs_pin; }
  float get_setup_priority() const override;
  void set_reset_pin(GPIOPin *reset) { this->reset_pin_ = reset; }
  void set_busy_pin(GPIOPin *busy) { this->busy_pin_ = busy; }
  void set_reset_duration(uint32_t reset_duration) { this->reset_duration_ = reset_duration; }

  void set_value_saver(COGDataSaver *restore_value_saver) { this->pref_ = restore_value_saver; }

  void start_command_();
  void end_command_();
  void start_data_();
  void end_data_();
  void end_command_start_data_();

  void command(uint8_t value);
  void cmd_data(uint8_t cmd, uint8_t value);
  void cmd_data(uint8_t cmd, const uint8_t *data, size_t length);
  void cmd_data_fixed(uint8_t cmd, uint8_t data, size_t length);
  // void cmd_data(const uint8_t *data, size_t length);
  uint8_t readSPI3();
  void writeSPI3(uint8_t value);

  void display();
  void initialize();
  void dump_config() override;
  void deep_sleep();
  bool is_busy();
  void update() override;

  void setup() override;
  void get_COG_data();

  void on_safe_shutdown() override;
  void set_full_update_every(uint32_t full_update_every);

  void fill(Color color) override;

  display::DisplayType get_display_type() override { return display::DisplayType::DISPLAY_TYPE_BINARY; }

  void set_inverted(bool v) { inverted = v; }
  bool inverted = false;
  void set_use_threshold(bool v) { useThreshold = v; }
  bool useThreshold = false;
  void set_threshold(int v) { threshold = v; }
  int threshold = 128;

 protected:
  bool busy_wait(bool state = true);

  void setup_pins_();

  void reset_() {
    if (this->reset_pin_ != nullptr) {
      // this->reset_pin_->digital_write(false);
      // delay(reset_duration_);  // NOLINT
      // this->reset_pin_->digital_write(true);
      // delay(20);

      delay(1);  // Wait for power stabilisation
      this->reset_pin_->digital_write(true);
      delay(5);
      this->reset_pin_->digital_write(false);
      delay(5);
      this->reset_pin_->digital_write(true);
      delay(10);
      this->cs_->digital_write(true);
      delay(20);
    }

    // switch (u_eScreen_EPD)
    //{
    // case eScreen_EPD_150_KS_0J:
    // case eScreen_EPD_152_KS_0J:

    //    if (digitalRead(_busy_pin) == HIGH)
    //    {
    //        Serial.println();
    //        log(LEVEL_CRITICAL, "Incorrect type for 1.52-Wide");
    //        exit(0x01);
    //    }
    //    break;

    // default:

    //    break;
    //}
  }

  int get_width_controller();
  int get_width_internal() override;

  int get_height_internal() override;

  uint32_t reset_duration_{200};

  GPIOPin *reset_pin_{nullptr};
  GPIOPin *dc_pin_;
  // GPIOPin *cs_pin_;
  GPIOPin *busy_pin_{nullptr};
  COGDataSaver *pref_;

  int8_t u_temperature = 0;  // = 25;
  uint8_t COG_data[2];       // OTP
  bool s_flag50;             // Register 0x50
  ulong frameCount = 0UL;
  bool isWaiting = false;
  bool changed = true;
  uint16_t b_delayCS = 50;  // ms

  void draw_absolute_pixel_internal(int x, int y, Color color) override;
  uint32_t get_frame_length_();
  uint32_t get_buffer_length_();

  uint32_t full_update_every_{30};
  uint32_t at_update_{0};
  Pervasive_EPD_Model model_;
  uint32_t idle_timeout_();

  bool deep_sleep_between_updates_{false};
#ifdef USE_CUSTOM_SPI
  SPISettings _settingScreen;
#endif
};

}  // namespace pervasive_epd
}  // namespace esphome
