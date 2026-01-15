#pragma once

#include "utils.h"

#define COUNTRY_BORDER_MESH_FILE_MAGIC 0xBBDDBBDD
#define COUNTRY_BORDER_FILE_MAGIC 0xBBDDBBDE

typedef struct country_border_mesh_file_header_t
{
    u32 magic;
    u32 vertex_count;
    u32 index_count;
    u32 vertex_stride;
    u32 index_stride;
    u32 index_range_count;
} country_border_mesh_file_header_t;

typedef struct country_border_mesh_index_range_t
{
    u32 index_offset;
    u32 index_count;
} country_border_mesh_index_range_t;

typedef struct country_border_mesh_vertex_t
{
    vec3 prev;
    vec3 current;
    vec3 next;
    f32 side;
} country_border_mesh_vertex_t;

typedef struct country_border_file_header_t
{
    u32 magic;
    u32 point_count;
    u32 part_range_count;
    u32 country_range_count;
} country_border_file_header_t;

typedef struct country_border_point_t
{
    vec2 lonlat;
} country_border_point_t;

typedef struct country_border_part_range_t
{
    u32 point_index;
    u32 point_count;
} country_border_part_range_t;

typedef struct country_range_t
{
    u32 part_offset;
    u32 part_count;
} country_range_t;

typedef struct country_name_t
{
    const char* name;
    u32 length;
} country_name_t;
