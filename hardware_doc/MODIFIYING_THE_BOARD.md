# Modifiying the board

This project is based on already existing hardware with 
slight modifications in order to work.

## Requirements
 - Arduino UNO R3 Board with an ATMEGA16U2
 - Soldering Iron
 - Solder Wire
 - Jumper wire
 - Blade

## Objective
The objective of this mod is to remove the capacitor connecting the usb-serial
converter DTR line (made with the atmega16u2) allowing to control the reset line.

This means that a couple of traces need to be cut because of interference.

After cutting we result with an unused reset button that will be rerouted to the
additional atmega16u2 header in order to serve as a software reset trigger.
In this case the mega16 will sense the line state change and wil reset the main
mcu. In addition, that button can be used for other kind of startup sequence for 
custom and future use.

## Modifications
Start by removing the main mcu from the board (if possible) in order 
not to damage it in any case

### DTR line

#### Shorting C5
C5 is the capacitor isolating the two lines. 
Use the soldering iron with a chisel tip to heat both pads and remove it.



