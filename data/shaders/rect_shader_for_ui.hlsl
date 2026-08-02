/*
---- Notes about the shader ----:

This shader is used for ui drawing, for the reasons for rendering we use a single shader if possible.
The shader can draw rounded rectangles and borders for them, but only separatelly.
If border width is non 0 then the shader will draw the border, otherwise it 
will draw the whole rectangle. The borders are drawn inside the rectangle.
So to be able to draw a rounded rectangle with borders the user should 
do 2 draw calls: first one for teh background and also set border width to 0,
and the second call where they set the border width to the width they want.

The separation between the rect and the borders dont really need to be 
done in 2 calls and could just be dont in a single draw call, but 
since the this shader is for the ui and we Clay for ui backend and clay 
has different commands for rectangles and rectangle borders for some reason,
this makes us have to have this work so either only the background or the border is 
drawn to be able to conform more to clay's public api logic.

DD, TODO: We no longer conform to clay borders, so might colapse this into a single thing or some like that.
*/

cbuffer cbuffer0 : register(b0) {
  float u_window_width;
  float u_window_height;
};

sampler sampler0 : register(s0);                           
Texture2D<float4> texture0 : register(t0);                 

#define UV__top_left     0
#define UV__top_right    1
#define UV__bottom_left  2
#define UV__bottom_right 3
#define UV__COUNT 4 

struct VertexInput {
  float4 rect_color_top_left            : RECT_00_COLOR;  // TODO: Change these names here 
  float4 rect_color_top_right           : RECT_10_COLOR;  
  float4 rect_color_bottom_left         : RECT_01_COLOR;  
  float4 rect_color_bottom_right        : RECT_11_COLOR;  
  
  float rect_origin_x         : RECT_ORIGIN_X; 
  float rect_origin_y         : RECT_ORIGIN_Y; 
  float rect_width            : RECT_WIDTH;
  float rect_height           : RECT_HEIGHT;

  // DD: These are in px
  float rect_corner_radius_top_left     : RECT_00_CORNER_RADIUS; // TODO: Change these names here
  float rect_corner_radius_top_right    : RECT_10_CORNER_RADIUS;
  float rect_corner_radius_bottom_left  : RECT_01_CORNER_RADIUS;
  float rect_corner_radius_bottom_right : RECT_11_CORNER_RADIUS;
  
  float4 rect_border_color    : RECT_BORDER_COLOR;
  float rect_border_thickness : RECT_BORNER_THICKNESS;
  
  float softness_inner : SOFTNESS_INNER;
  float softness_outer : SOFTNESS_OUTER;

  bool is_texture : IS_TEXTURE;
  
  float2 texture_rect_origin  : TEXTURE_RECT_ORIGIN;
  float2 texture_rect_size    : TEXTURE_RECT_SIZE;

  uint vertex_id : SV_VertexID;
};

struct PixelInput {
  float4 vertex_color[UV__COUNT] : PER_VERTEX_COLOR;

  nointerpolation float2 rect_origin     : RECT_ORIGIN;
  nointerpolation float2 rect_dims       : RECT_DIMS;

  // todo: pass 4 differnt nointerpolation cor_rs
  nointerpolation float corner_radius  : CORNER_R;
  
  nointerpolation float4 border_color      : RECT_BORDER_COLOR;
  nointerpolation float border_thickness   : BORDER_THICH;
  
  nointerpolation float softness_inner : SOFTNESS_INNER;
  nointerpolation float softness_outer : SOFTNESS_OUTER;

  bool is_texture : IS_TEXTURE_BOOL;

  float2 texture_to_sample_uv : TEXTURE_TO_SAMPLE_UV;

  float4 pos : SV_POSITION;
};

float sdf_rounded_rect(float2 rect_origin, float2 rect_dims, float2 p, float r)
{
  float2 rect_max_point = rect_origin + rect_dims;
  float2 rect_half_dims = rect_dims / 2.0;
  float2 rect_mid       = (rect_max_point + rect_origin) / 2.0;
  
  float2 diff = abs(p - rect_mid) - rect_half_dims + float2(r, r); 
	return length(max(diff, 0.0)) + min(max(diff.x, diff.y), 0.0) - r;
}

bool is_point_inside_rect(float2 p, float2 origin, float2 dims)
{
  return (origin.x <= p.x && p.x < origin.x + dims.x && origin.y <= p.y && p.y < origin.y + dims.y);
}

