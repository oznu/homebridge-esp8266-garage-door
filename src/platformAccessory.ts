import { Service, PlatformAccessory, CharacteristicValue, CharacteristicSetCallback } from 'homebridge';
import { WebSocket } from '@oznu/ws-connect';

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
    private readonly config: { host: string, port: number, name: string, serial: string },
  ) {

    this.socket = new WebSocket(`ws://${this.platform.config.username}:${this.platform.config.password}@${this.config.host}:${this.config.port}`, {
      options: {
        handshakeTimeout: 10000,
      },
    });

    this.socket.on('websocket-status', (msg) => {
      this.platform.log.info(msg);
    });

    this.socket.on('json', this.parseStatus.bind(this));

    // set accessory information
    this.accessory.getService(this.platform.Service.AccessoryInformation)!
      .setCharacteristic(this.platform.Characteristic.Name, 'Irrigation System')
      .setCharacteristic(this.platform.Characteristic.Manufacturer, 'oznu-platform')
      .setCharacteristic(this.platform.Characteristic.Model, 'garage-door')
      .setCharacteristic(this.platform.Characteristic.SerialNumber, this.config.serial);

    // create service
    this.service = this.accessory.getService(this.platform.Service.GarageDoorOpener) || this.accessory.addService(this.platform.Service.GarageDoorOpener);
    this.service.setCharacteristic(this.platform.Characteristic.Name, 'Garage Door');

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
  }

  setTargetDoorState(value: CharacteristicValue, callback: CharacteristicSetCallback) {
    callback();

    const targetDoorState = value ? 'CLOSED' : 'OPEN';

    this.platform.log.info(`Sending ${targetDoorState} to Garage Door.`);

    this.socket.sendJson({
      TargetDoorState: targetDoorState,
      contactTime: this.platform.config.contactTime || 1000,
    });
  }

}
