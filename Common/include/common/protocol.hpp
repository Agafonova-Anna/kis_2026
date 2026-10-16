#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace common {

namespace msg {
inline constexpr std::string_view kHello = "HELLO";
inline constexpr std::string_view kAccept = "ACCEPT";
inline constexpr std::string_view kDecline = "DECLINE";
inline constexpr std::string_view kMove = "MOVE";
inline constexpr std::string_view kAck = "ACK";

inline constexpr std::string_view kWelcome = "WELCOME";
inline constexpr std::string_view kInvite = "INVITE";
inline constexpr std::string_view kGame = "GAME";
inline constexpr std::string_view kError = "ERROR";
} // namespace msg

inline constexpr std::uint16_t kDefaultPort = 5555;
inline constexpr std::size_t kMaxNameLength = 32;

std::vector<std::string> split(std::string_view line);
std::optional<int> parse_int(std::string_view text);
bool is_valid_name(std::string_view name);

} // namespace common
