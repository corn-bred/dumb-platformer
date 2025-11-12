#define DEBUG
#include <SDL3/SDL.h>
#include <iostream>
#include <cmath>
#include <math.h>
#include <vector>
#include <string>
#include <sstream>
#include <thread>

#define WINDOW_HEIGHT 600
#define WINDOW_WIDTH 600

using namespace std;

int clamp(int value, int min, int max) {
if (value < min) {
return min;
} else if (value > max) {
return max;
}
return value;
}






double xvelo = 0, yvelo = 0, resist = 0.7;
int sinDeg = 0;
SDL_Window* win;
SDL_Renderer* render;
//texture is the actual texture
SDL_Texture *texture = NULL;
//grabs the surface height and width
int texture_width = 0, texture_height = 0;
//
SDL_Surface *surface = NULL;
char *bmp_path = NULL;

SDL_Event event;

SDL_FRect rect;

int airtime = 0;

static double camx = 0, camy = 0;

bool lastframepressed;
#ifdef DEBUG
bool debug = true;
#else
bool debug = false;
#endif

class Entity {
    private:
    SDL_Texture *texture;
    SDL_FRect rect;
    
    public:
    float x, y, h, w;
    
    Entity(char* imageName, float startx, float starty) {
        {
        char *bitmapPath;

        SDL_asprintf(&bitmapPath, imageName, SDL_GetBasePath());

        SDL_Surface *surface = SDL_LoadBMP(bitmapPath);

        if (!surface) cout << "Failed to load Bitmap: " << imageName << ". SDL Error: " << SDL_GetError();

        texture = SDL_CreateTextureFromSurface(render, surface);

        if (!texture) cout << "Failed to create Texture: " << imageName << ". SDL Error: " << SDL_GetError();

        h = surface->h;
        w = surface->w;
        x = startx;
        y = starty;
        SDL_DestroySurface(surface);
        SDL_free(bitmapPath);
        }
    }

    void renderEntity() {
        rect.x = x + camx;
        rect.y = y + camy;
        rect.h = h;
        rect.w = w;
        SDL_RenderTexture(render, this->texture, NULL, &rect);
    }
};

class Tilemap {
    private:
    
    public:
    const int _detail;
    SDL_FRect _rectTile;
    int chunkX, chunkY;
    bool tilesquare[15][15];
    Tilemap(int det, int chunkX, int chunkY): _detail(det) {
        this->chunkX = chunkX;
        this->chunkY = chunkY;
        _rectTile.h = WINDOW_HEIGHT / _detail;
        _rectTile.w = WINDOW_WIDTH / _detail;
        for (int i = 0; i < 15;) {
            for (int j = 0; j < 15;) {
                tilesquare[i][j] = false;
                j++;
            }
            i++;
        }
    }
    
};



bool Rendertilemap(Tilemap tilemap, double offsetx, double offsety) {
        for (int i = 0; i < 15;) {
            for (int j = 0; j < 15;) {
                switch (tilemap.tilesquare[i][j]) {
                    case false: break;
                    case true: tilemap._rectTile.x = j * (WINDOW_WIDTH / 15) - offsetx + ( tilemap.chunkX * 15 * (WINDOW_WIDTH / 15)); tilemap._rectTile.y = i * (WINDOW_HEIGHT / 15) + offsety + (tilemap.chunkY * 15 * (WINDOW_HEIGHT / 15)); 
                    if (tilemap.chunkY == 35) {
                        SDL_SetRenderDrawColor(render, clamp(197 - i * 17, 0, 255), clamp(125 + j * 2, 0, 255), clamp(0 - i, 0, 255), 255);
                    } else {
                        SDL_SetRenderDrawColor(render, clamp(144 - i * 1.5, 0, 255), clamp(85 - j * 1.5, 0, 255), clamp(30 - ((i + j) / 2) * 1.5, 0, 255), 255);
                    }
                    
                    SDL_RenderFillRect(render, &tilemap._rectTile); break;
                    default: return false; break;     
                }
                j++;
            }
            i++;
        }
    return true;
}

double Deg2Rad(double Deg) {
    return (Deg * M_PI / 180);
    
}








