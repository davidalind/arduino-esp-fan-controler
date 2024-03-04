# arduino-esp-fan-controler
## General
ESP8266 fan controler with WiFi Access Point.

Control interface is webpage with current PRM and duty cycle. New duty cycle can be set through the webpage.

PRM and duty cycle are pushed from fan controler unsing websocket.

## Connecting to UI
Default SSID: fan_controler

Default PASSWORD: 12341234

The IP of the control interface is: 192.168.4.1


## Electrical
* Vin: 5-15 VDC
* Vout = Vin

> [!CAUTION]
> Only connect 4 pin computer fan.

> [!CAUTION]
> Warning: Do no connect USB and Vin at the same time.

## Circuit
Board: Node MCU w/ ESP8266

No external components needed.

GND is connected to G and fan GND.

Vin is connected to Vin and fan 12V.

Fan PWM is connected to D1.

Fan tacho is connected to D4.
