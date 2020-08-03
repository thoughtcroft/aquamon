# Aquamon

Aquaponics monitoring using Arduino managed sensors.

I have an aquaponics setup with a 1,000 litre fish tank, 700 litre sump
tank and 3 x 450 litre media beds made from IBCs in a chop and flip
style. This system is a CHIFT-PIST system (constant height in fish tank,
pump in sump tank) and I wanted to automate monitoring of key elements
governing water quality in the system.

This project uses two different micro-controllers:

* Arduino Uno to collect data from various sensors
* D1 mini ESP8266 to connect to ThingsBoard over WiFi

There are several components that work together.

## Components

### ThingsBoard IoT Platform

I am using [ThingsBoard Community](https://thingsboard.io/) software for managing IoT services
including device management, rules based actions, dashboards etc.

There is a comprehensive rules engine for managing events such as device
off-line, data range violations etc. I am using a Telegram bot to alert
me to such events on my phone.

### Network connectivity

The D1 Mini ESP8266 controller board is responsible for connecting to
the ThingsBoard server via my home WiFi network. The
ThingsBoard_WiFi_Interface Sketch is basically a loop that connects to
WiFi, connects to the ThingsBoard server and sends a JSON request over
the Serial connection then waits for data to send on to ThingsBoard.

### Sensor data collector

The Arduino Uno controller board is responsible for collecting data from
the different sensors and sending them to the D1 Mini over the Serial
connection whenever data has been requested. The
ThingsBoard_Sensor_Interface Sketch is a loop that listens to the Serial
connection for a request and then reads the different sensors and
forwards the data using JSON in a format that ThingsBoard understands.

## Acknowledgements

I am indebted to the internet community for so much good content on how
to set up and develop code using the Arduino IDE.

The two systems use a serial connection to communicate using an approach
that I borrowed from [Acrobotic](https://youtu.be/6-RXqFS_UtU).

The different sensors have been interfaced using ideas from:

* [OneWire
  based](https://lastminuteengineers.com/ds18b20-arduino-tutorial/)  temperature sensors setup
* [two point
  calibration](https://www.instructables.com/id/Calibration-of-DS18B20-Sensor-With-Arduino-UNO/) of the temperature sensors
* using the [water flow
  sensor](https://www.instructables.com/id/How-to-Use-Water-Flow-Sensor-Arduino-Tutorial/)

In each case though, I have attempted to simplify and improve the
suggested approaches. Feel free to comment or suggest additional
improvements or borrow this approach to suit your own situation.
