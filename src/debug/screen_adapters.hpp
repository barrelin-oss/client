#pragma once

#include "debug/positionable.hpp"
#include <cstdint>
#include <string>

namespace hb::debug {

// Adapter that wraps a simple (x, y) point as a positionable element
// with a fixed size (for elements that are just a position like a panel origin)
class screen_point_adapter : public positionable
{
public:
    screen_point_adapter(std::string id, int32_t& x_ref, int32_t& y_ref,
                         int32_t width, int32_t height)
        : id_(std::move(id))
        , x_ref_(x_ref)
        , y_ref_(y_ref)
        , width_(width)
        , height_(height)
    {
    }

    std::string position_id() const override
    {
        return id_;
    }

    bounds get_bounds() const override
    {
        return {x_ref_, y_ref_, width_, height_};
    }

    void set_position(int32_t x, int32_t y) override
    {
        x_ref_ = x;
        y_ref_ = y;
    }

private:
    std::string id_;
    int32_t& x_ref_;
    int32_t& y_ref_;
    int32_t width_;
    int32_t height_;
};

// Adapter that wraps an (x, y, width, height) rect as a positionable element
// Used when both position and size might be variable
class screen_rect_adapter : public positionable
{
public:
    screen_rect_adapter(std::string id, int32_t& x_ref, int32_t& y_ref,
                        int32_t& width_ref, int32_t& height_ref)
        : id_(std::move(id))
        , x_ref_(x_ref)
        , y_ref_(y_ref)
        , width_ref_(width_ref)
        , height_ref_(height_ref)
    {
    }

    std::string position_id() const override
    {
        return id_;
    }

    bounds get_bounds() const override
    {
        return {x_ref_, y_ref_, width_ref_, height_ref_};
    }

    void set_position(int32_t x, int32_t y) override
    {
        x_ref_ = x;
        y_ref_ = y;
    }

private:
    std::string id_;
    int32_t& x_ref_;
    int32_t& y_ref_;
    int32_t& width_ref_;
    int32_t& height_ref_;
};

// Adapter for elements that only need to track a button-like position
// (position + fixed dimensions derived from sprite)
class button_adapter : public positionable
{
public:
    button_adapter(std::string id, int32_t& x_ref, int32_t& y_ref,
                   int32_t width, int32_t height)
        : id_(std::move(id))
        , x_ref_(x_ref)
        , y_ref_(y_ref)
        , width_(width)
        , height_(height)
    {
    }

    std::string position_id() const override
    {
        return id_;
    }

    bounds get_bounds() const override
    {
        return {x_ref_, y_ref_, width_, height_};
    }

    void set_position(int32_t x, int32_t y) override
    {
        x_ref_ = x;
        y_ref_ = y;
    }

private:
    std::string id_;
    int32_t& x_ref_;
    int32_t& y_ref_;
    int32_t width_;
    int32_t height_;
};

} // namespace hb::debug
