#include "patcher/ui.hpp"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <commctrl.h>

#pragma comment(lib, "comctl32.lib")

namespace hb::patcher
{

namespace
{
constexpr WORD channel_button_id = 1001;
} // namespace

class win32_patcher_ui : public patcher_ui
{
public:
    win32_patcher_ui() = default;
    ~win32_patcher_ui() override { close(); }

    auto create() -> bool override
    {
        INITCOMMONCONTROLSEX icex;
        icex.dwSize = sizeof(icex);
        icex.dwICC = ICC_PROGRESS_CLASS;
        InitCommonControlsEx(&icex);

        WNDCLASSEXW wc = {};
        wc.cbSize = sizeof(wc);
        wc.lpfnWndProc = wnd_proc;
        wc.hInstance = GetModuleHandleW(nullptr);
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
        wc.lpszClassName = L"HelbreathPatcher";

        RegisterClassExW(&wc);

        window_ = CreateWindowExW(
            0, L"HelbreathPatcher", L"Helbreath Patcher",
            WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
            CW_USEDEFAULT, CW_USEDEFAULT, 470, 200,
            nullptr, nullptr, wc.hInstance, this);

        if (!window_)
        {
            return false;
        }

        // Status label
        status_label_ = CreateWindowExW(
            0, L"STATIC", L"Initializing...",
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            20, 15, 420, 20,
            window_, nullptr, wc.hInstance, nullptr);

        // Progress bar
        progress_bar_ = CreateWindowExW(
            0, PROGRESS_CLASSW, nullptr,
            WS_CHILD | WS_VISIBLE | PBS_SMOOTH,
            20, 45, 420, 25,
            window_, nullptr, wc.hInstance, nullptr);
        SendMessageW(progress_bar_, PBM_SETRANGE, 0, MAKELPARAM(0, 1000));

        // Detail label
        detail_label_ = CreateWindowExW(
            0, L"STATIC", L"",
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            20, 80, 420, 20,
            window_, nullptr, wc.hInstance, nullptr);

        // Channel label
        channel_label_ = CreateWindowExW(
            0, L"STATIC", L"Channel:",
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            20, 115, 60, 20,
            window_, nullptr, wc.hInstance, nullptr);

        // Channel toggle button
        channel_button_ = CreateWindowExW(
            0, L"BUTTON", L"Stable",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            85, 112, 100, 24,
            window_, reinterpret_cast<HMENU>(channel_button_id), wc.hInstance, nullptr);

        // Set font
        auto* font = static_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
        SendMessageW(status_label_, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
        SendMessageW(detail_label_, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
        SendMessageW(channel_label_, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
        SendMessageW(channel_button_, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);

        ShowWindow(window_, SW_SHOW);
        UpdateWindow(window_);
        return true;
    }

    void set_status(const std::string& text) override
    {
        if (status_label_)
        {
            SetWindowTextA(status_label_, text.c_str());
        }
    }

    void set_detail(const std::string& text) override
    {
        if (detail_label_)
        {
            SetWindowTextA(detail_label_, text.c_str());
        }
    }

    void set_progress(double fraction) override
    {
        if (progress_bar_)
        {
            auto pos = static_cast<int>(fraction * 1000.0);
            SendMessageW(progress_bar_, PBM_SETPOS, pos, 0);
        }
    }

    void show_error(const std::string& title, const std::string& message) override
    {
        MessageBoxA(window_, message.c_str(), title.c_str(), MB_OK | MB_ICONERROR);
    }

    void pump_events() override
    {
        MSG msg;
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    void close() override
    {
        if (window_)
        {
            DestroyWindow(window_);
            window_ = nullptr;
        }
    }

    auto is_closed() const -> bool override
    {
        return closed_;
    }

    void set_channels(const std::vector<std::string>& channels, const std::string& active) override
    {
        channels_ = channels;
        active_index_ = 0;
        for (size_t i = 0; i < channels.size(); ++i)
        {
            if (channels[i] == active)
            {
                active_index_ = static_cast<int>(i);
                break;
            }
        }
        update_button_label();
    }

    void set_channel_enabled(bool enabled) override
    {
        if (channel_button_)
        {
            EnableWindow(channel_button_, enabled ? TRUE : FALSE);
        }
    }

private:
    void update_button_label()
    {
        if (!channel_button_ || channels_.empty())
            return;

        auto display = channels_[active_index_];
        if (!display.empty())
            display[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(display[0])));

        SetWindowTextA(channel_button_, display.c_str());
    }

    void handle_channel_click()
    {
        if (channels_.size() < 2)
            return;

        active_index_ = (active_index_ + 1) % static_cast<int>(channels_.size());
        update_button_label();

        if (on_channel_changed_)
        {
            on_channel_changed_(channels_[active_index_]);
        }
    }

    static LRESULT CALLBACK wnd_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
    {
        if (msg == WM_CREATE)
        {
            auto* cs = reinterpret_cast<CREATESTRUCTW*>(lparam);
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(cs->lpCreateParams));
            return 0;
        }

        auto* self = reinterpret_cast<win32_patcher_ui*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));

        if (msg == WM_COMMAND)
        {
            if (LOWORD(wparam) == channel_button_id && HIWORD(wparam) == BN_CLICKED)
            {
                if (self)
                {
                    self->handle_channel_click();
                }
                return 0;
            }
        }

        if (msg == WM_CLOSE)
        {
            if (self)
            {
                self->closed_ = true;
            }
            DestroyWindow(hwnd);
            return 0;
        }

        if (msg == WM_DESTROY)
        {
            if (self)
            {
                self->window_ = nullptr;
            }
            return 0;
        }

        return DefWindowProcW(hwnd, msg, wparam, lparam);
    }

    HWND window_ = nullptr;
    HWND status_label_ = nullptr;
    HWND detail_label_ = nullptr;
    HWND progress_bar_ = nullptr;
    HWND channel_label_ = nullptr;
    HWND channel_button_ = nullptr;
    std::vector<std::string> channels_;
    int active_index_ = 0;
    bool closed_ = false;
};

auto create_patcher_ui() -> std::unique_ptr<patcher_ui>
{
    return std::make_unique<win32_patcher_ui>();
}

} // namespace hb::patcher
