# esphome-maxxfan-protocol

Maxxfan infrared remote control protocol component for ESPHome.

This component was originally developed for [Minuet](https://github.com/j9brown/minuet), a smart brushless DC motor controller for the [MAXXAIR Maxxfan](https://www.maxxair.com/products/fans/maxxfan-deluxe/).  You can also use it for other projects.

## Configuration

### Including the component

Add this stanza to your ESPHome configuration to pull the `maxxfan_protocol` component from github.

```yaml
# Import the maxxfan protocol component from Github.
external_components:
  - source: github://j9brown/esphome-maxxfan-protocol@main
    components: [ maxxfan_protocol ]

# Enable the Maxxfan protocol.
maxxfan_protocol:
```

### Receiving Maxxfan remote control messages

The remote receiver's `on_maxxfan` event handler receives the [MaxxfanData](components/maxxfan_protocol/maxxfan_protocol.h) object that was parsed from the received message.

Here's what you need to add to your ESPHome configuration to receive Maxxfan messages, assuming you have connected a 38 KHz infrared receiver to pin 2.

```yaml
# Configure the IR receiver.
remote_receiver:
  - id: ir_receiver
    pin:
      number: 2  # Change this to the pin your IR receiver is attached to
      mode: input
      inverted: true
    dump: maxxfan
    on_maxxfan:
      then:
        lambda: |-
          // The variable 'x' holds a MaxxfanData object parsed from the received message
          ESP_LOGD("maxxfan-example", "Fan state: %s", x.fan_on ? "on" : "off");
```

See [maxxfan-receiver-example.yaml](maxxfan-receiver-example.yaml) for a complete example that dumps all received Maxxfan IR messages.

### Transmitting Maxxfan remote control messages

The `remote_transmitter.transmit_maxxfan` action transmits a message to the fan and sets all parameters at once.  Any parameters that you don't specify in the action will be set to their default values.  All parameters are templatable.

| Parameter        | Value                                                                         | Default |
| ---------------- | ----------------------------------------------------------------------------- | ------- |
| fan_on           | Fan state: `true` turns fan on, `false` turns fan off                         | `false` |
| fan_speed        | Fan speed percent: 10, 20, 30, 40, 50, 60, 70, 80, 90, 100                    | `10`    |
| fan_exhaust      | Fan direction: `true` for exhaust, `false` for intake                         | `false` |
| cover_open       | Cover state: `true` for open, `false` for close                               | `false` |
| auto_mode        | Mode: `true` for automatic mode (thermostat control), `false` for manual mode | `false` |
| auto_temperature | Thermostat setpoint in Fahrenheight: range 29 to 99                           | `78`    |
| special          | Special modes: see [protocol details](#protocol-details)                      | `false` |
| warn             | Emit warning tone: `true` to beep twice, `false` to not beep                  | `false` |

Here's what you need to add to your ESPHome configuration to transmit Maxxfan messages, assuming you have connected an infrared LED to pin 10 (with a suitable current limiting resistor).

```yaml
# Configure the IR transmitter.
remote_transmitter:
  - id: ir_transmitter
    pin:
      number: 10  # Change this to the pin your IR transmitter is attached to
      inverted: false
    carrier_duty_percent: 50%

# Turn the fan on high when the script is invoked.
script:
  - id: fan_high
    then:
      - logger.log: "Transmitting message to Maxxfan"
      - remote_transmitter.transmit_maxxfan:
          transmitter_id: ir_transmitter
          fan_on: true
          fan_exhaust: true
          fan_speed: 10
          cover_open: true
          #auto_mode: false
          #auto_temperature: 78
          #special: false
          #warn: false
```

See [maxxfan-transmitter-example.yaml](maxxfan-transmitter-example.yaml) for a complete example with a simple web-based user interface that acts as a remote control for the Maxxfan and shows how to use ESPHome template entities to track the state of the device.

## Protocol details

The Maxxfan IR remote protocol looks like RS232 with 1 start bit (zero), 8 data bits encoded least-significant-bit first, no parity bits, and 2 stop bits (ones). A mark symbol encodes a zero bit and a space symbol encodes a one bit.  Each bit period is 800 us.  The transmission ends with a series of space symbols.

The packet is 16 bytes long and consists of a fixed preamble followed by some control fields and a checksum.

Here's an example of a packet and its decoding:

| Field      | Encoded bits    | Decoded bits | Meaning                                             |
| ---------- | --------------- | ------------ | -------------------------------------------------   |
| preamble   | `0 01011010 11` | `01011010`   | 0x5A bit pattern                                    |
| preamble   | `0 10100101 11` | `10100101`   | 0xA5 bit pattern (0x5A inverted)                    |
| preamble   | `0 00000001 11` | `10000000`   | 0x80 bit pattern                                    |
| preamble   | `0 11111110 11` | `01111111`   | 0x7F bit pattern (0x80 inverted)                    |
| preamble   | `0 00000010 11` | `01000000`   | 0x40 bit pattern                                    |
| preamble   | `0 11111101 11` | `10111111`   | 0xBF bit pattern (0x40 inverted)                    |
| preamble   | `0 00000100 11` | `00100000`   | 0x20 bit pattern                                    |
| preamble   | `0 11111011 11` | `11011111`   | 0xDF bit pattern (0x20 inverted)                    |
| preamble   | `0 00001000 11` | `00010000`   | 0x10 bit pattern                                    |
| preamble   | `0 00110011 11` | `11001100`   | 0xCC bit pattern                                    |
| state      | `0 00100100 11` | `00100100`   | State: fan off, fan exhaust, cover close, warn      |
| speed      | `0 00100110 11` | `01100100`   | Speed percent: 100%                                 |
| auto temp  | `0 00100010 11` | `01000100`   | Thermostat temperature setpoint in Fahrenheit: 68 F |
| ???        | `0 11111111 11` | `11111111`   | Unknown purpose, always 0xff                        |
| ???        | `0 11000100 11` | `00100011`   | Unknown purpose, always 0x23                        |
| checksum   | `0 00011011 11` | `11011000`   | XOR of previous 5 bytes                             |
| end        | `11111111`      |              | End of transmission                                 |

The *speed* field sets fan speed percentage as a multiple of 10 between 0 and 100 percent inclusively.

The *auto temp* field sets the auto mode thermostat temperature setpoint in the range from 29 to 99 Fahrenheit inclusively.

The *state* field holds a combination of flags as follows (numbered from LSB):

| Bit | Flag            | State | Meaning                                                                                                          |
| --- | --------------- | ----- | ---------------------------------------------------------------------------------------------------------------- |
| 0   | fan state       | `0`   | Fan off                                                                                                          |
|     |                 | `1`   | Fan on                                                                                                           |
| 1   | special         | `0`   | Fan operating normally, the cover opens when the fan turns on and closes when the fan turns off                  |
|     |                 | `1`   | Either auto mode (thermostat controlled) or ceiling fan mode (fan on with cover closed) is active                |
| 2   | fan direction   | `0`   | Intake                                                                                                           |
|     |                 | `1`   | Exhaust                                                                                                          |
| 3   | cover state     | `0`   | Cover closed                                                                                                     |
|     |                 | `1`   | Cover open                                                                                                       |
| 4   | mode            | `0`   | Manual mode                                                                                                      |
|     |                 | `1`   | Auto mode (thermostat controlled)                                                                                |
| 5   | warn            | `0`   | No warning                                                                                                       |
|     |                 | `1`   | Warn (beep twice) because the user attempted to raise or lower the speed or temperature beyond the allowed range |
| 6   | ???             | `0`   | Unknown purpose, always 0                                                                                        |
| 7   | ???             | `0`   | Unknown purpose, always 0                                                                                        |

The meaning of the *special* flag is a bit unclear.  It can be used to detect ceiling fan mode (fan state = 1, special = 1, mode = 0).  Its purpose might be to override the normal behavior of the fan and the cover.  Further experimentation is needed.

## Acknowledgements

Thanks to [skypeachblue](https://github.com/skypeachblue) and [wingspinner](https://github.com/wingspinner) for publishing information about their [reverse engineering](https://github.com/skypeachblue/maxxfan-reversing) of the Maxxfan IR remote control protocol.  It helped me create this component.
