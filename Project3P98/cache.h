#ifndef CS3P98_CHUNK_CACHE_H
#define CS3P98_CHUNK_CACHE_H

#include "chunk.h"
#include <GLFW/glfw3.h>
#include <vector>
#include <iostream>
#include <queue>
#include <thread>
#include <chrono>

/*
	Chunk Cache Structure
	@author Tennyson Demchuk
	@date 02.13.2021
	Handles chunk loading and generation.
	Drawing of terrain chunks should be done through a cache object so that the cache remains up to date.

	Cache maintains a square DIMENSION * DIMENSION matrix of chunk objects in memory. This dimension should be at least
	as large as 2 * render distance (in # chunks) of the world. 
	
	When a chunk is requested to be drawn via the cache, the cache first checks to see if that chunk is cached.
	If so, the cached chunk is accessed and drawn. If not, the cache domain lines are shifted approriately and
	the corresponding row or column of old chunk data is reloaded with updated chunks. 
	
	During a reload, only one chunk should be loaded in the offending row/column at a time, starting with the chunk that
	is central to the players direction. This requires knowledge of the players position and velocity direction. We can 
	snap to the chunk nearest (in angle) to the velocity direction as the first chunk. Chunks should then be loaded in 
	an oscillating pattern of left to right in distance from the players velocity direction.

	TODO - make each Chunk load on a separate thread - use thread pool? - gl calls must be made by single thread (or setup shared dual gl context)
*/
class Cache {
private:

	// cache status codes
	enum class CACHESTATUS {VALID, QUEUED, INVALID};	// status for chunks in cache - valid for drawing | queued to be loaded | invalidated, must be queued

	// Cached chunk wrapper
	struct CachedChunk {
		volatile CACHESTATUS status = CACHESTATUS::INVALID;
		Chunk chunk;
		CachedChunk() : chunk(true) {}
	};

	// Load request queue wrapper
	struct ChunkLoadRequest {
		CachedChunk* chunk;								// Cached chunk to load into
		int chunkx;										// chunk coordinate to load
		int chunkz;
	};

	// class constants
	static constexpr int DIM = 10;						// cache matrix dimension
	static constexpr int CACHE_VOL = DIM * DIM;			// cache volume - maximum total chunks cached at any time
	static constexpr int POLL_DELAY_MILLIS = 200;
	const std::chrono::milliseconds POLL_DELAY;

	// class helper functions
	inline int index(int x, int y) {					// compute 1d index from 2d index
		return y * DIM + x;
	}
	inline int wrap(int a) {							// compute cache dimension wrapped index
		return (a + DIM) % DIM;
	}
	inline void invalidateColumn(int x) {				// invalidate cache column x - x must be valid array index (NOT CHUNK COORD)
		for (int row = 0; row < DIM; row++) cache[index(x, row)].status = CACHESTATUS::INVALID;
	}
	inline void invalidateRow(int z) {					// invalidate cache row - z must be valid array index
		for (int col = 0; col < DIM; col++) cache[index(col, z)].status = CACHESTATUS::INVALID;
	}

	// instance data
	int refx, refz;										// chunk coordinates for reference chunk - lower, leftmost chunk stored in cache grid
	int domx, domz;										// initial array coordinate for cache domain boundaries (intersection corr. with array location of reference chunk)
	std::vector<CachedChunk> cache;						// cache matrix
	std::queue<ChunkLoadRequest> loadQueue;				// queue of chunks to be loaded
	//bool polling;										// flag that signals if load queue should be continuously polled
	//std::thread load_t;

public:

	// Constructor
	// Defines a matrix of loaded chunks beginning at reference chunk coordinate (referencex, referencez)
	Cache(int referencex = 0, int referencez = 0) :
		POLL_DELAY(POLL_DELAY_MILLIS), refx(referencex), refz(referencez), domx(0), domz(0)//, polling(true)
	{
		// allocate cache matrix
		cache.reserve(CACHE_VOL);
		for (int y = 0; y < DIM; y++) {
			for (int x = 0; x < DIM; x++) {
				CachedChunk cc;
				cache.push_back(cc);
			}
		}

		// init cache load thread
		//load_t = std::thread(&Cache::pollLoadRequests, this);
	}

