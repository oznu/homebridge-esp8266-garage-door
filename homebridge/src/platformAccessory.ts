import { Service, PlatformAccessory, CharacteristicValue, CharacteristicSetCallback } from 'homebridge';
import { WebSocket } from '@oznu/ws-connect';
import { resolve4 } from 'mdns-resolver';

import { HomebridgeEsp8266GaragePlatform } from './platform';

interface StatusPayload {
  TargetDoorState: 'OPEN' | 'CLOSED';
  CurrentDoorState: 'OPEN' | 'CLOSED' | 'OPENING' | 'CLOSING' | 'STOPPED';
  ObstructionDetected: boolean;
}

export class HomebridgeEsp8266GarageAccessory {
  private service: Service;
  private socket: WebSocket;

  constructor(
    private readonly platform: HomebridgeEsp8266GaragePlatform,
    private readonly accessory: PlatformAccessory,
    private readonly config: { host: string; port: number; name: string; serial: string },
  ) {

    this.socket = new WebSocket('', {
      options: {
        handshakeTimeout: 10000,
      },
      beforeConnect: async () => {
        try {
          const hostIp = await resolve4(this.config.host);
          const socketAddress = `ws://${this.platform.config.username}:${this.platform.config.password}@${hostIp}:${this.config.port}`;
          this.socket.setAddresss(socketAddress);
        } catch (e) {
          this.platform.log.warn(e.message);
        }
      },
    });

    this.socket.on('websocket-status', (msg) => {
      this.platform.log.info(msg);
    });

    this.socket.on('open', () => {
      this.socket.sendJson({
        reverseObstructionSensor: this.platform.config.reverseObstructionSensor || false,
      });
    });

    this.socket.on('json', this.parseStatus.bind(this));

    // set accessory information
    this.accessory.getService(this.platform.Service.AccessoryInformation)!
      .setCharacteristic(this.platform.Characteristic.Name, 'Garage Door')
      .setCharacteristic(this.platform.Characteristic.Manufacturer, 'oznu-platform')
      .setCharacteristic(this.platform.Characteristic.Model, 'garage-door')
      .setCharacteristic(this.platform.Characteristic.SerialNumber, this.config.serial);

    // create service
    this.service = this.accessory.getService(this.platform.Service.GarageDoorOpener) || this.accessory.addService(this.platform.Service.GarageDoorOpener);

    this.service.getCharacteristic(this.platform.Characteristic.TargetDoorState)
      .on('set', this.setTargetDoorState.bind(this));
  }

  // parse events from the garage door controller
  parseStatus(payload: StatusPayload) {
    this.platform.log.debug(JSON.stringify(payload));

    // update the current door state
    const currentDoorState = this.platform.Characteristic.CurrentDoorState[payload.CurrentDoorState];
    if (currentDoorState !== undefined) {
      this.service.updateCharacteristic(this.platform.Characteristic.CurrentDoorState, currentDoorState);
    }

    // update the target door status
    const targetDoorState = this.platform.Characteristic.TargetDoorState[payload.TargetDoorState];
    if (targetDoorState !== undefined) {
      this.service.updateCharacteristic(this.platform.Characteristic.TargetDoorState, targetDoorState);
    }

    // set the ObstructionDetected characteristic
    if (typeof payload.ObstructionDetected === 'boolean') {
      this.service.updateCharacteristic(this.platform.Characteristic.ObstructionDetected, payload.ObstructionDetected);
    }
  }

  setTargetDoorState(value: CharacteristicValue, callback: CharacteristicSetCallback) {
    if (!this.socket.isConnected()) {
      this.platform.log.error(`Garage Door Not Connected - ${this.config.host}`);
      return callback(new Error('Garage Door Not Connected'));
    }

    callback();

    const targetDoorState = value ? 'CLOSED' : 'OPEN';

    this.platform.log.info(`Sending ${targetDoorState} to Garage Door.`);

    this.socket.sendJson({
      TargetDoorState: targetDoorState,
      contactTime: this.platform.config.contactTime || 1000,
    });
  }

}
