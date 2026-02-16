#include "patcher/ui.hpp"

#include <gtk/gtk.h>

namespace hb::patcher
{

class gtk_patcher_ui : public patcher_ui
{
public:
    gtk_patcher_ui() = default;
    ~gtk_patcher_ui() override { close(); }

    auto create() -> bool override
    {
        gtk_init(nullptr, nullptr);

        window_ = gtk_window_new(GTK_WINDOW_TOPLEVEL);
        gtk_window_set_title(GTK_WINDOW(window_), "Helbreath Patcher");
        gtk_window_set_default_size(GTK_WINDOW(window_), 450, 180);
        gtk_window_set_resizable(GTK_WINDOW(window_), FALSE);
        gtk_window_set_position(GTK_WINDOW(window_), GTK_WIN_POS_CENTER);
        gtk_container_set_border_width(GTK_CONTAINER(window_), 20);

        g_signal_connect(window_, "destroy", G_CALLBACK(on_destroy), this);

        // Vertical box layout
        auto* vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
        gtk_container_add(GTK_CONTAINER(window_), vbox);

        // Status label
        status_label_ = gtk_label_new("Initializing...");
        gtk_label_set_xalign(GTK_LABEL(status_label_), 0.0f);
        gtk_box_pack_start(GTK_BOX(vbox), status_label_, FALSE, FALSE, 0);

        // Progress bar
        progress_bar_ = gtk_progress_bar_new();
        gtk_progress_bar_set_show_text(GTK_PROGRESS_BAR(progress_bar_), TRUE);
        gtk_box_pack_start(GTK_BOX(vbox), progress_bar_, FALSE, FALSE, 4);

        // Detail label
        detail_label_ = gtk_label_new("");
        gtk_label_set_xalign(GTK_LABEL(detail_label_), 0.0f);
        auto* attrs = pango_attr_list_new();
        pango_attr_list_insert(attrs, pango_attr_scale_new(0.85));
        gtk_label_set_attributes(GTK_LABEL(detail_label_), attrs);
        pango_attr_list_unref(attrs);
        gtk_box_pack_start(GTK_BOX(vbox), detail_label_, FALSE, FALSE, 0);

        // Channel toggle button row
        auto* channel_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
        gtk_box_pack_end(GTK_BOX(vbox), channel_box, FALSE, FALSE, 4);

        auto* channel_label = gtk_label_new("Channel:");
        gtk_box_pack_start(GTK_BOX(channel_box), channel_label, FALSE, FALSE, 0);

        channel_button_ = gtk_button_new_with_label("Stable");
        gtk_widget_set_size_request(channel_button_, 100, -1);
        gtk_box_pack_start(GTK_BOX(channel_box), channel_button_, FALSE, FALSE, 0);

        g_signal_connect(channel_button_, "clicked", G_CALLBACK(on_channel_clicked), this);

        gtk_widget_show_all(window_);
        gtk_window_present(GTK_WINDOW(window_));
        return true;
    }

    void set_status(const std::string& text) override
    {
        if (status_label_)
        {
            gtk_label_set_text(GTK_LABEL(status_label_), text.c_str());
        }
    }

    void set_detail(const std::string& text) override
    {
        if (detail_label_)
        {
            gtk_label_set_text(GTK_LABEL(detail_label_), text.c_str());
        }
    }

    void set_progress(double fraction) override
    {
        if (progress_bar_)
        {
            gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress_bar_), fraction);

            auto pct = static_cast<int>(fraction * 100.0);
            auto text = std::to_string(pct) + "%";
            gtk_progress_bar_set_text(GTK_PROGRESS_BAR(progress_bar_), text.c_str());
        }
    }

    void show_error(const std::string& title, const std::string& message) override
    {
        auto* dialog = gtk_message_dialog_new(
            window_ ? GTK_WINDOW(window_) : nullptr,
            GTK_DIALOG_MODAL,
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_OK,
            "%s",
            message.c_str());
        gtk_window_set_title(GTK_WINDOW(dialog), title.c_str());
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
    }

    void pump_events() override
    {
        while (gtk_events_pending())
        {
            gtk_main_iteration();
        }
    }

    void close() override
    {
        if (window_)
        {
            gtk_widget_destroy(window_);
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
            gtk_widget_set_sensitive(channel_button_, enabled ? TRUE : FALSE);
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

        gtk_button_set_label(GTK_BUTTON(channel_button_), display.c_str());
    }

    static void on_destroy(GtkWidget* /*widget*/, gpointer data)
    {
        auto* self = static_cast<gtk_patcher_ui*>(data);
        self->window_ = nullptr;
        self->closed_ = true;
    }

    static void on_channel_clicked(GtkButton* /*button*/, gpointer data)
    {
        auto* self = static_cast<gtk_patcher_ui*>(data);
        if (self->channels_.size() < 2)
            return;

        self->active_index_ = (self->active_index_ + 1) % static_cast<int>(self->channels_.size());
        self->update_button_label();

        if (self->on_channel_changed_)
        {
            self->on_channel_changed_(self->channels_[self->active_index_]);
        }
    }

    GtkWidget* window_ = nullptr;
    GtkWidget* status_label_ = nullptr;
    GtkWidget* detail_label_ = nullptr;
    GtkWidget* progress_bar_ = nullptr;
    GtkWidget* channel_button_ = nullptr;
    std::vector<std::string> channels_;
    int active_index_ = 0;
    bool closed_ = false;
};

auto create_patcher_ui() -> std::unique_ptr<patcher_ui>
{
    return std::make_unique<gtk_patcher_ui>();
}

} // namespace hb::patcher
