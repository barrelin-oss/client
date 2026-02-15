#pragma once

#include "network/messages/common.hpp"

namespace hb
{

inline json make_set_chat_preferences_request(bool filter_profanity)
{
    return message_builder(msg_type::set_chat_preferences).set("filter_profanity", filter_profanity).build();
}

} // namespace hb
