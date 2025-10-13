#pragma once

#include <math.h>

#define PI      3.14159265358979323846
#define DEG2RAD (PI / 180.0)

typedef union vec2
{
    struct
    {
        f32 x, y;
    };

    struct
    {
        f32 u, v;
    };

    struct
    {
        f32 left, right;
    };
    
    struct
    {
        f32 width, height;
    };

    f32 values[2];
} vec2;

typedef union vec3
{
    struct
    {
        f32 x, y, z;
    };

    struct
    {
        f32 u, v, w;
    };

    struct
    {
        f32 r, g, b;
    };

    struct
    {
        vec2 xy;
        f32 _0;
    };

    struct
    {
        f32 _1;
        vec2 yz;
    };

    struct
    {
        vec2 uv;
        f32 _2;
    };

    struct
    {
        f32 _4;
        vec2 vw;
    };
    
    f32 values[3];
} vec3;

typedef union vec4
{
    struct
    {
        union
        {
            vec3 xyz;
        
            struct
            {
                f32 x, y, z;
            };
        };
        
        f32 w;
    };

    struct
    {
        union
        {
            vec3 rgb;
        
            struct
            {
                f32 r, g, b;
            };
        };
        
        f32 a;
    };

    struct
    {
        vec2 xy;
        f32 _0;
        f32 _1;
    };

    struct
    {
        f32 _2;
        vec2 yz;
        f32 _3;
    };

    struct
    {
        f32 _4;
        f32 _5;
        vec2 zw;
    };

    f32 values[4];
} vec4;

typedef union mat4x4
{
    f32 values[4][4];
    vec4 columns[4];
} mat4x4;


static inline f32 clamp(f32 min, f32 v, f32 max)
{
    f32 result = v;

    if (result < min)
    {
        result = min;
    }
    else if (result > max)
    {
        result = max;
    }

    return result;
}

// NOTE: Vector initialization.

static inline vec2 v2(f32 x, f32 y)
{
    vec2 result;

    result.x = x;
    result.y = y;

    return result;
}

static inline vec3 v3(f32 x, f32 y, f32 z)
{
    vec3 result;

    result.x = x;
    result.y = y;
    result.z = z;

    return result;
}

static inline vec4 v4(f32 x, f32 y, f32 z, f32 w)
{
    vec4 result;

    result.x = x;
    result.y = y;
    result.z = z;
    result.w = w;

    return result;
}

static inline vec4 v4v(vec3 xyz, f32 w)
{
    vec4 result;

    result.xyz = xyz;
    result.w = w;

    return result;
}

// NOTE: Vector unary operations.

static inline vec2 v2_neg(vec2 v)
{
    vec2 result;

    result.x = -v.x;
    result.y = -v.y;

    return result;
}

static inline vec3 v3_neg(vec3 v)
{
    vec3 result;

    result.x = -v.x;
    result.y = -v.y;
    result.z = -v.z;

    return result;
}

static inline vec4 v4_neg(vec4 v)
{
    vec4 result;

    result.x = -v.x;
    result.y = -v.y;
    result.z = -v.z;
    result.w = -v.w;

    return result;
}

// NOTE: Vector binary operations.

static inline vec2 v2_add(vec2 left, vec2 right)
{
    vec2 result;

    result.x = left.x + right.x;
    result.y = left.y + right.y;

    return result;
}

static inline vec3 v3_add(vec3 left, vec3 right)
{
    vec3 result;

    result.x = left.x + right.x;
    result.y = left.y + right.y;
    result.z = left.z + right.z;

    return result;
}

static inline vec4 v4_add(vec4 left, vec4 right)
{
    vec4 result;

    result.x = left.x + right.x;
    result.y = left.y + right.y;
    result.z = left.z + right.z;
    result.w = left.w + right.w;

    return result;
}

static inline vec2 v2_sub(vec2 left, vec2 right)
{
    vec2 result;

    result.x = left.x - right.x;
    result.y = left.y - right.y;

    return result;
}

static inline vec3 v3_sub(vec3 left, vec3 right)
{
    vec3 result;

    result.x = left.x - right.x;
    result.y = left.y - right.y;
    result.z = left.z - right.z;

    return result;
}

static inline vec4 v4_sub(vec4 left, vec4 right)
{
    vec4 result;

    result.x = left.x - right.x;
    result.y = left.y - right.y;
    result.z = left.z - right.z;
    result.w = left.w - right.w;

    return result;
}

