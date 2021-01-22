# Lib0t
Lib0t is a simple WIFI relay to 433Mhz device. 
It supports CHACON (v1) and OTIO brand devices.
- 433Mhz transmitter by HTTP / GET.
- Response with interior temperature via the DHT21 sensor.
- LCD which displays the time retrieved from the internet and information on temperature and ambient humidity level.
- Rest button for setting the wifi (switches to AP mode).

- [Schema](lib0t/schema/readme.md)
- [3D Enclosure](lib0t/enclosure/readme.md)
- [Firmware](lib0t/firmware/readme.md)

## Screenshot 
<p align="center">
  <img src="https://raw.githubusercontent.com/libre/lib0t/images/20200705_021503.jpg" width="450" title="Screenshot">
</p>

## Materials
- ESP8622
- Pushbutton
- 0.96 ’OLED LCD
- 433Mhz transmitter
- DHT21 temperature probe (or then DHT22,11).

## 3D Printing Info
- FDM PLA
- 0.3mm
- 60mm / sec
- 20% infill

## Support 433Mhz
- Device RF433 Chacon
- Device RF433 Otio
- Device RF433 Vachet shutter


## Exemple 
Exemple for power ON device CHACON canal A1.
```
curl "http://ipforlib0t/CHACON/A1/ON"
```

## License

[GPLv3](https://www.gnu.org/licenses/gpl-3.0.html)