#include "country.h"

#include "shape_meta_data.inl"

typedef struct country_mesh_data_t
{
    graphics_buffer_t vertex_buffer;
    graphics_buffer_t index_buffer;
    u32 vertex_count;
    u32 index_count;
    u32 vertex_stride;
    country_border_mesh_index_range_t* index_ranges;
    u32 index_range_count;
} country_mesh_data_t;

typedef struct country_query_data_t
{
    country_border_point_t* border_points;
    country_border_part_range_t* border_part_ranges;
    country_range_t* ranges;

    u32 border_point_count;
    u32 border_part_range_count;
    u32 range_count;

    u8* cells;
} country_query_data_t;

typedef struct country_data_t
{
    country_mesh_data_t mesh;
    country_query_data_t query;
} country_data_t;

#define COUNTRY_CELL_X_COUNT    360
#define COUNTRY_CELL_Y_COUNT    180
#define COUNTRY_CELL_SLOT_COUNT 10

#define COUNTRY_INVALID_INDEX 0xFF

// NOTE: [-180, 180] -> [0, 360)
static inline u32 country_cell_lon_to_x(f32 lon)
{
    f32 u = (lon + 180.0f) / 360.0f;
    u32 x = (u32)floorf(u * 360.0f);
    x = clamp_u32(0, x, COUNTRY_CELL_X_COUNT - 1);

    return x;
}

// NOTE: [-90, 90] -> [0, 180)
static inline u32 country_cell_lat_to_y(f32 lat)
{
    f32 v = (lat + 90.0f) / 180.0f;
    u32 y = (u32)floor(v * 180.0f);
    y = clamp_u32(0, y, COUNTRY_CELL_Y_COUNT - 1);

    return y;
}

static inline u32 country_cell_index(u32 x, u32 y)
{
    u32 result = (y * COUNTRY_CELL_X_COUNT + x) * COUNTRY_CELL_SLOT_COUNT;

    return result;
}

static inline void country_cell_insert_slot(u8* cells, u8 country_index, u32 x, u32 y)
{
    u32 index = country_cell_index(x, y);

    for (u32 slot_index = 0; slot_index < COUNTRY_CELL_SLOT_COUNT; ++slot_index)
    {
        u8* country_id = cells + index + slot_index;

        if (*country_id == country_index + 1)
        {
            break;
        }
        else if (*country_id != 0)
        {
            continue;
        }
        else
        {
            *country_id = country_index + 1;
            break;
        }
    }
}

static void country_cell_insert(u8* cells, u8 country_index, f32 lon_min, f32 lon_max, f32 lat_min, f32 lat_max)
{
    u32 x_min = country_cell_lon_to_x(lon_min);
    u32 x_max = country_cell_lon_to_x(lon_max);
    u32 y_min = country_cell_lat_to_y(lat_min);
    u32 y_max = country_cell_lat_to_y(lat_max);

    for (u32 y = y_min; y < y_max; ++y)
    {
        for (u32 x = x_min; x < x_max; ++x)
        {
            country_cell_insert_slot(cells, country_index, x, y);
        }
    }
}

static bool country_is_inside(const country_border_point_t* points, u32 point_count, f32 lon, f32 lat)
{
    bool inside = false;

    for (u32 i = 0, j = point_count - 1; i < point_count; j = i++)
    {
        f32 xi = points[i].lonlat.x;
        f32 yi = points[i].lonlat.y;
        f32 xj = points[j].lonlat.x;
        f32 yj = points[j].lonlat.y;

        bool hit = ((yi > lat) != (yj > lat)) && (lon < (xj - xi) * (lat - yi) / (yj - yi) + xi);

        inside ^= hit;
    }

    return inside;
}

static u8 country_cell_get_id(country_query_data_t* query_data, f32 lon, f32 lat)
{
    u32 result = 0;
    u32 x = country_cell_lon_to_x(lon);
    u32 y = country_cell_lat_to_y(lat);
    u32 index = country_cell_index(x, y);

    for (u32 slot_index = 0; slot_index < COUNTRY_CELL_SLOT_COUNT; ++slot_index)
    {
        u32 country_id = query_data->cells[index + slot_index];
        
        if (country_id != 0)
        {
            u32 country_index = country_id - 1;
            country_range_t country_range = query_data->ranges[country_index];

            u32 part_offset = country_range.part_offset;
            u32 part_count = country_range.part_count;
            
            for (u32 part_index = 0; part_index < part_count; ++part_index)
            {
                country_border_part_range_t part_range = query_data->border_part_ranges[part_index + part_offset];

                if (country_is_inside(query_data->border_points + part_range.point_index, part_range.point_count, lon, lat))
                {
                    result = country_id;
                    break;
                }
            }
        }
        else
        {
            break;
        }
    }

    return result;
}

