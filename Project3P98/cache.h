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

	Cache should be used only to draw contiguous(?) rectangular areas of chunks.

	Handles chunk loading and generation.
	Drawing of terrain chunks should be done through a cache object so that the cache remains up to date.

	Cache maintains a square DIMENSION * DIMENSION matrix of chunk objects in memory. This dimension should be at least
	as large as 2 * render distance (in # chunks) of the world. 
	
	When a chunk is requested to be drawn via the cache, the cache first checks to see if that chunk is cached.
	If so, the cached chunk is accessed and drawn. If not, the cache domain lines are shifted approriately and
	the corresponding row or column of old chunk data is reloaded with updated chunks.
*/
class Cache {
private:

	// cache status codes
	enum class CACHESTATUS {VALID, QUEUED, INVALID};	// status for chunks in cache - valid for drawing | queued to be loaded | invalidated, must be queued

	// Cached chunk wrapper
	struct CachedChunk {
		CACHESTATUS status = CACHESTATUS::INVALID;
		Chunk chunk;
		CachedChunk() : chunk(true) {}					// initialize as dummy chunk without computed mesh data
	};

	// Load request queue wrapper
	struct ChunkLoadRequest {
		CachedChunk* chunk = nullptr;					// Cached chunk to load into
		int chunkx;										// chunk coordinate to load
		int chunkz;
	};

	// GL Init request wrapper
	struct GLInitRequest {
		CachedChunk* chunk = nullptr;					// cached chunk to glLoad
	};

	// class constants
	static constexpr int DIM = 30;						// cache matrix dimension [recommended >= 10] - SHOULD BE LARGE ENOUGH TO FIT WORLD RENDER WIDTH
	static constexpr int CACHE_VOLUME = DIM * DIM;		// cache volume - maximum total chunks cached at any time
	static constexpr bool CACHE_PRELOAD = 0;			// preload all chunks in cache on initialization on main thread - !WARNING! COMPUTATIONALLY AND SPACE INTENSIVE
	static constexpr int POLL_DELAY_MILLIS = 200;		// millisecond delay between load request polling while empty
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

	// chunk loading routine
	void pollLoadRequests() {
		while (polling) {
			static GLInitRequest glr;
			if (loadQueue.empty()) std::this_thread::sleep_for(POLL_DELAY);	// poll after delay for efficiency - maybe use condition variable instead of poll delay to avoid busy waiting?
			else {
				printf("generating chunk [%d, %d]\n", loadQueue.front().chunkx, loadQueue.front().chunkz);
				loadQueue.front().chunk->chunk = Chunk(loadQueue.front().chunkx, loadQueue.front().chunkz);	// load requested chunk
				glr.chunk = loadQueue.front().chunk;
				initQueue.push(glr);																		// create gl init request
				loadQueue.pop();
			}
			
		}
	}

	// instance data
	int refx, refz;										// chunk coordinates for reference chunk - lower, leftmost chunk stored in cache grid
	int domx, domz;										// domain boundary indices (intersection corr. with array location of reference chunk)
	std::vector<CachedChunk> cache;						// cache matrix
	std::queue<ChunkLoadRequest> loadQueue;				// queue of chunks to be loaded - polled by loading thread
	std::queue<GLInitRequest> initQueue;				// queue of chunks to be initialized for opengl usage - polled by main thread
	bool polling;										// flag that signals if load queue should be continuously polled
	std::thread load_t;									// chunk loading thread

public:

	// Constructor
	// Defines a matrix of loaded chunks beginning at reference chunk coordinate (referencex, referencez)
	Cache(int referencex = 0, int referencez = 0) :
		POLL_DELAY(POLL_DELAY_MILLIS), refx(referencex), refz(referencez), domx(0), domz(0), polling(true)
	{
		// compute shared resources for chunk objects
		Chunk::computeSharedResources();

		// allocate cache matrix
		cache.reserve(CACHE_VOLUME);
		for (int y = 0; y < DIM; y++) {
			for (int x = 0; x < DIM; x++) {
				CachedChunk cc;
				cache.push_back(cc);
			}
		}

		// preload entire cache if enabled - COMPUTATIONALLY EXPENSIVE and SPACE INTENSIVE
		// this is done on the main thread and will block until completed
		if (CACHE_PRELOAD) {
			printf("Preloading cache of volume %d ... ", CACHE_VOLUME);
			double time = glfwGetTime();
			int cx = refx, cz = refz;
			int i;
			for (int y = 0; y < DIM; y++) {
				for (int x = 0; x < DIM; x++) {
					i = index(x,y);
					cache[i].chunk = Chunk(cx, cz);
					cache[i].chunk.glLoad();
					cache[i].status = CACHESTATUS::VALID;
					cx++;
				}
				cx = refx;
				cz++;
			}
			time = glfwGetTime() - time;
			printf("done - %fs.\n", time);
		}

		// init cache load thread
		load_t = std::thread(&Cache::pollLoadRequests, this);
	}

	// Destructor - clean up cache load thread
	~Cache() {
		polling = false;
		load_t.join();

		// free shared chunk resources
		Chunk::freeSharedResources();
	}

	// delete copy constructor, copy assignment operator, and move constructor
	Cache(const Cache& other) = delete;
	Cache& operator=(Cache other) = delete;
	Cache(Cache&& other) = delete;

	// cache dimension
	static constexpr int dim() { return DIM; }

	// chunk initialization routine - call this once per render loop from gl context thread
	void pollInitRequests() {
		if (!initQueue.empty()) {
			initQueue.front().chunk->chunk.glLoad();
			initQueue.front().chunk->status = CACHESTATUS::VALID;
			initQueue.pop();
		}
	}

	// draw chunk at specified chunk coordinate
	// Appropriate shader must be setup prior to calling this method
	void draw(int chunkx, int chunkz) {
		int distx = chunkx - refx;					// compute distance from reference point in chunk space
		int distz = chunkz - refz;
		if (distx < -1 || distx > DIM || distz < -1 || distz > DIM) {
			printf("INVALID CHUNK COORDINATE [%d, %d] PROVIDED. CACHE COORDINATE MUST NOT EXCEED 1 PAST THE EXISTING CACHE BOUNDARIES.\n", chunkx, chunkz);
			exit(EXIT_FAILURE);
		}

		int index_x = wrap(domx + distx);			// compute corresponding cache matrix index as distance from domain boundaries
		int index_z = wrap(domz + distz);

		if (distx < 0) {						// west cache miss
			domx = wrap(domx - 1);		// shift horizontal domain boundary line
			refx--;						// shift reference coordinate accordingly
			invalidateColumn(domx);		// invalidate old chunk data
		}
		else if (distx >= DIM) {				// east cache miss
			domx = wrap(domx + 1);
			refx++;
			invalidateColumn(wrap(domx - 1));
		}
		if (distz < 0) {						// south cache miss
			domz = wrap(domz - 1);
			refz--;
			invalidateRow(domz);
		}
		else if (distz >= DIM) {				// north cache miss
			domz = wrap(domz + 1);
			refz++;
			invalidateRow(wrap(domz - 1));
		}
		CachedChunk& cc = cache[index(index_x, index_z)];
		if (cc.status == CACHESTATUS::VALID) {						// draw valid cached chunk
			cc.chunk.draw();
		}
		else if (cc.status == CACHESTATUS::INVALID) {				// request this chunk to be loaded into cache, then fail the draw gracefully
			//printf("requesting load chunk [%d, %d] at array [%d, %d]\n", chunkx, chunkz, index_x, index_z);
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