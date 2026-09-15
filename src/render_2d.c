#include "../shader/rect_vertex_shader.h"
#include "../shader/rect_pixel_shader.h"

typedef struct rect_vertex_data_t
{
    vec2 position;
    vec2 uv;
    vec2 size;
    vec4 color;
    vec4 clip;
    f32 border_thickness;
    f32 has_texture;
} rect_vertex_data_t;

typedef struct render_2d_t
{
    graphics_t* graphics;
    font_t* font;
    bool init;
    
    rect_vertex_data_t* vertex_data;
    u32 vertex_data_count;
    u32 max_vertex_data_count;
    
    graphics_texture_t current_texture;
    graphics_buffer_t parameter_buffer;
    graphics_buffer_t vertex_buffer;
    graphics_program_t program;
    graphics_sampler_t sampler;
    graphics_pipeline_t pipeline;
} render_2d_t;

static render_2d_t global_render_2d;

static void render_2d_begin(graphics_t* graphics, font_t* font, memory_arena_t* memory_arena, usize max_size)
{
    global_render_2d.graphics = graphics;
    global_render_2d.font = font;
    global_render_2d.max_vertex_data_count = (u32)(max_size / sizeof(rect_vertex_data_t));
    global_render_2d.vertex_data = (rect_vertex_data_t*)ma_push_size(memory_arena, global_render_2d.max_vertex_data_count * sizeof(rect_vertex_data_t));

    if (!global_render_2d.init)
    {
        graphics_buffer_t vertex_buffer = graphics->create_buffer(&(graphics_buffer_desc_t)
        {
            // TODO: This assumes max_size will never change.
            .size = (u32)max_size,
            .usage = USAGE_DYNAMIC,
            .bind = BIND_VERTEX_BUFFER,
        });

        graphics_buffer_t parameter_buffer = graphics->create_buffer(&(graphics_buffer_desc_t)
        {
            .size = 16,
            .usage = USAGE_DYNAMIC,
            .bind = BIND_CONSTANT_BUFFER,
        });
    
        graphics_shader_t vertex_shader = graphics->create_shader(&(graphics_shader_desc_t)
        {
            .bytecode = rect_vshader,
            .bytecode_size = sizeof(rect_vshader),
            .stage = STAGE_VERTEX_SHADER,
        });

        graphics_shader_t pixel_shader = graphics->create_shader(&(graphics_shader_desc_t)
        {
            .bytecode = rect_pshader,
            .bytecode_size = sizeof(rect_pshader),
            .stage = STAGE_PIXEL_SHADER,
        });

        graphics_program_t program = graphics->create_program(&(graphics_program_desc_t)
        {
            .vertex_shader = vertex_shader,
            .pixel_shader = pixel_shader,
            .attributes = (graphics_vertex_attribute_t[])
            {
                { "POSITION", FORMAT_R32G32_FLOAT, offsetof(rect_vertex_data_t, position), 0, 0, 0, 0 },
                { "TEXCOORD", FORMAT_R32G32_FLOAT, offsetof(rect_vertex_data_t, uv), 0, 0, 0, 0 },
                { "SIZE", FORMAT_R32G32_FLOAT, offsetof(rect_vertex_data_t, size), 0, 0, 0, 0 },
                { "COLOR", FORMAT_R32G32B32A32_FLOAT, offsetof(rect_vertex_data_t, color), 0, 0, 0, 0 },
                { "CLIP", FORMAT_R32G32B32A32_FLOAT, offsetof(rect_vertex_data_t, clip), 0, 0, 0, 0 },
                { "BORDERTHICKNESS", FORMAT_R32_FLOAT, offsetof(rect_vertex_data_t, border_thickness), 0, 0, 0, 0 },
                { "HASTEXTURE", FORMAT_R32_FLOAT, offsetof(rect_vertex_data_t, has_texture), 0, 0, 0, 0 }
            },
            .attribute_count = 7,
        });

        graphics_sampler_t sampler = graphics->create_sampler(&(graphics_sampler_desc_t)
        {
            .filter = FILTER_MIN_MAG_MIP_POINT,
            .address_u = TEXTURE_ADDRESS_WRAP,
            .address_v = TEXTURE_ADDRESS_WRAP,
            .address_w = TEXTURE_ADDRESS_WRAP,
        });

        graphics_pipeline_t pipeline = graphics->create_pipeline(&(graphics_pipeline_desc_t)
        {
            .wireframe = false,
            .cull = true,
            .depth_test = false,
            .depth_write = false,
            .blend = BLEND_ALPHA,
        });

        global_render_2d.init = true;
        global_render_2d.parameter_buffer = parameter_buffer;
        global_render_2d.vertex_buffer = vertex_buffer;
        global_render_2d.program = program;
        global_render_2d.sampler = sampler;
        global_render_2d.pipeline = pipeline;
     }
}

