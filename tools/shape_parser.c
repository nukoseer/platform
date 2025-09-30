#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include "../src/utils.h"
#include "../src/maths.h"

#define PRINT_PARTS    1
#define PRINT_INDICES  2
#define PRINT_POINTS   4
#define PRINT_VECTORS  8

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
    fprintf(stderr, "Usage: %s --shapefile <shp file> --parts | --indices | --points | --vectors\n", name);
}

void parse_shape_file(uint8_t* shape_file_buffer, size_t shape_file_size, u32 print_format)
{
    uint8_t* initial_record_offset = shape_file_buffer + sizeof(shape_file_header_t);
    u32 offset = 0;
    u32 part_index = 0;
    u32 polygon_count = 0;
    
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

            if ((print_format & PRINT_PARTS) || (print_format & PRINT_INDICES))
            {
                if (print_format & PRINT_PARTS)
                {
                    for (u32 i = 0; i < shape_content->part_count; ++i)
                    {
                        printf("%u, ", parts[i] + part_index);
                    }
                }

                if (print_format & PRINT_INDICES)
                {
                    for (u32 i = 0; i < shape_content->part_count; ++i)
                    {
                        u32 start = parts[i] + part_index;
                        u32 end = ((i + 1 < shape_content->part_count) ? parts[i + 1] : shape_content->point_count) + part_index;

                        for (u32 j = start; j + 1 < end; ++j)
                        {
                            printf("%u, %u, ", j, j + 1);
                        }

                        printf("%u, %u, ", end - 1, start);
                    }
                }
                
                part_index += shape_content->point_count;
            }
            
            if ((print_format & PRINT_POINTS) || (print_format & PRINT_VECTORS))
            {
                uint8_t* point_ptr = part_ptr + (shape_content->part_count * sizeof(u32));
                point_t* points = (point_t*)(point_ptr);

                
                for (u32 i = 0; i < shape_content->point_count; ++i)
                {
                    if (print_format & PRINT_VECTORS)
                    {
                        f32 lon = (f32)points[i].x;
                        f32 lat = (f32)points[i].y;

                        f32 lat_rad = lat * (f32)DEG2RAD;
                        f32 lon_rad = lon * (f32)DEG2RAD;

                        // f32 x = cosf(lat_rad) * cosf(lon_rad) /* * radius */;
                        // f32 y = cosf(lat_rad) * sinf(lon_rad) /* * radius */;
                        // f32 z = sinf(lat_rad) /* * radius */;

                        f32 x = cosf(lat_rad) * cosf(lon_rad) /* * radius */;
                        f32 y = sinf(lat_rad) /* * radius */;
                        f32 z = cosf(lat_rad) * sinf(lon_rad) /* * radius */;

                        printf("{ %+3.12ff, %+3.12ff, %+3.12ff }, ", x, y, z);
                    }
                    else
                    {
                        printf("%+3.12ff, %+3.12ff, ", (f32)points[i].x, (f32)points[i].y);
                    }
                }
            }

            ++polygon_count;
        }
        else
        {
            assert(!"Unknown type!");
        }

        offset += sizeof(shape_record_header_t) + content_length_byte;
    }
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
        if (!strcmp("--vectors", arg))
        {
            print_format |= PRINT_VECTORS;
        }
    }
    
    if (!print_format)
    {
        fprintf(stderr, "Expected '--parts' | '--points' | --'vectors'\n");
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

    if (print_format & PRINT_INDICES)
    {
        fseek(shape_file, 0, SEEK_SET);
        printf("static u16 global_globe_part_indices[] =\n{\n");
        parse_shape_file(shape_file_buffer, shape_file_size, PRINT_INDICES);
        printf("\n};\n\n");
    }

    if (print_format & PRINT_VECTORS)
    {
        printf("static vec3 global_globe_vectors[] =\n{\n");
        parse_shape_file(shape_file_buffer, shape_file_size, PRINT_VECTORS);
        printf("\n};\n\n");
    }

    free(shape_file_buffer);
    fclose(shape_file);

    return 0;
}
