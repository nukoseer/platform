#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include "../src/utils.h"
#include "../src/maths.h"
#include "../src/country_borders.h"

#define PRINT_PARTS          1
#define PRINT_PART_COUNTS    2
#define PRINT_POINTS         4
#define PRINT_MESH_POINTS    8
#define PRINT_INDICES        16
#define PRINT_MESH_INDICES   32
#define PRINT_INDEX_COUNTS   64

#pragma pack(push, 1)
typedef struct shape_file_header_t
{
    i32 file_code;     // big endian
    i32 unused[5];
    i32 file_length;   // big endian
    i32 version;       // little endian
    u32 shape_type;   // little endian
    f64 x_min, y_min, x_max, y_max;
    f64 z_min, z_max;
    f64 m_min, m_max;
} shape_file_header_t;

typedef struct shape_record_header_t
{
    u32 record_number;  // big endian
    u32 content_length; // big endian
} shape_record_header_t;

typedef struct shape_content_t
{
    u32 shape_type;
    f64 x_min, y_min, x_max, y_max;
    u32 part_count;
    u32 point_count; 
} shape_content_t;

typedef struct point_t
{
    f64 x, y;
} point_t;
#pragma pack(pop)

static u32 print_format = 0;

static inline u32 swap_endianness(u32 value)
{
    u32 result = ((value & 0xFF000000) >> 24 |
                  (value & 0x00FF0000) >> 8 |
                  (value & 0x0000FF00) << 8 |
                  (value & 0x000000FF) << 24);

    return result;
}

static inline void usage(const char* name)
{
    fprintf(stderr, "Usage: %s --shapefile <shp file> --parts | --indices | --points\n", name);
}

