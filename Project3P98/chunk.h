#ifndef CS3P98_TERRAIN_CHUNK_H
#define CS3P98_TERRAIN_CHUNK_H

#include "shader.h"
#include <glad/glad.h>		// OpenGL function pointers
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/noise.hpp>
#include <iostream>
#include <thread>

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
	static constexpr int	CHUNK_WIDTH = 1000;							// chunk consumes a square width by width grid in world space - MAKE MULTIPLE OF 2 - Default 1000
	static constexpr float  SCALE		= 10.0f;						// width of one cell in world space - SHOULD DIVIDE CHUNK_WIDTH EVENLY [LARGER = BETTER PERFORMANCE, WORSE DETAIL]
	static constexpr float	DENSITY		= 1.0f / SCALE;					// determines poly density in terrain chunk mesh - inversely proportional to cell scale (> 1 = smaller cells = more polys in mesh)
	static constexpr int	DIM			= (int)(DENSITY * CHUNK_WIDTH);	// dimension of terrain grid in # quads
	static constexpr int	VDIM		= DIM + 1;						// dimension of terrain grid in # vertices (celldim + 1)
	static constexpr float	MAX_AMPLITUDE = 24.3f;						// maximum height or depth of terrain
	static constexpr float	FREQUENCY = 0.001f;							// terrain variance scaling factor
	static int*				chunk_index;								// index array for all chunk objects
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
	static void computeSharedResources() {												// generate and link shared chunk data
		//mesh = new float[meshElements()];
		initIndexArray();
		glGenBuffers(1, &ebo);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexElements() * sizeof(int), chunk_index, GL_STATIC_DRAW);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		delete[] chunk_index;			// no longer need index array in memory
	}
	static void freeSharedResources() {													// cleanup shared chunk data
		glDeleteBuffers(1, &ebo);
	}
	void glLoad() {																		// prepare object for rendering with opengl - only call this on thread associated with opengl context
		// setup GL buffers
		glGenVertexArrays(1, &vao);
		glGenBuffers(1, &vbo);

		glBindVertexArray(vao);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, meshElements() * sizeof(float), mesh, GL_STATIC_DRAW);		// upload mesh data to graphics card
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);													// bind EBO that was already uploaded to GPU

		glEnableVertexAttribArray(0);			// position attribute
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, STRIDE * sizeof(float), (void*)0);
		glEnableVertexAttribArray(1);			// normal attribute
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, STRIDE * sizeof(float), (void*)(3 * sizeof(float)));

		// mesh data unnecessary after GPU upload
		delete[] mesh;
		mesh = nullptr;
	}
	inline float computeHeight(float x, float z) {										// compute and return height at specified XZ plane coordinate in world space
		glm::vec2 coord(x, z);															// https://www.redblobgames.com/maps/terrain-from-noise/
		coord *= FREQUENCY;
		float elevation =
			(glm::simplex(coord) + 1) / 2.0f +				// apply common frequency scale to all octaves
			0.5f * (glm::simplex(2.0f * coord) + 1) / 2.0f +
			0.25f * (glm::simplex(4.0f * coord) + 1) / 2.0f;
		elevation = (float)pow(elevation, 4);
		return MAX_AMPLITUDE * elevation - MAX_AMPLITUDE;
		//return (float)(cos(0.7 * (double)x)); - test sinusoidal heightmap
	}
	void generateMeshData(unsigned int startz, unsigned int endz, float worldx, float worldz) {
		// generate mesh data
		float vz = worldz + (SCALE * startz);
		float vx = worldx;
		float vy = 0;
		unsigned int index = VDIM * startz;
		unsigned int vindex = 3 * index;
		index *= STRIDE;
		for (unsigned int y = startz; y < endz; y++) {
			for (unsigned int x = 0; x < VDIM; x++) {
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
			}
			vx = worldx;
			vz += SCALE;
		}
	}
	inline float height(float* mesh, int x, int z, float wx, float wz) {				// return height of specified vertex - must compute out of bounds coordinates
		if (x < 0 || x > DIM || z < 0 || z > DIM) return computeHeight(wx, wz);	
		return mesh[STRIDE * (z * VDIM + x) + 1];		// return y component of vertex
	}
	inline glm::vec3 computeNormal(float* mesh, int x, int z, float wx, float wz) {		// return height-approximated normal vector for provided vertex
		float l, r, d, u;																// uses "finite difference" method - https://stackoverflow.com/questions/13983189/opengl-how-to-calculate-normals-in-a-terrain-height-grid
		l = height(mesh, x - 1, z, wx - SCALE, wz);												// optional more accurate method: https://stackoverflow.com/questions/45477806/general-method-for-calculating-smooth-vertex-normals-with-100-smoothness
		r = height(mesh, x + 1, z, wx + SCALE, wz);
		u = height(mesh, x, z - 1, wx, wz - SCALE);
		d = height(mesh, x, z + 1, wx, wz + SCALE);
		return glm::normalize(glm::vec3(l - r, 2.0f, d - u));
	}

	// instance data
	float* vertex;					// vertex positions
	float* mesh;					// vertex, normal, and texture data for terrain chunk to be uploaded to GPU
	unsigned int vao, vbo;			// GL vertex array, buffer object ID's

	// give cache class private access
	friend class Cache;		

