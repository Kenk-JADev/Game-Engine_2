/**
 * @file types.hpp
 * @brief Grundlegende Typaliase und Hilfstypen der Aether Engine.
 */
#pragma once

#include <cstdint>
#include <cstddef>
#include <string>
#include <string_view>

namespace aether {

// -----------------------------------------------------------------------------
// Feste Integer-Breiten
// -----------------------------------------------------------------------------
using u8  = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;

using i8  = std::int8_t;
using i16 = std::int16_t;
using i32 = std::int32_t;
using i64 = std::int64_t;

using f32 = float;
using f64 = double;

using usize = std::size_t;
using isize = std::ptrdiff_t;

// -----------------------------------------------------------------------------
// Engine-IDs (stark typisiert über using – Erweiterung zu strong typedef möglich)
// -----------------------------------------------------------------------------
using EntityId   = u32;
using AssetId    = u32;
using MapId      = u32;
using EventId    = u32;
using ActorId    = u32;

inline constexpr EntityId kInvalidEntity = 0;
inline constexpr AssetId  kInvalidAsset  = 0;

// -----------------------------------------------------------------------------
// Ergebnis-Hilfen
// -----------------------------------------------------------------------------

/**
 * @brief Einfacher Fehlerwert mit Nachricht (ohne Exceptions im Hot-Path).
 */
struct Error {
    std::string message;

    Error() = default;
    explicit Error(std::string msg) : message(std::move(msg)) {}
    explicit Error(std::string_view msg) : message(msg) {}

    [[nodiscard]] const std::string& what() const noexcept { return message; }
    [[nodiscard]] bool empty() const noexcept { return message.empty(); }
};

/**
 * @brief Minimaler Result-Typ: entweder Wert oder Fehler.
 * @tparam T Erfolgstyp
 */
template <typename T>
class Result {
public:
    static Result ok(T value) {
        Result r;
        r.value_ = std::move(value);
        r.ok_ = true;
        return r;
    }

    static Result fail(Error err) {
        Result r;
        r.error_ = std::move(err);
        r.ok_ = false;
        return r;
    }

    static Result fail(std::string msg) {
        return fail(Error{std::move(msg)});
    }

    [[nodiscard]] bool is_ok() const noexcept { return ok_; }
    [[nodiscard]] explicit operator bool() const noexcept { return ok_; }

    [[nodiscard]] T& value() & { return value_; }
    [[nodiscard]] const T& value() const& { return value_; }
    [[nodiscard]] T&& value() && { return std::move(value_); }

    [[nodiscard]] const Error& error() const noexcept { return error_; }

private:
    Result() = default;
    bool ok_ = false;
    T value_{};
    Error error_{};
};

/**
 * @brief Result-Spezialisierung für void-Erfolg.
 */
template <>
class Result<void> {
public:
    static Result ok() {
        Result r;
        r.ok_ = true;
        return r;
    }

    static Result fail(Error err) {
        Result r;
        r.error_ = std::move(err);
        r.ok_ = false;
        return r;
    }

    static Result fail(std::string msg) {
        return fail(Error{std::move(msg)});
    }

    [[nodiscard]] bool is_ok() const noexcept { return ok_; }
    [[nodiscard]] explicit operator bool() const noexcept { return ok_; }
    [[nodiscard]] const Error& error() const noexcept { return error_; }

private:
    bool ok_ = false;
    Error error_{};
};

// -----------------------------------------------------------------------------
// NonCopyable / NonMovable Mixins
// -----------------------------------------------------------------------------

struct NonCopyable {
    NonCopyable() = default;
    NonCopyable(const NonCopyable&) = delete;
    NonCopyable& operator=(const NonCopyable&) = delete;
    NonCopyable(NonCopyable&&) = default;
    NonCopyable& operator=(NonCopyable&&) = default;
};

struct NonMovable {
    NonMovable() = default;
    NonMovable(const NonMovable&) = delete;
    NonMovable& operator=(const NonMovable&) = delete;
    NonMovable(NonMovable&&) = delete;
    NonMovable& operator=(NonMovable&&) = delete;
};

} // namespace aether
