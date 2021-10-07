#ifndef CALLBACKS_H_
#define CALLBACKS_H_

#include "ModbusServerWiFi.h"

// FC01: act on 0x01 requests - READ_COIL
ModbusMessage FC01(ModbusMessage request);
// FC03: act on 0x03 and 0x04 requests - READ_HOLD_REGISTER and READ_INPUT_REGISTER
ModbusMessage FC03(ModbusMessage request);
// FC05: act on 0x05 requests - WRITE_COIL
ModbusMessage FC05(ModbusMessage request);
// FC06: act on 0x06 requests - WRITE_HOLD_REGISTER
ModbusMessage FC06(ModbusMessage request);
// FC0F: act on 0x0F requests - WRITE_MULT_COILS
ModbusMessage FC0F(ModbusMessage request);

#endif // CALLBACKS_H_