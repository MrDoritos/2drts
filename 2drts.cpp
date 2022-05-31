#include "advancedConsole.h"
#include "colorMappingFaster.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <math.h>
#include <chrono>
#include <vector>

struct tile_t;
struct tilestate_t;
struct world_t;

float scale = 4;
float scalingFactor = 0.5f;
float viewX = 0.0f;
float viewY = 0.0f;
int tileMapWidth = 45;
int tileMapHeight = 45;
//clash of clans 1.33:1
const char *texturePath = "quarter.png";
float xfact = 2.0f;//1.0f;//0.7071067812f;
float yfact = 1.5f;//0.5f;//0.7071067812f;
int textureSize = 25;
//rct 2:1
//const char *texturePath = "textures.png";
//float xfact = 1.0f;
//float yfact = 0.5f;
//int textureSize = 128;
//console font compensation
float wfact = 2.0f;
float hfact = 1.0f;
bool infoMode = false, statsMode = false;
struct world_t *worldObj = nullptr;

unsigned char *texture;
int textureHeight;
int textureWidth;
int bpp;

struct pixel {
	pixel() {
		r = 0;
		g = 0;
		b = 0;
		a = 255;
	}
	pixel(color_t r, color_t g, color_t b) {
		this->r = r;
		this->g = g;
		this->b = b;
		this->a = 255;
	}
	color_t r,g,b,a;
};

struct ch_co_t {
	wchar_t ch;
	color_t co;
	color_t a;
};

ch_co_t *texturechco;

pixel sampleImage(float x, float y) { 
	int imX = x * textureWidth;
	int imY = y * textureHeight;
	
	pixel pix;
	pix.r = texture[imY * (textureWidth * bpp) + (imX * bpp) + 0];
	pix.g = texture[imY * (textureWidth * bpp) + (imX * bpp) + 1];
	pix.b = texture[imY * (textureWidth * bpp) + (imX * bpp) + 2];
	
	if (bpp == 4)
		pix.a = texture[imY * (textureWidth * bpp) + (imX * bpp) + 3];
	
	return pix;
}

ch_co_t sampleImageCHCO(float x, float y) {
	int imX = x * textureWidth;
	int imY = y * textureHeight;
	ch_co_t chco = texturechco[imY * textureWidth + imX];
	return chco;
}

void loadTextures(const char *path) {
	texture = stbi_load(path, (int*)&textureWidth, (int*)&textureHeight, &bpp, 0);
	
	//convert texture to wchar_t and color_t
	texturechco = new ch_co_t[textureWidth * textureHeight];
	
	for (int x = 0; x < textureWidth; x++) {
		for (int y = 0; y < textureHeight; y++) {
			pixel pix = sampleImage(float(x) / textureWidth, float(y) / textureHeight);
			ch_co_t chco;
			chco.a = pix.a;
			getDitherColored(pix.r, pix.g, pix.b, &chco.ch, &chco.co);
			texturechco[int(y * textureWidth) + int(x)] = chco;
		}
	}
}

float getOffsetX(float x, float y) {
	//return round(((((y * yfact) + (x * xfact)) * xfact) + viewX) * 1000.0f) / 1000.0f;
	return (x * xfact) - (-y * xfact) + viewX;
}

float getOffsetY(float x, float y) {
	return (x * yfact) + (-y * yfact) + viewY;
	//return round(((((y * yfact) - (x * xfact)) * yfact) + viewY) * 1000.0f) / 1000.0f;
}

float getWidth() {
	return wfact * scale;
}

float getHeight() {
	return hfact * scale;
}

struct tile_t;
struct entity_t;

struct tiles {
	static tile_t *tileRegistry[100];
	static int id;
	
	static void add(tile_t *tile);
	static tile_t *get(int id);
	
	static tile_t *DEFAULT_TILE;
	static tile_t *GRASS;
	static tile_t *WALL;
};

struct tilestate_t {
	tilestate_t() {
		id = 0;
		data = 0;
		looking = 0;
	}
	int id;
	int data;
	int player;
	int looking;
	int hitpoints;
	int maxhitpoints;
	bool slave;
};

struct tilecomplete_t {
	tilecomplete_t() {}
	tilecomplete_t(tile_t *tile, tilestate_t *state, int tileX, int tileY) {
		this->tile = tile;
		this->state = state;
		this->tileX = tileX;
		this->tileY = tileY;
	}
	
	tile_t *tile;
	tilestate_t *state;
	int tileX, tileY;
};

