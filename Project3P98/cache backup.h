#ifndef CS3P98_TERRAIN_CHUNK_H
#define CS3P98_TERRAIN_CHUNK_H

#include "shader.h"
#include <glad/glad.h>		// OpenGL function pointers
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/noise.hpp>
#include <iostream>
#include <thread>			// for multithreading

// class data		
glm::vec3	chunk_color		= glm::vec3(0.105f, 0.713f, 0.227f);

/*
	Simplified Terrain Chunk Class
	One terrain chunk represents a square grid of terrain oriented along the horizontal XZ plane in world space
	@author Tennyson Demchuk | 6190532 | td16qg@brocku.ca
	@date 02.13.2021
*/
class Chunk {
private:

	// class constants
	static constexpr int	STRIDE		= 6;							// stride for mesh data - # components per vertex [3 position, 3 normal, 3 tex (coming soon)]
	static constexpr int	CHUNK_WIDTH = 500;							// chunk consumes a square width by width grid in world space - MAKE MULTIPLE OF 2
	static constexpr float  SCALE		= 2.0f;							// width of one cell in world space - SHOULD DIVIDE CHUNK_WIDTH EVENLY
	static constexpr float	DENSITY		= 1.0f / SCALE;					// determines poly density in terrain chunk mesh - inversely proportional to cell scale (> 1 = smaller cells = more polys in mesh)
	static constexpr int	DIM			= (int)(DENSITY * CHUNK_WIDTH);	// dimension of terrain grid in # quads
	static constexpr int	VDIM		= DIM + 1;						// dimension of terrain grid in # vertices (celldim + 1)
	static float*			mesh;										// vertex, normal, and texture data for terrain chunk
	static int*				chunk_index;								// index array for all chunk objects
	static int				chunk_refnum;								// number of instantiated chunk object using shared resources (index, mesh array, ebo)
	static unsigned int		ebo;


	// compile time helper functions
	static constexpr int numVertices() { return VDIM * VDIM; }
	static constexpr int vertexElements() { return 3 * numVertices(); }
	static constexpr int meshElements() { return STRIDE * numVertices(); }
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
	inline float computeHeight(float x, float z) {										// compute and return height at specified XZ plane coordinate in world space
		constexpr float MAX_AMPLITUDE = 18.3f;											// https://www.redblobgames.com/maps/terrain-from-noise/
		constexpr float FREQUENCY = 0.003f;
		glm::vec2 coord(x, z);
		float elevation =
			1.0f * (glm::simplex(1.0f * FREQUENCY * coord) + 1) / 2.0f +				// apply common frequency scale to all octaves
			0.5f * (glm::simplex(2.0f * FREQUENCY * coord) + 1) / 2.0f +
			0.25f * (glm::simplex(4.0f * FREQUENCY * coord) + 1) / 2.0f;
		elevation = (float)pow(elevation, 2.5);
		return MAX_AMPLITUDE * elevation - MAX_AMPLITUDE;
		//return (float)(cos(0.7 * (double)x)); - test sinusoidal heightmap
	}
	void generateMeshData(unsigned int start, unsigned int end, float worldx, float worldz) {	// computes mesh position data. start and end should be multiple of STRIDE, goes from start until end (not inclusive)
		// generate mesh data
		float vx0 = worldx - boundaryOffset();	// initial vertex coords in world space
		float vz = worldz - boundaryOffset();
		float vx = vx0;
		float vy = 0;
		unsigned int index = start, vindex = 3 * index / STRIDE;
		for (int y = 0; y < VDIM; y++) {
			for (int x = 0; x < VDIM; x++) {
				// store vertex positions
				vy = computeHeight(vx, vz);		// compute vertex height via noise or other method here
				mesh[index++] = vx;
				mesh[index++] = vy;
				mesh[index++] = vz;
				vertex[vindex++] = vx;
				vertex[vindex++] = vy;
				vertex[vindex++] = vz;
				index += 3;
				vx += SCALE;
				if (index >= end) return;
			}
			vx = vx0;
			vz += SCALE;
		}
	}
	inline float height(float* mesh, int x, int z, float wx, float wz) {				// return height of specified vertex - must compute out of bounds coordinates
		if (x < 0 || x > DIM || z < 0 || z > DIM) return computeHeight(wx, wz);	
		return mesh[STRIDE * (z * VDIM + x) + 1];		// return y component of vertex
	}
	inline glm::vec3 computeNormal(float* mesh, int x, int z, float wx, float wz) {		// return height-approximated normal vector for provided vertex
		float l, r, d, u;																// uses "finite difference" method - https://stackoverflow.com/questions/13983189/opengl-how-to-calculate-normals-in-a-terrain-height-grid
		l = height(mesh, x - 1, z, wx, wz);												// optional more accurate method: https://stackoverflow.com/questions/45477806/general-method-for-calculating-smooth-vertex-normals-with-100-smoothness
		r = height(mesh, x + 1, z, wx, wz);
		u = height(mesh, x, z - 1, wx, wz);
		d = height(mesh, x, z + 1, wx, wz);
		return glm::normalize(glm::vec3(l - r, 2.0f, d - u));
	}

