#include "../shader/vertex_shader_shape.h"
#include "../shader/pixel_shader_shape.h"

#include "../shader/vertex_shader_sphere.h"
#include "../shader/pixel_shader_sphere.h"

#include "../shader/vertex_shader_sphere_grid.h"
#include "../shader/pixel_shader_sphere_grid.h"

#include "../shader/glow_mask_pixel_shader.h"

#include "ray.h"
#include "country.h"
#include "blur.h"

typedef struct earth_glow_mask_parameters_t
{
    vec3 glow_color;
    f32 glow;
} earth_glow_mask_parameters_t;

typedef struct earth_glow_mask_graphics_t
{
    earth_glow_mask_parameters_t parameters;
    graphics_buffer_t parameter_buffer;
    
    graphics_texture_t texture;
    graphics_texture_t msaa_texture;
    graphics_target_t msaa_target;
    
    graphics_shader_t vertex_shader;
    graphics_shader_t pixel_shader;
    graphics_program_t program;
} earth_glow_mask_graphics_t;

typedef struct earth_glow_merge_parameters_t
{
    vec2 viewport_size;
    f32 intensity;
    f32 glow;
} earth_glow_merge_parameters_t;

typedef struct earth_glow_merge_graphics_t
{
    earth_glow_merge_parameters_t parameters;
    graphics_buffer_t parameter_buffer;

    graphics_texture_t texture;
    graphics_texture_t msaa_texture;
    graphics_target_t msaa_target;
    
    graphics_shader_t vertex_shader;
    graphics_shader_t pixel_shader;
    graphics_program_t program;
} earth_glow_merge_graphics_t;

typedef struct earth_glow_graphics_t
{
    graphics_texture_t x_texture;
    graphics_texture_t y_texture;
    graphics_target_t x_target;
    graphics_target_t y_target;
    earth_glow_mask_graphics_t mask_graphics;
    earth_glow_merge_graphics_t merge_graphics;
} earth_glow_graphics_t;

typedef struct earth_sphere_parameters_t
{
    vec4 color;
} earth_sphere_parameters_t;

typedef struct earth_sphere_graphics_t
{
    earth_sphere_parameters_t parameters;
    graphics_buffer_t parameter_buffer;

    graphics_buffer_t vertex_buffer;
    graphics_buffer_t index_buffer;
    graphics_shader_t vertex_shader;
    graphics_shader_t pixel_shader;
    graphics_shader_t vertex_shader_grid;
    graphics_shader_t pixel_shader_grid;
    graphics_program_t program;
    graphics_program_t program_grid;
} earth_sphere_graphics_t;

typedef struct earth_parameters_t
{
    f32 shape;
    f32 yaw;
    f32 pitch;
    f32 line_thickness;
    vec2 scale;
    vec2 viewport_size;
    vec4 color;
    f32 lift;
    f32 _pad[3];
} earth_parameters_t;

typedef struct earth_transform_parameters_t
{
    mat4x4 world;
    mat4x4 view;
    mat4x4 projection;
} earth_transform_parameters_t;

typedef struct earth_graphics_t
{
    graphics_buffer_t transform_buffer;
    earth_transform_parameters_t transform_parameters;
    
    earth_parameters_t parameters;
    graphics_buffer_t parameter_buffer;

    graphics_texture_t texture;
    graphics_texture_t msaa_texture;
    graphics_texture_t msaa_depth_texture;

    // graphics_target_t target;
    graphics_target_t msaa_target;
    graphics_target_t msaa_depth_target;
    
    graphics_buffer_t vertex_buffer;
    graphics_buffer_t index_buffer;
    graphics_shader_t vertex_shader;
    graphics_shader_t pixel_shader;
        
    graphics_vertex_attribute_t attributes[4];
    u32 attribute_count;
    graphics_program_t program;

    graphics_pipeline_t depth_test_write_pipeline;
} earth_graphics_t;

typedef struct earth_t
{
    u32 prev_width;
    u32 prev_height;
    u32 width;
    u32 height;
    
    f32 morph; // NOTE: 1.0f is globe map, 0.0f flat map.
    f32 morph_direction;
    f32 yaw;
    f32 pitch;
    bool orbiting;
    bool reset;

    country_data_t country_data;
    u8 country_index;

    earth_graphics_t earth_graphics;
    earth_sphere_graphics_t sphere_graphics;
    blur_graphics_t blur_graphics;
    earth_glow_graphics_t glow_graphics;
} earth_t;

