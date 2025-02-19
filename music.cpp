#include <gtk/gtk.h>
#include <SDL.h>
#include <SDL_mixer.h>
#include <iostream>
#include <thread>
#include <atomic>

std::atomic<bool> is_playing(false);
std::atomic<bool> is_paused(false);

bool init_audio() {
    if (SDL_Init(SDL_INIT_AUDIO) < 0) {
        std::cerr << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
        return false;
    }
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        std::cerr << "SDL_mixer could not initialize! SDL_mixer Error: " << Mix_GetError() << std::endl;
        return false;
    }
    return true;
}

void close_audio() {
    Mix_Quit();
    SDL_Quit();
}

void playMP3(const char* path, GtkWidget* button, GtkWidget* play_image, GtkWidget* pause_image) {
    Mix_Music *music = Mix_LoadMUS(path);
    if (!music) {
        std::cerr << "Failed to load music! SDL_mixer Error: " << Mix_GetError() << std::endl;
        return;
    }

    is_playing = true;
    is_paused = false;

    g_idle_add([](gpointer data) -> gboolean {
        auto* params = static_cast<std::tuple<GtkWidget*, GtkWidget*>*>(data);
        gtk_button_set_image(GTK_BUTTON(std::get<0>(*params)), std::get<1>(*params));
        delete params;
        return FALSE;
    }, new std::tuple<GtkWidget*, GtkWidget*>(button, pause_image));

    if (Mix_PlayMusic(music, -1) == -1) { // Loop indefinitely
        std::cerr << "Failed to play music! SDL_mixer Error: " << Mix_GetError() << std::endl;
        Mix_FreeMusic(music);
        return;
    }

    while (is_playing) {
        SDL_Delay(100);
    }

    Mix_HaltMusic();
    Mix_FreeMusic(music);

    g_idle_add([](gpointer data) -> gboolean {
        auto* params = static_cast<std::tuple<GtkWidget*, GtkWidget*>*>(data);
        gtk_button_set_image(GTK_BUTTON(std::get<0>(*params)), std::get<1>(*params));
        delete params;
        return FALSE;
    }, new std::tuple<GtkWidget*, GtkWidget*>(button, play_image));
}

static void on_play_button_clicked(GtkWidget* widget, gpointer data) {
    const char* mp3_path = static_cast<const char*>(data);
    GtkWidget* button = static_cast<GtkWidget*>(g_object_get_data(G_OBJECT(widget), "button"));
    GtkWidget* play_image = static_cast<GtkWidget*>(g_object_get_data(G_OBJECT(widget), "play_image"));
    GtkWidget* pause_image = static_cast<GtkWidget*>(g_object_get_data(G_OBJECT(widget), "pause_image"));

    if (is_playing && !is_paused) {
        Mix_PauseMusic();
        is_paused = true;
        gtk_button_set_image(GTK_BUTTON(button), play_image);
    } else if (is_playing && is_paused) {
        Mix_ResumeMusic();
        is_paused = false;
        gtk_button_set_image(GTK_BUTTON(button), pause_image);
    } else {
        std::thread(playMP3, mp3_path, button, play_image, pause_image).detach();
    }
}

static void activate(GtkApplication* app, gpointer user_data) {
    GtkWidget *window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "Music Player");
    gtk_window_set_default_size(GTK_WINDOW(window), 400, 400);

    GtkWidget *grid = gtk_grid_new();

    // Create image widgets for the play and pause buttons
    GtkWidget *play_image = gtk_image_new_from_file("img/play.png");
    GtkWidget *pause_image = gtk_image_new_from_file("img/pause.png");

    // Resize the images to be 1/4 of their original size
    GdkPixbuf *play_pixbuf = gtk_image_get_pixbuf(GTK_IMAGE(play_image));
    int width = gdk_pixbuf_get_width(play_pixbuf) / 4;
    int height = gdk_pixbuf_get_height(play_pixbuf) / 4;
    GdkPixbuf *scaled_play_pixbuf = gdk_pixbuf_scale_simple(play_pixbuf, width, height, GDK_INTERP_BILINEAR);
    gtk_image_set_from_pixbuf(GTK_IMAGE(play_image), scaled_play_pixbuf);

    GdkPixbuf *pause_pixbuf = gtk_image_get_pixbuf(GTK_IMAGE(pause_image));
    GdkPixbuf *scaled_pause_pixbuf = gdk_pixbuf_scale_simple(pause_pixbuf, width, height, GDK_INTERP_BILINEAR);
    gtk_image_set_from_pixbuf(GTK_IMAGE(pause_image), scaled_pause_pixbuf);

    // Create a button and add the play image to it
    GtkWidget *play_button = gtk_button_new();
    gtk_button_set_image(GTK_BUTTON(play_button), play_image);

    const char* mp3_path = "/home/kevin2holt/Music/01-overture.mp3";

    // Store references to the images and button in the button's data
    g_object_set_data(G_OBJECT(play_button), "button", play_button);
    g_object_set_data(G_OBJECT(play_button), "play_image", play_image);
    g_object_set_data(G_OBJECT(play_button), "pause_image", pause_image);

    g_signal_connect(play_button, "clicked", G_CALLBACK(on_play_button_clicked), (gpointer)mp3_path);

    // Attach the button to the grid, centered along the bottom edge
    gtk_grid_attach(GTK_GRID(grid), play_button, 1, 2, 1, 1);
    gtk_grid_set_column_homogeneous(GTK_GRID(grid), TRUE);
    gtk_grid_set_row_homogeneous(GTK_GRID(grid), TRUE);

    gtk_container_add(GTK_CONTAINER(window), grid);

    gtk_widget_show_all(window);
}

int main(int argc, char **argv) {
    if (!init_audio()) {
        std::cerr << "Failed to initialize audio!" << std::endl;
        return 1;
    }

    GtkApplication *app = gtk_application_new("com.Avnior.MusicPlayer", G_APPLICATION_FLAGS_NONE);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);

    close_audio();
    return status;
}