struct entity_t {
	int id;
	int player;
	float posX, posY;
	
	//target
	entity_t *entityTarget;
	int tileTargetX, tileTargetY;
	int targetType; //build, destroy
	bool standGround;
	
	unsigned char textureAtlas[4];
	
	entity_t(int t0, int t1, int t2, int t3) {
		setAtlas(t0,t1,t2,t3);
	}
	void setAtlas(int a, int b, int c, int d) {
		textureAtlas[0] = a;
		textureAtlas[1] = b;
		textureAtlas[2] = c;
		textureAtlas[3] = d;
	}
	virtual void draw(float offsetx, float offsety, float sizex, float sizey) {
		//Default is drawing texture from the atlas. ignoring transparent and retaining scale
		int textureSizeW = textureAtlas[2] - textureAtlas[0];
		int textureSizeH = textureAtlas[3] - textureAtlas[1];
		for (int x = 0; x < sizex * textureSizeW; x++) {
			for (int y = 0; y < sizey * textureSizeH; y++) {
				if (offsetx + x >= adv::width || offsety + y - (textureSizeH - 1) * sizey >= adv::height || offsetx + x < 0 || offsety + y - ((textureSizeH - 1) * sizey) < 0)
					continue;
				float xf = ((textureAtlas[0] * textureSize) + ((float(x) / sizex) * textureSize)) / textureWidth;
				float yf = ((textureAtlas[1] * textureSize) + ((float(y) / sizey) * textureSize)) / textureHeight;
				pixel pix = sampleImage(xf, yf);
				if (pix.a < 255)
					continue;
				ch_co_t chco = sampleImageCHCO(xf, yf);
				adv::write(offsetx + x , offsety + y - ((textureSizeH - 1) * sizey), chco.ch, chco.co);
			}
		}
	}
	virtual bool onCreate() { return false; }
	virtual bool onUpdate() { return false; }
	virtual bool onDestroy() { return false; }
	virtual bool onRandomTick() { return false; } 
};

struct engineer_entity : public entity_t {
	engineer_entity():entity_t(0,6,4,12){}
	
	float move_rate = 0.01f;
	
	void move_tick() {
		if (standGround)
			return;
		
		float targetX, targetY;
		
		if (entityTarget) {
			targetX = entityTarget->posX;
			targetY = entityTarget->posY;
		} else {
			targetX = tileTargetX;
			targetY = tileTargetY;
		}
		
		
	}
	
	bool onUpdate() override {
		//posX += 0.001f;
		move_tick();
		return false;
	}
};

struct tile_t {
	int width;
	int id;
	unsigned char textureAtlas[4];
	
	tile_t(int t0, int t1, int t2, int t3, bool skip = false) {
		if (!skip)
			tiles::add(this);
		setAtlas(t0,t1,t2,t3);
	}
	void setAtlas(int a, int b, int c, int d) {
		textureAtlas[0] = a;
		textureAtlas[1] = b;
		textureAtlas[2] = c;
		textureAtlas[3] = d;
	}
	virtual tilestate_t getDefaultState() {
		tilestate_t state;
		state.id = id;
		state.data = 0;
		return state;
	}
	
