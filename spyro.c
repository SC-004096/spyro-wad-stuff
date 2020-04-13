#include "spyro.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

// Spyro 1 skies have 3 vertices per polygon
// Spyro 2/3 can have up to 11 vertices per polygon


// All functions take in a file to begin reading from or write to.
// It is assumed one is already at the correct offset for proper data

void sky_free(SpyroSky *sky) {
	unsigned int i = 0;
	if(!sky || !sky->sector_count) return;
	for(i = 0; i < sky->sector_count; i++) {
		if(sky->polygons_misc) {
			free(sky->polygons_misc[i]);
			sky->polygons_misc[i] = NULL;
		}
		free(sky->polygons[i]);
		sky->polygons[i] = NULL;
		free(sky->colors[i]);
		sky->colors[i] = NULL;
		free(sky->vertices[i]);
		sky->vertices[i] = NULL;
	}
	free(sky->polygons_misc);
	sky->polygons_misc = NULL;
	free(sky->polygons);
	sky->polygons = NULL;
	free(sky->colors);
	sky->colors = NULL;
	free(sky->vertices);
	sky->vertices = NULL;
	free(sky->coordinates);
	sky->coordinates = NULL;
	free(sky->unknown);
	sky->unknown = NULL;
	
	sky->background.val = 0;
	sky->vcp.polygon_misc = 0;
	sky->vcp.polygon = 0;
	sky->vcp.color = 0;
	sky->vcp.vertex = 0;
	sky->sector_count = 0;
}

