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


## In Progress

I have a prototype pcb using the lillygo t-display I am using with the Falcon 2 and FluidNC. [Video](https://www.youtube.com/shorts/gS-xerEGvpA) It features a screen that out puts alarm codes and fire sensor support. I am working on the best way to sense control the 24V power coming in so tha tit goes idle when the laser is not in use (to turn off the fans). The Falcon 2 controller will turn on the fans any time power is sent to a motor or the laser. The T-Display has buttons and a screen to make these options as configurable as possible.

I am currently working on a smaller footprint for the pcb and a break out cable to allow placement of the display at the controller or laser.


## Pinouts

![Falcon 2 Laser Pinout](https://raw.githubusercontent.com/drewgreenwell/Falcon2Universal/main/content/Falcon-2-Laser-Pinout.png)

![Falcon 2 Laser Adapter](https://raw.githubusercontent.com/drewgreenwell/Falcon2Universal/main/content/Falcon-2-Adapter-Pinout.png)

## Raw Data

[Laatest Raw Boot Files and WIP](https://github.com/drewgreenwell/Falcon2Universal/tree/main/content/3.0.5-Dumps)
