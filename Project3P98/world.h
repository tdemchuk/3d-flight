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

	// helper class for generating coordinates in a spiral iteratively - taken from https://stackoverflow.com/a/14010215
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
	static constexpr int	RENDER_RADIUS = 5;											// radial render distance in # chunks (beyond the central chunk). ie. render dist 1 will render the central (active) chunk and 1 beyond it in every direction for 9 chunks total
	static constexpr int	RENDER_WIDTH = 2 * RENDER_RADIUS + 1;						// width of render area in # chunks - render width is always an odd number
	static constexpr int	RENDER_VOLUME = RENDER_WIDTH * RENDER_WIDTH;				// # chunks to be rendered each pass
	static constexpr float	WORLD_RENDER_DIST = (float)(Chunk::width() * RENDER_RADIUS);// maximum render distance in world space - using this will guarantee pop-in
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
	Shader			waterShader;
	glm::vec3		sunPosition;						// position of the sun in the world - directional light

	unsigned int waterVAO, waterVBO, waterEBO;

public:

	// Constructor
	World() = delete;
	World(TestCamera& camera) :
		cam(camera),
		activeChunk(mapchunk(cam.Position.x), mapchunk(cam.Position.z)),
		cache(activeChunk.x - Cache::dim()/2, activeChunk.y - Cache::dim()/2),
		spit(),
		chunkshader("shaders/chunkshader.vs", "shaders/chunkshader.fs"),
		waterShader("shaders/basic.vs", "shaders/basicwatershader.fs"),
		origin(0.0f)
	{
		// setup shader values
		chunkshader.use();
		sunPosition = glm::vec3(14, 60, 22);
		glm::vec3 lightdir = glm::normalize(origin - sunPosition);
		chunkshader.setVec3("dlight.direction", lightdir);
		chunkshader.setVec3("dlight.ambient", 0.2f, 0.2f, 0.2f);
		chunkshader.setVec3("dlight.diffuse", 0.5f, 0.5f, 0.5f);
		chunkshader.setVec3("dlight.specular", 0.2f, 0.2f, 0.2f);

		waterShader.use();
		waterShader.setVec3("dlight.direction", lightdir);
		waterShader.setVec3("dlight.ambient", 0.2f, 0.2f, 0.2f);
		waterShader.setVec3("dlight.diffuse", 0.5f, 0.5f, 0.5f);
		waterShader.setVec3("dlight.specular", 0.2f, 0.2f, 0.2f);


		// setup water table plane - for demo purposes - make part of chunk eventually
		constexpr float f0 = -1000;
		constexpr float f1 = 1000;
		constexpr float wHeight = 0.0f;
		float waterTableVertices[] = {						// define 4 vertices of infinite water table plane
			f0, wHeight, f0,
			f1, wHeight, f0,
			f0, wHeight, f1,
			f1, wHeight, f1,
		};
		int waterTableIndices[] = {
			0, 1, 2,
			2, 1, 3
		};
		glGenVertexArrays(1, &waterVAO);
		glGenBuffers(1, &waterVBO);
		glGenBuffers(1, &waterEBO);
		glBindVertexArray(waterVAO);
		glBindBuffer(GL_ARRAY_BUFFER, waterVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(waterTableVertices), waterTableVertices, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, waterEBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(waterTableIndices), waterTableIndices, GL_STATIC_DRAW);
		glBindVertexArray(0);
	}

	// destructor
	~World() {
		glDeleteVertexArrays(1, &waterVAO);
		glDeleteBuffers(1, &waterVBO);
		glDeleteBuffers(1, &waterEBO);
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
		glBindTexture(GL_TEXTURE_2D, Chunk::texID());

		// draw chunks within render distance in a spiral originating at the active chunk
		// this ensures the central chunk will be loaded first (at least on startup)
		spit.reset();
		cache.pollInitRequests();
		for (int i = 0; i < RENDER_VOLUME; i++) {
			cache.draw(spit.getx() + activeChunk.x, spit.getz() + activeChunk.y);
			spit.next();
		}

		// draw water table
		waterShader.use();
		waterShader.setVec3("viewpos", cam.Position);
		waterShader.setMat4("projectionViewMatrix", cam.proj * cam.GetViewMatrix());
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glBindVertexArray(waterVAO);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
		glDisable(GL_BLEND);
	}
};

#endif