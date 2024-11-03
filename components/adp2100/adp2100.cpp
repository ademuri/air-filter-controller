#include "adp2100.h"

#include "esphome/core/helpers.h"
#include "esphome/core/log.h"

namespace esphome::adp2100 {

using ErrorCode = ::esphome::i2c::ErrorCode;

static const uint8_t STATUS_BIT_POWER = 6;
static const uint8_t STATUS_BIT_BUSY = 5;
static const uint8_t STATUS_BIT_ERROR = 2;
static const uint8_t STATUS_MATH_SAT = 0;

static const char *const TAG = "adp2100";

float ADP2100Sensor::get_pressure() { return this->last_pressure_; }

float ADP2100Sensor::get_temperature() { return this->last_temperature_; }

void ADP2100Sensor::setup() {
  ESP_LOGI(TAG, "Setup ADP2100 Sensor");

  ErrorCode error = this->write(kProductTypeCommand, kCommandLength);
  if (error != ErrorCode::NO_ERROR) {
    ESP_LOGE(TAG, "Error writing product type command");
    this->mark_failed();
    setup_status_ = SetupStatus::WRITING_PRODUCT_TYPE_COMMAND;
    i2c_error_code_ = error;
    return;
  }

  assert(kDataLength >= 3);
  error = this->read(data_, 3);
  if (error != ErrorCode::NO_ERROR) {
    ESP_LOGE(TAG, "Error reading product type");
    this->mark_failed();
    setup_status_ = SetupStatus::READING_PRODUCT_TYPE;
    i2c_error_code_ = error;
    return;
  }

  // TODO: verify CRC
  if (!(data_[0] == 0x41 && data_[1] == 0x53)) {
    ESP_LOGE(
        TAG,
        "Got incorrect product type. Expected: 0x41 0x53, got: 0x%2X 0x%2X",
        data_[0], data_[1]);
    this->mark_failed();
    setup_status_ = SetupStatus::INCORRECT_PRODUCT_TYPE;
    i2c_error_code_ = error;
    return;
  }
}

void ADP2100Sensor::update() {
  ESP_LOGI(TAG, "Update ADP2100 Sensor");

  ErrorCode error = this->write(kContinuousMeasurementCommand, kCommandLength);
  if (error != ErrorCode::NO_ERROR) {
    ESP_LOGE(TAG, "Error writing continuous measurement command");
  }

  assert(kDataLength >= 6);
  error = this->read(data_, 6);
  if (error != ErrorCode::NO_ERROR) {
    ESP_LOGE(TAG, "Error reading continuous measurement data");
  }

  // TODO: verify CRC
  uint16_t raw_pressure = (data_[0] << 8) | data_[1];

  if (this->pressure_sensor_) {
    this->pressure_sensor_->publish_state(raw_pressure / 60.0);
  }
}

void ADP2100Sensor::dump_config() {
  ESP_LOGCONFIG(TAG, "ADP2100:");

  switch (setup_status_) {
    case SetupStatus::SUCCESS:
      ESP_LOGCONFIG(TAG, "  Initialization success");
      break;

    case SetupStatus::WRITING_PRODUCT_TYPE_COMMAND:
      ESP_LOGE(TAG, "  Failed to write product type command");
      break;

    case SetupStatus::READING_PRODUCT_TYPE:
      ESP_LOGE(TAG, "  Failed to read product type");
      break;

    case SetupStatus::INCORRECT_PRODUCT_TYPE:
      ESP_LOGE(TAG, "  Incorrect product type");
      break;
  }

  if (i2c_error_code_ != ErrorCode::NO_ERROR) {
    ESP_LOGE(TAG, "  I2C error code: %u", i2c_error_code_);
  }

  LOG_I2C_DEVICE(this);
  LOG_UPDATE_INTERVAL(this);
}

}  // namespace esphome::adp2100