public:

	// dummy constructor - stupid hack
	Chunk(bool isDummy) {
		if (true) {
			mesh = new float[meshElements()];
			vertex = new float[vertexElements()];
			vao = 0;
			vbo = 0;
		}
		else {
			printf("MUST INIT AS DUMMY OBJECT.\n");
			exit(0);
		}
	}

	// Constructor
	Chunk(int chunkcoordx = 0, int chunkcoordz = 0) {

		// allocate
		mesh = new float[meshElements()];
		vertex = new float[vertexElements()];

		// transform chunk coord to world coords - points to centre of chunk
		float worldx = (float)(int)(CHUNK_WIDTH * chunkcoordx);
		float worldz = (float)(int)(CHUNK_WIDTH * chunkcoordz);
		worldx -= boundaryOffset();								// transform coords to point to lower leftmost vertex of chunk
		worldz -= boundaryOffset();

		// generate mesh position data in parallel - 3 threads - TODO: move to thread pool eventually to improve performance
		static constexpr int NUMTHREADS = 3;
		static constexpr int ZSPLIT1 = VDIM / NUMTHREADS;
		static constexpr int ZSPLIT2 = ZSPLIT1 + ZSPLIT1;
		std::thread t1(&Chunk::generateMeshData, this, 0, ZSPLIT1, worldx, worldz);
		std::thread t2(&Chunk::generateMeshData, this, ZSPLIT1, ZSPLIT2, worldx, worldz);
		generateMeshData(ZSPLIT2, VDIM, worldx, worldz);
		t1.join();
		t2.join();
		//t3.join();

		// generate vertex normals for mesh
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

		
	}

	// Destructor - cleanup
	~Chunk() {
		delete[] mesh;
		delete[] vertex;
		glDeleteVertexArrays(1, &vao);
		glDeleteBuffers(1, &vbo);
	}

	// copy and swap - https://stackoverflow.com/questions/3279543/what-is-the-copy-and-swap-idiom
	friend void swap(Chunk& first, Chunk& second) {
		using std::swap;
		swap(first.vao, second.vao);
		swap(first.vbo, second.vbo);
		swap(first.vertex, second.vertex);
		swap(first.mesh, second.mesh);
	}

	// Copy constructor
	Chunk(const Chunk& other) : vertex(new float[vertexElements()]), mesh(new float[meshElements()]), vao(other.vao), vbo(other.vbo) {
		std::copy(other.vertex, other.vertex + vertexElements(), vertex);
		std::copy(other.mesh, other.mesh + meshElements(), mesh);
	}

	// Copy assign
	Chunk& operator=(Chunk other) {
		swap(*this, other);
		return *this;
	}

	// Move constructor
	Chunk(Chunk&& other) noexcept : vao(0), vbo(0), vertex(), mesh() {
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
unsigned int Chunk::ebo = 0;

#endif