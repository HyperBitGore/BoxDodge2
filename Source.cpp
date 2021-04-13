#include <iostream>
#include <windows.h>
#include <vector>
#include <thread>
#include <time.h>
#include <string>
#include <chrono>
#include "SDL.h"
#include "SDL_ttf.h"
#include "SDL_image.h"
#undef main

bool exitf = false;
bool dead = true;
SDL_Renderer *rend;
SDL_Window *window;
SDL_Surface *surface;
SDL_Texture* light;
double delta = 0;
Uint64 LAST = 0;
int cbox = 0;
int score = 0;
int scale = 100;
int speed = 200;
int spawnc = 0;
class Entity {
public:
	float x;
	float y;
	int w;
	int h;
	int(*Update)(Entity e);
	bool erase;
	bool deathcall;
};
std::vector<Entity> boxes;
Entity player = {300, 300, 20, 20};
Uint32 * lpixels = new Uint32[600*600];

bool isColliding(Entity e, Entity e2) {
	if (e.x < e2.x + e2.w && e.x + e.w > e2.x && e.y < e2.y + e2.h && e.y + e.h > e2.y) {
		return true;
	}
	return false;
}
void death() {
	dead = true;
	player.y = 300;
	player.x = 300;
	boxes.clear();
	scale = 100;
	spawnc = 0;
	speed = 200;
}




int player_u(Entity p) {
	if (GetAsyncKeyState(VkKeyScanA('w')) & 0x8000) {
		player.y-= (float)500*delta;
	}
	else if (GetAsyncKeyState(VkKeyScanA('s')) & 0x8000) {
		player.y+= (float)500*delta;
	}
	if (GetAsyncKeyState(VkKeyScanA('d')) & 0x8000) {
		player.x+= (float)500*delta;
	}else if (GetAsyncKeyState(VkKeyScanA('a')) & 0x8000) {
		player.x-= (float)500*delta;
	}
	SDL_Rect rec = { p.x, p.y, p.w, p.h};
	SDL_SetRenderDrawColor(rend, 0, 0, 255, 0);
	score++;
	SDL_RenderFillRect(rend, &rec);
	return 0;
}
int box_u(Entity b) {
	b.y += (float)speed * delta;
	if (isColliding(b, player)) {
		b.deathcall = true;
	}
	SDL_Rect rec = { b.x, b.y, b.w, b.h };
	SDL_SetRenderDrawColor(rend, 255, 255, 255, 0);
	
	SDL_RenderFillRect(rend, &rec);
	if (b.y > 601) {
		b.erase = true;
	}
	b.Update = &box_u;
	boxes[cbox] = b;
	return 0;
}

void spawn_thread() {
	while (!exitf) {
		if (!dead) {
				Entity e;
				e.x = rand() % 600;
				e.y = 0;
				e.w = rand() % 30 + 20;
				e.h = rand() % 30 + 20;
				e.erase = false;
				e.Update = &box_u;
				e.deathcall = false;
				boxes.push_back(e);
				spawnc++;
			std::this_thread::sleep_for(std::chrono::milliseconds(scale));
		}
	}
}
//Write algorthim to handle caluclation of what alpha value should be
//Also probably rewrite this to be more efficient 
void updateLighting() {
	int lpower = 255;
	for (int i = 0; i < 600; i++) {
		for (int j = 0; j < 600; j++) {
			if (i == (int)player.y && j == (int)player.x) {
				for (int l = i-10; l < i + player.h + 10; l++) {
					for (int k = j-10; k < j + player.w + 10; k++) {
						lpixels[l * 600 + k] = lpower;
					}
				}
				
			}
		}
	}

}

