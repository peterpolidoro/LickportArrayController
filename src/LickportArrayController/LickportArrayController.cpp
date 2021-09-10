// ----------------------------------------------------------------------------
// LickportArrayController.cpp
//
//
// Authors:
// Peter Polidoro peterpolidoro@gmail.com
// ----------------------------------------------------------------------------
#include "../LickportArrayController.h"


using namespace lickport_array_controller;

LickportArrayController::LickportArrayController()
{
}

void LickportArrayController::setup()
{
  // Parent Setup
  DigitalController::setup();

  // Reset Watchdog
  resetWatchdog();

  // Variable Setup

  // Set Device ID
  modular_server_.setDeviceName(constants::device_name);

  // Pin Setup
  for (size_t channel=0; channel<constants::CHANNEL_COUNT; ++channel)
  {
    pinMode(constants::channel_pins[channel],OUTPUT);
  }

  // Add Hardware
  modular_server_.addHardware(constants::hardware_info,
    pins_);

  // Pins
  modular_server::Pin & change_pin = modular_server_.createPin(constants::change_pin_name,
    constants::change_pin_number);
  change_pin.setModeDigitalInput();

  modular_server::Pin & lick_detected_pin = modular_server_.createPin(constants::lick_detected_pin_name,
    constants::lick_detected_pin_number);
  lick_detected_pin.setModePulseRising();

  modular_server::Pin & sync_pin = modular_server_.createPin(constants::sync_pin_name,
    constants::sync_pin_number);
  sync_pin.setModePulseFalling();

  // Add Firmware
  modular_server_.addFirmware(constants::firmware_info,
    properties_,
    parameters_,
    functions_,
    callbacks_);

  // Properties
  modular_server::Property & channel_count_property = modular_server_.property(digital_controller::constants::channel_count_property_name);
  channel_count_property.disableFunctors();
  channel_count_property.setDefaultValue(constants::channel_count);
  channel_count_property.setRange(constants::channel_count,constants::channel_count);
  channel_count_property.reenableFunctors();

  // Parameters
  modular_server::Parameter & dispense_duration_parameter = modular_server_.createParameter(constants::dispense_duration_parameter_name);
  dispense_duration_parameter.setRange(constants::dispense_duration_min,constants::dispense_duration_max);
  dispense_duration_parameter.setUnits(digital_controller::constants::ms_units);

  setChannelCountHandler();

  // Functions
  modular_server::Function & dispense_all_for_duration_function = modular_server_.createFunction(constants::dispense_all_for_duration_function_name);
  dispense_all_for_duration_function.attachFunctor(makeFunctor((Functor0 *)0,*this,&LickportArrayController::dispenseAllForDurationHandler));
  dispense_all_for_duration_function.addParameter(dispense_duration_parameter);

  // Callbacks
  modular_server::Callback & check_lick_status_callback = modular_server_.createCallback(constants::check_lick_status_callback_name);
  check_lick_status_callback.attachFunctor(makeFunctor((Functor1<modular_server::Pin *> *)0,*this,&LickportArrayController::checkLickStatusHandler));
  check_lick_status_callback.attachTo(change_pin,modular_server::constants::pin_mode_interrupt_falling);

  setAllChannelsOff();

  lick_sensor_.begin();
  lick_sensor_.reset();
  check_lick_status_ = false;
}

void LickportArrayController::update()
{
  // Parent Update
  DigitalController::update();

  checkLickStatus();
}

void LickportArrayController::dispenseAllForDuration(uint32_t dispense_duration)
{
  uint32_t channels = 0;
  for (uint8_t channel=0; channel<constants::channel_count; ++channel)
  {
    channels |= (1 << channel);
  }
  addPwm(channels,
    constants::dispense_power,
    constants::dispense_delay,
    dispense_duration,
    dispense_duration,
    constants::dispense_count);
}

double LickportArrayController::setChannelToPower(size_t channel,
  double power)
{
  if (power > digital_controller::constants::power_mid)
  {
    power = digital_controller::constants::power_max;
    digitalWrite(constants::channel_pins[channel],HIGH);
  }
  else
  {
    power = digital_controller::constants::power_min;
    digitalWrite(constants::channel_pins[channel],LOW);
  }
  return power;
}

void LickportArrayController::checkLickStatus()
{
  if (check_lick_status_)
  {
    check_lick_status_ = false;
    LickSensorStatus lick_sensor_status = lick_sensor_.getStatus();
    if (lick_sensor_status.any_key_touched)
    {
      modular_server::Pin & lick_detected_pin = modular_server_.pin(constants::lick_detected_pin_name);
      lick_detected_pin.setValue(constants::lick_detected_pulse_duration);
    }
  }
}

void LickportArrayController::checkLickStatusHandler(modular_server::Pin * pin_ptr)
{
  check_lick_status_ = true;
}

void LickportArrayController::dispenseAllForDurationHandler()
{
  long dispense_duration;
  modular_server_.parameter(constants::dispense_duration_parameter_name).getValue(dispense_duration);
  dispenseAllForDuration(dispense_duration);
}