static inline u8 country_cell_get_index(country_query_data_t* query_data, f32 lon, f32 lat)
{
    u8 country_id = country_cell_get_id(query_data, lon, lat);
    u8 country_index = COUNTRY_INVALID_INDEX;
    
    if (country_id)
    {
        country_index = country_id - 1;
    }

    return country_index;
}

static inline bool country_is_valid_index(u8 country_index)
{
    bool result = country_index != COUNTRY_INVALID_INDEX;

    return result;
}

static void init_country_cells(country_query_data_t* query_data)
{
    u32 total_part_count = query_data->border_part_range_count;
    
    f32* part_outlines = calloc(1, total_part_count * sizeof(f32) * 4);
    u32 part_outline_count = 0;

    for (u8 country_index = 0; country_index < query_data->range_count; ++country_index)
    {
        country_range_t country_range = query_data->ranges[country_index];

        u32 part_offset = country_range.part_offset;
        u32 part_count = country_range.part_count;

        for (u32 part_index = 0; part_index < part_count; ++part_index)
        {
            country_border_part_range_t part_range = query_data->border_part_ranges[part_index + part_offset];

            f32 lon_min = 1000.0f;
            f32 lon_max = -1000.0f;
            f32 lat_min = 1000.0f;
            f32 lat_max = -1000.0f;

            for (u32 k = 0; k < part_range.point_count; ++k)
            {
                vec2 point = query_data->border_points[k + part_range.point_index].lonlat;

                if (point.x < lon_min)
                {
                    lon_min = point.x;
                }

                if (point.y < lat_min)
                {
                    lat_min = point.y;
                }

                if (point.x > lon_max)
                {
                    lon_max = point.x;
                }

                if (point.y > lat_max)
                {
                    lat_max = point.y;
                }
            }

            part_outlines[part_outline_count + 0] = lon_min;
            part_outlines[part_outline_count + 1] = lon_max;
            part_outlines[part_outline_count + 2] = lat_min;
            part_outlines[part_outline_count + 3] = lat_max;
            part_outline_count += 4;
        }
    }

    assert(total_part_count * 4 == part_outline_count);

    query_data->cells = calloc(1, COUNTRY_CELL_X_COUNT * COUNTRY_CELL_Y_COUNT * COUNTRY_CELL_SLOT_COUNT);
    f32* outlines = part_outlines;

    for (u8 country_index = 0; country_index < query_data->range_count; ++country_index)
    {
        country_range_t country_range = query_data->ranges[country_index];
        
        for (u32 part_index = 0; part_index < country_range.part_count; ++part_index)
        {
            f32 lon_min = outlines[0];
            f32 lon_max = outlines[1];
            f32 lat_min = outlines[2];
            f32 lat_max = outlines[3];
            outlines += 4;
            
            country_cell_insert(query_data->cells, country_index, lon_min, lon_max, lat_min, lat_max);
        }
    }

    free(part_outlines);
}

static inline country_name_t country_get_name(u8 country_index)
{
    country_name_t name = { 0 };

    if (country_is_valid_index(country_index) && country_index < array_count(global_shape_country_names))
    {
        name = global_shape_country_names[country_index];
    }

    return name;
}

