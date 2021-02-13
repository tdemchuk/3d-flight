#ifndef CS3P98_WORLD_H
#define CS3P98_WORLD_H

/*
	World - Header only class file
	@author Tennyson Demchuk
	@date 02.12.2021
*/


#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>


typedef struct {
	glm::vec2 chunkcoord;	// coordinate in chunk space - map to world space as chunkcoord * chunk width - (chunkwidth / 2)
	glm::mat4 model;		// model matrix for chunk - encodes position in world space
	glm::mat4 normal;		// normal matrix for chunk
} chunk_data;


/*
	World Class
	Represents an explorable, prcedurally generated world
*/
class World {
private:

	// terrain chunk cache vars
	chunk_data* cache	= nullptr;		// cache array
	int cache_width		= 0;			// width of cache grid (should be an odd number)
	int xboundary		= 0;			// bounding indices of 2D cache array [x = row boundary, y = column boundary]
	int yboundary		= 0;
	glm::vec2 intpoint;					// boundary intersection point represents the lower leftmost chunk coordinate stored in cache

public:
	// glob vars
	static constexpr int MAX_RENDER_DIST = 16;			// maximum render distance for world in # chunks (out from originating chunk)
	static constexpr int DEFAULT_RENDER_DIST = 4;		// 

	/*
		Constructor - initializes the world object and generates enough terrain chunks to fill cache,
		centred around the origin chunk (default = [0,0])
		Then renders chunks within render distance
	*/

};

#endif