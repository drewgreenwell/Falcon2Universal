# Falcon 2 Universal
Reverse Engineering a universal adapter for the Falcon 2 laser module
## Description

This repository includes pinouts, raw serial data, and code samples to unlock the shackles from your Falcon 2 laser module.

## Why

The [40W Falcon 2 Laser Module](https://www.elecrow.com/esp32-display-7-inch-hmi-display-rgb-tft-lcd-touch-screen-support-lvgl.html) is available separately at a sometimes not outrageous price. Additionally, since the Falcon 2 controllers are not available separately, you can often find units on ebay or local marketplaces for cheap.

The downside to the falcon laser module is that it uses a proprietary connection (10 pin, 2.2mm pitch, 2 rows). Creality does not and will not provide any technical documentation (e.g. pinouts, data sheets, communication protocols). 

The laser module is really powerful and featured but it is accompanied with basic hardware that is proving to not be as fast as advertised and offers very little in terms of extensibility.

## What is done

The current basic example is confirmed working with the 12W and 40W laser module. The code is technically not needed for the laser to work, but without it the sensors will flash yellow and not report their alarm codes. The laser can be fired though. 

In the most basic sense, this code just handshakes with the laser, updates a received packet, and echos it back to the laser.


## In Progress

I'm currently working on pcb adapter board with an oled output of alarm codes for monitoring alarm codes 1-15.


## Pinouts

![Falcon 2 Laser Pinout](https://raw.githubusercontent.com/drewgreenwell/Falcon2Universal/main/content/Falcon-2-Laser-Pinout.png)

![Falcon 2 Laser Adapter](https://raw.githubusercontent.com/drewgreenwell/Falcon2Universal/main/content/Falcon-2-Adapter-Pinout.png)

## Raw Data

![Raw Boot Sequence RX](https://raw.githubusercontent.com/drewgreenwell/Falcon2Universal/main/content/falcon-boot-rx.txt)

![Raw Boot Sequence TX](https://raw.githubusercontent.com/drewgreenwell/Falcon2Universal/main/content/falcon-boot-tx.txt)

 ![Semi Parsed Boot Sequence RX](https://raw.githubusercontent.com/drewgreenwell/Falcon2Universal/main/content/falcon-boot-rx-parsed.txt)

 ![Semi Parsed Boot Sequence TX](https://raw.githubusercontent.com/drewgreenwell/Falcon2Universal/main/content/falcon-boot-tx-parsed.txt)