size_t parse_shape_file(void* header, u8* memory, u8* shape_file_buffer, size_t shape_file_size, u32 print_format)
{
    uint8_t* initial_record_offset = shape_file_buffer + sizeof(shape_file_header_t);
    size_t memory_offset = 0;
    u32 offset = 0;
    u32 part_index = 0;
    u32 part_offset = 0;
    u32 index_count = 0;
    
    while (offset < shape_file_size - sizeof(shape_file_header_t))
    {
        shape_record_header_t* shape_record_header = (shape_record_header_t*)(initial_record_offset + offset);
        uint8_t* shape_content_ptr = (uint8_t*)(initial_record_offset + offset + sizeof(shape_record_header_t));
        shape_content_t* shape_content = (shape_content_t*)(shape_content_ptr);
        u32 content_length_byte = swap_endianness(shape_record_header->content_length) * 2;

        // NOTE: Polygon
        if (shape_content->shape_type == 5)
        {
            u8* part_ptr = shape_content_ptr + sizeof(shape_content_t);
            u32* parts = (u32*)part_ptr;

            if ((print_format & PRINT_POINTS) || (print_format & PRINT_PARTS) ||
                (print_format & PRINT_PART_COUNTS) || (print_format & PRINT_MESH_INDICES) ||
                (print_format & PRINT_INDEX_COUNTS))
            {
                if (print_format & PRINT_PARTS)
                {
                    for (u32 i = 0; i < shape_content->part_count; ++i)
                    {
                        u32 start = parts[i] + part_index;
                        u32 end = ((i + 1 < shape_content->part_count) ? parts[i + 1] : shape_content->point_count) + part_index;
                        border_part_range_t border_part_range =
                        {
                            .point_index = start,
                            .point_count = end - start,
                        };

                        memcpy(memory + memory_offset, &border_part_range, sizeof(border_part_range_t));
                        memory_offset += sizeof(border_part_range_t);

                        ((border_file_header_t*)header)->part_range_count += 1;
                    }
                }

                if (print_format & PRINT_PART_COUNTS)
                {
                    country_range_t country_range =
                    {
                        .part_offset = part_offset,
                        .part_count = shape_content->part_count,
                    };
                    memcpy(memory + memory_offset, &country_range, sizeof(country_range_t));
                    memory_offset += sizeof(country_range_t);

                    ((border_file_header_t*)header)->country_range_count += 1;
                }

                u32 temp_index_count = 0;
                
                if (print_format & PRINT_MESH_INDICES || print_format & PRINT_INDEX_COUNTS)
                {
                    for (u32 i = 0; i < shape_content->part_count; ++i)
                    {
                        u32 start = parts[i] + part_index;
                        u32 end = ((i + 1 < shape_content->part_count) ? parts[i + 1] : shape_content->point_count) + part_index;

                        for (u32 j = start; j + 1 < end; ++j)
                        {
                            u32 num_points = end - start;

                            for (u32 k = 0; k < num_points; ++k)
                            {
                                u32 current = start + k;
                                u32 next = start + ((k + 1) % num_points);
                                
                                u32 current_left = 2 * current + 0;
                                u32 current_right = 2 * current + 1;
                                u32 next_left = 2 * next + 0;
                                u32 next_right = 2 * next + 1;

                                if (print_format & PRINT_MESH_INDICES)
                                {
                                    *(u16*)(memory + memory_offset) = current_left;
                                    memory_offset += sizeof(u16);
                                    *(u16*)(memory + memory_offset) = current_right;
                                    memory_offset += sizeof(u16);
                                    *(u16*)(memory + memory_offset) = next_left;
                                    memory_offset += sizeof(u16);
                                    *(u16*)(memory + memory_offset) = next_left;
                                    memory_offset += sizeof(u16);
                                    *(u16*)(memory + memory_offset) = current_right;
                                    memory_offset += sizeof(u16);
                                    *(u16*)(memory + memory_offset) = next_right;
                                    memory_offset += sizeof(u16);

                                    ((border_mesh_file_header_t*)header)->index_count += 6;
                                }
                                ++temp_index_count;
                            }
                        }
                    }
                }

                if (print_format & PRINT_INDEX_COUNTS)
                {
                    printf("{ %u, %u }, ", index_count * 6, temp_index_count * 6);
                }
                index_count += temp_index_count;

                part_offset += shape_content->part_count;
                part_index += shape_content->point_count;
            }
            
            if (print_format & PRINT_MESH_POINTS || print_format & PRINT_POINTS)
            {
                uint8_t* point_ptr = part_ptr + (shape_content->part_count * sizeof(u32));
                point_t* points = (point_t*)(point_ptr);

                for (u32 i = 0; i < shape_content->point_count; ++i)
                {
                    u32 p_i = (i == 0) ? shape_content->point_count - 1 : i - 1;
                    u32 c_i = i;
                    u32 n_i = (i + 1) % shape_content->point_count;

                    f32 p_lon = (f32)points[p_i].x;
                    f32 p_lat = (f32)points[p_i].y;
                    f32 n_lon = (f32)points[n_i].x;
                    f32 n_lat = (f32)points[n_i].y;
                    
                    f32 lon = (f32)points[c_i].x;
                    f32 lat = (f32)points[c_i].y;

                    f32 lon_rad = lon * (f32)DEG2RAD;
                    f32 lat_rad = lat * (f32)DEG2RAD;

                    f32 x = cosf(lat_rad) * sinf(lon_rad) /* radius */;
                    f32 y = sinf(lat_rad) /* radius */;
                    f32 z = cosf(lat_rad) * cosf(lon_rad) /* radius */;
                    
                    if (print_format & PRINT_MESH_POINTS)
                    {
                        border_mesh_vertex_t v1 =
                        {
                            .prev = v3(p_lon, p_lat, 0.0f),
                            .current = v3(lon, lat, 0.0f),
                            .next = v3(n_lon, n_lat, 0.0f),
                            .side = -1.0f,
                        };

                        border_mesh_vertex_t v2 =
                        {
                            .prev = v3(p_lon, p_lat, 0.0f),
                            .current = v3(lon, lat, 0.0f),
                            .next = v3(n_lon, n_lat, 0.0f),
                            .side = 1.0f,
                        };

                        memcpy(memory + memory_offset, &v1, sizeof(border_mesh_vertex_t));
                        memory_offset += sizeof(border_mesh_vertex_t);
                        memcpy(memory + memory_offset, &v2, sizeof(border_mesh_vertex_t));
                        memory_offset += sizeof(border_mesh_vertex_t);

                        ((border_mesh_file_header_t*)header)->vertex_count += 2;
                    }
                    else if (print_format & PRINT_POINTS)
                    {
                        border_point_t border_point = { lon, lat };

                        memcpy(memory + memory_offset, &border_point, sizeof(border_point_t));
                        memory_offset += sizeof(border_point_t);

                        ((border_file_header_t*)header)->point_count += 1;
                    }
                }
            }
        }
        else
        {
            assert(!"Unknown type!");
        }

        offset += sizeof(shape_record_header_t) + content_length_byte;
    }

    return memory_offset;
}