	/*
	virtual void draw(tilecomplete_t *tc, float offsetx, float offsety, float sizex, float sizey) {
		tilestate_t *tp = tc->state;
		if (offsetx >= adv::width || offsety >= adv::height || offsetx + sizex < 0 || offsety + sizey < 0)
			return;
		for (int x = 0; x < sizex; x++) {
			for (int y = 0; y < sizey; y++) {
				if (offsetx + x >= adv::width || offsety + y >= adv::height || offsetx + x < 0 || offsety + y < 0)
					continue;
				float xf = ((textureAtlas[0] * textureSize) + ((float(x) / sizex) * textureSize)) / textureWidth;
				float yf = ((textureAtlas[1] * textureSize) + ((float(y) / sizey) * textureSize)) / textureHeight;
				ch_co_t chco = sampleImageCHCO(xf, yf);
				if (chco.a < 255)
					continue;
				adv::write(offsetx + x, offsety + y, chco.ch, chco.co);
			}
		}
		
		adv::write(offsetx, offsety, L'#', FCYAN|BRED);
		adv::write(offsetx, offsety + sizey, L'#', FCYAN|BRED);
		adv::write(offsetx + sizex, offsety, L'#', FCYAN|BRED);
		adv::write(offsetx + sizex, offsety + sizey, L'#', FCYAN|BRED);
	}
	*/
	
	
	virtual void draw(tilecomplete_t *tc, float offsetx, float offsety, float sizex, float sizey) {
		tilestate_t *tp = tc->state;
		//Default is drawing texture from the atlas. ignoring transparent and retaining scale
		int textureSizeW = textureAtlas[2] - textureAtlas[0];
		int textureSizeH = textureAtlas[3] - textureAtlas[1];
		for (int x = 0; x < sizex * textureSizeW; x++) {
			for (int y = 0; y < sizey * textureSizeH; y++) {
				if (offsetx + x >= adv::width || offsety + y - (textureSizeH - 1) * sizey >= adv::height || offsetx + x < 0 || offsety + y - ((textureSizeH - 1) * sizey) < 0)
					continue;
				float xf = ((textureAtlas[0] * textureSize) + ((float(x) / sizex) * textureSize)) / textureWidth;
				float yf = ((textureAtlas[1] * textureSize) + ((float(y) / sizey) * textureSize)) / textureHeight;
				pixel pix = sampleImage(xf, yf);
				if (pix.a < 255)
					continue;
				//wchar_t ch;
				//color_t co;
				//getDitherColored(pix.r,pix.g,pix.b,&ch,&co);
				ch_co_t chco = sampleImageCHCO(xf, yf);
				adv::write(offsetx + x , offsety + y - ((textureSizeH - 1) * sizey), chco.ch, chco.co);
			}
		}
		
		//adv::write(offsetx, offsety, L'#', FCYAN|BRED);
		//adv::write(offsetx, offsety + sizey, L'#', FCYAN|BRED);
		//adv::write(offsetx + sizex, offsety, L'#', FCYAN|BRED);
		//adv::write(offsetx + sizex, offsety + sizey, L'#', FCYAN|BRED);
	}
	
	
	virtual void onCreate(tilecomplete_t *tc) {}
	virtual void onUpdate(tilecomplete_t *tc) {}
	virtual void onDestroy(tilecomplete_t *tc) {}
	virtual void onRandomTick(tilecomplete_t *tc) {}
	virtual bool renderInBounds(tilecomplete_t *tc, int offsetx, int offsety, int width, int height) {
		int halfHeight = height * yfact;
		int fullHeight = height * yfact * 2;
		int halfWidth = width * xfact;
		int fullWidth = width * xfact * 2;
		return !(offsetx + width < -halfWidth || offsety * height + height < 0 || offsetx * width + width - width > adv::width || offsety * height + height - height - halfHeight > adv::height);
	}
};

struct wall_tile : public tile_t {
	wall_tile(int a, int b, int c, int d):tile_t(a,b,c,d) {
		
	}
};

struct elixir_storage : public tile_t {
	elixir_storage(int a, int b, int c, int d) : tile_t(a,b,c,d) {
		width = 3;
	}
};

//tile_t *tiles::DEFAULT_TILE = new tile_t(0,0,2,1);
tile_t *tiles::DEFAULT_TILE = new tile_t(0,0,4,3);
tile_t *grass2 = new tile_t(0,3,4,6,true);
tile_t *tiles::GRASS = new tile_t(0,1,2,2);
tile_t *tiles::WALL = new wall_tile(4,0,8,6);
tile_t *elixir_storage = new struct elixir_storage(8,0,20,9);

tile_t *tiles::tileRegistry[100];
int tiles::id = 0;

tile_t *tiles::get(int id) {
	return tiles::tileRegistry[id];
}

void tiles::add(tile_t *tile) {
	tile->id = id++;
	tiles::tileRegistry[tile->id] = tile;
}

struct world_t {
	tilestate_t *tiles;
	std::vector<entity_t*> entities;
	
	world_t() {
		tiles = new tilestate_t[tileMapWidth * tileMapHeight];
	}
	~world_t() {
		delete [] tiles;
	}

	tilestate_t *getTileState(int x, int y) {
		return &tiles[y * tileMapWidth + x];
	}
	tilecomplete_t getTileComplete(int x, int y) {
		tilecomplete_t tc;
		tc.state = getTileState(x,y);
		tc.tile = tiles::get(tc.state->id);
		tc.tileX = x;
		tc.tileY = y;
		return tc;
	}
	void setTileState(int x, int y, tilestate_t state) {
		*getTileState(x,y) = state;
	}
	void placeTile(int ox, int oy, tile_t *tile) {
		int width = tile->width;
		int p = (width - 1) / 2;
		for (int x = ox - p; x < ox - p + width; x++) {
			for (int y = oy - p; y < oy - p + width; y++) {
				bool slavef = true;
				if (x == ox && y == oy)
					slavef = false;
				tilestate_t newState = tile->getDefaultState();
				newState.slave = slavef;
				setTileState(x,y,newState);
			}			
		}
	}
	void spawnEntity(entity_t *em) {
		entities.push_back(em);
	}
	