static inline vec2 v2_mul(vec2 left, vec2 right)
{
    vec2 result;

    result.x = left.x * right.x;
    result.y = left.y * right.y;

    return result;
}

static inline vec3 v3_mul(vec3 left, vec3 right)
{
    vec3 result;

    result.x = left.x * right.x;
    result.y = left.y * right.y;
    result.z = left.z * right.z;

    return result;
}

static inline vec4 v4_mul(vec4 left, vec4 right)
{
    vec4 result;

    result.x = left.x * right.x;
    result.y = left.y * right.y;
    result.z = left.z * right.z;
    result.w = left.w * right.w;

    return result;
}

static inline vec2 v2_mulf(vec2 left, f32 right)
{
    vec2 result;

    result.x = left.x * right;
    result.y = left.y * right;

    return result;
}

static inline vec3 v3_mulf(vec3 left, f32 right)
{
    vec3 result;

    result.x = left.x * right;
    result.y = left.y * right;
    result.z = left.z * right;

    return result;
}

static inline vec4 v4_mulf(vec4 left, f32 right)
{
    vec4 result;

    result.x = left.x * right;
    result.y = left.y * right;
    result.z = left.z * right;
    result.w = left.w * right;

    return result;
}

static inline vec2 v2_div(vec2 left, vec2 right)
{
    vec2 result;

    result.x = left.x / right.x;
    result.y = left.y / right.y;

    return result;
}

static inline vec3 v3_div(vec3 left, vec3 right)
{
    vec3 result;

    result.x = left.x / right.x;
    result.y = left.y / right.y;
    result.z = left.z / right.z;

    return result;
}

static inline vec4 v4_div(vec4 left, vec4 right)
{
    vec4 result;

    result.x = left.x / right.x;
    result.y = left.y / right.y;
    result.z = left.z / right.z;
    result.w = left.w / right.w;

    return result;
}

static inline vec2 v2_divf(vec2 left, f32 right)
{
    vec2 result;

    result.x = left.x / right;
    result.y = left.y / right;

    return result;
}

static inline vec3 v3_divf(vec3 left, f32 right)
{
    vec3 result;

    result.x = left.x / right;
    result.y = left.y / right;
    result.z = left.z / right;

    return result;
}

static inline vec4 v4_divf(vec4 left, f32 right)
{
    vec4 result;

    result.x = left.x / right;
    result.y = left.y / right;
    result.z = left.z / right;
    result.w = left.w / right;

    return result;
}

static inline bool v2_equal(vec2 left, vec2 right)
{
    bool result = (left.x == right.x &&
                   left.y == right.y);

    return result;
}

static inline bool v3_equal(vec3 left, vec3 right)
{
    bool result = (left.x == right.x &&
                   left.y == right.y &&
                   left.z == right.z);

    return result;
}

static inline bool v4_equal(vec4 left, vec4 right)
{
    bool result = (left.x == right.x &&
                   left.y == right.y &&
                   left.z == right.z &&
                   left.w == right.w);

    return result;
}

static inline f32 v2_dot(vec2 left, vec2 right)
{
    f32 result = (left.x * right.x +
                  left.y * right.y);

    return result;
}

static inline f32 v3_dot(vec3 left, vec3 right)
{
    f32 result = (left.x * right.x +
                  left.y * right.y +
                  left.z * right.z);

    return result;
}

static inline f32 v4_dot(vec4 left, vec4 right)
{
    f32 result = (left.x * right.x +
                  left.y * right.y +
                  left.z * right.z +
                  left.w * right.w);

    return result;
}

static inline vec3 v3_cross(vec3 left, vec3 right)
{
    vec3 result;

    result.x = (left.y * right.z) - (left.z * right.y);
    result.y = (left.z * right.x) - (left.x * right.z);
    result.z = (left.x * right.y) - (left.y * right.x);

    return result;
}

// NOTE: Vector unary operations.

static inline f32 v2_length_squared(vec2 v)
{
    f32 result = v2_dot(v, v);

    return result;
}

static inline f32 v3_length_squared(vec3 v)
{
    f32 result = v3_dot(v, v);

    return result;
}

static inline f32 v4_length_squared(vec4 v)
{
    f32 result = v4_dot(v, v);

    return result;
}

static inline f32 v2_length(vec2 v)
{
    f32 result = sqrtf(v2_length_squared(v));

    return result;
}

static inline f32 v3_length(vec3 v)
{
    f32 result = sqrtf(v3_length_squared(v));

    return result;
}