	// instance data
	float* vertex;					// vertex positions
	unsigned int vao, vbo;			// GL vertex array, buffer object ID's

public:

	// Constructor - no default constructor
	Chunk(int chunkcoordx = 0, int chunkcoordz = 0) {

		// allocate
		vertex = new float[vertexElements()];		
		glGenVertexArrays(1, &vao);
		glGenBuffers(1, &vbo);

		static int num = 0;

		// handle index generation if first chunk
		if (chunk_index == nullptr) {
			mesh = new float[meshElements()];
			initIndexArray();
			glGenBuffers(1, &ebo);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexElements() * sizeof(int), chunk_index, GL_STATIC_DRAW);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
			delete[] chunk_index;								// no longer need index array in memory
		}
		chunk_refnum++;

		// transform chunk coord to world coords - points to centre of chunk
		float worldx = (int)(CHUNK_WIDTH * chunkcoordx);
		float worldz = (int)(CHUNK_WIDTH * chunkcoordz);

		int numthreads = 2;										// 2 processing threads (main and thread t)
		int splitPoint = STRIDE * (int)(numVertices() / 2);		// compute mesh index that represents the division in work between the threads - integer division rounds down
		std::thread t(&Chunk::generateMeshData, this, splitPoint, meshElements(), worldx, worldz);
		generateMeshData(0, splitPoint, worldx, worldz);
		t.join();												// wait until thread done

		unsigned int index = 0;
		float vx, vz;
		glm::vec3 norm;
		for (int y = 0; y < VDIM; y++) {
			for (int x = 0; x < VDIM; x++) {
				// store vertex normals
				vx = mesh[index];
				index += 2;
				vz = mesh[index++];
				norm = computeNormal(mesh, x, y, vx, vz);
				mesh[index++] = norm.x;
				mesh[index++] = norm.y;
				mesh[index++] = norm.z;
			}
		}

		// setup GL buffers
		glBindVertexArray(vao);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, meshElements() * sizeof(float), mesh, GL_STATIC_DRAW);		// upload mesh data to graphics card
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);													// bind EBO that was already uploaded to GPU

		glEnableVertexAttribArray(0);			// position attribute
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, STRIDE * sizeof(float), (void*)0);
		glEnableVertexAttribArray(1);			// normal attribute
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, STRIDE * sizeof(float), (void*)(3 * sizeof(float)));

		printf("Chunk generated\n");
	}

	// Destructor - cleanup
	~Chunk() {
		delete[] vertex;
		chunk_refnum--;
		if (chunk_refnum == 0) {
			delete[] mesh;
			glDeleteBuffers(1, &ebo);
		}
		glDeleteVertexArrays(1, &vao);
		glDeleteBuffers(1, &vbo);
	}

	// copy and swap - https://stackoverflow.com/questions/3279543/what-is-the-copy-and-swap-idiom
	friend void swap(Chunk& first, Chunk& second) {
		using std::swap;
		swap(first.vao, second.vao);
		swap(first.vbo, second.vbo);
		swap(first.vertex, second.vertex);
	}

	// Copy constructor
	Chunk(const Chunk& other) : vertex(new float[vertexElements()]), vao(other.vao), vbo(other.vbo) {
		chunk_refnum++;
		std::copy(other.vertex, other.vertex + vertexElements(), vertex);
	}

	// Copy assign
	Chunk& operator=(Chunk other) {
		chunk_refnum++;
		swap(*this, other);
		return *this;
	}

	// Move constructor
	Chunk(Chunk&& other) noexcept : vao(0), vbo(0), vertex() {
		chunk_refnum++;
		swap(*this, other);
	}

	// returns width of one chunk in world space
	static constexpr int width() {
		return CHUNK_WIDTH;
	}

	// draws this terrain chunk to the screen
	// ensure to setup terrain shader beforehand
	void draw() {
		glBindVertexArray(vao);
		glDrawElements(GL_TRIANGLES, indexElements(), GL_UNSIGNED_INT, 0);
	}
};

// Initialize static values
int* Chunk::chunk_index = nullptr;
float* Chunk::mesh = nullptr;
int Chunk::chunk_refnum = 0;
unsigned int Chunk::ebo = 0;

#endif