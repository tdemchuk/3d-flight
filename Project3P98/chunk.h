#ifndef CS3P98_TERRAIN_CHUNK_H
#define CS3P98_TERRAIN_CHUNK_H

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "shader.h"
#include <glad/glad.h>		// OpenGL function pointers
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/noise.hpp>
#include <iostream>
#include <thread>

// uncomment to draw chunk borders
//#define DRAW_CHUNK_BORDERS

/*
	Simplified Terrain Chunk Class
	One terrain chunk represents a square grid of terrain oriented along the horizontal XZ plane in world space
	Using grass texture - http://texturelib.com/texture/?path=/Textures/grass/grass/grass_grass_0124
	@author Tennyson Demchuk | 6190532 | td16qg@brocku.ca
	@date 02.13.2021
*/
class Chunk {
private:

	// class constants
	static constexpr int	STRIDE		= 8;							// stride for mesh data - # components per vertex [3 position, 3 normal, 2 tex]
	static constexpr int	CHUNK_WIDTH = 256;							// chunk consumes a square width by width grid in world space - MAKE MULTIPLE OF 2 - Default 1000
	static constexpr float  SCALE		= 2.0f;							// width of one cell in world space - SHOULD DIVIDE CHUNK_WIDTH EVENLY [LARGER = BETTER PERFORMANCE, WORSE DETAIL]
	static constexpr float	DENSITY		= 1.0f / SCALE;					// determines poly density in terrain chunk mesh - inversely proportional to cell scale (> 1 = smaller cells = more polys in mesh)
	static constexpr int	DIM			= (int)(DENSITY * CHUNK_WIDTH);	// dimension of terrain grid in # quads
	static constexpr int	VDIM		= DIM + 1;						// dimension of terrain grid in # vertices (celldim + 1)
	static constexpr float	TEX_SCALE	= 2.0f;							// width of texture used in world space	- SHOULD DIVIDE CHUNK_WIDTH EVENLY	
	static constexpr float	MAX_AMPLITUDE = 14.3f;						// maximum height or depth of terrain
	static constexpr float	FREQUENCY	= 0.003;//0.0005f;				// terrain variance scaling factor
	static int*				chunk_index;								// index array for all chunk objects
	static unsigned int		ebo;
	static unsigned int		tex;


	// compile time helper functions
	static constexpr int numVertices() { return VDIM * VDIM; }
	static constexpr int vertexElements() { return 3 * numVertices(); }
	static constexpr int meshElements() { return STRIDE * numVertices(); }
	static constexpr int numTriangles() { return 2 * DIM * DIM; }
	static constexpr int indexElements() { return 3 * numTriangles(); }
	static constexpr float boundaryOffset() { return SCALE * DIM / 2.0f; }
	static constexpr float texIncrement() { return SCALE / TEX_SCALE; }

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
	static void computeSharedResources() {												// generate and link shared chunk data - call from main thread
		// load and generate grass texture
		int texwidth, texheight, texnumchannels;
		unsigned char* img = stbi_load("textures/grass_top.png",&texwidth, &texheight, &texnumchannels, 0);
		if (!img) {
			printf("Chunk texture load failed.\n");
			stbi_image_free(img);
			exit(0);
		}
		glGenTextures(1, &tex);
		glBindTexture(GL_TEXTURE_2D, tex);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, texwidth, texheight, 0, GL_RGBA, GL_UNSIGNED_BYTE, img);	// GL_RGBA if png, GL_RBG if jpeg
		glGenerateMipmap(GL_TEXTURE_2D);
		stbi_image_free(img);