static void init_earth_glow_mask_graphics(const graphics_t* graphics, earth_graphics_t* earth_graphics, earth_glow_mask_graphics_t* glow_mask_graphics)
{
    glow_mask_graphics->parameter_buffer = graphics->create_buffer(&(graphics_buffer_desc_t)
    {
        .size = 16,
        .usage = USAGE_DYNAMIC,
        .bind = BIND_CONSTANT_BUFFER,
    });

    glow_mask_graphics->pixel_shader = graphics->create_shader(&(graphics_shader_desc_t)
    {
        .bytecode = glow_mask_pshader,
        .bytecode_size = sizeof(glow_mask_pshader),
        .stage = STAGE_PIXEL_SHADER,
    });

    glow_mask_graphics->program = graphics->create_program(&(graphics_program_desc_t)
    {
        .vertex_shader = earth_graphics->vertex_shader,
        .pixel_shader = glow_mask_graphics->pixel_shader,
        .attributes = earth_graphics->attributes,
        .attribute_count = earth_graphics->attribute_count,
    });
}

static void init_earth_glow_merge_graphics(const graphics_t* graphics, graphics_shader_t blur_vertex_shader, earth_glow_merge_graphics_t* glow_merge_graphics)
{
    glow_merge_graphics->parameter_buffer = graphics->create_buffer(&(graphics_buffer_desc_t)
    {
        .size = sizeof(earth_glow_merge_parameters_t),
        .usage = USAGE_DYNAMIC,
        .bind = BIND_CONSTANT_BUFFER,
    });

    glow_merge_graphics->vertex_shader = blur_vertex_shader;

    glow_merge_graphics->pixel_shader = graphics->create_shader(&(graphics_shader_desc_t)
    {
        .bytecode = glow_merge_pshader,
        .bytecode_size = sizeof(glow_merge_pshader),
        .stage = STAGE_PIXEL_SHADER,
    });

    glow_merge_graphics->program = graphics->create_program(&(graphics_program_desc_t)
    {
        .vertex_shader = glow_merge_graphics->vertex_shader,
        .pixel_shader = glow_merge_graphics->pixel_shader,
        // NOTE: No input layout.
        .attributes = 0,
        .attribute_count = 0,
    });
}

static void init_sphere_graphics(const graphics_t* graphics, earth_sphere_graphics_t* sphere_graphics)
{
    sphere_graphics->parameter_buffer = graphics->create_buffer(&(graphics_buffer_desc_t)
    {
        .size = sizeof(earth_sphere_parameters_t),
        .usage = USAGE_DYNAMIC,
        .bind = BIND_CONSTANT_BUFFER,
    });

    sphere_graphics->vertex_buffer = graphics->create_buffer(&(graphics_buffer_desc_t)
    {
        .data = global_sphere_vertices,
        .size = sizeof(global_sphere_vertices),
        .usage = USAGE_IMMUTABLE,
        .bind = BIND_VERTEX_BUFFER,
    });

    sphere_graphics->index_buffer = graphics->create_buffer(&(graphics_buffer_desc_t)
    {
        .data = global_sphere_indices,
        .size = array_count(global_sphere_indices),
        .usage = USAGE_IMMUTABLE,
        .bind = BIND_INDEX_BUFFER,
        .index_format = FORMAT_R16_UINT,
    });

    sphere_graphics->vertex_shader = graphics->create_shader(&(graphics_shader_desc_t)
    {
        .bytecode = vshader_sphere,
        .bytecode_size = sizeof(vshader_sphere),
        .stage = STAGE_VERTEX_SHADER,
    });

    sphere_graphics->pixel_shader = graphics->create_shader(&(graphics_shader_desc_t)
    {
        .bytecode = pshader_sphere,
        .bytecode_size = sizeof(pshader_sphere),
        .stage = STAGE_PIXEL_SHADER,
    });

    sphere_graphics->program = graphics->create_program(&(graphics_program_desc_t)
    {
        .vertex_shader = sphere_graphics->vertex_shader,
        .pixel_shader = sphere_graphics->pixel_shader,
        .attributes = (graphics_vertex_attribute_t[])
        {
            { "POSITION", FORMAT_R32G32B32_FLOAT, 0, 0, 0, 0, 0 },
        },
        .attribute_count = 1,
    });

    sphere_graphics->vertex_shader_grid = graphics->create_shader(&(graphics_shader_desc_t)
    {
        .bytecode = vshader_sphere_grid,
        .bytecode_size = sizeof(vshader_sphere_grid),
        .stage = STAGE_VERTEX_SHADER,
    });

    sphere_graphics->pixel_shader_grid = graphics->create_shader(&(graphics_shader_desc_t)
    {
        .bytecode = pshader_sphere_grid,
        .bytecode_size = sizeof(pshader_sphere_grid),
        .stage = STAGE_PIXEL_SHADER,
    });

    sphere_graphics->program_grid = graphics->create_program(&(graphics_program_desc_t)
    {
        .vertex_shader = sphere_graphics->vertex_shader_grid,
        .pixel_shader = sphere_graphics->pixel_shader_grid,
        .attributes = (graphics_vertex_attribute_t[])
        {
            { "POSITION", FORMAT_R32G32B32_FLOAT, 0, 0, 0, 0, 0 },
        },
        .attribute_count = 1,
    });
}

