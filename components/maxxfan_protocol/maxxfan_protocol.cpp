#include "maxxfan_protocol.h"

#include "esphome/core/log.h"

namespace esphome {
namespace remote_base {

static const char *const TAG = "remote.maxxfan";

class BitReader {
 public:
  BitReader(const RemoteReceiveData& data) : data_(data) {}

  int next_bit() {
    constexpr int32_t BIT_TIME_US = 800;
    for (;;) {
      if (!data_.is_valid(index_)) return -1; // no more data
      const int32_t duration = data_[index_];
      const int32_t remaining = abs(duration) - consumed_;
      if (remaining > BIT_TIME_US * 2 / 3) {
        consumed_ += BIT_TIME_US;
        return duration > 0 ? 0 : 1;
      }
      if (remaining > BIT_TIME_US / 3) return -1; // malformed bit
      index_ += 1;
      consumed_ = 0;
    }
  }

 private:
  const RemoteReceiveData& data_;
  uint32_t index_{};
  int32_t consumed_{};
};


void MaxxfanProtocol::encode(RemoteTransmitData *dst, const MaxxfanData &data) {
    // NOT IMPLEMENTED
}

optional<MaxxfanData> MaxxfanProtocol::decode(RemoteReceiveData src) {
  if (false) {
    char buffer[256];
    size_t index = 0;
    BitReader reader(src);
    int bit;
    while ((bit = reader.next_bit()) >= 0 && index < sizeof(buffer) - 1) {
      buffer[index++] = bit ? '1' : '0';
    }
    buffer[index] = 0;
    ESP_LOGD(TAG, "Received bits: %s", buffer);
  }

  uint8_t packet[16];
  BitReader reader(src);
  for (size_t i = 0; i < sizeof(packet); i++) {
    uint8_t byte = 0;
    if (reader.next_bit() != 0) return {}; // expected start bit
    for (uint32_t j = 0; j < 8; j++) {
      int bit = reader.next_bit();
      if (bit < 0) return {}; // expected data bit
      byte = (byte >> 1) | (bit ? 0x80 : 0x00);
    }
    if (reader.next_bit() != 1 || reader.next_bit() != 1) return {}; // expected two stop bits
    packet[i] = byte;
  }
  if (reader.next_bit() != 1) return {}; // expected end of transmission

  if (false) {
    ESP_LOGD(TAG, "Received packet: %02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x",
        packet[0], packet[1], packet[2], packet[3],
        packet[4], packet[5], packet[6], packet[7],
        packet[8], packet[9], packet[10], packet[11],
        packet[12], packet[13], packet[14], packet[15]);
  }

  constexpr uint8_t preamble[] = { 0x5a, 0xa5, 0x80, 0x7f, 0x40, 0xbf, 0x20, 0xdf, 0x10, 0xcc };
  if (memcmp(packet, preamble, sizeof(preamble))) return {}; // preamble mismatch

  const uint8_t state = packet[10];
  const uint8_t fan_speed = packet[11];
  const uint8_t auto_temperature = packet[12];
  const uint8_t unknown1 = packet[13];
  const uint8_t unknown2 = packet[14];
  const uint8_t checksum = packet[15];
  if ((state ^ fan_speed ^ auto_temperature ^ unknown1 ^ unknown2) != checksum) return {}; // checksum mismatch

  if (unknown1 != 0xff || unknown2 != 0x23 || state & 0xc0) {
    ESP_LOGD(TAG, "Packet has curious data of unknown purpose: unknown1=0x%02x, unknown2=0x%02x, state=0x%02x",
        unknown1, unknown2, state);
  }

  MaxxfanData out{
    .fan_on = bool(state & 0x01),
    .cover_override = bool(state & 0x02),
    .fan_exhaust = bool(state & 0x04),
    .cover_open = bool(state & 0x08),
    .auto_mode = bool(state & 0x10),
    .warn = bool(state & 0x20),
    .fan_speed = fan_speed,
    .auto_temperature = auto_temperature
  };
  return out;
}

void MaxxfanProtocol::dump(const MaxxfanData &data) {
  ESP_LOGD(TAG, "Received Maxxfan: fan_on=%d, fan_exhaust=%d, fan_speed=%d, cover_open=%d, cover_override=%d, auto_mode=%d, auto_temperature=%d, warn=%d",
      data.fan_on, data.fan_exhaust, data.fan_speed, data.cover_open, data.cover_override, data.auto_mode, data.auto_temperature, data.warn);
}

}  // namespace remote_base
}  // namespace esphome
