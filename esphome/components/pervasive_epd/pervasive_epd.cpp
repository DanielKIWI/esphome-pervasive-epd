#include "pervasive_epd.h"
#include <bitset>
#include <cinttypes>
#include "esphome/core/application.h"
#include "esphome/core/helpers.h"
#include "esphome/core/log.h"
#include "SPI.h"

namespace esphome {
namespace pervasive_epd {

static const char *const TAG = "pervasive_epd";

// clang-format on

Pervasive_EPD::Pervasive_EPD(Pervasive_EPD_Model model) : model_(model) {}

void Pervasive_EPD::setup() {
  this->init_internal_(this->get_buffer_length_());
  this->fill(Color::WHITE);
  this->setup_pins_();
  this->reset_();
  // Below causes spi to no longer work afterwards
  this->spi_teardown();
  // ESP_LOGI(TAG, "initialize");
  this->initialize();
  // ESP_LOGI(TAG, "COG_data: 0x%02x, 0x%02x", COG_data[0], COG_data[1]);  // COG_data: 0x0f, 0x0e
  //   ESP_LOGI(TAG, "pinMode MOSI back from spi3");
  //   pinMode(MOSI, OUTPUT);
  // End

  // spiAttachSCK(_spi, SCK);
  // spiAttachMOSI(_spi, MOSI);
  // spiAttachMISO(_spi, MISO);

  // ESP_LOGI(TAG, "spi setup");
  this->spi_setup();

  // ESP_LOGI(TAG, "pins: cs: %i %i, dc: %i %i, reset: %i %i, busy: %i %i", ((InternalGPIOPin *) this->cs_)->get_pin(),
  //          ((InternalGPIOPin *) this->cs_)->is_inverted(), ((InternalGPIOPin *) this->dc_pin_)->get_pin(),
  //          ((InternalGPIOPin *) this->dc_pin_)->is_inverted(), ((InternalGPIOPin *) this->reset_pin_)->get_pin(),
  //          ((InternalGPIOPin *) this->reset_pin_)->is_inverted(), ((InternalGPIOPin *) this->busy_pin_)->get_pin(),
  //          ((InternalGPIOPin *) this->busy_pin_)->is_inverted());
  // ESP_LOGI(TAG, "setup done. Screen size: %i x %i", get_width(), get_height());
}
void Pervasive_EPD::setup_pins_() {
  this->dc_pin_->setup();  // OUTPUT
  this->dc_pin_->digital_write(true);
  this->cs_->setup();  // OUTPUT
  this->cs_->digital_write(true);
  if (this->reset_pin_ != nullptr) {
    this->reset_pin_->setup();  // OUTPUT
    this->reset_pin_->digital_write(true);
  }
  if (this->busy_pin_ != nullptr) {
    this->busy_pin_->setup();  // INPUT
  }
}
float Pervasive_EPD::get_setup_priority() const { return setup_priority::PROCESSOR; }

void Pervasive_EPD::start_command_() {
  this->dc_pin_->digital_write(false);
  this->enable();  // this->cs_pin_->digital_write(false);
}
void Pervasive_EPD::end_command_() { this->disable(); }
void Pervasive_EPD::start_data_() {
  this->dc_pin_->digital_write(true);
  this->enable();  // this->cs_pin_->digital_write(false);
}
void Pervasive_EPD::end_command_start_data_() { this->dc_pin_->digital_write(true); }
void Pervasive_EPD::end_data_() { this->disable(); }

void Pervasive_EPD::command(uint8_t value) {
  // ESP_LOGI(TAG, "command 0x%02x", value);
  this->start_command_();
  this->write_byte(value);
  this->end_command_();
}
void Pervasive_EPD::cmd_data(uint8_t cmd, uint8_t data) {
  // ESP_LOGI(TAG, "cmd_data 0x%02x - 0x%02x", cmd, data);
  this->start_command_();
  this->write_byte(cmd);
  this->end_command_start_data_();
  this->write_byte(data);
  this->end_data_();
}
void Pervasive_EPD::cmd_data(uint8_t cmd, const uint8_t *data, size_t size) {
  // ESP_LOGI(TAG, "cmd_data 0x%02x - [%i]", cmd, size);
  // if (size == 2)
  //   ESP_LOGI(TAG, "[0x%02x, 0x%02x]", data[0], data[1]);

  this->start_command_();
  delayMicroseconds(b_delayCS);
  this->write_byte(cmd);
  // SPI.transfer(cmd);
  delayMicroseconds(b_delayCS);
  this->end_command_start_data_();
  // this->end_command_();
  // this->start_data_();
  delayMicroseconds(b_delayCS);
  for (size_t i = 0; i < size; i++) {
    this->write_byte(data[i]);
  }
  delayMicroseconds(b_delayCS);
  this->end_data_();
  delayMicroseconds(b_delayCS);
}
void Pervasive_EPD::cmd_data_fixed(uint8_t cmd, uint8_t data, size_t size) {
  // ESP_LOGI(TAG, "cmd_data_fixed 0x%02x - 0x%02x [%i]", cmd, data, size);
  this->start_command_();
  delayMicroseconds(b_delayCS);
  this->write_byte(cmd);
  delayMicroseconds(b_delayCS);
  this->end_command_start_data_();
  delayMicroseconds(b_delayCS);
  for (size_t i = 0; i < size; i++) {
    this->write_byte(data);
  }
  delayMicroseconds(b_delayCS);
  this->end_data_();
  delayMicroseconds(b_delayCS);
}

#define SCK 18
#define MOSI 23
uint8_t Pervasive_EPD::readSPI3() {
  // return this->read_byte();

  uint8_t value = 0;

  pinMode(SCK, OUTPUT);
  pinMode(MOSI, INPUT);

  for (uint8_t i = 0; i < 8; ++i) {
    digitalWrite(SCK, HIGH);
    delayMicroseconds(1);
    value |= digitalRead(MOSI) << (7 - i);
    digitalWrite(SCK, LOW);
    delayMicroseconds(1);
  }
  return value;
}

void Pervasive_EPD::writeSPI3(uint8_t value) {
  // return this->write_byte(value);
  pinMode(SCK, OUTPUT);
  pinMode(MOSI, OUTPUT);

  for (uint8_t i = 0; i < 8; i++) {
    digitalWrite(MOSI, !!(value & (1 << (7 - i))));
    delayMicroseconds(1);
    digitalWrite(SCK, HIGH);
    delayMicroseconds(1);
    digitalWrite(SCK, LOW);
    delayMicroseconds(1);
  }
}
// void Pervasive_EPD::data(uint8_t value) {
//   this->start_data_();
//   this->write_byte(value);
//   this->end_data_();
// }
//
//// write a command followed by one or more bytes of data.
//// The command is the first byte, length is the total including cmd.
// void Pervasive_EPD::cmd_data(const uint8_t *c_data, size_t length) {
//   this->dc_pin_->digital_write(false);
//   this->enable();
//   this->write_byte(c_data[0]);
//   this->dc_pin_->digital_write(true);
//   this->write_array(c_data + 1, length - 1);
//   this->disable();
// }
bool Pervasive_EPD::busy_wait(bool state) {
  if (this->busy_pin_ == nullptr) {
    // ESP_LOGI(TAG, "busy_pin is null");
    return true;
  }
  if (this->busy_pin_->digital_read() == state) {
    // ESP_LOGI(TAG, "busy_pin is %i", state);
    return true;
  }

  // ESP_LOGI(TAG, "busy_wait");
  const uint32_t start = millis();

  while (this->busy_pin_->digital_read() != state)  // || millis() - start < 100)
  {
    if (millis() - start > this->idle_timeout_()) {
      ESP_LOGE(TAG, "Timeout while displaying image!");
      return false;
    }
    delay(1);
  }
  // ESP_LOGI(TAG, "busy_wait %i took %i", state, (millis() - start));

  return true;
}

void Pervasive_EPD::update() {
  frameCount++;
  // ESP_LOGI(TAG, "update");
  this->do_update_();
  if (is_busy())
    return;
  if (!changed)
    return;
  changed = false;
  this->display();
}
void Pervasive_EPD::fill(Color color) {
  // flip logic
  const uint8_t fill = color.is_on() ? 0xFF : 0x00;
  for (uint32_t i = 0; i < this->get_frame_length_(); i++)
    this->buffer_[i] = fill;
}
void HOT Pervasive_EPD::draw_absolute_pixel_internal(int x, int y, Color color) {
  if (x >= this->get_width_internal() || y >= this->get_height_internal() || x < 0 || y < 0)
    return;
  uint16_t addr = ((uint32_t) (this->get_width_internal() - 1 - x) * (uint32_t) this->get_height_internal() + y) / 8;

  uint8_t b = this->buffer_[addr];
  uint8_t p = b;
  if (!color.is_on()) {
    b |= (1 << (7 - y % 8));
  } else {
    b &= ~(1 << (7 - y % 8));
  }
  if (b != p) {
    this->buffer_[addr] = b;
    changed = true;
  }
}

uint32_t Pervasive_EPD::get_frame_length_() {
  return this->get_width_controller() * this->get_height_internal() / 8u;
}  // just a black buffer
uint32_t Pervasive_EPD::get_buffer_length_() {
  return this->get_frame_length_() * 2;
  // return this->get_width_controller() * this->get_height_internal() / 8u;
}  // just a black buffer
// uint32_t WaveshareEPaperBWR::get_buffer_length_() {
//   return this->get_width_controller() * this->get_height_internal() / 4u;
// }  // black and red buffer
// uint32_t WaveshareEPaper7C::get_buffer_length_() {
//   return this->get_width_controller() * this->get_height_internal() / 8u * 3u;
// }  // 7 colors buffer, 1 pixel = 3 bits, we will store 8 pixels in 24 bits = 3 bytes

// void Pervasive_EPD::start_command_() {
//   this->dc_pin_->digital_write(false);
//   this->enable();
// }
// void Pervasive_EPD::end_command_() { this->disable(); }
// void Pervasive_EPD::start_data_() {
//   this->dc_pin_->digital_write(true);
//   this->enable();
// }
// void Pervasive_EPD::end_data_() { this->disable(); }
void Pervasive_EPD::on_safe_shutdown() { this->deep_sleep(); }

void Pervasive_EPD::deep_sleep() {
  switch (this->model_) {
    case EPD_150_KS_0J:
    case EPD_152_KS_0J:

      break;

    default:

      command(0x02);  // Turn off DC/DC
      busy_wait();
      break;
  }
}

// ========================================================
//                          Type A
// ========================================================

void Pervasive_EPD::initialize() {
  // Achieve display intialization
  // pref_->setup()
  // Read OTP

  // Application note � 3. Read OTP memory
  // Register 0x50 flag
  // Additional settings for fast update, 154 206 213 266 271A 370 and 437 screens (s_flag50)
  switch (model_) {
    case EPD_154_KS_0C:
    case EPD_206_KS_0E:
    case EPD_213_KS_0E:
    case EPD_266_KS_0C:
    case EPD_271_KS_0C:  // 2.71(A)
    case EPD_370_KS_0C:
    case EPD_437_KS_0C:

      s_flag50 = true;
      break;

    default:

      s_flag50 = false;
      break;
  }

  // Screens with no OTP
  switch (model_) {
    case EPD_150_KS_0J:
    case EPD_152_KS_0J:
    case EPD_290_KS_0F:

      ESP_LOGI(TAG, "OTP check passed - embedded PSR");
      return;  // No PSR
      break;

    default:

      break;
  }

  this->dc_pin_->digital_write(true);
  this->reset_pin_->digital_write(true);
  this->cs_->digital_write(true);

  // b_reset(0, 5, 5, 10, 20);
  reset_();

  uint32_t key = fnv1_hash("pervasive_epd");
  key += (uint32_t) model_;
  if (this->pref_->load(key, COG_data)) {
    ESP_LOGI(TAG, "Loaded COG_data: 0x%02x, 0x%02x", COG_data[0], COG_data[1]);
  } else {
    get_COG_data();
    this->pref_->save(COG_data);

    ESP_LOGI(TAG, "Read COG_data from display: 0x%02x, 0x%02x", COG_data[0], COG_data[1]);
    ESP_LOGE(TAG, "Restart Required");
    global_preferences->sync();
    delay(100);
    App.safe_reboot();
    // delay(500);
    // exit(0x02);
  }
}

void Pervasive_EPD::get_COG_data() {
  // switch (model_) {

  // case EPD_271_KS_09:
  //     COG_data[0] = 0xcf;
  //     COG_data[1] = 0x82;
  //     return;
  // case EPD_154_KS_0C:
  // case EPD_266_KS_0C:
  // case EPD_271_KS_0C: // 2.71(A)
  // case EPD_370_KS_0C:
  // case EPD_437_KS_0C:
  //     // offsetPSR = (bank == 0) ? 0x0fb4 : 0x1fb4;
  //     // offsetA5 = (bank == 0) ? 0x0000 : 0x1000;
  //     // break;
  //     return;
  // case EPD_206_KS_0E:
  // case EPD_213_KS_0E:
  //     //TODO fill
  //     // offsetPSR = (bank == 0) ? 0x0b1b : 0x171b;
  //     // offsetA5 = (bank == 0) ? 0x0000 : 0x0c00;
  //     // break;
  //     return;
  // case EPD_417_KS_0D:

  //     // COG_data[0] = 0xcf;
  //     // COG_data[1] = 0x82;
  //     // return;

  //     offsetPSR = (bank == 0) ? 0x0b1f : 0x171f;
  //     offsetA5 = (bank == 0) ? 0x0000 : 0x0c00;
  //     break;

  // default:
  //     Serial.println();
  //     ESP_LOGE(TAG, "OTP check failed - Screen %i not supported", model_);
  //     exit(0x01);
  //     break;
  // }

  uint8_t ui8 = 0;
  uint16_t offsetA5 = 0x0000;
  uint16_t offsetPSR = 0x0000;
  uint16_t u_readBytes = 2;

  this->dc_pin_->digital_write(false);  // Command
  this->cs_->digital_write(false);      // Select
  writeSPI3(0xa2);
  this->cs_->digital_write(true);  // Unselect
  delay(10);

  this->dc_pin_->digital_write(true);  // Data
  this->cs_->digital_write(false);     // Select
  ui8 = readSPI3();                    // Dummy
  this->cs_->digital_write(true);      // Unselect

  this->cs_->digital_write(false);  // Select
  ui8 = readSPI3();                 // First byte to be checked
  this->cs_->digital_write(true);   // Unselect

  // Check bank
  uint8_t bank = ((ui8 == 0xa5) ? 0 : 1);

  switch (model_) {
    case EPD_271_KS_09:

      offsetPSR = 0x004b;
      offsetA5 = 0x0000;

      if (bank > 0) {
        COG_data[0] = 0xcf;
        COG_data[1] = 0x82;
        return;
      }
      break;

    case EPD_154_KS_0C:
    case EPD_266_KS_0C:
    case EPD_271_KS_0C:  // 2.71(A)
    case EPD_370_KS_0C:
    case EPD_437_KS_0C:

      offsetPSR = (bank == 0) ? 0x0fb4 : 0x1fb4;
      offsetA5 = (bank == 0) ? 0x0000 : 0x1000;
      break;

    case EPD_206_KS_0E:
    case EPD_213_KS_0E:

      offsetPSR = (bank == 0) ? 0x0b1b : 0x171b;
      offsetA5 = (bank == 0) ? 0x0000 : 0x0c00;
      break;

    case EPD_417_KS_0D:

      // COG_data[0] = 0xcf;
      // COG_data[1] = 0x82;
      // return;

      offsetPSR = (bank == 0) ? 0x0b1f : 0x171f;
      offsetA5 = (bank == 0) ? 0x0000 : 0x0c00;
      break;

    default:
      Serial.println();
      ESP_LOGE(TAG, "OTP check failed - Screen %i not supported", model_);
      exit(0x01);
      break;
  }

  // Check second bank
  if (offsetA5 > 0x0000) {
    for (uint16_t index = 1; index < offsetA5; index += 1) {
      this->cs_->digital_write(false);  // CS low = Select
      ui8 = readSPI3();
      this->cs_->digital_write(true);  // CS high = Unselect
    }

    this->cs_->digital_write(false);  // CS low = Select
    ui8 = readSPI3();                 // First byte to be checked
    this->cs_->digital_write(true);   // CS high = Unselect

    if (ui8 != 0xa5) {
      ESP_LOGE(TAG, "OTP ERROR!");
      delay(100);
      ESP_LOGE(TAG, "OTP check failed - Bank %i, first 0x%02x, expected 0x%02x", bank, ui8, 0xa5);
      delay(1000);
      exit(0x01);
    }
  }

  switch (this->model_) {
    case EPD_271_KS_09:

      ESP_LOGI(TAG, TAG, "OTP check passed - Bank %i, first 0x%02x %s", bank, ui8,
               (bank == 0) ? "as expected" : "not checked");
      break;

    default:

      ESP_LOGI(TAG, "OTP check passed - Bank %i, first 0x%02x as expected", bank, ui8);
      break;
  }

  for (uint16_t index = offsetA5 + 1; index < offsetPSR; index += 1) {
    this->cs_->digital_write(false);  // Select
    ui8 = readSPI3();
    this->cs_->digital_write(true);  // Unselect
  }
  // Populate COG_initialData
  for (uint16_t index = 0; index < u_readBytes; index += 1) {
    this->cs_->digital_write(false);  // Select
    COG_data[index] = readSPI3();     // Read OTP
    this->cs_->digital_write(true);   // Unselect
  }
}

void Pervasive_EPD::dump_config() {
  LOG_DISPLAY("", "Pervasive E-Paper", this);
  switch (this->model_) {
    case EPD_150_KS_0J:
      ESP_LOGCONFIG(TAG, "  Model: EPD_150_KS_0J");
      break;
    case EPD_152_KS_0J:
      ESP_LOGCONFIG(TAG, "  Model: EPD_152_KS_0J");
      break;
    case EPD_154_KS_0C:
      ESP_LOGCONFIG(TAG, "  Model: EPD_154_KS_0C");
      break;
    case EPD_206_KS_0E:
      ESP_LOGCONFIG(TAG, "  Model: EPD_206_KS_0E");
      break;
    case EPD_213_KS_0E:
      ESP_LOGCONFIG(TAG, "  Model: EPD_213_KS_0E");
      break;
    case EPD_266_KS_0C:
      ESP_LOGCONFIG(TAG, "  Model: EPD_266_KS_0C");
      break;
    case EPD_271_KS_09:
      ESP_LOGCONFIG(TAG, "  Model: EPD_271_KS_09");
      break;
    case EPD_271_KS_0C:
      ESP_LOGCONFIG(TAG, "  Model: EPD_271_KS_0C");
      break;
    case EPD_290_KS_0F:
      ESP_LOGCONFIG(TAG, "  Model: EPD_290_KS_0F");
      break;
    case EPD_370_KS_0C:
      ESP_LOGCONFIG(TAG, "  Model: EPD_370_KS_0C");
      break;
    case EPD_417_KS_0D:
      ESP_LOGCONFIG(TAG, "  Model: EPD_417_KS_0D");
      break;
    case EPD_437_KS_0C:
      ESP_LOGCONFIG(TAG, "  Model: EPD_437_KS_0C");
      break;
  }
  ESP_LOGCONFIG(TAG, "  Full Update Every: %" PRIu32, this->full_update_every_);
  LOG_PIN("  Reset Pin: ", this->reset_pin_);
  LOG_PIN("  DC Pin: ", this->dc_pin_);
  LOG_PIN("  CS Pin: ", this->cs_);
  LOG_PIN("  Busy Pin: ", this->busy_pin_);
  LOG_UPDATE_INTERVAL(this);
}

void HOT Pervasive_EPD::display() {
  bool full_update = this->at_update_ == 0;
  bool prev_full_update = this->at_update_ == 1;

  // ESP_LOGI(TAG, "update full: %i", full_update);
  // b_resume(); // GPIO
  reset_();

  // Start SPI
  // spi_setup();
  // no need, already started

  // COG_initial

  // Application note § 4. Input initial command
  switch (this->model_) {
    case EPD_150_KS_0J:
    case EPD_152_KS_0J:

      // Soft reset
      command(0x12);
      // this->dc_pin_->digital_write(false);
      busy_wait(LOW);  // 150 and 152 specific

      // Work settings
      cmd_data(0x1a, u_temperature);

      if (full_update) {
        cmd_data(0x22, 0xd7);
      } else {
        cmd_data(0x3c, 0xc0);
        cmd_data(0x22, 0xdf);
      }
      break;

    default:
      // Work settings
      uint8_t indexTemperature;  // Temperature
      uint8_t index00_work[2];   // PSR

      // ESP_LOGI(TAG, "u_temperature %i - 0x%02x", u_temperature, u_temperature);
      // FILM_K already checked
      if (!full_update)  // Specific settings for fast update
      {
        indexTemperature = u_temperature | 0x40;  // temperature | 0x40
        index00_work[0] = COG_data[0] | 0x10;     // PSR0 | 0x10
        index00_work[1] = COG_data[1] | 0x02;     // PSR1 | 0x02
      } else                                      // Common settings
      {
        indexTemperature = u_temperature;  // Temperature
        index00_work[0] = COG_data[0];     // PSR0
        index00_work[1] = COG_data[1];     // PSR1
      }                                    // u_codeExtra updateMode

      // New algorithm
      cmd_data(0x00, 0x0e);  // Soft-reset
      busy_wait();

      cmd_data(0xe5, indexTemperature);  // Input Temperature
      cmd_data(0xe0, 0x02);              // Activate Temperature
      switch (this->model_) {
        case EPD_290_KS_0F:  // No PSR

          cmd_data(0x4d, 0x55);
          cmd_data(0xe9, 0x02);
          break;

        default:

          cmd_data(0x00, index00_work, 2);  // PSR
          break;
      }
      // Specific settings for fast update, all screens
      // FILM_K already checked
      if (!full_update) {
        cmd_data(0x50, 0x07);  // Vcom and data interval setting
      }
      break;
  }
  //
  uint8_t *firstFrame = buffer_;

  uint32_t sizeFrame = get_frame_length_();

  if (full_update) {
    // COG_sendImageDataNormal(buffer1, buffer1_size);

    // Send image data
    switch (this->model_) {
      case EPD_150_KS_0J:
      case EPD_152_KS_0J:

        cmd_data(0x24, firstFrame, sizeFrame);  // Next frame, blackBuffer
        cmd_data_fixed(0x26, 0x00, sizeFrame);  // Previous frame, 0x00
        break;

      default:

        cmd_data(0x10, firstFrame, sizeFrame);  // First frame, blackBuffer
        cmd_data_fixed(0x13, 0x00, sizeFrame);  // Second frame, 0x00
        break;
    }  // this->model_
  } else {
    // COG_sendImageDataFast(buffer1, buffer2, buffer1_size);

    uint8_t *secondFrame = buffer_ + sizeFrame;
    // firstFrame: New Image for NORMAL, Old Image for FAST
    // secondFrame: 0x00 data for NORMAL, New Image for FAST

    // Send image data
    switch (this->model_) {
      case EPD_150_KS_0J:
      case EPD_152_KS_0J:

        cmd_data(0x24, secondFrame, sizeFrame);  // Next frame, blackBuffer
        cmd_data(0x26, firstFrame, sizeFrame);   // Previous frame, 0x00
        break;

      default:
        // Additional settings for fast update, 154 213 266 370 and 437 screens (s_flag50)
        if (s_flag50) {
          cmd_data(0x50, 0x27);  // Vcom and data interval setting
        }

        cmd_data(0x10, secondFrame, sizeFrame);  // First frame, blackBuffer
        cmd_data(0x13, firstFrame, sizeFrame);   // Second frame, 0x00

        // Additional settings for fast update, 154 213 266 370 and 437 screens (s_flag50)
        if (s_flag50) {
          cmd_data(0x50, 0x07);  // Vcom and data interval setting
        }
        break;
    }
  }

  // COG_update

  // saveLastFrame();
  for (int i = 0; i < sizeFrame; i++) {
    buffer_[i + sizeFrame] = buffer_[i];
  }

  // Application note § 6. Send updating command
  switch (this->model_) {
    case EPD_150_KS_0J:
    case EPD_152_KS_0J:

      busy_wait(LOW);                  // 152 specific
      command(0x20);                   // Display Refresh
      this->cs_->digital_write(true);  // CS# = 1
      // busy_wait(LOW); // 152 specific
      // Now Waiting in is_busy()
      break;

    default:

      busy_wait();

      command(0x04);  // Power on
      busy_wait();

      command(0x12);  // Display Refresh

      // busy_wait();
      // Now Waiting in is_busy()
      break;
  }

  isWaiting = true;
  // COG_stopDCDC(); // Power off

  // Application note § 7. Turn-off DC/DC

  this->at_update_ = (this->at_update_ + 1) % this->full_update_every_;
}

bool Pervasive_EPD::is_busy() {
  if (!isWaiting)
    return false;
  switch (this->model_) {
    case EPD_150_KS_0J:
    case EPD_152_KS_0J:
      // busy_wait(LOW); // 152 specific
      if (this->busy_pin_->digital_read() != false)
        return true;
    default:
      // busy_wait();
      if (this->busy_pin_->digital_read() != true)
        return true;
  }
  isWaiting = false;
  deep_sleep();
  return false;
}

int Pervasive_EPD::get_width_internal() {
  switch (this->model_) {
    case EPD_150_KS_0J:
    case EPD_152_KS_0J:
      return 200;
    case EPD_154_KS_0C:
      return 152;
    case EPD_206_KS_0E:
      return 128;
    case EPD_213_KS_0E:
      return 104;
    case EPD_266_KS_0C:
      return 152;
    case EPD_271_KS_09:
    case EPD_271_KS_0C:
      return 176;
    case EPD_290_KS_0F:
      return 168;
    case EPD_370_KS_0C:
      return 240;
    case EPD_417_KS_0D:
      return 300;
    case EPD_437_KS_0C:
      return 176;

    default:
      return 0;
  }
}
// The controller of the 2.13" displays has a buffer larger than screen size
int Pervasive_EPD::get_width_controller() { return this->get_width_internal(); }
int Pervasive_EPD::get_height_internal() {
  switch (this->model_) {
    case EPD_150_KS_0J:
    case EPD_152_KS_0J:
      return 200;
    case EPD_154_KS_0C:
      return 152;
    case EPD_206_KS_0E:
      return 248;
    case EPD_213_KS_0E:
      return 212;
    case EPD_266_KS_0C:
      return 296;
    case EPD_271_KS_09:
    case EPD_271_KS_0C:
      return 264;
    case EPD_290_KS_0F:
      return 384;
    case EPD_370_KS_0C:
      return 416;
    case EPD_417_KS_0D:
      return 400;
    case EPD_437_KS_0C:
      return 480;

    default:
      return 0;
  }
}

void Pervasive_EPD::set_full_update_every(uint32_t full_update_every) { this->full_update_every_ = full_update_every; }

uint32_t Pervasive_EPD::idle_timeout_() {
  return 6000;
  // return 2500;
  // switch (this->model_)
  // {
  // case EPD_150_KS_0J:
  // case EPD_152_KS_0J:
  // default:
  // }
}

}  // namespace pervasive_epd
}  // namespace esphome