static void init_earth_graphics(const graphics_t* graphics, earth_graphics_t* earth_graphics)
{
    earth_graphics->transform_buffer = graphics->create_buffer(&(graphics_buffer_desc_t)
    {
        .size = sizeof(earth_transform_parameters_t),
        .usage = USAGE_DYNAMIC,
        .bind = BIND_CONSTANT_BUFFER,
    });
    
    earth_graphics->parameter_buffer = graphics->create_buffer(&(graphics_buffer_desc_t)
    {
        .size = sizeof(earth_parameters_t),
        .usage = USAGE_DYNAMIC,
        .bind = BIND_CONSTANT_BUFFER,
    });

    earth_graphics->vertex_shader = graphics->create_shader(&(graphics_shader_desc_t)
    {
        .bytecode = vshader_shape,
        .bytecode_size = sizeof(vshader_shape),
        .stage = STAGE_VERTEX_SHADER,
    });

    earth_graphics->pixel_shader = graphics->create_shader(&(graphics_shader_desc_t)
    {
        .bytecode = pshader_shape,
        .bytecode_size = sizeof(pshader_shape),
        .stage = STAGE_PIXEL_SHADER,
    });

    earth_graphics->attributes[earth_graphics->attribute_count++] = (graphics_vertex_attribute_t){ "TEXCOORD", FORMAT_R32G32B32_FLOAT, offsetof(country_border_mesh_vertex_t, prev), 0, 0, 0, 0 },
    earth_graphics->attributes[earth_graphics->attribute_count++] = (graphics_vertex_attribute_t){ "POSITION", FORMAT_R32G32B32_FLOAT, offsetof(country_border_mesh_vertex_t, current), 0, 0, 0, 0 },
    earth_graphics->attributes[earth_graphics->attribute_count++] = (graphics_vertex_attribute_t){ "TEXCOORD", FORMAT_R32G32B32_FLOAT, offsetof(country_border_mesh_vertex_t, next), 1, 0, 0, 0 },
    earth_graphics->attributes[earth_graphics->attribute_count++] = (graphics_vertex_attribute_t){ "TEXCOORD", FORMAT_R32_FLOAT,       offsetof(country_border_mesh_vertex_t, side), 2, 0, 0, 0 },
    
    earth_graphics->program = graphics->create_program(&(graphics_program_desc_t)
    {
        .vertex_shader = earth_graphics->vertex_shader,
        .pixel_shader = earth_graphics->pixel_shader,
        .attributes = earth_graphics->attributes,
        .attribute_count = 4,
    });

    earth_graphics->depth_test_write_pipeline = graphics->create_pipeline(&(graphics_pipeline_desc_t)
    {
        .cull = true,
        .depth_test = true,
        .depth_write = true,
        .blend = BLEND_ALPHA,
    });
}
 
static vec2 earth_xy_to_lon_lat(f32 x, f32 y, f32 width, f32 height)
{
    vec2 result = { 0 };
    vec2 normalized_position = v2(x / width, y / height);
    // NOTE: Map to lon(-180, 180), lat(-90, 90).
    result.x = (2.0f * normalized_position.x - 1.0f) * 180.0f;
    result.y = (1.0f - 2.0f * normalized_position.y) * 90.0f;

    return result;
}

