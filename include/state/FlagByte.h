//
// Created by LeeEeZian on 20/8/2026.
//

#ifndef TCP_FLAGBYTE_H
#define TCP_FLAGBYTE_H
#include <cstdint>
#include <type_traits>

template<typename T>
concept EnumType = std::is_enum_v<T>;

template<EnumType T>
class FlagByte {
public:
    FlagByte() = default;

    explicit FlagByte(const uint8_t flagByte) : flagByte_ {flagByte} {
    }

    void setFlag(T flag) {
        flagByte_ |= static_cast<uint8_t>(flag);
    }

    void clearFlag(T flag) {
        flagByte_ &= ~static_cast<uint8_t>(flag);
    }

    [[nodiscard]] bool hasFlag(T flag) const {
        return flagByte_ & static_cast<uint8_t>(flag);
    }

    void setFlagByte(const uint8_t flagByte) {
        flagByte_ = flagByte;
    }
    void reset() { flagByte_ = 0; }

    [[nodiscard]] uint8_t getHexa() const { return flagByte_; }

private:
    uint8_t flagByte_{};


};

#endif //TCP_FLAGBYTE_H