int main(int argc, char* argv[])
{
    if (argc < 4)
    {
        usage(argv[0]);
        return 1;
    }

    const char* shape_file_opt = argv[1];
    const char* shape_file_name = argv[2];
    
    if (strcmp("--shapefile", shape_file_opt))
    {
        fprintf(stderr, "Expected '--shapefile' found: '%s'\n", shape_file_opt);
        return 1;
    }
    for (i32 i = 3; i < argc; ++i)
    {
        char* arg = argv[i];

        if (!strcmp("--parts", arg))
        {
            print_format |= PRINT_PARTS;
        }
        if (!strcmp("--indices", arg))
        {
            print_format |= PRINT_INDICES;
        }
        if (!strcmp("--points", arg))
        {
            print_format |= PRINT_POINTS;
        }
    }
    
    if (!print_format)
    {
        fprintf(stderr, "Expected '--parts' | '--points'\n");
        return 1;
    }

    FILE* shape_file = fopen(shape_file_name, "rb");

    if (!shape_file)
    {
        fprintf(stderr, "Could not open the shape file: %s\n", shape_file_name);
        return 1;
    }

    fseek(shape_file, 0, SEEK_END);
    size_t shape_file_size = ftell(shape_file);
    fseek(shape_file, 0, SEEK_SET);

    uint8_t* shape_file_buffer = malloc(shape_file_size + 1);

    if (!shape_file_buffer)
    {
        fprintf(stderr, "Could not allocate the shape file buffer\n");
        return 1;
    }

    shape_file_buffer[shape_file_size] = '\0';

    size_t read_size = fread(shape_file_buffer, shape_file_size, 1, shape_file);

    if (read_size != 1)
    {
        fprintf(stderr, "Could not read the shape file: %s\n", shape_file_name);
        fclose(shape_file);
        free(shape_file_buffer);
        return 1;
    }

    size_t mesh_total_size = GIBIBYTES(2);
    size_t mesh_offset = 0;
    u8* mesh_memory = calloc(1, mesh_total_size);

    border_mesh_file_header_t* border_mesh_file_header = (border_mesh_file_header_t*)mesh_memory;
    border_mesh_file_header->magic = BORDER_MESH_FILE_MAGIC;
    border_mesh_file_header->vertex_count = 0;
    border_mesh_file_header->index_count = 0;
    border_mesh_file_header->vertex_stride = sizeof(border_mesh_vertex_t);
    border_mesh_file_header->index_stride = sizeof(u16);

    mesh_offset += sizeof(border_mesh_file_header_t);

    size_t total_size = GIBIBYTES(2);
    size_t offset = 0;
    u8* memory = calloc(1, total_size);

    border_file_header_t* border_file_header = (border_file_header_t*)memory;
    border_file_header->magic = BORDER_FILE_MAGIC;
    border_file_header->point_count = 0;
    border_file_header->part_range_count = 0;
    border_file_header->country_range_count = 0;

    offset += sizeof(border_file_header_t);

    // NOTE: Border information CPU representation.
    if (print_format & PRINT_POINTS)
    {
        offset += parse_shape_file(border_file_header, memory + offset, shape_file_buffer, shape_file_size, PRINT_POINTS);
    }

    if (print_format & PRINT_PARTS)
    {
        // printf("static u32 global_shape_parts[][2] =\n{\n");
        offset += parse_shape_file(border_file_header, memory + offset, shape_file_buffer, shape_file_size, PRINT_PARTS);
        // printf("\n};\n\n");
    }

    if (print_format & PRINT_PARTS)
    {
        // printf("static u16 global_shape_part_offset_counts[][2] =\n{\n");
        offset += parse_shape_file(border_file_header, memory + offset, shape_file_buffer, shape_file_size, PRINT_PART_COUNTS);
        // printf("\n};\n\n");
    }

    // NOTE: Border mesh GPU representation.
    if (print_format & PRINT_POINTS)
    {
        // printf("typedef struct border_vertex_t border_vertex_t;\n");
        // printf("""struct border_vertex_t\n"""
        //     """{\n"""
        //     """    vec3 prev;\n"""
        //     """    vec3 current;\n"""
        //     """    vec3 next;\n"""
        //     """    f32 side;\n"""
        //     """} static global_shape_points[] =\n"""
        //     """{\n"""
        // );
        mesh_offset += parse_shape_file(border_mesh_file_header, mesh_memory + mesh_offset, shape_file_buffer, shape_file_size, PRINT_MESH_POINTS);

        // printf("\n};\n\n");
    }
    
    if (print_format & PRINT_INDICES)
    {
        // printf("static u16 global_shape_indices[] =\n{\n");
        mesh_offset += parse_shape_file(border_mesh_file_header, mesh_memory + mesh_offset, shape_file_buffer, shape_file_size, PRINT_MESH_INDICES);
        // printf("\n};\n\n");

        // printf("static u32 global_shape_offset_index_counts[][2] =\n{\n");
        // parse_shape_file(shape_file_buffer, shape_file_size, PRINT_INDEX_COUNTS);
        // printf("\n};\n\n");
    }

    const char* border_mesh_file_name = "border_mesh.bin";
    FILE* border_mesh_file = fopen(border_mesh_file_name, "wb");

    fprintf(stderr, "Written border mesh - vertex count: %u, index count: %u\n", border_mesh_file_header->vertex_count, border_mesh_file_header->index_count);
    
    const char* border_file_name = "border.bin";
    FILE* border_file = fopen(border_file_name, "wb");

    fprintf(stderr, "Written border - point count: %u, part range count: %u, country range count: %u\n",
            border_file_header->point_count,
            border_file_header->part_range_count,
            border_file_header->country_range_count);

    fwrite(mesh_memory, mesh_offset, 1, border_mesh_file);
    fwrite(memory, offset, 1, border_file);

    return 0;
}