//https://wiki.libsdl.org/SDL_BlendMode?highlight=%28%5CbCategoryEnum%5Cb%29%7C%28SDLEnumTemplate%29
int main(int argc, char **argv) {
	srand(time(NULL));
	if (SDL_Init(SDL_INIT_VIDEO) > 0) {
		std::cout << "SDL Init failed: " << SDL_GetError << std::endl;
	}
	if (!(IMG_Init(IMG_INIT_PNG))) {
		std::cout << "Image init failed: " << IMG_GetError << std::endl;
	}
	if (!TTF_Init()) {
		std::cout << "Font init failed" << TTF_GetError << std::endl;
	}
	window = SDL_CreateWindow("Box Dodge 2", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 600, 600, SDL_WINDOW_SHOWN);
	rend = SDL_CreateRenderer(window, -1, 0);
	surface = SDL_GetWindowSurface(window);
	player.Update = &player_u;
	Uint64 NOW = SDL_GetPerformanceCounter();
	TTF_Font* font = TTF_OpenFont("MetalMania-Regular.ttf", 24);
	TTF_Font* numfont = TTF_OpenFont("DelaGothicOne-Regular.ttf", 12);
	SDL_Event e;
	SDL_Color white = { 255, 255, 255 };
	SDL_Surface* text = TTF_RenderText_Solid(font, "Click to continue", white);
	SDL_Texture* rtext = SDL_CreateTextureFromSurface(rend, text);
	text = TTF_RenderText_Solid(font, "Your Score ", white);
	SDL_Texture* rtext2 = SDL_CreateTextureFromSurface(rend, text);
	SDL_Rect screenm;
	screenm.x = 0;
	screenm.y = 0;
	screenm.w = 600;
	screenm.h = 600;
	SDL_Rect msize;
	msize.x = 10;
	msize.y = 300;
	msize.w = 600;
	msize.h = 100;
	float scalein = 0.0f;
	light = SDL_CreateTexture(rend, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, 600, 600);
	memset(lpixels, 20, 600 * 600 * sizeof(Uint32));
	std::thread box_manager(spawn_thread);
	while (!exitf) {
		LAST = NOW;
		NOW = SDL_GetPerformanceCounter();
		delta = (double)((NOW - LAST) * 1000 / (double)SDL_GetPerformanceFrequency() );
		delta = delta * 0.001;
		while (SDL_PollEvent(&e)) {
			switch (e.type) {
			case SDL_QUIT:
				exitf = true;
				break;
			case SDL_KEYUP:
				//e.key
				break;
			}
		}
		SDL_SetRenderDrawColor(rend, 0, 0, 0, 0);
		SDL_RenderClear(rend);
		if (!dead) {
			updateLighting();
			SDL_UpdateTexture(light, NULL, lpixels, 600 * sizeof(Uint32));
			SDL_RenderCopy(rend, light, NULL, &screenm);
			std::string out = std::to_string(score);
			text = TTF_RenderText_Solid(numfont, out.c_str(), white);
			SDL_Texture* rtext3 = SDL_CreateTextureFromSurface(rend, text);
			msize.y = -10;
			msize.w = 70;
			SDL_RenderCopy(rend, rtext3, NULL, &msize);
			SDL_DestroyTexture(rtext3);
			SDL_FreeSurface(text);
				player.Update(player);
				for (auto& i : boxes) {
					if (i.deathcall) {
						death();
						break;
					}
					if (!i.Update(i)) {
						cbox++;
					}
					else {
						i.erase = true;
					}
					if (i.erase) {
						boxes.erase(boxes.begin() + cbox);
						cbox--;
					}
				}
				cbox = 0;
				scalein += delta;
				if (scalein >= 2.0f) {
					if (scale >= 81) {
						scale--;
						speed+=10;
						std::cout << speed << std::endl;
					}
					scalein = 0;
				}
		}
		else {
			std::string out = std::to_string(score);
			text = TTF_RenderText_Solid(numfont, out.c_str(), white);
			SDL_Texture* rtext3 = SDL_CreateTextureFromSurface(rend, text);
			msize.y = 100;
			msize.w = 600;
			SDL_RenderCopy(rend, rtext, NULL, &msize);
			msize.y = 200;
			SDL_RenderCopy(rend, rtext2, NULL, &msize);
			msize.y = 300;
			msize.w = 300;
			SDL_RenderCopy(rend, rtext3, NULL, &msize);
			SDL_DestroyTexture(rtext3);
			SDL_FreeSurface(text);
			if (SDL_GetMouseState(NULL, NULL) & SDL_BUTTON(SDL_BUTTON_LEFT)) {
				score = 0;
				dead = false;
			}
		}
		SDL_RenderPresent(rend);
	}
	box_manager.join();
	delete[] lpixels;
	TTF_Quit();
	SDL_DestroyTexture(rtext);
	SDL_DestroyTexture(rtext2);
	SDL_DestroyRenderer(rend);
	SDL_FreeSurface(surface);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}