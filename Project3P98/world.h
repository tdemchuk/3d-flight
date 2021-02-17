#ifndef CS3P98_WORLD_H
#define CS3P98_WORLD_H

/*
	World Class
	Contains scene information, player data, camera object, manages physics, lighting, etc...
	@author Tennyson Demchuk
	@date 02.12.2021
*/

#include "testcamera.h"
#include "cache.h"
#include "shader.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

class World {
private:

	// helper class for generating iterative coordinates in a spiral - taken from https://stackoverflow.com/a/14010215
	class SpiralIterator {
	private:
		int x, z;
		unsigned int leg, layer;
	public:
		SpiralIterator() : x(0), z(0), leg(0), layer(1) {}
		void next() {
			switch (leg) {
			case 0: x++; if (x == layer)	leg++;				break;
			case 1: z++; if (z == layer)	leg++;				break;
			case 2: x--; if (-x == layer)	leg++;				break;
			case 3: z--; if (-z == layer) { leg = 0; layer++; }	break;
			}
		}
		int getx() { return x; }
		int getz() { return z; }
		void reset() { x = 0; z = 0; leg = 0; layer = 1; }
	};

	// class constants
	static constexpr int	RENDER_DIST = 1;											// radial render distance in # chunks (beyond the central chunk). ie. render dist 1 will render the central (active) chunk and 1 beyond it in every direction for 9 chunks total
	static constexpr int	RENDER_WIDTH = 2 * RENDER_DIST + 1;							// width of render area in # chunks
	static constexpr int	RENDER_VOLUME = RENDER_WIDTH * RENDER_WIDTH;				// # chunks to be rendered each pass
	static constexpr float	WORLD_RENDER_DIST = (float)(Chunk::width() * RENDER_DIST);	// maximum render distance in world space
	const glm::vec3 origin;

	// helper functions
	static inline int mapchunk(float x) {				// computes coordinate of chunk that provided world space position resides in
		return (int)floor((x + Chunk::width() / 2) / (float)Chunk::width());
	}

	// instance data
	TestCamera&		cam;								// camera object - represents player position, direction, view
	glm::vec2		activeChunk;						// coordinate of chunk that player position is within
	Cache			cache;								// terrain cache
	SpiralIterator	spit;
	Shader			chunkshader;						// shader programs used in world
	glm::vec3		sunPosition;						// position of the sun in the world - directional light

public:

	// Constructor
	World() = delete;
	World(TestCamera& camera) :
		cam(camera),
		activeChunk(mapchunk(cam.Position.x), mapchunk(cam.Position.z)),
		cache(activeChunk.x - Cache::dim()/2, activeChunk.y - Cache::dim()/2),
		spit(),
		chunkshader("shaders/chunkshader.vs", "shaders/chunkshader.fs"),
		origin(0.0f)
	{
		// setup shader values
		chunkshader.use();
		sunPosition = glm::vec3(14, 20, 22);
		glm::vec3 lightdir = glm::normalize(origin - sunPosition);
		chunkshader.setVec3("objcolor", chunk_color);					// <-- temp, unnecessary once textures are added
		chunkshader.setVec3("dlight.direction", lightdir);
		chunkshader.setVec3("dlight.ambient", 0.2f, 0.2f, 0.2f);
		chunkshader.setVec3("dlight.diffuse", 0.5f, 0.5f, 0.5f);
		chunkshader.setVec3("dlight.specular", 0.2f, 0.2f, 0.2f);
	}

	// update world - perform physics updates, draw world within render distance, etc...
	// - deltatime = time difference between current and previous frames [useful for physics]
	void update(double deltatime) {

		// compute active chunk coords
		activeChunk.x = mapchunk(cam.Position.x);
		activeChunk.y = mapchunk(cam.Position.z);

		// setup chunk shader for drawing
		chunkshader.use();
		chunkshader.setVec3("viewpos", cam.Position);
		chunkshader.setMat4("projectionViewMatrix", cam.proj * cam.GetViewMatrix());

		// draw chunks within render distance in a spiral originating at the active chunk
		// this ensures the central chunk will be loaded first (at least on startup)
		spit.reset();
		cache.pollInitRequests();
		for (int i = 0; i < RENDER_VOLUME; i++) {
			cache.draw(spit.getx() + activeChunk.x, spit.getz() + activeChunk.y);
			spit.next();
		}
	}
};

#endif