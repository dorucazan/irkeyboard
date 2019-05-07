# irkeyboard
USB keyboard based on ATmega32U4 Pro Micro board with IR Receiver<br/>
<br/>
It appears as Keyboard HID device and sends multimedia keys (e.g. VolumeUp, VolumeDown)<br/>
It works in parallel with existing keyboard.<br/>

To learn new IR codes, open any text editor and rapidly toggle CAPS-LOCK led on-off 10 times.<br/>
Next follow the instructions printed in the text editor.<br/>
Note: On MAC you need software (e.g. https://pqrs.org/osx/karabiner/) to sync CAPS-LOCK led between all keyboards connected to computer<br/>
<br/>
<br/>
<br/>
[Schematic](https://github.com/dorucazan/irkeyboard/blob/master/IRKeyboard%20schematic.png):<br/>
Connect a Vishay TSOP38238, 38kHz IR Receiver to D4, VCC and GND pins of the ATmega32U4 Pro Micro board.<br/>
<br/>
<br/>
<br/>
Boards you can use:<br/>
- [SparkFun Pro Micro - 5V/16MHz](https://www.sparkfun.com/products/12640)<br/>
- [Adafruit ItsyBitsy 32u4 - 5V 16MHz](https://www.adafruit.com/product/3677)<br/>
<br/>
MIT license