		// compute mesh element index array
		initIndexArray();
		glGenBuffers(1, &ebo);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexElements() * sizeof(int), chunk_index, GL_STATIC_DRAW);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		delete[] chunk_index;			// no longer need index array in memory
	}
	static void freeSharedResources() {													// cleanup shared chunk data - call from main thread
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
		glEnableVertexAttribArray(2);			// texture attribute
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, STRIDE * sizeof(float), (void*)(6 * sizeof(float)));

		// mesh data unnecessary after GPU upload
		delete[] mesh;
		mesh = nullptr;
	}
	inline float computeHeight(float x, float z) {										// compute and return height at specified XZ plane coordinate in world space
		glm::vec2 coord(x, z);															// https://www.redblobgames.com/maps/terrain-from-noise/
		coord *= FREQUENCY;
		float elevation =
		(glm::simplex(coord) + 1) +							// apply common frequency scale to all octaves
		0.5f * (glm::simplex(1.93f * coord)) +
		0.25f * (glm::simplex(4.07f * coord)) +
		0.125f * (glm::simplex(7.91f * coord)) +
		0.0625f * (glm::simplex(16.1f * coord)) +
		0.03125f * (glm::simplex(32.07f * coord));
		elevation /= 1.5f;
		elevation = (float)pow(elevation, 2);
		return MAX_AMPLITUDE * elevation - MAX_AMPLITUDE / 2;
		//return (float)(cos(0.7 * (double)x)); - test sinusoidal heightmap
	}
	void generateMeshData(unsigned int startz, unsigned int endz, float worldx, float worldz, float startTexV) {
		// generate mesh data
		float px = worldx;
		float py = 0.0f;
		float pz = worldz + (SCALE * startz);
		float tu = 0, tv = startTexV;
		unsigned int index = VDIM * startz;
		unsigned int vindex = 3 * index;
		index *= STRIDE;
		for (unsigned int y = startz; y < endz; y++) {
			for (unsigned int x = 0; x < VDIM; x++) {
				// store vertex positions
				py = computeHeight(px, pz);		// compute vertex height via noise or other method here
				mesh[index++] = px;
				mesh[index++] = py;
				mesh[index++] = pz;
				vertex[vindex++] = px;
				vertex[vindex++] = py;
				vertex[vindex++] = pz;
				index += 3;						// skip normal component for now
				mesh[index++] = tu;
				mesh[index++] = tv;
				px += SCALE;
				tu += texIncrement();
			}
			px = worldx;
			pz += SCALE;
			tu = 0.0f;
			tv += texIncrement();
		}
	}
	inline float height(float* mesh, int x, int z, float wx, float wz) {				// return height of specified vertex - must compute out of bounds coordinates
		if (x < 0 || x > DIM || z < 0 || z > DIM) return computeHeight(wx, wz);	
		return mesh[STRIDE * (z * VDIM + x) + 1];		// return y component of vertex
	}
	inline glm::vec3 computeNormal(float* mesh, int x, int z, float wx, float wz) {		// return height-approximated normal vector for provided vertex
		float l, r, d, u;																// uses "finite difference" method - https://stackoverflow.com/questions/13983189/opengl-how-to-calculate-normals-in-a-terrain-height-grid
		l = height(mesh, x - 1, z, wx - SCALE, wz);										// optional more accurate method: https://stackoverflow.com/questions/45477806/general-method-for-calculating-smooth-vertex-normals-with-100-smoothness
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
		std::thread t1(&Chunk::generateMeshData, this, 0, ZSPLIT1, worldx, worldz, 0);
		std::thread t2(&Chunk::generateMeshData, this, ZSPLIT1, ZSPLIT2, worldx, worldz, (ZSPLIT1 * texIncrement()));
		generateMeshData(ZSPLIT2, VDIM, worldx, worldz, (ZSPLIT2 * texIncrement()));
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
				index += 2;										// skip y component
				vz = mesh[index++];
				norm = computeNormal(mesh, x, y, vx, vz);
#ifdef DRAW_CHUNK_BORDERS
				if (x == 0 || x == DIM || y == 0 || y == DIM) norm *= -1;		// invert normal to show chunk borders
#endif
				mesh[index++] = norm.x;
				mesh[index++] = norm.y;
				mesh[index++] = norm.z;
				index += 2;										// skip texture coords
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

	static unsigned int texID() {
		return tex;
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
unsigned int Chunk::tex = 0;

#endif