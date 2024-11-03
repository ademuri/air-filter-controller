#include "adp2100.h"

#include "esphome/core/helpers.h"
#include "esphome/core/log.h"

namespace esphome::adp2100 {

using ErrorCode = ::esphome::i2c::ErrorCode;

static const char *const TAG = "adp2100";

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
  error = this->read(data_.data(), 3);
  if (error != ErrorCode::NO_ERROR) {
    ESP_LOGE(TAG, "Error reading product type");
    this->mark_failed();
    setup_status_ = SetupStatus::READING_PRODUCT_TYPE;
    i2c_error_code_ = error;
    return;
  }

  uint8_t expected_crc = CalculateCRC(data_.data(), 2);
  if (data_[2] != expected_crc) {
    ESP_LOGE(TAG, "Product type CRC check failed. Expected: 0x%2X, got: 0x%2X",
             expected_crc, data_[2]);
    this->mark_failed();
    setup_status_ = SetupStatus::PRODUCT_TYPE_CRC_FAILED;
    i2c_error_code_ = error;
    return;
  }

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
  ErrorCode error = this->write(kContinuousMeasurementCommand, kCommandLength);
  if (error != ErrorCode::NO_ERROR) {
    ESP_LOGE(TAG, "Error writing continuous measurement command");
  }

  assert(kDataLength >= 6);
  error = this->read(data_.data(), 6);
  if (error != ErrorCode::NO_ERROR) {
    ESP_LOGE(TAG, "Error reading continuous measurement data");
  }

  uint8_t expected_crc = CalculateCRC(data_.data(), 2);
  if (data_[2] != expected_crc) {
    ESP_LOGE(TAG,
             "Invalid CRC when reading pressure. Expected: 0x%2X, got: 0x%2X.",
             expected_crc, data_[2]);
  } else {
    uint16_t raw_pressure = (data_[0] << 8) | data_[1];

    if (this->pressure_sensor_) {
      this->pressure_sensor_->publish_state(raw_pressure / 60.0);
    }
  }

  expected_crc = CalculateCRC(&data_[3], 2);
  if (data_[5] != expected_crc) {
    ESP_LOGE(
        TAG,
        "Invalid CRC when reading temperature. Expected: 0x%2X, got: 0x%2X.",
        expected_crc, data_[5]);
  } else {
    uint16_t raw_temperature = (data_[3] << 8) | data_[4];

    if (this->temperature_sensor_) {
      this->temperature_sensor_->publish_state(raw_temperature / 200.0);
    }
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

    case SetupStatus::PRODUCT_TYPE_CRC_FAILED:
      ESP_LOGE(TAG, "  Product type CRC failed");
      break;
  }

  if (i2c_error_code_ != ErrorCode::NO_ERROR) {
    ESP_LOGE(TAG, "  I2C error code: %u", i2c_error_code_);
  }

  LOG_I2C_DEVICE(this);
  LOG_UPDATE_INTERVAL(this);
}

uint8_t ADP2100Sensor::CalculateCRC(uint8_t *data, uint8_t length) {
  uint8_t crc = 0xFF;

  for (uint8_t byte = 0; byte < length; ++byte) {
    crc ^= data[byte];
    for (uint8_t bit = 8; bit > 0; --bit) {
      if (crc & 0x80) {
        crc = (crc << 1) ^ 0x31;
      } else {
        crc = crc << 1;
      }
    }
  }
  return crc;
}

}  // namespace esphome::adp2100