	void spawnEntity(entity_t *em, float posX, float posY, int player) {
		em->posX = posX;
		em->posY = posY;
		em->player = player;
		spawnEntity(em);
	}
	
	bool isOccupied(int x, int y) {
		return getTileState(x,y)->id != tiles::DEFAULT_TILE->id;
	}
	bool canPlaceTile(int ox, int oy, tile_t *tile) {
		int width = tile->width;
		int p = (width - 1) / 2;
		for (int x = ox - p; x < ox + width; x++) {
			for (int y = oy - p; y < oy + width; y++) {
				if (isOccupied(x,y))
					return false;
			}
		}
		return true;
	}
	
	void tick() {
		for (auto et : entities) {
			et->onUpdate();
		}
		
		for (int x = 0; x < tileMapWidth; x++) {
			for (int y = 0; y < tileMapHeight; y++) {
				tilecomplete_t tc = getTileComplete(x,y);
				tc.tile->onUpdate(&tc);
			}
		}
	}
	
	void display() {
		float width = getWidth();
		float height = getHeight();
		//entity display
		for (auto et : entities) {
			float offsetx = getOffsetX(et->posX, et->posY);
			float offsety = getOffsetY(et->posX, et->posY);
			et->draw(offsetx * width, offsety * height, width, height);
		}
	}
};
//clash of clans: 100h 133w
//rct : 1h 2w
//bos wars : 1h 1w
void default_structures() {
	worldObj->setTileState(1,1,tiles::WALL->getDefaultState());
	worldObj->spawnEntity(new engineer_entity, 1.0f, 1.0f, 1);
}

void displayTileMap() {
	float width = getWidth();
	float height = getHeight();
	tilecomplete_t tc;
	
	//for (int x = tileMapWidth - 1; x > -1; x--) {
	//	for (int y = 0; y < tileMapHeight; y++) {
		
	//Grass first pass
	for (int x = 0; x < tileMapWidth; x++) {
		for (int y = tileMapHeight - 1; y > -1; y--) {
			float offsetx = getOffsetX(x,y);
			float offsety = getOffsetY(x,y);
			
			if (!tiles::DEFAULT_TILE->renderInBounds(nullptr, offsetx, offsety, width, height))
				continue;
			
			tile_t *grass = tiles::DEFAULT_TILE;
			if ((x + y) % 2 == 0)
				grass = grass2;
			grass->draw(&tc, offsetx * width, offsety * height, width, height);
		}
	}
	
	//Actual tiles
	for (int x = 0; x < tileMapWidth; x++) {
		for (int y = tileMapHeight - 1; y > -1; y--) {
			tc = worldObj->getTileComplete(x,y);
			if (tc.state->id == tiles::DEFAULT_TILE->id)
				continue;
			
			int p = 0;
			if (tc.tile->width > 1 && !tc.state->slave)
				p = (tc.tile->width - 1) / 2;
			
			float offsetx = getOffsetX(x - p,y - p);
			float offsety = getOffsetY(x + p,y - p);
			
			if (!tc.tile->renderInBounds(&tc, offsetx, offsety, width, height))
				continue;
			
			if (!tc.state->slave)
				tc.tile->draw(&tc, offsetx * width, offsety * height, width, height);
		}
	}
}

void displayMapOutline() {
	//Edges of the map
	float halfWidth = xfact / 2.0f;
	float halfHeight = yfact / 2.0f;
	float corners[] = {
		//0.5f,-0.5f,
		0.5f, -0.5f,
		tileMapWidth+0.5f,-0.5f,
		tileMapWidth+0.5f,tileMapHeight-0.5f,
		0.5f,tileMapHeight-0.5f,
		//0.5f,-0.5f,
		100.0f,100.0f	
	};
	for (int i = 0; i < 8; i+=2) {
		//break;
		//int next = (i + 2) % 8;
		int next = i + 2;
		int width = getWidth();
		int height = getHeight();
		float offsetx1 = getOffsetX(corners[i + 1], corners[i]) - halfWidth;// - (xfact / 2.0f);
		float offsety1 = getOffsetY(corners[i + 1], corners[i]) + halfHeight;// + (yfact / 2.0f);
		float offsetx2 = getOffsetX(corners[next + 1], corners[next]) - halfWidth;// - (xfact / 2.0f);
		float offsety2 = getOffsetY(corners[next + 1], corners[next]) + halfHeight;// + (yfact / 2.0f);
		adv::line(offsetx1 * width, offsety1 * height, offsetx2 * width, offsety2 * height, '#', FWHITE|BBLACK);
		//	adv::triangle(offset)
		adv::write(offsetx1 * width, offsety1 * height, 'X', FRED|BBLACK);
	}	
}

