#include <gtk/gtk.h>
#include <SDL.h>
#include <SDL_mixer.h>
#include <iostream>
#include <thread>
#include <atomic>
#include <memory>
#include <mutex>

// Forward declarations
struct GtkApp;
struct GtkGui;
struct AudioStream;
struct Graphics;
struct Button;
struct Label;

extern GtkApp app;
extern GtkGui gui;
extern AudioStream audio;
extern Graphics graphics;

static void init_gui(GtkApplication* app, gpointer data);
static void btn_play(GtkWidget* widget, gpointer data);
static void btn_pause(GtkWidget* widget, gpointer data);

struct AudioStream {
	std::string status = "STOP";
	std::string curPath;
	std::unique_ptr<Mix_Music, decltype(&Mix_FreeMusic)> audio;
	std::mutex mtx;

	AudioStream() : audio(nullptr, Mix_FreeMusic) {}
	void init() {
		if (SDL_Init(SDL_INIT_AUDIO) < 0) {
			std::cerr << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
		}
		if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
			std::cerr << "SDL_mixer could not initialize! SDL_mixer Error: " << Mix_GetError() << std::endl;
		}
	}
	void close() {
		Mix_Quit();
		SDL_Quit();
	}
	void play(const std::string& audioPath) {
		std::thread(&AudioStream::set_audio, this, audioPath).detach();
	}
	void play() {
		std::lock_guard<std::mutex> lock(mtx);
		if (status == "PAUSE") {
			Mix_ResumeMusic();
			status = "PLAY";
		}
	}
	void pause() {
		std::lock_guard<std::mutex> lock(mtx);
		if (status == "PLAY") {
			Mix_PauseMusic();
			status = "PAUSE";
		}
	}
	void set_audio(const std::string& path) {
		std::lock_guard<std::mutex> lock(mtx);
		audio.reset(Mix_LoadMUS(path.c_str()));
		if (!audio) {
			std::cerr << "Failed to load audio! SDL_mixer Error: " << Mix_GetError() << std::endl;
			return;
		}
		if (Mix_PlayMusic(audio.get(), 1) == -1) {
			std::cerr << "Failed to play music! SDL_mixer Error: " << Mix_GetError() << std::endl;
			return;
		}
		status = "PLAY";

		while (status == "PLAY") {
			SDL_Delay(100);
		}
	}
};

struct GtkApp {
	std::unique_ptr<GtkApplication, decltype(&g_object_unref)> app;
	int status;

	GtkApp() : app(nullptr, g_object_unref), status(0) {}
	void init(int argc, char **argv) {
		app.reset(gtk_application_new("com.Avnior.MusicPlayor", G_APPLICATION_FLAGS_NONE));
		g_signal_connect(app.get(), "activate", G_CALLBACK(init_gui), nullptr);
		status = g_application_run(G_APPLICATION(app.get()), argc, argv);
	}
	void close() {
		audio.close();
	}
};

struct GtkGui {
	std::unique_ptr<GtkWidget, decltype(&g_object_unref)> window;
	std::unique_ptr<GtkWidget, decltype(&g_object_unref)> grid;

	GtkGui() : window(nullptr, g_object_unref), grid(nullptr, g_object_unref) {}
	void init(const std::string& newTitle, int newX, int newY) {
		window.reset(gtk_application_window_new(GTK_APPLICATION(app.app.get())));
		gtk_window_set_title(GTK_WINDOW(window.get()), newTitle.c_str());
		gtk_window_set_default_size(GTK_WINDOW(window.get()), newX, newY);
		g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), nullptr);

		grid.reset(gtk_grid_new());
		gtk_container_add(GTK_CONTAINER(window.get()), grid.get());
	}
	void show() {
		gtk_widget_show_all(window.get());
	}
};

struct Graphics {
	std::shared_ptr<GtkWidget> play;
	std::shared_ptr<GtkWidget> pause;

	void init() {
		play = std::shared_ptr<GtkWidget>(gtk_image_new_from_file("img/play.png"), g_object_unref);
		pause = std::shared_ptr<GtkWidget>(gtk_image_new_from_file("img/pause.png"), g_object_unref);
	}
};

struct Button {
	std::unique_ptr<GtkWidget, decltype(&g_object_unref)> btn;

	Button() : btn(gtk_button_new(), g_object_unref) {}
	void set_img(GtkWidget* img) {
		gtk_button_set_image(GTK_BUTTON(btn.get()), img);
	}
	void set_onClick(void (*callback)(GtkWidget*, gpointer)) {
		g_signal_connect(btn.get(), "clicked", G_CALLBACK(callback), nullptr);
	}
	void set_gridPos(int col, int row, int width = 1, int height = 1) {
		gtk_grid_attach(GTK_GRID(gui.grid.get()), btn.get(), col, row, width, height);
	}
};
struct Label {
	std::unique_ptr<GtkWidget, decltype(&g_object_unref)> lbl;

	Label() : lbl(gtk_label_new(""), g_object_unref) {}
	Label(string newLbl) : lbl(gtk_label_new(newLbl), g_object_unref) {}
	void set_text(string newLbl) {
		gtk_label_set_text(lbl,newLbl);
	}
	void set_gridPos(int col, int row, int width = 1, int height = 1) {
		gtk_grid_attach(GTK_GRID(gui.grid.get()), lbl.get(), col, row, width, height);
	}
}

GtkApp app;
GtkGui gui;
AudioStream audio;
Graphics graphics;

static void btn_play(GtkWidget* widget, gpointer data) {
	if (audio.status == "STOP") {
		audio.play("/home/kevin2holt/Music/01-overture.mp3");
	} else {
		audio.play();
	}
	playBtn.set_img(graphics.pause.get());
	playBtn.set_onClick(btn_pause);
}
static void btn_pause(GtkWidget* widget, gpointer data) {
	audio.pause();
	playBtn.set_img(graphics.play.get());
	playBtn.set_onClick(btn_play);
}

static void init_gui(GtkApplication* app, gpointer data) {
	gui.init("Music Player", 400, 400);

	Button playBtn;
	playBtn.set_img(graphics.play.get());
	playBtn.set_onClick(btn_play);
	playBtn.set_gridPos(0,0);

	Label text;
	text.set_text(audio.status);
	text.set_gridPos(0,1);


	gui.show();
}

int main(int argc, char **argv) {
	audio.init();
	app.init(argc, argv);
	app.close();
	return app.status;
}
