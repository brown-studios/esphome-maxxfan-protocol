#pragma once

#include "esphome/components/remote_base/remote_base.h"

namespace esphome {
namespace remote_base {

struct MaxxfanData {
  bool fan_on;
  bool special;
  bool fan_exhaust;
  bool cover_open;
  bool auto_mode;
  bool warn;
  uint8_t fan_speed;
  uint8_t auto_temperature;

  bool operator==(const MaxxfanData &rhs) const {
    return fan_on == rhs.fan_on && special == rhs.special && fan_exhaust == rhs.fan_exhaust &&
        cover_open == rhs.cover_open && auto_mode == rhs.auto_mode && warn == rhs.warn &&
        fan_speed == rhs.fan_speed &&
        auto_temperature == rhs.auto_temperature;
  }
};

class MaxxfanProtocol : public RemoteProtocol<MaxxfanData> {
 public:
#if ESPHOME_VERSION_CODE >= VERSION_CODE(2026, 10, 0)
  void encode(RemoteTransmitData *dst, const MaxxfanData &data);
  optional<MaxxfanData> decode(RemoteReceiveData src);
  void dump(const MaxxfanData &data);
#else
  void encode(RemoteTransmitData *dst, const MaxxfanData &data) override;
  optional<MaxxfanData> decode(RemoteReceiveData src) override;
  void dump(const MaxxfanData &data) override;
#endif
};

DECLARE_REMOTE_PROTOCOL(Maxxfan)

template<typename... Ts> class MaxxfanAction : public RemoteTransmitterActionBase<Ts...> {
 public:
  TEMPLATABLE_VALUE(bool, fan_on)
  TEMPLATABLE_VALUE(uint8_t, fan_speed)
  TEMPLATABLE_VALUE(bool, fan_exhaust)
  TEMPLATABLE_VALUE(bool, cover_open)
  TEMPLATABLE_VALUE(bool, auto_mode)
  TEMPLATABLE_VALUE(uint8_t, auto_temperature)
  TEMPLATABLE_VALUE(bool, special)
  TEMPLATABLE_VALUE(bool, warn)

  void encode(RemoteTransmitData *dst, Ts... x) override {
    MaxxfanData data{};
    data.fan_on = this->fan_on_.value(x...);
    data.fan_speed = this->fan_speed_.value(x...);
    data.fan_exhaust = this->fan_exhaust_.value(x...);
    data.cover_open = this->cover_open_.value(x...);
    data.auto_mode = this->auto_mode_.value(x...);
    data.auto_temperature = this->auto_temperature_.value(x...);
    data.special = this->special_.value(x...);
    data.warn = this->warn_.value(x...);
    MaxxfanProtocol().encode(dst, data);
  }
};

}  // namespace remote_base
}  // namespace esphome