static void render_2d_end(void)
{
    if (global_render_2d.vertex_data_count > 0)
    {
        graphics_t* graphics = global_render_2d.graphics;
        graphics_target_t backbuffer = graphics->get_backbuffer_target();

        rect_vertex_data_t* vertex_data = global_render_2d.vertex_data;
        u32 vertex_data_count = global_render_2d.vertex_data_count;
        vec4 viewport_size = v4((f32)backbuffer.width, (f32)backbuffer.height, 0.0f, 0.0f);
    
        graphics->begin_pass(backbuffer, &(graphics_pass_desc_t){ 0 });
        {
            graphics->update_buffer(global_render_2d.parameter_buffer, &viewport_size, 0, sizeof(viewport_size));
            graphics->update_buffer(global_render_2d.vertex_buffer, vertex_data, 0, sizeof(rect_vertex_data_t) * global_render_2d.max_vertex_data_count);
            graphics->set_buffer(global_render_2d.parameter_buffer, STAGE_VERTEX_SHADER, 0, 0, 0);
            graphics->set_vertex_buffer(global_render_2d.vertex_buffer, 0, sizeof(rect_vertex_data_t), 0);
            graphics->set_program(global_render_2d.program);
            graphics->set_pipeline(global_render_2d.pipeline);
            graphics->set_srvs(STAGE_PIXEL_SHADER, &global_render_2d.current_texture, 1, 0);
            graphics->draw(TOPOLOGY_TRIANGLE_LIST, vertex_data_count, 0);
        }
        graphics->end_pass();

        global_render_2d.vertex_data_count = 0;
    }
}

static void render_2d_set_texture(graphics_texture_t texture)
{
    if (memcmp(&global_render_2d.current_texture, &texture, sizeof(graphics_texture_t)))
    {
        render_2d_end();
    }

    global_render_2d.current_texture = texture;
}

static void render_2d_draw_rect(f32 x, f32 y, f32 width, f32 height, f32 border_thickness, vec4 color, vec4 clip)
{
    f32 x0 = x;
    f32 y0 = y;
    f32 x1 = x + width;
    f32 y1 = y + height;
    vec4 clip_rect = v4(x0, y0, x1, y1);
    
    if (clip.z > 0.0f && clip.w > 0.0f)
    {
        clip_rect = v4(clip.x, clip.y, clip.x + clip.z, clip.y + clip.w);
    }

    const u32 vertex_per_rect = 6;
    rect_vertex_data_t* vertex_data = global_render_2d.vertex_data;
    u32 vertex_data_count = global_render_2d.vertex_data_count;

    assert(vertex_data_count + vertex_per_rect < global_render_2d.max_vertex_data_count && "[RENDER2D] Vertex data is full.");

    vertex_data[vertex_data_count++] = (rect_vertex_data_t){ x0, y0, 0.0f, 0.0f, width, height, color, clip_rect, border_thickness, 0.0f };
    vertex_data[vertex_data_count++] = (rect_vertex_data_t){ x0, y1, 0.0f, 0.0f, width, height, color, clip_rect, border_thickness, 0.0f };
    vertex_data[vertex_data_count++] = (rect_vertex_data_t){ x1, y1, 0.0f, 0.0f, width, height, color, clip_rect, border_thickness, 0.0f };
    vertex_data[vertex_data_count++] = (rect_vertex_data_t){ x1, y1, 0.0f, 0.0f, width, height, color, clip_rect, border_thickness, 0.0f };
    vertex_data[vertex_data_count++] = (rect_vertex_data_t){ x1, y0, 0.0f, 0.0f, width, height, color, clip_rect, border_thickness, 0.0f };
    vertex_data[vertex_data_count++] = (rect_vertex_data_t){ x0, y0, 0.0f, 0.0f, width, height, color, clip_rect, border_thickness, 0.0f };

    global_render_2d.vertex_data_count = vertex_data_count;
}