class AABB {  // [x, y]
    public:
    float topleft[2], topright[2], bottomleft[2], bottomright[2];
    AABB(SDL_FRect coll) {
        topleft[0] = coll.x;                topleft[1] = coll.y;
        topright[0] = coll.x + coll.w;      topright[1] = coll.y;
        bottomleft[0] = coll.x;             bottomleft[1] = coll.y + coll.h;
        bottomright[0] = coll.x + coll.w;   bottomright[1] = coll.y + coll.h;
    }
};

bool findelement (vector<int> list, int value) {
    for (int tmp : list) {
        if(tmp == value) return true;
    }
    return false;
}

int findquadrant(float x, float y) {
    if (x > 0 && y > 0) return 1;
    if (x < 0 && y > 0) return 2;
    if (x < 0 && y < 0) return 3;
    if (x > 0 && y < 0) return 4;
    if (x == 0 || y == 0) return 0;
    cerr << "Error in findquadrant(), please bugtest" << endl;
    return -1;
}

bool detectcollision(AABB a, AABB b) {
    vector<int> alreadylisted;
    float addedup[4][2] = {
        {a.bottomleft[0] - b.topright[0], a.bottomleft[1] - b.topright[1]},
        {a.bottomright[0] - b.topleft[0], a.bottomright[1] - b.topleft[1]},
        {a.topleft[0] - b.bottomright[0], a.topleft[1] - b.bottomright[1]},
        {a.topright[0] - b.bottomleft[0], a.topright[1] - b.bottomleft[1]}
    };

    if(!findelement(alreadylisted, findquadrant(addedup[0][0], addedup[0][1]))) {
        alreadylisted.push_back(findquadrant(addedup[0][0], addedup[0][1]));
    }

    if(!findelement(alreadylisted, findquadrant(addedup[1][0], addedup[1][1]))) {
        alreadylisted.push_back(findquadrant(addedup[1][0], addedup[1][1]));
    }

    if(!findelement(alreadylisted, findquadrant(addedup[2][0], addedup[2][1]))) {
        alreadylisted.push_back(findquadrant(addedup[2][0], addedup[2][1]));
    }

    if(!findelement(alreadylisted, findquadrant(addedup[3][0], addedup[3][1]))) {
        alreadylisted.push_back(findquadrant(addedup[3][0], addedup[3][1]));
    }

    if (alreadylisted.size() >= 3 && !findelement(alreadylisted, 0)) {
        return true;
    } else {
        return false;
    }
}

bool collision(Tilemap tilemap, double offsetx, double offsety) {

    AABB player(rect);
    SDL_FRect tile;
    tile.h = WINDOW_HEIGHT / 15;
    tile.w = WINDOW_WIDTH / 15;
    
    
    for(int i = 0; i < 15;) {
        for (int j = 0; j < 15;) {
                if(tilemap.tilesquare[i][j]) { 
                tile.x = j * (WINDOW_WIDTH / 15) - offsetx + (tilemap.chunkX * 15 * (WINDOW_WIDTH / 15)); tile.y = i * (WINDOW_HEIGHT / 15) + offsety + (tilemap.chunkY * 15 * (WINDOW_WIDTH / 15));
                AABB *tmp = new AABB(tile);

                if (debug) {
                SDL_SetRenderDrawColor(render, 255 - j * 17, 255 - i * 17, 25 - ((j + i) / 2) * 17, 255);
                SDL_RenderRect(render, &tile); }

                if (detectcollision(player, *tmp)) { delete tmp; return true; }
                    delete tmp;
                }

                j++;
        }
        i++;
    }
    return false;
}







void CleanNQuit() {
    SDL_DestroyWindow(win);
}



void check(Tilemap tilemap1, Tilemap tilemap2, Tilemap tilemap3, Tilemap tilemap4) {
    

    camx += xvelo;

		//colour of square
        if(collision(tilemap1, camx, camy) || collision(tilemap2, camx, camy) || collision(tilemap3, camx, camy) || collision(tilemap4, camx, camy)) {
            while(collision(tilemap1, camx, camy) || collision(tilemap2, camx, camy) || collision(tilemap3, camx, camy) || collision(tilemap4, camx, camy)) {
                if (xvelo > 0) {
                    camx -= 0.1;
                } else {
                    camx += 0.1;
                }
                
                
            }
            xvelo = 0;
            
        }

        camy += yvelo;

        if(collision(tilemap1, camx, camy) || collision(tilemap2, camx, camy) || collision(tilemap3, camx, camy) || collision(tilemap4, camx, camy)) {
            while(collision(tilemap1, camx, camy) || collision(tilemap2, camx, camy) || collision(tilemap3, camx, camy || collision(tilemap4, camx, camy))) {
                if (yvelo < 0) {
                    camy += 0.1;
                } else {
                    camy -= 0.1;
                }
                
                
            }
            yvelo = 0;
            airtime = 0;
            
        } else {
            airtime++;
        }
        Rendertilemap(tilemap1, camx, camy);
        Rendertilemap(tilemap2, camx, camy);
        Rendertilemap(tilemap3, camx, camy);
        Rendertilemap(tilemap4, camx, camy);
    }

