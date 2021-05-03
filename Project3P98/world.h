#ifndef CS3P98_WORLD_H
#define CS3P98_WORLD_H

/*
	World Class
	Contains scene information, player data, camera object, manages physics, lighting, etc...
	@author Tennyson Demchuk
	@author Daniel Sokic
	@author Aditya Rajyaguru
	@date 05.03.2021
*/

#include "models.h"
#include "texture.h"
#include "camera.h"
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
	Camera&			cam;								// camera object - represents player position, direction, view
	glm::vec2		activeChunk;						// coordinate of chunk that player position is within
	Cache			cache;								// terrain cache
	SpiralIterator	spit;
	Shader			chunkshader;						// shader programs used in world
	Shader			waterShader;
	Shader			modelShader;
	Shader			testShader;
	glm::vec3		sunPosition;						// position of the sun in the world - directional light
	Texture			grasstex;							// textures used in terrain
	Texture			sandtex;
	Texture			stonetex;
	//glm::vec3		objspawn;
	//Objective		obj;

public:

	// Constructor
	World() = delete;
	World(Camera& camera) :
		cam(camera),
		activeChunk(mapchunk(cam.camPos.x), mapchunk(cam.camPos.z)),
		cache(activeChunk.x - Cache::dim() / 2, activeChunk.y - Cache::dim() / 2),
		spit(),
		chunkshader("shaders/chunkshader.vs", "shaders/chunkshader.fs"),
		waterShader("shaders/basic.vs", "shaders/basicwatershader.fs"),
		modelShader("shaders/basic.vs", "shaders/basic.fs"),
		testShader("shaders/test.vs","shaders/test.fs"),
		origin(0.0f)
		//objspawn(0,60,0),
		//obj(objspawn)
	{
		// load and generate terrain textures
		grasstex.load("textures/grass_top.png");
		sandtex.load("textures/sand.png");
		stonetex.load("textures/stone.png");

		// setup chunkshader
		chunkshader.use();
		chunkshader.setInt("grasstex", 0);				// upload multiple textures to shader - https://stackoverflow.com/a/25252981
		chunkshader.setInt("sandtex", 1);				// using minecraft textures, all credit to mojang
		chunkshader.setInt("stonetex", 2);
		sunPosition = glm::vec3(14, 60, 22);
		glm::vec3 lightdir = glm::normalize(origin - sunPosition);
		chunkshader.setVec3("dlight.direction", lightdir);
		chunkshader.setVec3("dlight.ambient", 0.2f, 0.2f, 0.2f);
		chunkshader.setVec3("dlight.diffuse", 0.5f, 0.5f, 0.5f);
		chunkshader.setVec3("dlight.specular", 0.2f, 0.2f, 0.2f);

		// bind multiple textures for rendering terrain
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, grasstex.id);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, sandtex.id);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, stonetex.id);

		waterShader.use();
		waterShader.setMat4("modelMatrix", glm::mat4(1.0));
		waterShader.setVec3("dlight.direction", lightdir);
		waterShader.setVec3("dlight.ambient", 0.2f, 0.2f, 0.2f);
		waterShader.setVec3("dlight.diffuse", 0.5f, 0.5f, 0.5f);
		waterShader.setVec3("dlight.specular", 0.2f, 0.2f, 0.2f);

		modelShader.use();
		modelShader.setVec3("dlight.direction", lightdir);
		modelShader.setVec3("dlight.ambient", 0.2f, 0.2f, 0.2f);
		modelShader.setVec3("dlight.diffuse", 0.5f, 0.5f, 0.5f);
		modelShader.setVec3("dlight.specular", 0.2f, 0.2f, 0.2f);
	}

	// returns the height of the terrain at the given world coordinate
	inline float testHeight(float x, float y) {
		return cache.getHeight(mapchunk(x), mapchunk(y), x, y);
	}

	// update world - perform physics updates, draw world within render distance, etc...
	// - deltatime = time difference between current and previous frames [useful for physics]
	void update(double deltatime) {

		// compute active chunk coords
		activeChunk.x = mapchunk(cam.camPos.x);
		activeChunk.y = mapchunk(cam.camPos.z);
		// setup chunk shader for drawing
		chunkshader.use();
		chunkshader.setVec3("viewpos", cam.camPos);
		chunkshader.setMat4("projectionViewMatrix", cam.proj * cam.GetViewMatrix());

		//waterShader.use();
		//waterShader.setVec3("viewpos", cam.camPos);
		//waterShader.setMat4("projectionViewMatrix", cam.proj * cam.GetViewMatrix());
		
		// draw chunks within render distance in a spiral originating at the active chunk
		// this ensures the central chunk will be loaded first (at least on startup)
		spit.reset();
		cache.pollInitRequests();
		for (int i = 0; i < RENDER_VOLUME; i++) {
			cache.draw(spit.getx() + activeChunk.x, spit.getz() + activeChunk.y, chunkshader, waterShader);
			spit.next();
		}

		// setup simple shader for objective drawing
		//modelShader.use();
		//modelShader.setVec3("viewpos", cam.camPos);
		//modelShader.setMat4("projectionViewMatrix", cam.proj * cam.GetViewMatrix());
		/*testShader.use();
		testShader.setVec3("viewpos", cam.camPos);
		testShader.setMat4("projectionViewMatrix", cam.proj * cam.GetViewMatrix());
		obj.draw(deltatime, testShader);*/

		// draw water table
		waterShader.use();
		waterShader.setVec3("viewpos", cam.camPos);
		waterShader.setMat4("projectionViewMatrix", cam.proj * cam.GetViewMatrix());
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glBindVertexArray(waterVAO);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
		glDisable(GL_BLEND);
	}
};

#endif