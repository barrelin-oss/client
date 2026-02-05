#pragma once

namespace hb {

class game_state_manager;
class managed_dialog;

// Stateless wiring class that sets up all dialog callbacks.
// Separated from game_state_manager to keep dialog wiring code isolated.
class dialog_callbacks
{
public:
    dialog_callbacks() = default;

    void initialize(game_state_manager& game);
    void setup_callbacks();

private:
    void setup_system_menu_buttons(managed_dialog* sys_menu);

    game_state_manager* game_ = nullptr;
};

} // namespace hb