static Tilemap tilemap(15, 0, 0);
static Tilemap tilemap2(15, 1, 0);
static Tilemap tilemap3(15, 0, 1);
static Tilemap tilemap4(15, 1, 1);

    void generate() {
    for(int i = 0; i < 15;) {
        for (int j = 0; j < 15;) {
            tilemap.tilesquare[14][j] = true;
            j++;
        }
        i++;
    }
    for(int i = 0; i < 15;) {
        for (int j = 0; j < 15;) {
            tilemap3.tilesquare[i][j] = true;
            j++;
        }
        i++;
    }
    for (int i = 0; i < 15;) {
        if (i > 11) {
            tilemap.tilesquare[i][8] = true;
        }
        i++;
    }
    for (int i = 0; i < 15;) {
        if (i > 9) {
            tilemap.tilesquare[i][9] = true;
        }
        i++;
    }
    for (int i = 0; i < 15;) {
        if (i > 8) {
            tilemap.tilesquare[i][10] = true;
        }
        i++;
    }
    for (int i = 0; i < 15;) {
        if (i > 7) {
            tilemap.tilesquare[i][11] = true;
        }
        i++;
    }
    for (int i = 0; i < 15;) {
        if (i > 6) {
            tilemap.tilesquare[i][12] = true;
        }
        i++;
    }
    for (int i = 0; i < 15;) {
        if (i > 3) {
            tilemap.tilesquare[i][13] = true;
        }
        i++;
    }
    for (int i = 0; i < 15;) {
        if (i > 0) {
            tilemap.tilesquare[i][14] = true;
        }
        i++;
    }
    }



//  IIIIII  TTTTTT       BBBBB  EEEEEE   GGGG IIIIII N    N  SSSSS
//     I       T         B    B E       G        I   NN   N S       ...
//     I       T         BBBBBB EEEEEE GGGGGG    I   N N  N  SSSSS .....
//  IIIIII     T         BBBBB  EEEEEE  GGGG  IIIIII N  NNN SSSSS   ...


