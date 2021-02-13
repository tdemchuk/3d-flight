#ifndef CS3P98_TERRAIN_CHUNK_H
#define CS3P98_TERRAIN_CHUNK_H

#include "shader.h"
#include <glad/glad.h>		// OpenGL function pointers
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

// class data
int*		chunk_index		= nullptr;		// index array for all chunk objects
int			chunk_indexref	= 0;			// number of chunk objects that are referencing this data
glm::vec3	chunk_color			= glm::vec3(0.105f, 0.713f, 0.227f);

/*
	Simplified Terrain Chunk Class
	One terrain chunk represents a square grid of terrain oriented along the horizontal XZ plane in world space
	@author Tennyson Demchuk | 6190532 | td16qg@brocku.ca
	@date 02.13.2021
*/
class Chunk {
private:

	// class constants
	static constexpr int	STRIDE		= 6;						// stride for mesh data - # components per vertex [3 position, 3 normal, 3 tex (coming soon)]
	static constexpr int	CHUNK_WIDTH = 64;						// chunk consumes a square width by width grid in world space - MAKE MULTIPLE OF 2
	static constexpr float  SCALE		= 0.5f;						// width of one cell in world space
	static constexpr float	DENSITY		= 1.0f / SCALE;				// determines poly density in terrain chunk mesh - inversely proportional to cell scale (> 1 = smaller cells = more polys in mesh)
	static constexpr int	DIM			= DENSITY * CHUNK_WIDTH;	// dimension of terrain grid in # quads
	static constexpr int	VDIM		= DIM + 1;					// dimension of terrain grid in # vertices (celldim + 1)

	// compile time helper functions
	static constexpr int numVert() { return VDIM * VDIM; }
	static constexpr int meshElements() { return numVert() * STRIDE; }
	static constexpr int numTriangles() { return 2 * DIM * DIM; }
	static constexpr int indexElements() { return 3 * numTriangles(); }
	static constexpr float boundaryOffset() { return SCALE * DIM / 2.0f; }

	// helper functions
	static void initIndexArray() {
		chunk_index = new int[indexElements()];
		/*
			generate triangle indices (with CCW winding)

				b --- d		<- (a,b,c,d) are cell vertices relative to current cell
				|  \  |
				c --- a
		*/
		int index = 0;
		int a = 0, b = 0, c = 0, d = 0;
		for (unsigned int y = 0; y < DIM; y++) {	// iterate cells
			for (unsigned int x = 0; x < DIM; x++) {
				// compute vertex indices
				c = y * VDIM + x;
				a = c + 1;
				b = c + VDIM;
				d = a + VDIM;
				// add indices for triangle 1 and 2
				chunk_index[index++] = a;
				chunk_index[index++] = b;
				chunk_index[index++] = c;
				chunk_index[index++] = a;
				chunk_index[index++] = d;
				chunk_index[index++] = b;
			}
		}
	}
	inline float computeHeight(float x, float z) {				// compute and return height at specified XZ plane coordinate in world space
		return (float)(cos(0.7 * (double)x));
	}
	inline float height(int x, int z, float wx, float wz) {		// return height of specified vertex - must compute out of bounds coordinates
		if (x < 0 || x > DIM || z < 0 || z > DIM) return computeHeight(wx, wz);	
		return mesh[STRIDE * (z * VDIM + x) + 1];				// return y component of vertex
	}
	inline glm::vec3 computeNormal(int x, int z, float wx, float wz) {				// return height-approximated normal vector for provided vertex
		float l, r, d, u;
		l = height(x - 1, z, wx, wz);
		r = height(x + 1, z, wx, wz);
		u = height(x, z - 1, wx, wz);
		d = height(x, z + 1, wx, wz);
		return glm::normalize(glm::vec3(l - r, 2.0f, d - u));
	}

	// instance data
	float* mesh = nullptr;			// vertex, normal, texture data for terrain chunk
	unsigned int vao, vbo, ebo;		// GL vertex array, buffer, and element object ID's

public:
	
	//glm::mat4 model = glm::mat4(1.0f);		// model matrix - encodes scale and position in world space for this chunk - don't need this if chunk coord are transformed into world space during initialization
	//glm::mat4 normal = glm::mat4(1.0f);		// normal matrix - used to correct normals for unevenly scaled objects - does not apply to terrain chunks

	// Constructor - no default constructor
	Chunk(int chunkcoordx = 0, int chunkcoordz = 0) {

		// allocate
		mesh = new float[meshElements()];
		glGenVertexArrays(1, &vao);
		glGenBuffers(1, &vbo);
		glGenBuffers(1, &ebo);

		// handle index generation if first chunk
		if (chunk_index == nullptr) initIndexArray();
		chunk_indexref++;

		// transform chunk coord to world coords - points to centre of chunk
		float worldx = CHUNK_WIDTH * chunkcoordx;
		float worldz = CHUNK_WIDTH * chunkcoordz;

		// generate mesh data
		float vx0 = worldx - boundaryOffset();	// initial vertex coords in world space
		float vz = worldz - boundaryOffset();
		float vx = vx0;
		unsigned int index = 0;
		for (int y = 0; y < VDIM; y++) {
			for (int x = 0; x < VDIM; x++) {
				// store vertex positions
				mesh[index++] = vx;
				mesh[index++] = computeHeight(vx, vz);		// compute vertex height via noise or other method here
				mesh[index++] = vz;
				index += 3;
				vx += SCALE;
			}
			vx = vx0;
			vz += SCALE;
		}

		index = 0;
		glm::vec3 norm;
		for (int y = 0; y < VDIM; y++) {
			for (int x = 0; x < VDIM; x++) {
				// store vertex normals
				vx = mesh[index];
				index += 2;
				vz = mesh[index++];
				norm = computeNormal(x, y, vx, vz);
				mesh[index++] = norm.x;
				mesh[index++] = norm.y;
				mesh[index++] = norm.z;
				index += 3;
			}
		}

		// setup GL buffers
		glBindVertexArray(vao);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, meshElements() * sizeof(float), mesh, GL_STATIC_DRAW);		// upload mesh data to graphics card
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexElements() * sizeof(int), chunk_index, GL_STATIC_DRAW);

		glEnableVertexAttribArray(0);			// position attribute
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, STRIDE * sizeof(float), (void*)0);
		glEnableVertexAttribArray(1);			// normal attribute
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, STRIDE * sizeof(float), (void*)(3 * sizeof(float)));

		// can cleanup mesh data at this point 
		// vertex data may be needed again however - could use seperately stored vertex data without normals and tex coords
		printf("before del\n");
		delete[] mesh;
		printf("after del\n");
	}

	// Destructor - cleanup
	~Chunk() {
		glDeleteVertexArrays(1, &vao);
		glDeleteBuffers(1, &vbo);
		glDeleteBuffers(1, &ebo);
		chunk_indexref--;
		if (chunk_indexref == 0) delete[] chunk_index;
		//delete[] mesh;
	}

	// draws this terrain chunk to the screen
	// ensure to setup terrain shader beforehand
	void draw() {
		glBindVertexArray(vao);
		glDrawElements(GL_TRIANGLES, indexElements(), GL_UNSIGNED_INT, 0);
	}
};

#endif