static void render_2d_draw_text(font_handle_t font_handle, const char* text, size_t text_length, f32 x, f32 y, vec4 color, vec4 clip)
{
    if (text && text_length > 0)
    {
        font_t* font = global_render_2d.font;

        graphics_texture_t font_atlas = font->get_atlas(font_handle);
        render_2d_set_texture(font_atlas);
        
        rect_vertex_data_t* vertex_data = global_render_2d.vertex_data;
        u32 vertex_data_count = global_render_2d.vertex_data_count;
        const u32 vertex_per_glyph = 6;

        assert(text_length * vertex_per_glyph + vertex_data_count < global_render_2d.max_vertex_data_count &&
            "[RENDER2D] Vertex data is full.");

        font_info_t font_info = font->get_font_info(font_handle);
        f32 layout_x = x;
        f32 layout_y = y + font_info.ascent + font_info.line_gap;

        vec4 clip_rect = v4(0.0f, 0.0f, 10000.0f, 10000.0f);

        if (clip.z > 0.0f && clip.w > 0.0f)
        {
            clip_rect = v4(clip.x, clip.y, clip.x + clip.z, clip.y + clip.w);
        }
        
        for (u32 index = 0; index < text_length; ++index)
        {
            glyph_info_t glyph_info = font->get_glyph_info_from_codepoint(font_handle, text[index]);

            f32 x0 = layout_x + glyph_info.offset_x;
            f32 y0 = layout_y + glyph_info.offset_y;
            f32 x1 = x0 + glyph_info.width;
            f32 y1 = y0 + glyph_info.height;

            f32 uv_x = (f32)glyph_info.x / font_atlas.width;
            f32 uv_y = (f32)glyph_info.y / font_atlas.height;
            f32 uv_w = (f32)glyph_info.width / font_atlas.width;
            f32 uv_h = (f32)glyph_info.height / font_atlas.height;

            f32 u0 = uv_x;
            f32 v0 = uv_y;
            f32 u1 = u0 + uv_w;
            f32 v1 = v0 + uv_h;

            vertex_data[vertex_data_count++] = (rect_vertex_data_t){ x0, y0, u0, v0, 0.0f, 0.0f, color, clip_rect, 0.0f, 1.0f };
            vertex_data[vertex_data_count++] = (rect_vertex_data_t){ x0, y1, u0, v1, 0.0f, 0.0f, color, clip_rect, 0.0f, 1.0f };
            vertex_data[vertex_data_count++] = (rect_vertex_data_t){ x1, y1, u1, v1, 0.0f, 0.0f, color, clip_rect, 0.0f, 1.0f };
            vertex_data[vertex_data_count++] = (rect_vertex_data_t){ x1, y1, u1, v1, 0.0f, 0.0f, color, clip_rect, 0.0f, 1.0f };
            vertex_data[vertex_data_count++] = (rect_vertex_data_t){ x1, y0, u1, v0, 0.0f, 0.0f, color, clip_rect, 0.0f, 1.0f };
            vertex_data[vertex_data_count++] = (rect_vertex_data_t){ x0, y0, u0, v0, 0.0f, 0.0f, color, clip_rect, 0.0f, 1.0f };

            layout_x += glyph_info.advance;
        }

        global_render_2d.vertex_data_count = vertex_data_count;
    }
}