static ray_result_t earth_ray_to_lon_lat(ray_t ray, vec2* lon_lat, earth_t* earth)
{
    ray_result_t ray_result = ray_hit_unit_sphere(ray);
    
    if (ray_result.hit)
    {
        vec3 hit_position = v3_normalize(v3_add(ray.origin, v3_mulf(ray.direction, ray_result.t)));
        f32 yaw = earth->yaw;
        f32 pitch = earth->pitch;

        // NOTE: Undo pitch angle.
        f32 pitch_rad = -pitch * (f32)DEG2RAD;
        vec3 position = (vec3)
        {
            .x = hit_position.x,
            .y = cosf(pitch_rad) * hit_position.y + sinf(pitch_rad) * hit_position.z,
            .z = -sinf(pitch_rad) * hit_position.y + cosf(pitch_rad) * hit_position.z,
        };
        
        f32 lon = atan2f(position.x, position.z);
        f32 lat = asinf(position.y);

        // NOTE: Undo yaw angle.
        lon -= yaw * (f32)DEG2RAD;
        
        while (lon > (f32)PI)  lon -= 2.0f * (f32)PI;
        while (lon < (f32)-PI) lon += 2.0f * (f32)PI;

        lon_lat->x = lon / (f32)DEG2RAD;
        lon_lat->y = lat / (f32)DEG2RAD;
    }

    return ray_result;
}

static void earth_rotation(const input_t* input, f32 delta_time, earth_t* earth)
{
    if (input_is_key_released(input, KEY_O))
    {
        earth->orbiting = !earth->orbiting;
    }
    
    if (input_is_key_down(input, KEY_MOUSE_LEFT))
    {
        earth->yaw += 3.0f * input->mouse_delta.x * delta_time;
        earth->pitch += 3.0f * -input->mouse_delta.y * delta_time;
    }
    else if (earth->orbiting)
    {
        earth->yaw += 10.0f * delta_time;
    }

    earth->pitch = clamp(-90.0f, earth->pitch, 90.0f);

    if (input_is_key_released(input, KEY_R))
    {
        earth->reset = !earth->reset;
    }

    f32 earth_reset_speed = 200.0f * delta_time;
    
    if (earth->reset)
    {
        if (earth->yaw != 0.0f)
        {
            while (earth->yaw > 360.0f)
            {
                earth->yaw -= 360.0f;
            }

            while (earth->yaw < -360.0f)
            {
                earth->yaw += 360.0f;
            }

            if (fabs(earth->yaw) < earth_reset_speed)
            {
                earth->yaw = 0.0f;
            }
            else
            {
                earth->yaw += earth->yaw > 0.0f ? -earth_reset_speed : earth_reset_speed;
            }
        }
        if (earth->pitch != 0.0f)
        {
            if (fabs(earth->pitch) < earth_reset_speed)
            {
                earth->pitch = 0.0f;
            }
            else
            {
                earth->pitch += earth->pitch > 0.0f ? -earth_reset_speed : earth_reset_speed;
            }
        }

        if (earth->yaw == 0.0f && earth->pitch == 0.0f)
        {
            earth->reset = false;
        }
    }    
}

static void earth_morph(const input_t* input, f32 delta_time, earth_t* earth)
{
    earth->morph_direction = (input_is_key_released(input, KEY_T) ?
                              -earth->morph_direction : earth->morph_direction);
    earth->morph += 1.0f * delta_time * earth->morph_direction;
    earth->morph = clamp(0.0f, earth->morph, 1.0f);
}

static void earth_find_country_index_under_cursor(const input_t* input, const camera_t* camera, f32 width, f32 height, earth_t* earth)
{
    vec2 mouse_position = input->mouse_position;
    vec2 lon_lat = { 0 };
    
    if (earth->morph == 0.0f)
    {
        lon_lat = earth_xy_to_lon_lat(mouse_position.x, mouse_position.y, width, height);
    }
    else if (earth->morph == 1.0f)
    {
        ray_t mouse_ray = ray_world(mouse_position.x, mouse_position.y, width, height,
                                    camera->fov_y, camera->position, camera->view);

        vec2 ray_lon_lat = { 0 };    
        ray_result_t ray_result = earth_ray_to_lon_lat(mouse_ray, &ray_lon_lat, earth);

        if (ray_result.hit)
        {
            lon_lat = ray_lon_lat;
        }
    }

    earth->country_index = country_cell_get_index(&earth->country_data.query, lon_lat.x, lon_lat.y);
}

static void earth_update(const input_t* input, const camera_t* camera, f32 delta_time,
                         u32 width, u32 height, earth_t* earth)
{
    // NOTE: Rotation only works when the earth is in globe mode.
    if (earth->morph == 1.0f)
    {
        earth_rotation(input, delta_time, earth);
    }
    
    earth_morph(input, delta_time, earth);
    earth_find_country_index_under_cursor(input, camera, (f32)width, (f32)height, earth);
}

