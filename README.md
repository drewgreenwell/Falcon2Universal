# Falcon 2 Universal
Reverse Engineering a universal adapter for the Falcon 2 laser module
## Description

This repository includes pinouts, raw serial data, and code samples to unlock the shackles from your Falcon 2 laser module.

## Why

The [40W Falcon 2 Laser Module](https://store.creality.com/products/falcon2-40w-laser-module) is available separately at a sometimes not outrageous price. Additionally, since the Falcon 2 controllers are not available separately, you can often find units on ebay or local marketplaces for cheap.

The downside to the falcon laser module is that it uses a proprietary/hard to find connection. A 10 pin, 2mm pitch, 2 rows, 2mm row spacing does work, but the wings are non standard and I have yet to find a fully matched part without 3D printing a housing for a pin only connection. Creality does not and will not provide any technical documentation (e.g. pinouts, data sheets, communication protocols). 

The laser module is really powerful and featured but it is accompanied with basic hardware that is proving to not be as fast as advertised and offers very little in terms of extensibility.

## What is done

The current basic example is confirmed working with the latest firmware for the 12W and 40W laser module (I don't have a 22W to test). The code is technically not needed for the laser to work, but without it the sensors will flash yellow and not report their alarm codes. The laser can be fired though. 

In the most basic sense, this code just handshakes with the laser, updates received packets, and echos them back to the laser.

Falcon Screen is a functional UI for the adapter based on the LilyGo T Display. It enables configurable sleep / wake of the fans, alarm code /laser monitoring, and fire alaram output. It is geared to work on the adapter but will install and run fine on a T Display without the adapter board.

## In Progress

Version 2 of the PCB has arrived and parts ~~are trickling in~~ are here and working well. I am using the current version with the Falcon 2 40W and FluidNC running on an MKS DLC32. The adapter will let you run the Falcon lasers on any laser or CNC setup you have that supports a 3 pin 24V laser. The adapter features a screen that outputs alarm codes and laser info and has fire sensor support. WiFi Portal and OTA are currently in progress. I am also working on the best way to sense activity from x/y axis to control the 24V power to the laser so that I can put it in idle mode (no fans) when the laser is not in use. Note: I have currently settled on allowing wake on laser pulse or manual sleep/wake of the laser with plans for this to be configurable. The Falcon 2 controller will turn on the fans any time power is sent to a motor or the laser, this is controlled by the Falcon 2 controller. Perhaps an accelerometer or gyroscope is a good option here as well. The T-Display has buttons and a screen to make these options as configurable as possible.

[Video of original adapter (working on a new video)](https://www.youtube.com/shorts/gS-xerEGvpA)

I will add schematics, pcb layouts, parts list, stls, and options to order adapters here as well. If you are interested or wanting to start a project asap let me know here or via drewgreenwell on gmail or outlook. Adding "Falcon" in the subject should make sure I get your message.

### Falcon Hex Parser
A way to translate hex dumps into parsed messages. [Falcon Hex Parser](https://htmlpreview.github.io/?https://github.com/drewgreenwell/Falcon2Universal/blob/main/content/parser.html)


## Pinouts

![Falcon 2 Laser Pinout](https://raw.githubusercontent.com/drewgreenwell/Falcon2Universal/main/content/Falcon-2-Laser-Pinout.png)

![Falcon 2 Laser Adapter](https://raw.githubusercontent.com/drewgreenwell/Falcon2Universal/main/content/Falcon-2-Adapter-Pinout.png)

## Raw Data

[Laatest Raw Boot Files and WIP](https://github.com/drewgreenwell/Falcon2Universal/tree/main/content/3.0.5-Dumps)
