#pragma once

namespace abntpiano {

// Estado de sustentação (hold) de teclas
enum class HoldState {
    Idle,
    Holding,
    Released,
    Missed
};

} // namespace abntpiano