int sky_read(FILE *f, SpyroSky *sky, int game_type) {
	int success = 0;
	unsigned int i = 0;
	unsigned int j = 0;
	unsigned int k = 0;
	long base = -1;
	
	uint32_t sector_seek = 0;
	// TODO: err msgs
	if(!f) goto cleanup;
	if(!sky) goto cleanup;
	// TODO - gametype must match s1/s2/s3 enum.
	// note demo versions exist which may have 
	// different formats (but probably not)
	if(!game_type) goto cleanup; 


	base = ftell(f);
	if(base == -1) goto cleanup;
	// Make sure we free previous data if available before overwriting
	sky_free(sky);
	// fseek over the size of the sky
	if(fseek(f, sizeof(uint32_t), SEEK_CUR) == -1) goto cleanup;
	// read in the colors
	if(!fread(&sky->background.val, sizeof(sky->background.val), 1, f)) goto cleanup;
	if(!fread(&sky->sector_count, sizeof(sky->sector_count), 1, f)) goto cleanup;

	// Allocate header space data
	// sizeof(x) is pointer here
	sky->unknown = calloc(sky->sector_count, sizeof(SpyroModelSectorUnkData));
	if(!sky->unknown) goto cleanup;
	sky->coordinates = calloc(sky->sector_count, sizeof(SpyroSkyCoord));
	if(!sky->coordinates) goto cleanup;

	// vcp stored for each sector is different between games (s2/s3 has 
	if(game_type & S1_FULL) {
		sky->sector_vcp = calloc(sky->sector_count, sizeof(SecVCPCountS1));
	} else {
		sky->sector_vcp = calloc(sky->sector_count, sizeof(SecVCPCountS23));
	}
	if(!sky->sector_vcp) goto cleanup;

	// per sector - double ptrs
	sky->vertices = calloc(sky->sector_count, sizeof(sky->vertices));
	if(!sky->vertices) goto cleanup;
	sky->colors = calloc(sky->sector_count, sizeof(sky->colors));
	if(!sky->colors) goto cleanup;
	sky->polygons = calloc(sky->sector_count, sizeof(sky->polygons));
	if(!sky->polygons) goto cleanup;
	if(game_type & S2_FULL || game_type & S3_FULL) {
		sky->polygons_misc = calloc(sky->sector_count, sizeof(sky->polygons_misc));
		if(!sky->polygons_misc) goto cleanup;
	}

	for(i = 0; i < sky->sector_count; i++) {
		long sector = ftell(f);
		if(sector == -1) goto cleanup;

		if(!fread(&sector_seek, sizeof(uint32_t), 1, f)) goto cleanup;
		if(fseek(f, base + sector_seek, SEEK_SET) == -1) goto cleanup;
		if(!fread(&sky->unknown[i], sizeof(*sky->unknown), 1, f)) goto cleanup;
		if(!fread(&sky->coordinates[i].y, sizeof(uint16_t), 1, f)) goto cleanup;
		if(!fread(&sky->coordinates[i].z, sizeof(uint16_t), 1, f)) goto cleanup;
		if(game_type & S1_FULL) {
			// Header layout (courtesy of the SWV docs)
			// ------- 24 bytes -------
			// unk1 unk2 gygz vcgx pccc
			// g* = coordinate
			// vc = vertex count
			// pc = poly count 
			// cc = color count
			uint32_t ff = 0;
			SecVCPCountS1 *sector_vcp = sky->sector_vcp;
			if(!fread(&sector_vcp->vertex, sizeof(sector_vcp->vertex), 1, f)) goto cleanup;
			if(!fread(&sky->coordinates[i].x, sizeof(uint16_t), 1, f)) goto cleanup;
			if(!fread(&sector_vcp->polygon, sizeof(sector_vcp->polygon), 1, f)) goto cleanup;
			if(!fread(&sector_vcp->color, sizeof(sector_vcp->color), 1, f)) goto cleanup;

			// S1 only - make sure we are aligned. value read into FF must be 0xFFFFFFFF
			if(!fread(&ff, sizeof(uint32_t), 1, f)) goto cleanup;
			assert(ff == 0xFFFFFFFF);

			sky->vertices[i] = calloc(sector_vcp->vertex, sizeof(VertexSky));
			if(!sky->vertices[i]) goto cleanup;
			sky->colors[i] = calloc(sector_vcp->color, sizeof(Color));
			if(!sky->colors[i]) goto cleanup;
			sky->polygons[i] = calloc(sector_vcp->polygon, sizeof(PolygonSkyS1));
			if(!sky->polygons[i]) goto cleanup;

			// read data from the file
			if(!fread(sky->vertices[i], sector_vcp->vertex * sizeof(VertexSky), 1, f)) goto cleanup;
			if(!fread(sky->colors[i], sector_vcp->color * sizeof(Color), 1, f)) goto cleanup;
			if(!fread(sky->polygons[i], sector_vcp->polygon * sizeof(PolygonSkyS1), 1, f)) goto cleanup;
			// printf("Spyro 1: Sector %d has %d unique verts.\n", i + 1, sector_vcp->vertex);
			sky->vcp.vertex += sector_vcp->vertex;
			sky->vcp.color += sector_vcp->color;
			sky->vcp.polygon += sector_vcp->polygon;
		} else {
			// Header layout (courtesy of the SWV docs)
			//
			// ------- 24 bytes -------
			// unk1 unk2 gygz vcgx psms
			//
			// g* = coordinate
			// vc = vertex count
			// ps = all poly size
			// ms = all misc poly size
			SecVCPCountS23 *sector_vcp = sky->sector_vcp;
			uint32_t first_misc_count = 0;
			if(!fread(&sector_vcp->vertex, sizeof(sector_vcp->vertex), 1, f)) goto cleanup;
			if(!fread(&sector_vcp->color, sizeof(sector_vcp->color), 1, f)) goto cleanup;
			if(!fread(&sky->coordinates[i].x, sizeof(uint16_t), 1, f)) goto cleanup;
			if(!fread(&sector_vcp->polygon, sizeof(sector_vcp->polygon), 1, f)) goto cleanup;
			if(!fread(&sector_vcp->polygon_misc, sizeof(sector_vcp->polygon_misc), 1, f)) goto cleanup;
			// printf("Polygon misc size: %d\n", sector_vcp->polygon_misc);
			sky->vertices[i] = calloc(sector_vcp->vertex, sizeof(VertexSky));
			if(!sky->vertices[i]) goto cleanup;
			sky->colors[i] = calloc(sector_vcp->color, sizeof(Color));
			if(!sky->colors[i]) goto cleanup;
			sky->polygons[i] = calloc(sector_vcp->polygon, sizeof(PolygonSkyS2S3));
			if(!sky->polygons[i]) goto cleanup;

			if(!fread(sky->vertices[i], sector_vcp->vertex * sizeof(VertexSky), 1, f)) goto cleanup;
			if(!fread(sky->colors[i], sector_vcp->color * sizeof(Color), 1, f)) goto cleanup;
			if(!fread(sky->polygons[i], sector_vcp->polygon, 1, f)) goto cleanup;
			sky->polygons_misc[i] = calloc(sector_vcp->polygon_misc, sizeof(PolygonSkyS2S3Misc));
			if(!sky->polygons_misc[i]) goto cleanup;

			// sector_vcp->polygon_misc not necessarily 4/8/12/16, can be 2/6/10/14 - the 
			// last polygon will be cut off
			//
			//... at least I hope that's how that works...
			//
			// Would have to pull it into a loop of some sort if rendering is fucked up here
			// Also if this doesn't work then
			// Look at the full misc poly size in header (sector_vcp->polygon_misc) and maybe 
			// something with misc_count idk
			if(!fread(sky->polygons_misc[i], sector_vcp->polygon_misc, 1, f)) goto cleanup;
			// printf("Spyro 2/3: Sector %d has %d unique verts.\n", i + 1, sector_vcp->vertex);
			sky->vcp.vertex += sector_vcp->vertex;
			sky->vcp.color += sector_vcp->color;
			sky->vcp.polygon += sector_vcp->polygon;
			sky->vcp.polygon_misc += sector_vcp->polygon_misc;
		}
		if(fseek(f, sector + sizeof(uint32_t), SEEK_SET) == -1) goto cleanup;
		// fprintf(stdout, "Read sector %d of %d.\n", i + 1, sky->sector_count);
 	}
	sky->game_type = game_type;
	success = 1;
cleanup:
	if(!success) {
		sky_free(sky);
	}
	return success;
}