	// Destructor - clean up cache load thread
	~Cache() {
		//polling = false;
		//load_t.join();
	}

	// delete copy constructor, copy assignment operator, and move constructor
	Cache(const Cache& other) = delete;
	Cache& operator=(Cache other) = delete;
	Cache(Cache&& other) = delete;

	// cache dimension
	static constexpr int dim() { return DIM; }

	// chunk loading routine
	void pollLoadRequests() {
		//while (polling) {
		static ChunkLoadRequest clr;
		if (!loadQueue.empty()) {
			clr = loadQueue.front();
			loadQueue.pop();
			printf("generating chunk [%d, %d]\n", clr.chunkx, clr.chunkz);
			clr.chunk->chunk = Chunk(clr.chunkx, clr.chunkz);				// load requested chunk
			clr.chunk->status = CACHESTATUS::VALID;							// mark valid for drawing
		}
		//std::this_thread::sleep_for(POLL_DELAY);	// poll after delay for efficiency - maybe use condition variable instead of poll delay to avoid busy waiting?
		//}
	}

	// draw chunk at specified chunk coordinate
	// Appropriate shader must be setup prior to calling this method
	void draw(int chunkx, int chunkz) {
		int distx = chunkx - refx;
		int distz = chunkz - refz;
		if (distx < -1 || distx > DIM || distz < -1 || distz > DIM) {
			printf("INVALID CHUNK COORDINATE [%d, %d] PROVIDED. CACHE COORDINATE MUST NOT EXCEED 1 PAST THE EXISTING CACHE BOUNDARIES.\n", chunkx, chunkz);
			exit(EXIT_FAILURE);
		}

		int rel_index_x = distx - domx;			// compute index in cache array relative to reference point corrected by cache index boundaries
		int rel_index_z = distz - domz;
		if (rel_index_x < 0) {						// west cache miss
			printf("West cache miss.\n");
			domx = wrap(domx - 1);			// shift horizontal domain boundary line
			refx--;							// shift reference coordinate accordingly
			invalidateColumn(domx);
		}
		else if (rel_index_x >= DIM) {				// east cache miss
			printf("East cache miss.\n");
			domx = wrap(domx + 1);
			refx++;
			invalidateColumn(wrap(domx - 1));
		}
		if (rel_index_z < 0) {						// south cache miss
			printf("South cache miss.\n");
			domz = wrap(domz - 1);
			refz--;
			invalidateRow(domz);
		}
		else if (rel_index_z >= DIM) {				// north cache miss
			printf("North cache miss.\n");
			domz = wrap(domz + 1);
			refz++;
			invalidateRow(wrap(domz - 1));
		}
		int index_x = (rel_index_x + domx) % DIM;
		int index_z = (rel_index_z + domz) % DIM;
		//printf("drawing chunk [%d, %d] at array [%d, %d]\n", chunkx, chunkz, index_x, index_z);
		CachedChunk& cc = cache[index(index_x, index_z)];
		//printf("status = %d\n", cc.status);
		if (cc.status == CACHESTATUS::VALID) {						// draw valid cached chunk
			//printf("drawing chunk [%d, %d]\n", chunkx, chunkz);
			cc.chunk.draw();
		}
		else if (cc.status == CACHESTATUS::INVALID) {				// request this chunk to be loaded into cache, then fail the draw gracefully
			cc.status = CACHESTATUS::QUEUED;						// this way the chunk will be drawn when it is ready without causing massive lag and frame drops
			ChunkLoadRequest clr;
			clr.chunk = &cc;
			clr.chunkx = chunkx;
			clr.chunkz = chunkz;
			loadQueue.push(clr);
		}
	}
};

#endif