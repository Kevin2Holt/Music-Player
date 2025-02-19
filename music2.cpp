#include <gtk/gtk.h>
#include <SDL.h>
#include <SDL_mixer.h>
#include <iostream>
#include <thread>
#include <atomic>
#include <memory>

struct GtkApp {
	std::unique_ptr<GtkApplication> app;
	int status;

	void init() {
		app = std::make_unique<GtkApplication>(gtk_application_new("com.Avnior.MusicPlayor", G_APPLICATION_FLAGS_NONE));
		g_signal_connect(app, "activate", G_CALLBACK(init_gui), NULL);
		status= g_application_run(G_APPLICATION(app), argc, argv);
	}
	void close() {
		g_object_unref(app);
		audio.close();
	}
}

struct GtkGui {
	std::unique_ptr<GtkWidget> window;
	std::unique_ptr<GtkWidget> grid;

	void init(string newTitle, int newX, int newY) {
		window = std::make_unique<GtkWidget>(gtk_application_window_new(app.app));
		gtk_window_set_title(window, newTitle);
		gtk_window_set_default_size(window, newX, newY);

		grid = std::make_unique<GtkWidget>(gtk_grid_new());
		gtk_container_add(window, grid);
	}
	void show() {
		gtk_widget_show_all(window);
	}
}

struct AudioStream {
	string status = "STOP";
	string curPath;
	std::unique_ptr<Mix_Music> audio;

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
	void play(string audioPath) {
		std::thread(set_audio, audioPath).detach();
	}
	void play() {
		if (status == "PAUSE") {
			Mix_ResumeMusic();
			status = "PLAY";
		}
	}
	void pause() {
		if (status == "PLAY") {
			Mix_PauseMusic();
			status = "PAUSE";
		}
	}
	void set_audio(string path) {
		audio = std::make_unique<Mix_Music>(Mix_LoadMUS(path));
		if (!audio) {
			std::cerr << "Failed to load audio! SDL_mixer Error: " << Mix_GetError() << std::endl;
			return;
		}
		if (Mix_PlayMusic(audio, 1) == -1) {
			std::cerr << "Failed to play music! SDL_mixer Error: " << Mix_GetError() << std::endl;
			Mix_FreeMusic(music);
			return;
		}
		status = "PLAY";

		while(status == "PLAY") {
			SDL_Delay(100);
		}
	}
}

struct Graphics {
	std::shared_ptr<GtkWidget> play;
	std::shared_ptr<GtkWidget> pause;

	void init() {
		play	= std::make_shared<GtkWidget>(gtk_image_new_from_file("img/play.png"));
		pause	= std::make_shared<GtkWidget>(gtk_image_new_from_file("img/pause.png"));
	}
}

struct Button {
	std::unique_ptr<GtkWidget> btn = std::make_unique<GtkWidget>(gtk_button_new());

	void set_img(GtkWidget& img) {
		gtk_button_set_image(btn, img);
	}
	void set_onClick(void (*callback)(GtkWidget*, gpointer)) {
		g_signal_connect(btn, "clicked", callback, NULL);
	}
	void set_gridPos(int col, int row, int width = 1, int height = 1) {
		gtk_grid_attach(gui.grid, btn, col, row, width, height);
	}
}

GtkApp app;
GtkGui gui;
AudioStream audio;



static void init_gui(GtkApplication* app, gpointer data) {
	gui.init("Music Player", 400, 400);
	gui.show();
}

int main(int argc, char **argv) {
	audio.init();
	app.init();
	app.close();
	return app.status;
}