PixelInput vs_main(VertexInput vertex_input) 
{
  float2 vp_dims     = float2(u_window_width, u_window_height);

  float2 rect_origin = float2(vertex_input.rect_origin_x, vertex_input.rect_origin_y);
  float2 rect_dims   = float2(vertex_input.rect_width, vertex_input.rect_height);

  float2 texture_rect_origin = float2(vertex_input.texture_rect_origin.x, vertex_input.texture_rect_origin.y);
  float2 texture_rect_dims   = float2(vertex_input.texture_rect_size.x, vertex_input.texture_rect_size.y);

  float2 uv_vertex_coords[4] = {
    float2(0.0, 0.0), float2(1.0, 0.0),
    float2(0.0, 1.0), float2(1.0, 1.0),
  };

  float2 rect_vertex_in_px    = rect_origin + (uv_vertex_coords[vertex_input.vertex_id] * rect_dims);
  float2 texture_vertex_in_px = texture_rect_origin + (uv_vertex_coords[vertex_input.vertex_id] * texture_rect_dims);

  // note: I hate that this takes 3 lines
  float2 rect_vertex_in_ndc = (rect_vertex_in_px / vp_dims) * 2.0;
  rect_vertex_in_ndc.x      = rect_vertex_in_ndc.x - 1.0; 
  rect_vertex_in_ndc.y      = 1.0 - rect_vertex_in_ndc.y;

  float2 texture_vertex_in_uv = float2(0, 0);
  if (vertex_input.is_texture)
  {
    float texture_width;
    float texture_height;
    texture0.GetDimensions(texture_width, texture_height);
    texture_vertex_in_uv = (texture_vertex_in_px / float2(texture_width, texture_height));
  }

  float rect_vertex_corner_r[4];
  rect_vertex_corner_r[UV__top_left]     = vertex_input.rect_corner_radius_top_left; 
  rect_vertex_corner_r[UV__top_right]    = vertex_input.rect_corner_radius_top_right;
  rect_vertex_corner_r[UV__bottom_left]  = vertex_input.rect_corner_radius_bottom_left; 
  rect_vertex_corner_r[UV__bottom_right] = vertex_input.rect_corner_radius_bottom_right;

  PixelInput pixel_input;
  pixel_input.pos                                  = float4(rect_vertex_in_ndc, 0, 1);
  pixel_input.rect_origin                          = rect_origin;
  pixel_input.rect_dims                            = rect_dims;
  pixel_input.corner_radius                        = rect_vertex_corner_r[vertex_input.vertex_id];
  pixel_input.softness_inner                       = vertex_input.softness_inner;
  pixel_input.softness_outer                       = vertex_input.softness_outer;
  pixel_input.border_thickness                     = vertex_input.rect_border_thickness;
  pixel_input.border_color                         = vertex_input.rect_border_color;
  pixel_input.vertex_color[UV__top_left]           = vertex_input.rect_color_top_left;
  pixel_input.vertex_color[UV__top_right]          = vertex_input.rect_color_top_right;
  pixel_input.vertex_color[UV__bottom_left]        = vertex_input.rect_color_bottom_left;
  pixel_input.vertex_color[UV__bottom_right]       = vertex_input.rect_color_bottom_right;
  pixel_input.is_texture                           = vertex_input.is_texture;
  pixel_input.texture_to_sample_uv                 = texture_vertex_in_uv;
  // pixel_input.texture_to_sample_uv.y                 = 1;


  return pixel_input;
}

float4 ps_main(PixelInput pixel_input) : SV_TARGET
{
  float2 pos_px   = pixel_input.pos.xy;
  float2 pos_norm = (pos_px - pixel_input.rect_origin) / pixel_input.rect_dims; 

  // DD: Getting the final color for the pixels from the colors of the 4 corners
  float4 top_color        = lerp(pixel_input.vertex_color[UV__top_left], pixel_input.vertex_color[UV__top_right], pos_norm.x);
  float4 bottom_color     = lerp(pixel_input.vertex_color[UV__bottom_left], pixel_input.vertex_color[UV__bottom_right], pos_norm.x);
  float4 background_color = lerp(top_color, bottom_color, pos_norm.y);

  float radius_in_px = clamp(pixel_input.corner_radius, 0.0, (min(pixel_input.rect_dims.x, pixel_input.rect_dims.y) / 2.0));
  
  float sdf_pixel_to_rect = sdf_rounded_rect(pixel_input.rect_origin, pixel_input.rect_dims, pos_px, radius_in_px);

  float4 final_color = background_color;
  
  float outer_softness = pixel_input.softness_outer;
  float inner_softness = pixel_input.softness_inner;

  float outer_smoothing = 1.0;
  float inner_smoothing = 1.0;

  if (pixel_input.is_texture)
  {
    // pixel_input.texture_to_sample_uv.y = 0.7;
    final_color = texture0.Sample(sampler0, pixel_input.texture_to_sample_uv);
  }
  else 
  {
    if (pixel_input.border_thickness != 0.0)
    {
      if (pixel_input.corner_radius == 0.0)
      {
        if (-pixel_input.border_thickness < sdf_pixel_to_rect && sdf_pixel_to_rect < 0.0f)
        {
          final_color = pixel_input.border_color;
        }
      }
      else 
      {
        float inner_sdf  = sdf_pixel_to_rect + pixel_input.border_thickness;
        float smoothstep_res = smoothstep(-inner_softness, 0.0, inner_sdf);
        if (background_color.a != 0.0f)
        {
          final_color = lerp(background_color, pixel_input.border_color, smoothstep_res);
        }
        else
        {
          final_color     = pixel_input.border_color;
          inner_smoothing = smoothstep_res;
        }
      }
    }

    if (pixel_input.corner_radius != 0.0)
    {
      if (0) {}
      else if (sdf_pixel_to_rect > 0.0) { outer_smoothing = 0.0f; }
      else if (-outer_softness < sdf_pixel_to_rect && sdf_pixel_to_rect < 0.0)
      {
        outer_smoothing = smoothstep(0.0, -outer_softness, sdf_pixel_to_rect);
      }

      final_color.a *= outer_smoothing;
      final_color.a *= inner_smoothing;
    }
  }

  return final_color;
}