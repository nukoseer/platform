#pragma once

#include "utils.h"

#define BORDER_MESH_FILE_MAGIC 0xBBDDBBDD
#define BORDER_FILE_MAGIC 0xBBDDBBDE

typedef struct border_mesh_file_header_t
{
    u32 magic;
    u32 vertex_count;
    u32 index_count;
    u32 vertex_stride;
    u32 index_stride;
} border_mesh_file_header_t;

typedef struct border_mesh_vertex_t
{
    vec3 prev;
    vec3 current;
    vec3 next;
    f32 side;
} border_mesh_vertex_t;

typedef struct border_file_header_t
{
    u32 magic;
    u32 point_count;
    u32 part_range_count;
    u32 country_range_count;
} border_file_header_t;

typedef struct border_point_t
{
    vec2 lonlat;
} border_point_t;

typedef struct border_part_range_t
{
    u32 point_index;
    u32 point_count;
} border_part_range_t;

typedef struct country_range_t
{
    u32 part_offset;
    u32 part_count;
} country_range_t;

#ifdef BORDER_MESH_IMPLEMENTATION



#endif