static inline f32 v4_length(vec4 v)
{
    f32 result = sqrtf(v4_length_squared(v));

    return result;
}

static inline vec2 v2_normalize(vec2 v)
{
    vec2 result = v2_mulf(v, 1.0f / sqrtf(v2_dot(v, v)));

    return result;
}

static inline vec3 v3_normalize(vec3 v)
{
    vec3 result = v3_mulf(v, 1.0f / sqrtf(v3_dot(v, v)));

    return result;
}

static inline vec4 v4_normalize(vec4 v)
{
    vec4 result = v4_mulf(v, 1.0f / sqrtf(v4_dot(v, v)));

    return result;
}

// NOTE: 4x4 matrix

static inline mat4x4 m4x4(void)
{
    mat4x4 result = { 0 };

    return result;
}

static inline mat4x4 m4x4d(f32 diagonal)
{
    mat4x4 result = { 0 };

    result.values[0][0] = diagonal;
    result.values[1][1] = diagonal;
    result.values[2][2] = diagonal;
    result.values[3][3] = diagonal;

    return result;
}

static inline mat4x4 m4x4_transpose(mat4x4 m)
{
    mat4x4 result;

    result.values[0][0] = result.values[0][0];
    result.values[0][1] = result.values[1][0];
    result.values[0][2] = result.values[2][0];
    result.values[0][3] = result.values[3][0];
    result.values[1][0] = result.values[0][1];
    result.values[1][1] = result.values[1][1];
    result.values[1][2] = result.values[2][1];
    result.values[1][3] = result.values[3][1];
    result.values[2][0] = result.values[0][2];
    result.values[2][1] = result.values[1][2];
    result.values[2][2] = result.values[2][2];
    result.values[2][3] = result.values[3][2];
    result.values[3][0] = result.values[0][3];
    result.values[3][1] = result.values[1][3];
    result.values[3][2] = result.values[2][3];
    result.values[3][3] = result.values[3][3];
    
    return result;
}

static inline mat4x4 m4x4_add(mat4x4 left, mat4x4 right)
{
    mat4x4 result;

    result.columns[0] = v4_add(left.columns[0], right.columns[0]);
    result.columns[1] = v4_add(left.columns[1], right.columns[1]);
    result.columns[2] = v4_add(left.columns[2], right.columns[2]);
    result.columns[3] = v4_add(left.columns[3], right.columns[3]);

    return result;
}

static inline mat4x4 m4x4_sub(mat4x4 left, mat4x4 right)
{
    mat4x4 result;

    result.columns[0] = v4_sub(left.columns[0], right.columns[0]);
    result.columns[1] = v4_sub(left.columns[1], right.columns[1]);
    result.columns[2] = v4_sub(left.columns[2], right.columns[2]);
    result.columns[3] = v4_sub(left.columns[3], right.columns[3]);

    return result;
}

static inline vec4 m4x4_mulv4(mat4x4 left, vec4 right)
{
    vec4 result;

    result.values[0] = left.columns[0].x * right.values[0];
    result.values[1] = left.columns[0].y * right.values[0];
    result.values[2] = left.columns[0].z * right.values[0];
    result.values[3] = left.columns[0].w * right.values[0];

    result.values[0] += left.columns[1].x * right.values[1];
    result.values[1] += left.columns[1].y * right.values[1];
    result.values[2] += left.columns[1].z * right.values[1];
    result.values[3] += left.columns[1].w * right.values[1];

    result.values[0] += left.columns[2].x * right.values[2];
    result.values[1] += left.columns[2].y * right.values[2];
    result.values[2] += left.columns[2].z * right.values[2];
    result.values[3] += left.columns[2].w * right.values[2];

    result.values[0] += left.columns[3].x * right.values[3];
    result.values[1] += left.columns[3].y * right.values[3];
    result.values[2] += left.columns[3].z * right.values[3];
    result.values[3] += left.columns[3].w * right.values[3];

    return result;
}

static inline mat4x4 m4x4_mul(mat4x4 left, mat4x4 right)
{
    mat4x4 result;

    result.columns[0] = m4x4_mulv4(left, right.columns[0]);
    result.columns[1] = m4x4_mulv4(left, right.columns[1]);
    result.columns[2] = m4x4_mulv4(left, right.columns[2]);
    result.columns[3] = m4x4_mulv4(left, right.columns[3]);
    
    return result;
}
