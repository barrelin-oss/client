#pragma once

#include "ui/ui_system.hpp"

namespace hb
{

class level_up_settings_dialog : public dialog
{
public:
    level_up_settings_dialog();
    ~level_up_settings_dialog() override = default;

    void render(renderer& rend) override;
};

} // namespace hb
