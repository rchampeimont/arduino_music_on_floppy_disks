# Arduino project: playing music from floppy disks

Here is a video in which I present the result and explain the challenges: https://www.youtube.com/watch?v=-Ej4zM4t6mE

This project uses code from https://github.com/dhansel/ArduinoFDC

![General overview](/images/overview.jpg?raw=true)

# Required hardware
* 2 x Arduino Uno[1] R3 (probably works with earlier versions too but untested)
* 1 x regular 3.5" 1.44MB floppy drive
* 1 (or more) regular 1.44 MB floppy disk
* 13 x 1 kohm resitors (or 3 x 1khom resistors and 5 x 2 kohm resistors)
* 1 x 100 µF (ore more) capacitor
* optional: 1 x operational amplifier circuit (only needed to drive a "raw" speaker, you don't need it to use headphones or "computer" speakers)
* optional: LEDs and their adapted resistors if you want the LEDs (just for debug/fun but the circuit works without LEDs of course)

[1] Note: In the YouTube video I used an Arduino Leonardo as the floppy disk controller, but here I uploaded code that runs on two Arduino Unos to make it easier for you, as I assume Arduino Unos are more widely available.

# Software
Here is what each directory contains:
* **floppy_reader** is the Arduino code to upload to the "floppy drive controller" Arduino Uno
* **sound_player_4bit** is the Arduino code to upload to the "sound player" Arduino Uno
* **make_floppy_image** is a C program I wrote to convert a 8-bit 8kHz Mono WAV file to a 1.44 MB floppy image

To create a WAV file in the format expected by "make_floppy_image", you can use Audacity to change the sampling rate of an existing sound file (you both need to resample and change the project sample rate), merge to mono channel, and then export to "other uncompressed format" and select "Microsoft WAV" with "Unsigned 8-bit PCM" encoding.

To upload the floppy image to a floppy disk, you can do it with the Arduino by using the XMODEM interface provided in https://github.com/dhansel/ArduinoFDC (from which I re-used the low-level library). I have kept the default pin assignments from ArduinoFDC so you should be able to use the same circuit connections as in my project. You could also write the floppy disk image to a floppy disk simply using a computer (but I could not test this scenario as my computer motherboard does not have a floppy drive controller, as they are usually absent from modern motherboards).

# Circuit schematics
![Schematic for floppy drive controller](/images/circuit_fdc.jpg?raw=true)
![Schematic for sound player](/images/circuit_player.jpg?raw=true)
