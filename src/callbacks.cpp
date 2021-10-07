#include "callbacks.h"
// CoilData
#include "CoilData.h"
#include <Arduino.h>

extern CoilData led_coil;
extern bool led_triggered;
extern uint16_t distance_mm;
extern int duty_cycle;

// FC01: act on 0x01 requests - READ_COIL
ModbusMessage FC01(ModbusMessage request)
{
    ModbusMessage response;
    // Request parameters are first coil and number of coils to read
    uint16_t start = 0;
    uint16_t num_coils = 0;
    request.get(2, start, num_coils);

    // Are the parameters valid?
    if (start + num_coils <= led_coil.coils())
    {
        // Looks like it. Get the requested coils from our storage
        vector<uint8_t> coilset = led_coil.slice(start, num_coils);
        // Set up response according to the specs: serverID, function code, number of bytes to follow, packed coils
        response.add(request.getServerID(), request.getFunctionCode(), (uint8_t)coilset.size(), coilset);
    }
    else
    {
        // Something was wrong with the parameters
        response.setError(request.getServerID(), request.getFunctionCode(), ILLEGAL_DATA_ADDRESS);
    }
    // Return the response
    return response;
}

// FC03: act on 0x03 and 0x04 requests - READ_HOLD_REGISTER and READ_INPUT_REGISTER
ModbusMessage FC03(ModbusMessage request)
{
    ModbusMessage response; // The Modbus message we are going to give back
    uint16_t addr = 0;      // Start address
    uint16_t words = 0;     // # of words requested
    request.get(2, addr);   // read address from request
    request.get(4, words);  // read # of words from request

    // Address overflow?
    if ((addr + words) > 20)
    {
        // Yes - send respective error response
        response.setError(request.getServerID(), request.getFunctionCode(), ILLEGAL_DATA_ADDRESS);
    }
    // Set up response
    response.add(request.getServerID(), request.getFunctionCode(), (uint8_t)(words * 2));
    // Request for FC 0x03?
    if (request.getFunctionCode() == READ_HOLD_REGISTER)
    {
        // Yes. Complete response
        for (uint8_t i = 0; i < words; ++i)
        {
            // send increasing data values
            response.add(distance_mm);
        }
    }
    else
    {
        // No, this is for FC 0x04. Response is random
        for (uint8_t i = 0; i < words; ++i)
        {
            // send increasing data values
            response.add((uint16_t)random(1, 65535));
        }
    }
    // Send response back
    return response;
}

// FC05: act on 0x05 requests - WRITE_COIL
ModbusMessage FC05(ModbusMessage request)
{
    ModbusMessage response;
    // Request parameters are coil number and 0x0000 (OFF) or 0xFF00 (ON)
    uint16_t start = 0;
    uint16_t state = 0;
    request.get(2, start, state);

    // Is the coil number valid?
    if (start <= led_coil.coils())
    {
        // Looks like it. Is the ON/OFF parameter correct?
        if (state == 0x0000 || state == 0xFF00)
        {
            // Yes. We can set the coil
            if (led_coil.set(start, state))
            {
                // All fine, coil was set.
                response = ECHO_RESPONSE;
                // set LED trigger
                led_triggered = true;
            }
            else
            {
                // Setting the coil failed
                response.setError(request.getServerID(), request.getFunctionCode(), SERVER_DEVICE_FAILURE);
            }
        }
        else
        {
            // Wrong data parameter
            response.setError(request.getServerID(), request.getFunctionCode(), ILLEGAL_DATA_VALUE);
        }
    }
    else
    {
        // Something was wrong with the coil number
        response.setError(request.getServerID(), request.getFunctionCode(), ILLEGAL_DATA_ADDRESS);
    }
    // Return the response
    return response;
}

// FC06: act on 0x06 requests - WRITE_HOLD_REGISTER
ModbusMessage FC06(ModbusMessage request)
{
    ModbusMessage response;
    uint16_t addr = 0;
    uint16_t value = 0;
    request.get(2, addr, value);

    if (addr == 0)
    {
        duty_cycle = (int)(value > 255 ? 255 : value);
        led_triggered = true;
        response = ECHO_RESPONSE;
    }
    else
    {
        response.setError(request.getServerID(), request.getFunctionCode(), ILLEGAL_DATA_ADDRESS);
    }
    // Return the response
    return response;
}

// FC0F: act on 0x0F requests - WRITE_MULT_COILS
ModbusMessage FC0F(ModbusMessage request)
{
    ModbusMessage response;
    // Request parameters are first coil to be set, number of coils, number of bytes and packed coil bytes
    uint16_t start = 0;
    uint16_t num_coils = 0;
    uint8_t num_bytes = 0;
    uint16_t offset = 2; // Parameters start after serverID and FC
    offset = request.get(offset, start, num_coils, num_bytes);

    // Check the parameters so far
    if (num_coils && start + num_coils <= led_coil.coils())
    {
        // Packed coils will fit in our storage
        if (num_bytes == (num_coils >> 3) + 1)
        {
            // Byte count seems okay, so get the packed coil bytes now
            vector<uint8_t> coilset;
            request.get(offset, coilset, num_bytes);
            // Now set the coils
            if (led_coil.set(start, num_coils, coilset))
            {
                // All fine, return shortened echo response, like the standard says
                response.add(request.getServerID(), request.getFunctionCode(), start, num_coils);
                // set LED trigger
                led_triggered = true;
            }
            else
            {
                // Oops! Setting the coils seems to have failed
                response.setError(request.getServerID(), request.getFunctionCode(), SERVER_DEVICE_FAILURE);
            }
        }
        else
        {
            // num_bytes had a wrong value
            response.setError(request.getServerID(), request.getFunctionCode(), ILLEGAL_DATA_VALUE);
        }
    }
    else
    {
        // The given set will not fit to our coil storage
        response.setError(request.getServerID(), request.getFunctionCode(), ILLEGAL_DATA_ADDRESS);
    }
    return response;
}