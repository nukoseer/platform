#include "../shader/vertex_shader_shape.h"
#include "../shader/pixel_shader_shape.h"

#include "../shader/vertex_shader_sphere.h"
#include "../shader/pixel_shader_sphere.h"

#include "../shader/vertex_shader_sphere_grid.h"
#include "../shader/pixel_shader_sphere_grid.h"

#include "../shader/glow_mask_pixel_shader.h"
#include "../shader/glow_merge_pixel_shader.h"

#include "ray.h"
#include "country.h"

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
    f32 morph;
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

    graphics_target_t msaa_target;
    
    graphics_buffer_t vertex_buffer;
    graphics_buffer_t index_buffer;
    graphics_shader_t vertex_shader;
    graphics_shader_t pixel_shader;
        
    graphics_vertex_attribute_t attributes[4];
    u32 attribute_count;
    graphics_program_t program;
} earth_graphics_t;

typedef struct earth_t
{
    f32 prev_width;
    f32 prev_height;
    f32 width;
    f32 height;

    vec2 pressed_position;
    bool dragging;

    f32 morph; // NOTE: 1.0f is globe map, 0.0f flat map.
    f32 morph_direction;
    f32 yaw;
    f32 pitch;
    bool orbiting;
    bool reset;

    f32 glow_intensity;

    country_data_t country_data;
    u8 country_hover_index;
    u8 country_selected_index;
    vec2 country_card_position;

    earth_graphics_t earth_graphics;
    earth_sphere_graphics_t sphere_graphics;
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

static void earth_rotate(input_t* input, f32 delta_time, earth_t* earth)
{
    // NOTE: Rotation only works when the earth is in globe mode.
    if (earth->morph == 1.0f)
    {
        earth->yaw += 5.0f * input->mouse_delta.x * delta_time;
        earth->pitch += 5.0f * -input->mouse_delta.y * delta_time;
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
}

static inline void earth_morph(const input_t* input, f32 delta_time, earth_t* earth)
{
    earth->morph_direction = (input_is_key_released(input, KEY_T) ?
                              -earth->morph_direction : earth->morph_direction);
    earth->morph += 1.0f * delta_time * earth->morph_direction;
    earth->morph = clamp(0.0f, earth->morph, 1.0f);
}

static u8 earth_find_country_index_under_cursor(const input_t* input, const camera_t* camera, earth_t* earth)
{
    f32 width = earth->width;
    f32 height = earth->height;
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
    
    u8 index = country_cell_get_index(&earth->country_data.query, lon_lat.x, lon_lat.y);

    return index;
}

static void earth_find_country(input_t* input, const camera_t* camera, earth_t* earth)
{
    

}
    
static void earth_glow(const theme_t* theme, earth_t* earth)
{
    earth->glow_intensity = 2.0f;
    
    if (theme->dark_mode)
    {
        earth->glow_intensity = 0.3f;
    }
}

static void resize_earth_graphics(const graphics_t* graphics, f32 width, f32 height, earth_t* earth)
{
    earth_graphics_t* earth_graphics = &earth->earth_graphics;
    earth_glow_graphics_t* glow_graphics = &earth->glow_graphics;
    
    if (width != earth->prev_width || height != earth->prev_height)
    {
        if (earth->prev_width > 0 || earth->prev_height > 0)
        {
            graphics->delete_target(earth_graphics->msaa_target);
            graphics->delete_target(glow_graphics->merge_graphics.msaa_target);
            graphics->delete_target(glow_graphics->mask_graphics.msaa_target);

            graphics->delete_target(glow_graphics->x_target);
            graphics->delete_target(glow_graphics->y_target);

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

        earth->prev_width = width;
        earth->prev_height = height;
        earth->width = width;
        earth->height = height;

        earth_graphics->texture = graphics->create_texture_2d(&(graphics_texture_2d_desc_t)
        {
            .format = FORMAT_R16G16B16A16_FLOAT,
            .bind = BIND_SHADER_RESOURCE | BIND_RENDER_TARGET,
            .width = (u32)width,
            .height = (u32)height,
        }, 0, 0);
        
        earth_graphics->msaa_texture = graphics->create_texture_2d(&(graphics_texture_2d_desc_t)
        {
            .format = FORMAT_R16G16B16A16_FLOAT,
            .bind = BIND_SHADER_RESOURCE | BIND_RENDER_TARGET,
            .width = (u32)width,
            .height = (u32)height,
            .sample_count = 8,
        }, 0, 0);

        earth_graphics->msaa_depth_texture = graphics->create_texture_2d(&(graphics_texture_2d_desc_t)
        {
            .format = FORMAT_D24_UNORM_S8_UINT,
            .bind = BIND_DEPTH_STENCIL,
            .width = (u32)width,
            .height = (u32)height,
            .sample_count = 8,
        }, 0, 0);

        glow_graphics->mask_graphics.msaa_texture = graphics->create_texture_2d(&(graphics_texture_2d_desc_t)
        {
            .format = FORMAT_R16G16B16A16_FLOAT,
            .bind = BIND_SHADER_RESOURCE | BIND_RENDER_TARGET,
            .width = (u32)width,
            .height = (u32)height,
            .sample_count = 8,
        }, 0, 0);

        glow_graphics->mask_graphics.texture = graphics->create_texture_2d(&(graphics_texture_2d_desc_t)
        {
            .format = FORMAT_R16G16B16A16_FLOAT,
            .bind = BIND_SHADER_RESOURCE | BIND_RENDER_TARGET,
            .width = (u32)width,
            .height = (u32)height,
        }, 0, 0);

        glow_graphics->merge_graphics.msaa_texture = graphics->create_texture_2d(&(graphics_texture_2d_desc_t)
        {
            .format = FORMAT_R16G16B16A16_FLOAT,
            .bind = BIND_SHADER_RESOURCE | BIND_RENDER_TARGET,
            .width = (u32)width,
            .height = (u32)height,
            .sample_count = 8,
        }, 0, 0);

        glow_graphics->merge_graphics.texture = graphics->create_texture_2d(&(graphics_texture_2d_desc_t)
        {
            .format = FORMAT_R16G16B16A16_FLOAT,
            .bind = BIND_SHADER_RESOURCE | BIND_RENDER_TARGET,
            .width = (u32)width,
            .height = (u32)height,
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

        glow_graphics->x_target = graphics->create_target(&(graphics_target_desc_t)
        {
            .color = glow_graphics->x_texture,
        });

        glow_graphics->y_target = graphics->create_target(&(graphics_target_desc_t)
        {
            .color = glow_graphics->y_texture,
        });

        earth_graphics->msaa_target = graphics->create_target(&(graphics_target_desc_t)
        {
            .color = earth_graphics->msaa_texture,
            .depth = earth_graphics->msaa_depth_texture,
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

static void earth_init(memory_arena_t* arena, const graphics_t* graphics, const graphics_state_t* graphics_state, const io_t* io, earth_t* earth)
{
    earth->morph = 1.0f;
    earth->morph_direction = 1.0f;

    init_sphere_graphics(graphics, &earth->sphere_graphics);
    init_earth_graphics(graphics, &earth->earth_graphics);
    init_earth_glow_mask_graphics(graphics, &earth->earth_graphics, &earth->glow_graphics.mask_graphics);
    init_earth_glow_merge_graphics(graphics, graphics_state->blur_graphics.vertex_shader, &earth->glow_graphics.merge_graphics);

    country_data_init(arena, graphics, io, &earth->country_data);
    earth->country_hover_index = COUNTRY_INVALID_INDEX;
    earth->country_selected_index = COUNTRY_INVALID_INDEX;
    earth->country_card_position = v2(40.0f, 40.0f);
}

static void earth_ui_update(input_t* input, const theme_t* theme, earth_t* earth)
{
    ui_signal_t counry_card_signal = ui_widget_last_signal("country-card-group");

    if (counry_card_signal.held)
    {
        earth->country_card_position.x += input->mouse_delta.x;
        earth->country_card_position.y += input->mouse_delta.y;
    }

    country_name_t country_hover = country_get_name(earth->country_hover_index);
    country_name_t country_selected = country_get_name(earth->country_selected_index);

    if (country_hover.name)
    {
        ui_next_size(ui_content(1.0f), ui_content(1.0f));
        ui_next_padding(4.0f);
        ui_next_flags(UI_FLAG_BACKGROUND | UI_FLAG_FLOATING);
        ui_next_color(theme->bg_color);
        ui_next_font(theme->font_header, theme->font_header.pixel_size);
        ui_next_font_color(theme->fg_color);
        ui_next_border(1.0f, theme->border_color);
        {
            ui_widget_t * country_floating_widget = ui_widget_text("country-name-floating", country_hover.name);
            country_floating_widget->position.x = input->mouse_position.x;
            country_floating_widget->position.y = input->mouse_position.y - 32.0f;
        }
    }

    if  (country_selected.name)
    {
        ui_push_color(theme->bg_color);
        ui_push_font(theme->font_text, theme->font_text.pixel_size);
        ui_push_font_color(theme->fg_color);

        ui_next_size(ui_pixel(420.0f, 0.0f), ui_children(1.0f));
        ui_next_flags(UI_FLAG_BACKGROUND | UI_FLAG_FLOATING | UI_FLAG_CLICKABLE);
        ui_widget_group("country-card-group", earth->country_card_position.x, earth->country_card_position.y)
        {
            ui_next_size(ui_percent(1.0f, 0.0f), ui_children(1.0f));
            ui_next_border(1.0f, theme->border_color);
            ui_next_flags(UI_FLAG_BACKGROUND);
            ui_widget_named_column("country-card-column")
            {
                ui_next_anchored(UI_ANCHOR_TOP_RIGHT, UI_ANCHOR_CENTER_RIGHT, -8.0f, 16.0f);
                ui_next_color(theme->highlight_color);
                ui_next_size(ui_pixel(8.0f, 1.0f), ui_pixel(16.0f, 1.0f));
                ui_next_flags(UI_FLAG_BACKGROUND);
                ui_widget("country-active-mark");

                ui_next_size(ui_percent(1.0f, 0.0f), ui_children(1.0f));
                ui_next_padding(8.0f);
                ui_widget_named_column("country-data-column")
                {
                    ui_next_size(ui_content(1.0f), ui_content(1.0f));
                    ui_next_flags(UI_FLAG_BACKGROUND);
                    ui_next_text_alignment(ui_align_center());
                    ui_next_font(theme->font_header, theme->font_header.pixel_size);
                    ui_widget_text("country-data-header", "COUNTRY.DATA");

                    ui_next_size(ui_percent(1.0f, 0.0f), ui_children(1.0f));
                    ui_next_padding(8.0f);
                    ui_widget_named_column("country-selected-column")
                    {
                        ui_next_size(ui_percent(1.0f, 1.0f), ui_content(1.0f));
                        ui_next_font(theme->font_header, theme->font_header.pixel_size);
                        ui_next_text_alignment(ui_align_leading());
                        ui_widget_text("country-selected-name", country_selected.name ? country_selected.name : "Not Selected");

                        ui_widget_spacer(ui_pixel(8.0f, 1.0f));

                        ui_next_size(ui_percent(1.0f, 1.0f), ui_pixel(1.0f, 1.0f));
                        ui_next_color(theme->border_color); ui_next_flags(UI_FLAG_BACKGROUND);
                        ui_widget("country-name-separator");

                        ui_widget_spacer(ui_pixel(8.0f, 1.0f));
                    
                        ui_next_size(ui_percent(1.0f, 1.0f), ui_content(1.0f));
                        ui_next_text_alignment(ui_align_leading());
                        ui_widget_text("country-pop", "POP:  67.4M");
                    
                        ui_next_size(ui_percent(1.0f, 1.0f), ui_content(1.0f));
                        ui_next_text_alignment(ui_align_leading());
                        ui_widget_text("country-area", "AREA: 643K km2");
                    
                        ui_next_size(ui_percent(1.0f, 1.0f), ui_content(1.0f));
                        ui_next_text_alignment(ui_align_leading());
                        ui_widget_text("country-gov", "GOV:  REPUBLIC");
                    }
                }
            }
        }
        
        ui_pop_font_color();
        ui_pop_font();
        ui_pop_color();
    }
}

static void earth_update(input_t* input, const camera_t* camera, const theme_t* theme, f32 delta_time,
                         f32 width, f32 height, earth_t* earth)
{
    earth->width = width;
    earth->height = height;

    earth_morph(input, delta_time, earth);

    earth->country_hover_index = earth_find_country_index_under_cursor(input, camera, earth);
        
    if (input_is_mouse_pressed(input, KEY_MOUSE_LEFT))
    {
        input_set_owner(input, INPUT_OWNER_SCENE);
        earth->pressed_position = input->mouse_position;
    }

    if (input_is_owner(input, INPUT_OWNER_SCENE) && input_is_key_down(input, KEY_MOUSE_LEFT))
    {
        f32 total_moved = v2_length(v2_sub(input->mouse_position, earth->pressed_position));

        // NOTE: Arbitrary drag threshold.
        if (total_moved > 4.0f)
        {
            earth->dragging = true;
        }

        if (earth->dragging)
        {
            earth_rotate(input, delta_time, earth);
        }
    }
        
    if (input_is_owner(input, INPUT_OWNER_SCENE) && input_is_mouse_released(input, KEY_MOUSE_LEFT))
    {
        if (!earth->dragging)
        {
            earth->country_selected_index = earth->country_hover_index;
            input_consume_mouse_release(input, KEY_MOUSE_LEFT);
        }

        earth->dragging = false;
        input_set_owner(input, INPUT_OWNER_NULL);
    }

    earth_glow(theme, earth);
}

static void earth_render(const graphics_t* graphics, const graphics_state_t* graphics_state,
                         const camera_t* camera, const theme_t* theme, earth_t* earth)
{
    earth_graphics_t* earth_graphics = &earth->earth_graphics;
    earth_sphere_graphics_t* sphere_graphics = &earth->sphere_graphics;
    earth_glow_graphics_t* glow_graphics = &earth->glow_graphics;
    f32 width = earth->width;
    f32 height = earth->height;
    
    resize_earth_graphics(graphics, width, height, earth);
    update_earth_transform_buffer(camera, &earth->earth_graphics);

    graphics->begin_pass(earth_graphics->msaa_target, &(graphics_pass_desc_t)
    {
        .clear_color = true,
        .clear_rgba = v4v(srgb_to_linear(theme->bg_color.rgb), theme->bg_color.a),
        .clear_depth = true, .clear_depth_value = 1.0f
    });
    {
        sphere_graphics->parameters.color = v4v(srgb_to_linear(theme->bg_color.rgb), earth->morph);
        graphics->update_buffer(earth_graphics->transform_buffer, &earth_graphics->transform_parameters, 0, sizeof(earth_graphics->transform_parameters));
        graphics->update_buffer(sphere_graphics->parameter_buffer, &sphere_graphics->parameters, 0, sizeof(sphere_graphics->parameters));
        graphics->set_buffer(earth_graphics->transform_buffer, STAGE_VERTEX_SHADER | STAGE_PIXEL_SHADER, 0, 0, 0);
        graphics->set_buffer(sphere_graphics->parameter_buffer, STAGE_VERTEX_SHADER | STAGE_PIXEL_SHADER, 1, 0, 0);
        graphics->set_vertex_buffer(sphere_graphics->vertex_buffer, 0, sizeof(vec3), 0);
        graphics->set_index_buffer(sphere_graphics->index_buffer, 0);
        graphics->set_program(sphere_graphics->program);
        graphics->set_pipeline(graphics_state->depth_test_write_pipeline);
        graphics->draw_indexed(TOPOLOGY_TRIANGLE_LIST, sphere_graphics->index_buffer.size, 0, 0);
    }
    graphics->end_pass();

    f32 half_height = camera->position.z * tanf(camera->fov_y * (f32)DEG2RAD * 0.5f);
    f32 half_width = half_height * camera->aspect_ratio;
    f32 scale_x = half_width / (f32)PI;
    f32 scale_y = half_height / (0.5f * (f32)PI);

    earth_parameters_t* earth_parameters = &earth_graphics->parameters;
    earth_parameters->morph = earth->morph;
    earth_parameters->yaw = earth->yaw;
    earth_parameters->pitch = earth->pitch;
    earth_parameters->line_thickness = 1.0f;
    earth_parameters->scale = v2(scale_x, scale_y);
    earth_parameters->viewport_size = v2((f32)width, (f32)height);
    earth_parameters->lift = 0.0f;

    graphics->begin_pass(earth_graphics->msaa_target, &(graphics_pass_desc_t){ 0 });
    {
        earth_parameters->color = theme->dim_color;

        graphics->update_buffer(earth_graphics->transform_buffer, &earth_graphics->transform_parameters, 0, sizeof(earth_graphics->transform_parameters));
        graphics->update_buffer(earth_graphics->parameter_buffer, &earth_graphics->parameters, 0, sizeof(earth_graphics->parameters));
        graphics->set_buffer(earth_graphics->transform_buffer, STAGE_VERTEX_SHADER, 0, 0, 0);
        graphics->set_buffer(earth_graphics->parameter_buffer, STAGE_VERTEX_SHADER | STAGE_PIXEL_SHADER, 1, 0, 0);
        graphics->set_vertex_buffer(sphere_graphics->vertex_buffer, 0, sizeof(vec3), 0);
        graphics->set_index_buffer(sphere_graphics->index_buffer, 0);
        graphics->set_program(sphere_graphics->program_grid);
        graphics->set_pipeline(graphics_state->depth_test_pipeline);
        graphics->draw_indexed(TOPOLOGY_LINE_LIST, sphere_graphics->index_buffer.size, 0, 0);
    }
    graphics->end_pass();

    graphics->begin_pass(glow_graphics->mask_graphics.msaa_target, &(graphics_pass_desc_t) { .clear_color = true });
    {
        country_mesh_data_t* country_mesh_data = &earth->country_data.mesh;
        
        earth_parameters->line_thickness = 2.5f;
        
        graphics->update_buffer(earth_graphics->transform_buffer, &earth_graphics->transform_parameters, 0, sizeof(earth_graphics->transform_parameters));
        graphics->update_buffer(earth_graphics->parameter_buffer, &earth_graphics->parameters, 0, sizeof(earth_graphics->parameters));
        graphics->set_buffer(earth_graphics->transform_buffer, STAGE_VERTEX_SHADER, 0, 0, 0);
        graphics->set_buffer(earth_graphics->parameter_buffer, STAGE_VERTEX_SHADER, 1, 0, 0);
        graphics->set_vertex_buffer(country_mesh_data->vertex_buffer, 0, country_mesh_data->vertex_stride, 0);
        graphics->set_index_buffer(country_mesh_data->index_buffer, 0);

        glow_graphics->mask_graphics.parameters.glow_color = srgb_to_linear(theme->fg_color.rgb);
        glow_graphics->mask_graphics.parameters.glow = theme->dark_mode ? 1.0f : 0.0f;
        
        graphics->update_buffer(glow_graphics->mask_graphics.parameter_buffer, &glow_graphics->mask_graphics.parameters, 0, sizeof(glow_graphics->mask_graphics.parameters));
        graphics->set_buffer(glow_graphics->mask_graphics.parameter_buffer, STAGE_PIXEL_SHADER, 0, 0, 0);
        graphics->set_program(glow_graphics->mask_graphics.program);
        graphics->set_pipeline(graphics_state->depth_test_no_alpha_pipeline);
        graphics->draw_indexed(TOPOLOGY_TRIANGLE_LIST, country_mesh_data->index_count, 0, 0);
    }
    graphics->end_pass();

    graphics->resolve_texture(glow_graphics->mask_graphics.texture, glow_graphics->mask_graphics.msaa_texture);

    // NOTE: Horizontal blur.
    graphics->begin_pass(glow_graphics->x_target, &(graphics_pass_desc_t){ 0 });
    {
        const blur_graphics_t* blur_graphics = &graphics_state->blur_graphics;
        blur_parameters_t blur_parameters =
        {
            .inverse_viewport_size = { 1.0f / glow_graphics->x_texture.width, 1.0f / glow_graphics->x_texture.height },
            .direction = { 1.0f, 0.0f }   
        };

        graphics->update_buffer(blur_graphics->parameter_buffer, &blur_parameters, 0, sizeof(blur_parameters));
        graphics->set_buffer(blur_graphics->parameter_buffer, STAGE_PIXEL_SHADER, 0, 0, 0);
        graphics->set_program(blur_graphics->program);
        graphics->set_pipeline(graphics_state->depth_test_no_alpha_pipeline);
        graphics->set_samplers(STAGE_PIXEL_SHADER, &graphics_state->linear_sampler, 1, 0);
        graphics->set_srvs(STAGE_PIXEL_SHADER, &glow_graphics->mask_graphics.texture, 1, 0);
        graphics->draw(TOPOLOGY_TRIANGLE_LIST, 3, 0);
    }
    graphics->end_pass();

    // NOTE: Vertical blur.
    graphics->begin_pass(glow_graphics->y_target, &(graphics_pass_desc_t){ 0 });
    {
        const blur_graphics_t* blur_graphics = &graphics_state->blur_graphics;

        blur_parameters_t blur_parameters = (blur_parameters_t)
        {
            .inverse_viewport_size =
            {
                1.0f / glow_graphics->y_texture.width,
                1.0f / glow_graphics->y_texture.height
            },
            .direction = { 1.0f, 0.0f }
        };

        graphics->update_buffer(blur_graphics->parameter_buffer, &blur_parameters, 0, sizeof(blur_parameters));
        graphics->set_buffer(blur_graphics->parameter_buffer, STAGE_PIXEL_SHADER, 0, 0, 0);
        graphics->set_program(blur_graphics->program);
        graphics->set_pipeline(graphics_state->depth_test_no_alpha_pipeline);
        graphics->set_samplers(STAGE_PIXEL_SHADER, &graphics_state->linear_sampler, 1, 0);
        graphics->set_srvs(STAGE_PIXEL_SHADER, &glow_graphics->x_texture, 1, 0);
        graphics->draw(TOPOLOGY_TRIANGLE_LIST, 3, 0);
    }
    graphics->end_pass();

    graphics->resolve_texture(earth_graphics->texture, earth_graphics->msaa_texture);
    
    // NOTE: Glow merge pass.
    graphics->begin_pass(glow_graphics->merge_graphics.msaa_target, &(graphics_pass_desc_t){ 0 });
    {
        glow_graphics->merge_graphics.parameters.viewport_size = (vec2){ (f32)width, (f32)height };
        glow_graphics->merge_graphics.parameters.intensity = earth->glow_intensity;
        glow_graphics->merge_graphics.parameters.glow = theme->dark_mode ? 1.0f : 0.0f;
        
        graphics->update_buffer(glow_graphics->merge_graphics.parameter_buffer, &glow_graphics->merge_graphics.parameters, 0, sizeof(glow_graphics->merge_graphics.parameters));
        graphics->set_buffer(glow_graphics->merge_graphics.parameter_buffer, STAGE_PIXEL_SHADER, 0, 0, 0);
        graphics->set_program(glow_graphics->merge_graphics.program);
        graphics->set_pipeline(graphics_state->depth_test_no_alpha_pipeline);
        graphics_sampler_t samplers[] = { graphics_state->point_sampler, graphics_state->linear_sampler };
        graphics->set_samplers(STAGE_PIXEL_SHADER, samplers, 2, 0);
        graphics_texture_t srvs[] = { earth_graphics->texture, glow_graphics->y_texture };
        graphics->set_srvs(STAGE_PIXEL_SHADER, srvs, 2, 0);
        graphics->draw(TOPOLOGY_TRIANGLE_LIST, 3, 0);
    }
    graphics->end_pass();

    graphics->begin_pass(glow_graphics->merge_graphics.msaa_target, &(graphics_pass_desc_t){ 0 });
    {
        country_mesh_data_t* country_mesh_data = &earth->country_data.mesh;

        earth_parameters->color = v4v(srgb_to_linear(theme->fg_color.rgb), theme->fg_color.a);
        earth_parameters->line_thickness = 2.5f;
        
        graphics->update_buffer(earth_graphics->transform_buffer, &earth_graphics->transform_parameters, 0, sizeof(earth_graphics->transform_parameters));
        graphics->set_buffer(earth_graphics->transform_buffer, STAGE_VERTEX_SHADER, 0, 0, 0);
        graphics->update_buffer(earth_graphics->parameter_buffer, &earth_graphics->parameters, 0, sizeof(earth_graphics->parameters));
        graphics->set_buffer(earth_graphics->parameter_buffer, STAGE_VERTEX_SHADER | STAGE_PIXEL_SHADER, 1, 0, 0);
        graphics->set_vertex_buffer(country_mesh_data->vertex_buffer, 0, country_mesh_data->vertex_stride, 0);
        graphics->set_index_buffer(country_mesh_data->index_buffer, 0);
        graphics->set_program(earth_graphics->program);
        graphics->set_pipeline(graphics_state->depth_test_pipeline);
        graphics->draw_indexed(TOPOLOGY_TRIANGLE_LIST, country_mesh_data->index_count, 0, 0);

        if (country_is_valid_index(earth->country_hover_index))
        {
            earth_parameters->line_thickness = 4.0f;
            earth_parameters->lift = 0.01f;
            earth_parameters->color = v4v(srgb_to_linear(theme->highlight_color.rgb), theme->highlight_color.a);

            graphics->update_buffer(earth_graphics->parameter_buffer, &earth_graphics->parameters, 0, sizeof(earth_graphics->parameters));
            graphics->set_buffer(earth_graphics->parameter_buffer, STAGE_VERTEX_SHADER | STAGE_PIXEL_SHADER, 1, 0, 0);
            graphics->draw_indexed(TOPOLOGY_TRIANGLE_LIST,
                                   country_mesh_data->index_ranges[earth->country_hover_index].index_count,
                                   country_mesh_data->index_ranges[earth->country_hover_index].index_offset, 0);
        }
    }
    graphics->end_pass();

    graphics->resolve_texture(glow_graphics->merge_graphics.texture, glow_graphics->merge_graphics.msaa_texture);
    
    graphics->begin_pass(earth_graphics->msaa_target, &(graphics_pass_desc_t){ 0 });
    {
        graphics->set_program(graphics_state->composite_graphics.program);
        graphics->set_pipeline(graphics_state->depth_test_no_alpha_pipeline);
        graphics->set_samplers(STAGE_PIXEL_SHADER, &graphics_state->point_sampler, 1, 0);
        graphics->set_srvs(STAGE_PIXEL_SHADER, &glow_graphics->merge_graphics.texture, 1, 0);
        graphics->draw(TOPOLOGY_TRIANGLE_LIST, 3, 0);
    }
    graphics->end_pass();

    graphics->resolve_texture(earth_graphics->texture, earth_graphics->msaa_texture);

    graphics->begin_pass(graphics_state->offscreen_target_msaa, &(graphics_pass_desc_t)
    {
        .clear_color = true,
        .clear_depth = true, .clear_depth_value = 1.0f
    });
    {
        graphics->set_program(graphics_state->composite_graphics.program);
        graphics->set_pipeline(graphics_state->depth_test_no_alpha_pipeline);
        graphics->set_samplers(STAGE_PIXEL_SHADER, &graphics_state->point_sampler, 1, 0);
        graphics->set_srvs(STAGE_PIXEL_SHADER, &earth_graphics->texture, 1, 0);
        graphics->set_viewport(0.0f, 0.0f, width, height);
        graphics->draw(TOPOLOGY_TRIANGLE_LIST, 3, 0);
    }
    graphics->end_pass();
    
    graphics->resolve_texture(graphics_state->offscreen_scene, graphics_state->offscreen_scene_msaa);
}
