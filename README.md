# rf_gpio
rf_gpio simulates RFLink with a transmitter and receiver linked directly to the GPIO ports of raspberry pi.

This text comes from an automatic translation and probably contains a lot of errors. You'd better read the original text in Esperanto: LeguMin.md.

INSTALL:
cd / home / pi / rf_gpio
sudo cp -p rf_gpio.sh /etc/init.d
sudo update-rc.d rf_gpio.sh default
south service rf_gpio.sh start

connect the receiver to gpio2 bcm27 (pin 13)
connect the transmitter to gpio0 bcm17 (pin 11)

Use with domoticz:
Add a RFLink gateway with LAN-interface type hardware
remote address: 127.0.0.1
port: 10000
If the hardware is recognized, integration with domoticz is simple: simply use the"automatic detection"button on the"switch"tab, or turn on the"allow for 5 minutes"option.

Transmitters and Procurement Receivers:
* Chinese gear with a receiver receiver (asin = b00z9sznp0, mx-05v + mx-fs-03v), watched on Amazon at € 1.
the transmitter is good, but the receiver is very insensitive (about 4 m), for use only for learning codes.
* wl101-341 + wl102-341, superheterodine 433 mhz, seen at aliexpress at € 1.
the receiver has a better sensation than the previous one.

recognized equipment:
was successfully tested:
* Entries kyg (asin: b07dpmpvw1, marked intertek, views on amazon) (known as"impuls")
* Aneng thermometer-hybrometer (Chinese low cost, with screen lcd, viewed at aliexpress)
* digoo rg-8b-thermometrilo-hyigrometer (Chinese low cost, without screen, seen at AliExpress)

Other sensors are predefined in sensors.ini, but have not been tested.

add new sensor:
It is necessary to identify the data transfer protocol and the data format.
election 1:
follow the exit of rf_gpio, or launch it in a helmet header, or connect it with the"telnet 127.0.0.10000"command.
When the sensor sends data and the transfer protocol is recognized, you will see something similar to this:
20; 00; p0102, bits = 36, D0 = 529, D1 = 949, D2 = 1926, DS = 3865; duum = 011100110000000011011000111100100000, deksesumu = 7300d8f20;
20;  = Any RFLink transmission framework starts as much.
00;  = first delivered framework.
p0102, bits = 36, D0 = 529, D1 = 949, D2 = 1926, DS = 3865 = rf_gpio identified a protocol of the type:
Bit 0 = D0 D1
bit 1 = D0 D2
36 pieces of data
Duration D0 = 520 μs
duration D1 = 957 μs
duration D2 = 1936 μs
Synchronization time DS = 3881 μs
dues = 011100110000000011011000111100100000, hexadecimal = 7300d8f20;  : data received in binary and hexadecimal.

election 2:
run in shell
./analizi
and press the button on the remote control, or wait for the sensor to send data.  If the protocol is recognized, we see something similar to this:
73 impulses protocol:"xxx; p0001, bits = 36, D0 = 689, D1 = 1923, DS = 3890; ID: b1-b36"
Dummy data: 011100110000000011011000111100100000
Hexa data: 7300d8f20

Now you need to analyze the binary data to identify the meaning of each piece.
You can then add a line in the file sensors.ini, each line consists of three elements separated by a semicolon:
* first element: name of the material.  Attention, if it is a switch, must be part of the list of elements recognized by domoticz.
* second element: protocol.  Copy that that shows rf_gpio or analysis.
example: p0102, bits = 36, D0 = 561, D1 = 1899, D2 = 3845, DS = 9158
means: protocol p0102 (bit 0 = D0 D1, bit 1 = D0 D2), 36 bits per frame, duration D0 = 561 μs, duration D1 = 1899 μs, duration D2 = 3845 μs, synchronization time DS = 9158 μs
the parts of protocol, bit, D0 and D1 are used to differentiate the devices.
* third element: description of the data, each field separated from the others by comma
An example for multi-channel remote control, we can have: ID: b1-20, CMD: b21-21, SWITCH: b22-24
ID: b1-20 means that the remote control ID is in bits 1 to 20 (bit 1 = first bit transmitted)
CMD: b21-21 means that the command given (0/1) is in bit 21.
SWITCH: b22-24 means that the number of the powered plug is in bits 22 to 24.
To be recognized by domoticz, the field name must be in the acknowledged list (see senders.txt).  However, we can put what we want, just domoticz will ignore the field.
Several sequences of bits can be composed, eg: CMD: b17-17: b15-15: b16-16 conquesten bits 17 and 16 in this order.
One can test the value of some bits, for example: CST2: b43-48 = 1 will check that the bits 43 to 48 contain the value 1 (hexadecimal) in reception, and influence these bits in output, CST2: b43-48!  1 will check that the bits 43 to 48 do not contain the value 1 (hexadecimal) in reception.
You can declare BCD code fields: put B instead of b.  example: TEMP: B12-15: B16-19: B20-23 declares a temperature time whose first number is in bits 12-15, the second number in bits 16-20 and the third in bits 21-23.
A field ending with"-inv"is a special field that will take the opposite value (one's complement) of its same field in the show.
You can assign value to a field that is not in the data using"=".  example: CMD = ON
Having received all the lines that satisfy the condition, you will generate a line, if you want to avoid false properties, you can comment or remove the lines that do not match your material.
In a row, only the first line with the correct device name will be used.
If the protocol is not recognized, you can use to analyze it by studying it by increasing the details via the option -v, -vv or -vvv.  But rf_gpio can not recognize it without further development.

PROTOCOLS supported:
Only protocols with at least the following features are recognized:
* Press time alwaysu003e 100 μs
* No more than three different periods to represent the data.
* Each bit is coded with two pulses, in the same way.
* Each frame is surrounded by two synchronous signals with timeu003e 2500μs.
* Frame does not contain more than 200 pulses.
In shipment, the rotation codes and the calculations are not administered.