static void resize_earth_graphics(const graphics_t* graphics, u32 width, u32 height, earth_t* earth)
{
    earth_graphics_t* earth_graphics = &earth->earth_graphics;
    earth_glow_graphics_t* glow_graphics = &earth->glow_graphics;
    
    if ((earth->prev_width != 0 && width > 0) && (earth->prev_height != 0 && height > 0))
    {
        // graphics->delete_target(earth_graphics->target);
        graphics->delete_target(earth_graphics->msaa_target);
        graphics->delete_target(earth_graphics->msaa_depth_target);

        graphics->delete_target(glow_graphics->merge_graphics.msaa_target);
        graphics->delete_target(glow_graphics->mask_graphics.msaa_target);
        
        graphics->delete_texture_2d(earth_graphics->texture);
        graphics->delete_texture_2d(earth_graphics->msaa_texture);
        graphics->delete_texture_2d(earth_graphics->msaa_depth_texture);

        graphics->delete_texture_2d(glow_graphics->x_texture);
        graphics->delete_texture_2d(glow_graphics->y_texture);
        graphics->delete_texture_2d(glow_graphics->merge_graphics.msaa_texture);
        graphics->delete_texture_2d(glow_graphics->merge_graphics.texture);
        graphics->delete_texture_2d(glow_graphics->mask_graphics.msaa_texture);
        graphics->delete_texture_2d(glow_graphics->mask_graphics.texture);
    }
    
    if (width != earth->prev_width || height != earth->prev_height)
    {
        earth->prev_width = width;
        earth->prev_height = height;
        earth->width = width;
        earth->height = height;

        earth_graphics->texture = graphics->create_texture_2d(&(graphics_texture_2d_desc_t)
        {
            .format = FORMAT_R16G16B16A16_FLOAT,
            .bind = BIND_SHADER_RESOURCE | BIND_RENDER_TARGET,
            .width = width,
            .height = height,
        }, 0, 0);
        
        earth_graphics->msaa_texture = graphics->create_texture_2d(&(graphics_texture_2d_desc_t)
        {
            .format = FORMAT_R16G16B16A16_FLOAT,
            .bind = BIND_SHADER_RESOURCE | BIND_RENDER_TARGET,
            .width = width,
            .height = height,
            .sample_count = 8,
        }, 0, 0);

        earth_graphics->msaa_depth_texture = graphics->create_texture_2d(&(graphics_texture_2d_desc_t)
        {
            .format = FORMAT_D24_UNORM_S8_UINT,
            .bind = BIND_DEPTH_STENCIL,
            .width = width,
            .height = height,
            .sample_count = 8,
        }, 0, 0);

        glow_graphics->mask_graphics.msaa_texture = graphics->create_texture_2d(&(graphics_texture_2d_desc_t)
        {
            .format = FORMAT_R16G16B16A16_FLOAT,
            .bind = BIND_SHADER_RESOURCE | BIND_RENDER_TARGET,
            .width = width,
            .height = height,
            .sample_count = 8,
        }, 0, 0);

        glow_graphics->mask_graphics.texture = graphics->create_texture_2d(&(graphics_texture_2d_desc_t)
        {
            .format = FORMAT_R16G16B16A16_FLOAT,
            .bind = BIND_SHADER_RESOURCE | BIND_RENDER_TARGET,
            .width = width,
            .height = height,
        }, 0, 0);

        glow_graphics->merge_graphics.msaa_texture = graphics->create_texture_2d(&(graphics_texture_2d_desc_t)
        {
            .format = FORMAT_R16G16B16A16_FLOAT,
            .bind = BIND_SHADER_RESOURCE | BIND_RENDER_TARGET,
            .width = width,
            .height = height,
            .sample_count = 8,
        }, 0, 0);

        glow_graphics->merge_graphics.texture = graphics->create_texture_2d(&(graphics_texture_2d_desc_t)
        {
            .format = FORMAT_R16G16B16A16_FLOAT,
            .bind = BIND_SHADER_RESOURCE | BIND_RENDER_TARGET,
            .width = width,
            .height = height,
        }, 0, 0);

        glow_graphics->x_texture = graphics->create_texture_2d(&(graphics_texture_2d_desc_t)
        {
            .format = FORMAT_R16G16B16A16_FLOAT,
            .bind = BIND_SHADER_RESOURCE | BIND_RENDER_TARGET,
            .width = (u32)(width * 0.5f),
            .height = (u32)(height * 0.5f),
        }, 0, 0);

        glow_graphics->y_texture = graphics->create_texture_2d(&(graphics_texture_2d_desc_t)
        {
            .format = FORMAT_R16G16B16A16_FLOAT,
            .bind = BIND_SHADER_RESOURCE | BIND_RENDER_TARGET,
            .width = (u32)(width * 0.5f),
            .height = (u32)(height * 0.5f),
        }, 0, 0);

        earth_graphics->msaa_target = graphics->create_target(&(graphics_target_desc_t)
        {
            .color = earth_graphics->msaa_texture,
            .depth = earth_graphics->msaa_depth_texture,
        });

        glow_graphics->x_target = graphics->create_target(&(graphics_target_desc_t)
        {
            .color = glow_graphics->x_texture,
        });

        glow_graphics->y_target = graphics->create_target(&(graphics_target_desc_t)
        {
            .color = glow_graphics->y_texture,
        });

        glow_graphics->merge_graphics.msaa_target = graphics->create_target(&(graphics_target_desc_t)
        {
            .color = glow_graphics->merge_graphics.msaa_texture,
            .depth = earth_graphics->msaa_depth_texture,
        });

        glow_graphics->mask_graphics.msaa_target = graphics->create_target(&(graphics_target_desc_t)
        {
            .color = glow_graphics->mask_graphics.msaa_texture,
            .depth = earth_graphics->msaa_depth_texture,
        });
    }
}