int main(int argc, char *argv[])
{
    double deltatime, previoustime;
    

    win = SDL_CreateWindow("2039 - Make a small test game", WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_OPENGL);
    int quit = 0;
    int result = SDL_Init(SDL_INIT_VIDEO);
    
    

    thread thr(generate);
    thr.detach();


    

    
    //aaaa
    
	
	 
	
	if (win == NULL) CleanNQuit();
	render = SDL_CreateRenderer(win, NULL);
	if (render == NULL){
		cerr << "Renderer creation failed: %s\n" << SDL_GetError();
		CleanNQuit();
    }	
    


    //this entire thing is allocation and texturing below me
    SDL_asprintf(&bmp_path, "%sgoofy ahh thing.bmp", SDL_GetBasePath());  /* allocate a string of the full file path */
    
    surface = SDL_LoadBMP(bmp_path);

    if (!surface) {
        SDL_Log("Couldn't load bitmap: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    

    texture_width = surface->w / 3;
    texture_height = surface->h / 3;

    

    texture = SDL_CreateTextureFromSurface(render, surface);

    if (!texture) {
        SDL_Log("Couldn't generate static texture: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    SDL_asprintf(&bmp_path, "%sgoofy ahh thing but sad.bmp", SDL_GetBasePath());  /* allocate a string of the full file path */
    
    surface = SDL_LoadBMP(bmp_path);

    if (!surface) {
        SDL_Log("Couldn't load bitmap: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    int texture_width2 = surface->w / 3;
    int texture_height2 = surface->h / 3;

    SDL_Texture *texture2;

    texture2 = SDL_CreateTextureFromSurface(render, surface);

    if (!texture2) {
        SDL_Log("Couldn't generate static texture: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    SDL_SetWindowIcon(win, surface);

    SDL_free(bmp_path);  /* done with this, the file is loaded. */

    SDL_DestroySurface(surface);
    //this entire thing is allocation and texturing above me


    rect.x = (WINDOW_WIDTH / 2) - (texture_width / 2);
	rect.y = (WINDOW_HEIGHT / 2) - (texture_height / 2);

    Tilemap tilechunk[4] = {tilemap, tilemap2, tilemap3, tilemap4};

    yvelo = -0.1;

	while (!quit) {
		while (SDL_PollEvent(&event) != 0) {
			switch (event.type)
			{
			case SDL_EVENT_QUIT:
				quit = 1;
			break;
            /*case SDL_EVENT_KEY_DOWN:
                {
                    switch (event.key.key) {
                        case SDLK_LEFT: rect.x += -20; rect.y = (WINDOW_HEIGHT / 2 - RECT_HEIGHT / 2); break;
                        case SDLK_RIGHT: rect.x += 20; rect.y = (WINDOW_HEIGHT / 2 - RECT_HEIGHT / 2); break;
                        case SDLK_UP:  rect.x = (WINDOW_WIDTH / 2 - RECT_WIDTH / 2); rect.y += (WINDOW_HEIGHT / 2 - RECT_HEIGHT / 2) + -20; break;
                        case SDLK_DOWN:  rect.x = (WINDOW_WIDTH / 2 - RECT_WIDTH / 2); rect.y += (WINDOW_HEIGHT / 2 - RECT_HEIGHT / 2) + 20; break;
                    }
                }*/
			}
		}


        deltatime = (SDL_GetTicks() - previoustime) / 1000;
        //cout << deltatime << endl;
        

        xvelo *= resist;
        //yvelo *= resist;
        yvelo -= 20 * deltatime;

        const bool *keys = SDL_GetKeyboardState(nullptr);
        if (keys[SDL_SCANCODE_UP] && airtime <= 2) {
            yvelo = 8;
        }
        //if (keys[SDL_SCANCODE_DOWN]) {
        //    yvelo = -7;
        //}
        if (keys[SDL_SCANCODE_LEFT]) {
            xvelo = -7;
        }
        if (keys[SDL_SCANCODE_RIGHT]) {
            xvelo = 7;
        }
        if (keys[SDL_SCANCODE_ESCAPE]) {
            quit = true;  // Exit on Escape
        }
        if(keys[SDL_SCANCODE_D]) {
            if(lastframepressed == false) {
                if (debug) debug = false; else debug = true;
            }
            lastframepressed = true;
        } else {
            lastframepressed = false;
        }


        //sinDeg += 10;
        //rect.x = sin(Deg2Rad(sinDeg)) * 30 + (WINDOW_WIDTH / 2) - (RECT_WIDTH / 2);
        //rect.y = cos(Deg2Rad(sinDeg)) * 30 + (WINDOW_WIDTH / 2) - (RECT_WIDTH / 2);

        

        
        rect.h = texture_height / 1.3;
        rect.w = texture_width / 1.3;
        rect.x = (WINDOW_WIDTH / 2) - (texture_width / 2);
	    rect.y = (WINDOW_HEIGHT / 2) - (texture_height / 2);
        
        

        

		//colour of background
		SDL_SetRenderDrawColor(render, 23, 254, 246, 255);
        SDL_RenderClear(render);

        SDL_RenderTexture(render, texture, NULL, &rect);
        tilemap2.chunkX = 1;

		
        check(tilemap, tilemap2, tilemap3, tilemap4);

        SDL_SetRenderScale(render, 2, 2);

        stringstream strm;
        strm << camx;
        strm << "|";
        strm << camy;
        char* str;

        strm >> str;

        SDL_SetRenderDrawColor(render, 0, 0, 0, 255);

        SDL_RenderDebugText(render, 0, 0, str);

        SDL_SetRenderScale(render, 1, 1);
        
        

        //SDL_SetRenderDrawColor(render, 255, 0, 0, 255);
        //SDL_RenderRect(render, &rect);
		

        
		
		
		SDL_RenderPresent(render);
        previoustime = SDL_GetTicks();
		SDL_Delay( (1000 / 60) );
	}
    CleanNQuit();



	return 0;
}

