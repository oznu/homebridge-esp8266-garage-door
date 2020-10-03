# homebridge-esp8266-garage-door

An esp8266 powered garage door controller with HomeKit support powered by Homebridge.

## PlatformIO

This project uses [PlatformIO](https://platformio.org/) to manage dependencies and build process. See https://platformio.org/platformio-ide for installation instructions.

Setup:

1. Create an `auth.h` file based on the [`src/auth.h-template`](./src/auth.h-template) file and define a secure alphanumeric username and password.
2. Review the GPIO pins defined in [`src/settings.h`](./src/settings.h) and adjust as required.

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

## Homebridge Plugin

If you just want to install the plugin, search for and install [@oznu/homebridge-esp8266-garage-door](https://www.npmjs.com/package/@oznu/homebridge-esp8266-garage-door).


```
sudo npm install -g @oznu/homebridge-esp8266-garage-door
```

### Development

The Homebridge plugin is located in the [`./homebridge`](./homebridge) directory.

```
cd homebridge
```

### Build

TypeScript needs to be compiled into JavaScript before it can run. The following command will compile the contents of your [`src`](./src) directory and put the resulting code into the `dist` folder.

```
npm run build
```


### Link To Homebridge

Run this command so your global install of Homebridge can discover the plugin in your development environment:

```
npm link
```

You can now start Homebridge, use the `-D` flag so you can see debug log messages in your plugin:

```
homebridge -D
```

### Watch For Changes and Build Automatically

If you want to have your code compile automatically as you make changes, and restart Homebridge automatically between changes you can run:

```
npm run watch
```