#pragma once

#include <array>

#include "esphome/components/i2c/i2c.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/core/component.h"
#include "esphome/core/hal.h"

namespace esphome::adp2100 {

class ADP2100Sensor : public PollingComponent, public i2c::I2CDevice {
 public:
  void set_pressure_sensor(sensor::Sensor *pressure_sensor) {
    this->pressure_sensor_ = pressure_sensor;
  };
  void set_temperature_sensor(sensor::Sensor *temperature_sensor) {
    this->temperature_sensor_ = temperature_sensor;
  };

  void setup() override;
  void update() override;
  float get_setup_priority() const override { return setup_priority::DATA; };
  void dump_config() override;

 protected:
  uint8_t CalculateCRC(uint8_t *data, uint8_t length);

  sensor::Sensor *pressure_sensor_{nullptr};
  sensor::Sensor *temperature_sensor_{nullptr};

  static constexpr uint8_t kCommandLength = 2;
  const uint8_t kProductTypeCommand[kCommandLength] = {0xE2, 0x01};
  const uint8_t kContinuousMeasurementCommand[kCommandLength] = {0x36, 0x1E};

  static constexpr uint8_t kDataLength = 6;
  std::array<uint8_t, kDataLength> data_;

  enum class SetupStatus {
    SUCCESS,
    WRITING_PRODUCT_TYPE_COMMAND,
    READING_PRODUCT_TYPE,
    INCORRECT_PRODUCT_TYPE,
    PRODUCT_TYPE_CRC_FAILED,
  };
  SetupStatus setup_status_;

  i2c::ErrorCode i2c_error_code_;
};

}  // namespace esphome::adp2100
