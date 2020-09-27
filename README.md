# homebridge-esp8266-garage-door

An esp8266 powered garage door controller with HomeKit support powered by Homebridge.

## PlatformIO

This project uses [PlatformIO](https://platformio.org/) to manage dependencies and build process. See https://platformio.org/platformio-ide for installation instructions.

Build:

```
pio run
```

Upload

```
pio run -t upload
```

Monitor Serial

```
pio device monitor
```

You may need to adjust the `upload_port` and `monitor_port` in the [`platformio.ini`](./platformio.ini) file. You can list the available devices using:

```
pio device list
```