void display() {	
	displayMapOutline();
	displayTileMap();	
	worldObj->display();
	
	if (infoMode) {
		int y = 0;
		auto printVar = [&](const char* varname, float value) {
			char buf[100];
			int i = snprintf(&buf[0], 99, "%s: %f", varname, value);
			adv::write(0,y++,&buf[0]);
		};
		printVar("viewX", viewX);
		printVar("viewY", viewY);
		printVar("scale", scale);
		printVar("xfact", xfact);
		printVar("yfact", yfact);
		printVar("wfact", wfact);
		printVar("hfact", hfact);
		printVar("artFact", xfact / yfact);
	}
}

void init() {
	srand(time(NULL));
	
	viewX = 0;
	viewY = 0;
	
	scale = 4;
	
	statsMode = false;
	infoMode = false;
	
	if (worldObj)
		delete worldObj;
	worldObj = new world_t();
	
	default_structures();
}

int main() {
	colormapper_init_table();
	
	loadTextures(texturePath);
	//loadTextures("quarter.png");
	
	while (!adv::ready) console::sleep(10);
	
	adv::setThreadState(false);
	adv::setThreadSafety(false);
	
	init();
	
	int key = 0;
	
	auto tp1 = std::chrono::system_clock::now();
	auto tp2 = std::chrono::system_clock::now();
	
	while (!HASKEY(key = console::readKeyAsync(), VK_ESCAPE)) {
		adv::clear();
		
		switch (key) {
			case VK_RIGHT:
				viewX--;
				break;				
			case VK_UP:
				viewY++;
				break;
			case VK_LEFT:
				viewX++;
				break;
			case VK_DOWN:
				viewY--;
				break;
			case 'w':
					viewY++;
				break;
			case 'a':
					viewX++;
				break;
			case 's':
					viewY--;
				break;
			case 'd':
					viewX--;	
				break;
			case ',':
			//case 'z':
				scale+= scalingFactor;
				break;
			case '.':
			//case 'x':
				if (scale > scalingFactor)
					scale-= scalingFactor;
				break;
			case 'r':
				xfact += 0.05f;
				break;
			case 'f':
				xfact -= 0.05f;
				break;
			case 't':
				yfact += 0.05f;
				break;
			case 'g':
				yfact -= 0.05f;
				break;
			
			case 'y':
				wfact += 0.05f;
				break;
			case 'h':
				wfact -= 0.05f;
				break;
			case 'u':
				hfact += 0.05f;
				break;
			case 'j':
				hfact -= 0.05f;
				break;
				
			case 'l':
				worldObj->placeTile(6,6,elixir_storage);
				break;
				
			case '/':
				scale = 4;
				break;
			case '0':
				init();
				break;			
			case 'o':
				infoMode = !infoMode;
				break;
			case 'p':
				statsMode = !statsMode;
				break;
		}
		
		worldObj->tick();
		display();
		
		tp2 = std::chrono::system_clock::now();
		std::chrono::duration<float> elapsedTime = tp2 - tp1;
		float elapsedTimef = std::chrono::duration_cast< std::chrono::microseconds>(elapsedTime).count() / 1000.0f;
		//std::chrono::duration<float, std::milli> elapsedTimef = t2 - t1;
		tp1 = tp2;
			
		if (elapsedTimef < 33.333f)
		;//	console::sleep(33.333f - elapsedTimef);
		//console::sleep(20);
		{
			char buf[50];
			snprintf(&buf[0], 49, "%.1f fps - %.1f ms ft", (1.0f/elapsedTimef)*1000.0f, elapsedTimef);
			adv::write(adv::width/2.0f-(strlen(&buf[0])/2.0f), 0, &buf[0]);
		}
		
		adv::draw();
	}
	
	adv::construct.~_constructor();
	
	return 0;
}