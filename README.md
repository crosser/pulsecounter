# Electric pulse counter on msp430 + cc2541 BLE module

Counter of circuit close events on GPIO pins, using TI MSP430
microcontroller (on the LaunchPad MSP430G2) and TI CC2541 Bluetooth
Low Energy (Bluetooth Smart) module with Emmoco firmware, based
on Emmoco example code.

`msp430` directory contains microcontroller code for the sensor.
Connect the wires from the water meters to the ground pin and
pins P1.4 and P1.5 for cold and hot meters respectively. Internal
pullup is used. To save power, modify the code to disable internal
pullup, and use external pullup resistor of 5 - 10 MOhm. And
probably disable VCLK in sleep mode.

`linux` directory contains the code to be run on Linux host. It
uses BlueZ infrastructure. You will need a BT 4.0 compatible
hardware connected to the host.
