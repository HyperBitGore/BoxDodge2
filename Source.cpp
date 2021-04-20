#include "PixelEdit.h"
#include <windows.h>
#include <vector>
#include <thread>
#include <string>
#include <chrono>
#include "SDL_ttf.h"
#include "SDL_image.h"
#undef main

bool exitf = false;
bool dead = true;
SDL_Renderer *rend;
SDL_Window *window;
SDL_Surface *surface;
Gore::Edit edit;
double delta = 0;
Uint64 LAST = 0;
int cbox = 0;
int score = 0;
int scale = 100;
int speed = 200;
int spawnc = 0;
bool colliding[4] = {false, false, false, false};
class Entity {
public:
	float x;
	float y;
	int w;
	int h;
	int(*Update)(Entity* e);
	bool erase;
	bool deathcall;
};
std::vector<Entity> boxes;
Entity player = {300, 300, 20, 20};

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




int player_u(Entity *p) {
	if (player.y <= 0) {
		colliding[0] = true;
	}
	else if (player.y + player.h >= 600) {
		colliding[1] = true;
	}
	if (player.x <= 0) {
		colliding[3] = true;
	}
	else if (player.x + player.w >= 600) {
		colliding[2] = true;
	}
	if (GetAsyncKeyState(VkKeyScanA('w')) & 0x8000 && !colliding[0]) {
		player.y-= (float)500*delta;
	}
	else if (GetAsyncKeyState(VkKeyScanA('s')) & 0x8000 && !colliding[1]) {
		player.y+= (float)500*delta;
	}
	if (GetAsyncKeyState(VkKeyScanA('d')) & 0x8000 && !colliding[2]) {
		player.x+= (float)500*delta;
	}else if (GetAsyncKeyState(VkKeyScanA('a')) & 0x8000 && !colliding[3]) {
		player.x-= (float)500*delta;
	}
	for (int i = 0; i < 4; i++) {
		colliding[i] = false;
	}
	SDL_Rect rec = { (*p).x, (*p).y, (*p).w, (*p).h};
	SDL_SetRenderDrawColor(rend, 0, 0, 255, 0);
	score++;
	SDL_RenderFillRect(rend, &rec);
	return 0;
}
int box_u(Entity* b) {
	(*b).y += (float)speed * delta;
	if (isColliding((*b), player)) {
		(*b).deathcall = true;
	}
	SDL_Rect rec = { (*b).x, (*b).y, (*b).w, (*b).h };
	SDL_SetRenderDrawColor(rend, 255, 255, 255, 0);
	
	SDL_RenderFillRect(rend, &rec);
	if ((*b).y > 601) {
		(*b).erase = true;
	}
	(*b).Update = &box_u;
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
//Changed to using midpoint circle algorithm, is much quicker
void drawMidpointCircle(int cx, int cy, int r, Gore::RGB col) {
	int x = r;
	int y = 0;
	int P = 1 - r;
	while (x > y) {
		y++;
		if (P <= 0) {
			P = P + 2 * y + 1;
		}
		else {
			x--;
			P = P + 2 * y - 2 * x + 1;
		}
		if (x < y) {
			break;
		}
		edit.setPixelRGBASafe(surface, x + cx, y + cy, col.r, col.g, col.b, col.a, 599, 599);
		edit.setPixelRGBASafe(surface, -x + cx, y + cy, col.r, col.g, col.b, col.a, 599, 599);
		edit.setPixelRGBASafe(surface, x + cx, -y + cy, col.r, col.g, col.b, col.a, 599, 599);
		edit.setPixelRGBASafe(surface, -x + cx, -y + cy, col.r, col.g, col.b, col.a, 599, 599);
		if (x != y) {
			edit.setPixelRGBASafe(surface, y + cx, x + cy, col.r, col.g, col.b, col.a, 599, 599);
			edit.setPixelRGBASafe(surface, -y + cx, x + cy, col.r, col.g, col.b, col.a, 599, 599);
			edit.setPixelRGBASafe(surface, y + cx, -x + cy, col.r, col.g, col.b, col.a, 599, 599);
			edit.setPixelRGBASafe(surface, -y + cx, -x + cy, col.r, col.g, col.b, col.a, 599, 599);
		}
	}
}

void updateLightingMidpoint() {
	SDL_LockSurface(surface);
	int r = 10;
	int lpower = 255;
	for (int j = 0; j < 15; j++) {
		drawMidpointCircle(player.x+10, player.y+10, r, Gore::RGB{ 0, 255, 0, (Uint8)lpower });
		r++;
		lpower -= 17;
	}
	for (auto& i : boxes) {
		int mw = i.w / 2;
		int mh = i.h / 2;
		r = i.w / 2;
		lpower = 255;
		for (int j = 0; j < mw; j++) {
			drawMidpointCircle(i.x + mw, i.y + mh, r, Gore::RGB{ 255, 255, 255, (Uint8)lpower });
			r++;
			lpower -= (255/(mw));
			}
	}

	SDL_UnlockSurface(surface);
}

void destroyLighting() {
	SDL_LockSurface(surface);
	std::memset(surface->pixels, 0, (600 * 600)*(sizeof(surface->pixels)));
	SDL_UnlockSurface(surface);
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
	surface = SDL_CreateRGBSurfaceWithFormat(0, 600, 600, 32, SDL_PIXELFORMAT_RGBA8888);
	//SDL_SetSurfaceBlendMode(surface, SDL_BLENDMODE_BLEND);
	edit.setPixelRGBA(surface, 1, 1, 255, 0, 0, 255);
	edit.setPixelRGBA(surface, 2, 1, 255, 0, 0, 255);
	edit.setPixelRGBA(surface, 3, 1, 255, 0, 0, 255);
	edit.setPixelRGBA(surface, 1, 2, 255, 0, 0, 200);
	edit.setPixelRGBA(surface, 2, 2, 255, 0, 0, 200);
	edit.setPixelRGBA(surface, 3, 2, 255, 0, 0, 200);
	edit.setPixelRGBA(surface, 1, 3, 255, 0, 0, 170);
	edit.setPixelRGBA(surface, 2, 3, 255, 0, 0, 170);
	edit.setPixelRGBA(surface, 3, 3, 255, 0, 0, 170);
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
	std::thread box_manager(spawn_thread);
	while (!exitf) {
		delta = edit.getDelta();
		std::cout << delta << std::endl;
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
			updateLightingMidpoint();
			SDL_Texture* light = SDL_CreateTextureFromSurface(rend, surface);
			SDL_RenderCopy(rend, light, NULL, &screenm);
			SDL_DestroyTexture(light);
			std::string out = std::to_string(score);
			text = TTF_RenderText_Solid(numfont, out.c_str(), white);
			SDL_Texture* rtext3 = SDL_CreateTextureFromSurface(rend, text);
			msize.y = -10;
			msize.w = 70;
			SDL_RenderCopy(rend, rtext3, NULL, &msize);
			SDL_DestroyTexture(rtext3);
			SDL_FreeSurface(text);
				player.Update(&player);
				for (auto& i : boxes) {
					if (i.deathcall) {
						death();
						break;
					}
					if (i.Update(&i)) {
						i.erase = true;
					}
					if (i.erase) {
						boxes.erase(boxes.begin() + cbox);
						cbox--;
					}
					cbox++;
				}
				cbox = 0;
				scalein += delta;
				if (scalein >= 2.0f) {
					if (scale >= 81) {
						scale--;
						std::cout << speed << std::endl;
					}
					speed += 20;
					scalein = 0;
				}
				destroyLighting();
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
	TTF_Quit();
	SDL_DestroyTexture(rtext);
	SDL_DestroyTexture(rtext2);
	SDL_DestroyRenderer(rend);
	SDL_FreeSurface(surface);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}