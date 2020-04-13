#include <stdint.h>
#include <stdio.h>

enum game_ver {
	S1_FULL = 1 << 0,
	S2_FULL = 1 << 1,
	S3_FULL = 1 << 2
};

typedef uint64_t SpyroModelSectorUnkData; // Rotation data perhaps? At start of sky/world headers but skipped. Must be 8 bytes.
typedef void SpyroSkyPolygons; // Polygons between 1 and 2/3 differ. Cast when needed
typedef void SpyroSkyPolygonsMisc; // Only usable for spyro 2/3
typedef uint64_t SpyroWorldMDLFlags; // Flags for world model. S1 and S2/S3 flags differ. This is a placeholder for a struct. Must be 8 bytes.
typedef void SpyroWorldPolygonLOD; // long distance model format changes between S1 and S2/S3. Placeholder
typedef void SpyroModelVCP;

// ******************************** SKY ********************************

typedef struct VCPCount { // Used for allocating sizes
	uint32_t vertex;
	uint32_t color;
	uint32_t polygon;
	uint32_t polygon_misc;
} VCPCount;

typedef union VertexSky {
	uint32_t val;
	struct vertcomp {
		uint32_t z : 10;
		uint32_t y : 11;
		uint32_t x : 11;
	} comp;
} VertexSky;

typedef union Color {
	uint32_t val;
	struct rgbicomp {
		uint8_t r;
		uint8_t g;
		uint8_t b;
		uint8_t i;
	} comp;
} Color;

typedef union IndexSky {
	uint32_t val;
	struct indexcomp {
		uint32_t UNUSED : 2;
		uint32_t first : 10;
		uint32_t second : 10;
		uint32_t third : 10;
	} comp;
} IndexSky;

typedef struct SecVCPCountS23 {
	uint8_t vertex;
	uint8_t color;
	uint16_t polygon;
	uint16_t polygon_misc;
} SecVCPCountS23;

typedef struct SecVCPCountS1 {
	uint16_t vertex;
	uint16_t color;
	uint16_t polygon;
} SecVCPCountS1;

typedef struct SpyroSkyCoord {
	int16_t y;
	int16_t z;
	int16_t x;
} SpyroSkyCoord;

typedef union PolygonSkyS1 {
	uint64_t val;
	struct polygonskys1polycomp {
		IndexSky vert;
		IndexSky color;
	} comp;
} PolygonSkyS1;

typedef union PolygonSkyS2S3 {
	uint32_t val;
	struct polys2s3comp {
		uint32_t misc_count : 3;
		uint32_t color_first : 7;  // for all 3 verts
		uint32_t color_second : 7; // for all 3 verts
		uint32_t color_third : 7;  // for all 3 verts
		uint32_t first_vertex : 8; // index
	} comp;
} PolygonSkyS2S3;


typedef struct PolygonSkyS2S3Misc {
	uint8_t second_vertex; // index 
	uint8_t third_vertex;// index
	union polymisc {
		uint16_t val;
		struct polys2s3misccomp {
			uint16_t vertex : 8;
			uint16_t color : 7;
			uint16_t vertex_middle : 1;
		} comp;
	} misc; // sizeof *misc * misc_count
} PolygonSkyS2S3Misc;


typedef struct SpyroSky {
	// Header
	int game_type;
	uint32_t sector_count;
	Color background;
	VCPCount vcp;
	 
	// Data
	SpyroModelSectorUnkData *unknown; // 8 bytes - rotation data ("render this if camera is viewing") esque?
	SpyroSkyCoord *coordinates;
	SpyroModelVCP *sector_vcp;
	VertexSky **vertices;
	Color **colors;
	SpyroSkyPolygons **polygons; // CAST based on s1 or s2/s3
	SpyroSkyPolygonsMisc **polygons_misc; // CAST based on s1 or s2/s3 - will be NULL for s1!
} SpyroSky;


int sky_read(FILE *f, SpyroSky *sky, int game_type);
void sky_free(SpyroSky *sky);


// ******************************** PORTAL OBJECTS ********************************
/* each sky portal model has 20 bytes of zeroes at the end in the wad
and after the vertex count there are */

// dummy offset (4 bytes)
// vertex count (4 bytes)
// unk header data (16 bytes including FFFF)
// level transition ID ( 4 bytes)
// zyx/xyz/coordinates (vertex count * (3 * 4 bytes))
// unknown (4 bytes) (maybe rotation?)
// dummy zeroes - used at runtime (20 bytes) (runtime is sector count of sky, pointer to sky to render, third/fourth unk, fifth rgb background of sky)
typedef struct SpyroPortalDataS1 {
	uint32_t vertex_count;
	uint32_t header[4];
	uint32_t header_level_transition_jump_to_trigger; /* enter portal - spyro jumps to a specific trigger */
	uint32_t header_unk;
	uint32_t *vertcoords;
	uint32_t dummy_zeroes[5]; 
} SpyroPortalDataS1;