static void update_earth_transform_buffer(const camera_t* camera, earth_graphics_t* earth_graphics)
{
    earth_graphics->transform_parameters.world = m4x4d(1.0f);
    earth_graphics->transform_parameters.view = camera->view;
    earth_graphics->transform_parameters.projection = camera->projection;
}

static void earth_render(const graphics_t* graphics, const camera_t* camera, const theme_t* theme,
                         u32 width, u32 height, earth_t* earth)
{
    earth_graphics_t* earth_graphics = &earth->earth_graphics;

    resize_earth_graphics(graphics, width, height, earth);
    update_earth_transform_buffer(camera, &earth->earth_graphics);

    graphics->begin_pass(earth_graphics->msaa_target, &(graphics_pass_desc_t)
    {
        .clear_color = true,
        .clear_rgba = v4v(srgb_to_linear(theme->bg_color.rgb), theme->bg_color.a),
        .clear_depth = true, .clear_depth_value = 1.0f
    });
    {
        earth_sphere_graphics_t* sphere_graphics = &earth->sphere_graphics;
        sphere_graphics->parameters.color = v4v(srgb_to_linear(theme->bg_color.rgb), earth->morph);
        graphics->update_buffer(earth_graphics->transform_buffer, &earth_graphics->transform_parameters, 0, sizeof(earth_graphics->transform_parameters));
        graphics->update_buffer(sphere_graphics->parameter_buffer, &sphere_graphics->parameters, 0, sizeof(sphere_graphics->parameters));
        graphics->set_buffer(earth_graphics->transform_buffer, STAGE_VERTEX_SHADER | STAGE_PIXEL_SHADER, 0, 0, 0);
        graphics->set_buffer(sphere_graphics->parameter_buffer, STAGE_VERTEX_SHADER | STAGE_PIXEL_SHADER, 1, 0, 0);
        graphics->set_vertex_buffer(sphere_graphics->vertex_buffer, 0, sizeof(vec3), 0);
        graphics->set_index_buffer(sphere_graphics->index_buffer, 0);
        graphics->set_program(sphere_graphics->program);
        graphics->set_pipeline(earth_graphics->depth_test_write_pipeline);
        graphics->draw_indexed(TOPOLOGY_TRIANGLE_LIST, sphere_graphics->index_buffer.size, 0, 0);
    }
    graphics->end_pass();
}

static earth_t* earth_init(memory_arena_t* arena, const graphics_t* graphics, const io_t* io)
{
    earth_t* earth = ma_push_struct_zero(arena, earth_t);

    init_sphere_graphics(graphics, &earth->sphere_graphics);
    init_earth_graphics(graphics, &earth->earth_graphics);
    blur_graphics_create(graphics, &earth->blur_graphics);
    init_earth_glow_mask_graphics(graphics, &earth->earth_graphics, &earth->glow_graphics.mask_graphics);
    init_earth_glow_merge_graphics(graphics, earth->blur_graphics.vertex_shader, &earth->glow_graphics.merge_graphics);

    country_data_init(arena, graphics, io, &earth->country_data);

    return earth;
}