static void init_country_mesh_data(graphics_t* graphics, io_t* io, country_mesh_data_t* mesh_data)
{
    io_file_read_result_t country_border_mesh_file_result = io->read_file("..\\tools\\build\\country_border_mesh.bin");
    assert(country_border_mesh_file_result.data && country_border_mesh_file_result.size > 0);

    country_border_mesh_file_header_t* country_border_mesh_header = (country_border_mesh_file_header_t*)country_border_mesh_file_result.data;
    assert(country_border_mesh_header->magic == COUNTRY_BORDER_MESH_FILE_MAGIC);

    country_border_mesh_vertex_t* country_border_mesh_vertices = (country_border_mesh_vertex_t*)((u8*)country_border_mesh_file_result.data + sizeof(country_border_mesh_file_header_t));
    u16* country_border_mesh_indices = (u16*)((u8*)country_border_mesh_file_result.data + sizeof(country_border_mesh_file_header_t) + sizeof(country_border_mesh_vertex_t) * country_border_mesh_header->vertex_count);
    country_border_mesh_index_range_t* country_border_mesh_index_ranges = (country_border_mesh_index_range_t*)((u8*)country_border_mesh_file_result.data + sizeof(country_border_mesh_file_header_t) +
                                                                                                               sizeof(country_border_mesh_vertex_t) * country_border_mesh_header->vertex_count +
                                                                                                               sizeof(u16) * country_border_mesh_header->index_count);
    
    u32 country_border_mesh_vertex_count = country_border_mesh_header->vertex_count;
    u32 country_border_mesh_index_count = country_border_mesh_header->index_count;
    u32 country_border_mesh_vertex_stride = country_border_mesh_header->vertex_stride;
    u32 country_border_mesh_index_range_count = country_border_mesh_header->index_range_count;

    fprintf(stderr, "[COUNTRY] Border mesh - vertex count: %u, index count: %u index range count: %u\n",
            country_border_mesh_vertex_count, country_border_mesh_index_count, country_border_mesh_index_range_count);

    mesh_data->vertex_buffer = graphics->create_buffer(&(graphics_buffer_desc_t)
    {
        .data = country_border_mesh_vertices,
        .size = sizeof(country_border_mesh_vertex_t) * country_border_mesh_vertex_count,
        .usage = USAGE_IMMUTABLE,
        .bind = BIND_VERTEX_BUFFER,
    });

    mesh_data->index_buffer = graphics->create_buffer(&(graphics_buffer_desc_t)
    {
        .data = country_border_mesh_indices,
        .size = country_border_mesh_index_count,
        .usage = USAGE_IMMUTABLE,
        .bind = BIND_INDEX_BUFFER,
        .index_format = FORMAT_R16_UINT,
    });

    mesh_data->index_ranges = calloc(country_border_mesh_index_range_count, sizeof(country_border_mesh_index_range_t));
    memcpy(mesh_data->index_ranges, country_border_mesh_index_ranges, sizeof(country_border_mesh_index_range_t) * country_border_mesh_index_range_count);

    mesh_data->vertex_count = country_border_mesh_vertex_count;
    mesh_data->index_count = country_border_mesh_index_count;
    mesh_data->vertex_stride = country_border_mesh_vertex_stride;
    mesh_data->index_range_count = country_border_mesh_index_range_count;

    io->release_file_memory(country_border_mesh_file_result.data);
}

static void init_country_query_data(io_t* io, country_query_data_t* query_data)
{
    io_file_read_result_t country_border_file_result = io->read_file("..\\tools\\build\\country_border_query.bin");
    assert(country_border_file_result.data && country_border_file_result.size > 0);
    
    country_border_file_header_t* country_border_header = (country_border_file_header_t*)country_border_file_result.data;
    assert(country_border_header->magic == COUNTRY_BORDER_FILE_MAGIC);
    
    query_data->border_points = (country_border_point_t*)((u8*)country_border_file_result.data + sizeof(country_border_file_header_t));
    query_data->border_part_ranges = (country_border_part_range_t*)((u8*)country_border_file_result.data + sizeof(country_border_file_header_t) +
                                                                    sizeof(country_border_point_t) * country_border_header->point_count);
    query_data->ranges = (country_range_t*)((u8*)country_border_file_result.data + sizeof(country_border_file_header_t) +
                                            sizeof(country_border_point_t) * country_border_header->point_count +
                                            sizeof(country_border_part_range_t) * country_border_header->part_range_count);
    
    query_data->border_point_count = country_border_header->point_count;
    query_data->border_part_range_count = country_border_header->part_range_count;
    query_data->range_count = country_border_header->country_range_count;
    
    fprintf(stderr, "[COUNTRY] Border query - point count: %u, part_range_count: %u, country_range_count: %u\n",
            query_data->border_point_count, query_data->border_part_range_count, query_data->range_count);
}

static void init_country_data(graphics_t* graphics, io_t* io, country_data_t* country_data)
{
    init_country_mesh_data(graphics, io, &country_data->mesh);
    init_country_query_data(io, &country_data->query);
    init_country_cells(&country_data->query);
}
