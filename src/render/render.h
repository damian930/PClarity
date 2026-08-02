#ifndef RENDERER_D3D11_H
#define RENDERER_D3D11_H

// D3D 
#include "d3d11.h"
#include "dxgi.h"
#include "dxgidebug.h"
#include "dxgi1_3.h"
#include "d3dcompiler.h"

// DWM
#include "dwmapi.h"
#include "dcomp.h"

#include "__third_party/stb/stb_image.h"

#include "core/core_include.h"
#include "os/win32.h"

// Pre-defines for draw layer 
struct D_Command_batch_list; 

// todo: I dont really like it here, but i also dont have a clear place where to put this
struct Image {
  U8* data;
  U64 width_in_px;
  U64 height_in_px;
  U64 bytes_per_pixel;
  // U64 row_stride; // There are no images right now that might have extra padding
};

// - Stuff for the rect program
struct R_Rect_instance_data {
  V4F32 color_00;
  V4F32 color_10;
  V4F32 color_01;
  V4F32 color_11;
  
  F32 origin_x; 
  F32 origin_y; 

  F32 width;
  F32 height;

  F32 corner_radius_00;
  F32 corner_radius_10;
  F32 corner_radius_01;
  F32 corner_radius_11;

  V4F32 border_color;
  F32 border_thickness;
  
  F32 softness_inner;
  F32 softness_outer;

  B8 is_texture;
  V2F32 texture_rect_origin;
  V2F32 texture_rect_dims;

  F32 texture_width;
  F32 texture_height;

  F32 _padding_[2];
};
//
struct R_Rect_unifrom_data {
  F32 u_window_width;
  F32 u_window_height;
  F32 _padding_[2];
};  

// - Stuff for the texture program
struct R_Texture_instance_data {
  V4F32 tint;

  V2F32 dest_rect_origin;
  V2F32 dest_rect_size;
  
  V2F32 src_rect_origin;
  V2F32 src_rect_size;
  
  V2F32 src_texture_dims;

  F32 _padding_[3];
};
//
struct R_Texture_uniform_data {
  F32 u_window_width;
  F32 u_window_height;
  F32 _padding_[2];
};

struct R_Program {
  ID3D11VertexShader* v_shader;
  ID3D11PixelShader* p_shader;
  ID3D11InputLayout* input_layout;
};

enum R_Blend_kind {
  R_Blend_kind__alpha,
  R_Blend_kind__no_blend,
  R_Blend_kind__dest_out,
  R_Blend_kind__COUNT,
};

enum R_Fill_mode : U32 {
  R_Fill_mode__solid,
  R_Fill_mode__wireframe,
  R_Fill_mode__COUNT,
};

struct R_Handle {
  // This is shared for rtvs and swap chains 
  // (There are no textures right now which are not also rtvs)
  ID3D11Texture2D*        texture;
  ID3D11RenderTargetView* texture_rtv;

  // This is only present for swap chains
  HWND __win32_window_handle_for_assert;
  IDXGISwapChain1* swap_chain;
  //
  // This is optional for swap chain and only present if the window for which the swap chain is created 
  // was made transparent at its creation. (Transparent windows use different frame buffers)
  IDCompositionDevice* comp_device;
  
  // This is specific for texture
  ID3D11ShaderResourceView* srv;

  V2F32 texture_or_swap_chain_dims;
};

struct D3D_State {
  Arena* state_arena;

  // These we get at initialisation
  ID3D11Device*        device;
  ID3D11DeviceContext* context;
  // 
  ID3D11RasterizerState* rasterizer_states[R_Fill_mode__COUNT];
  ID3D11BlendState*      blend_states[R_Blend_kind__COUNT];
  ID3D11SamplerState*    sampler;
  //
  #define D3D_BUFFER_COUNT 8
  ID3D11Buffer* rect_program_ia_buffer[D3D_BUFFER_COUNT];
  ID3D11Buffer* rect_program_uniform_buffer[D3D_BUFFER_COUNT];
  R_Program     rect_program;
  //
  ID3D11Buffer* texture_program_ia_buffer[D3D_BUFFER_COUNT];
  ID3D11Buffer* texture_program_uniform_buffer[D3D_BUFFER_COUNT];
  R_Program     texture_program;

  // These are obtained after the base of the state is set
  R_Handle magenta_black_texture;
  // TODO, DD: Move the programs here as well

  // TODO: Try to remoe this and see if it changes anything
  U64 draw_generation;
};

extern global D3D_State* __d3d_g_state;

// - State
D3D_State* r_get_state();
void r_set_state(D3D_State* state);

void r_init();
void r_relesase();

// - Rendering work flow (this is in the order of how it could be used)
R_Handle r_attach_window(OS_Window window);
void r_prepare_canvas(R_Handle* chain);
void r_submit(R_Handle target, D_Command_batch_list* command_batch_list);
void r_present(R_Handle target, B32 vsync);

// - Handles
R_Handle r_handle_zero();
B32 r_handle_is_zero(R_Handle handle);
B32 r_handle_match(R_Handle handle, R_Handle other);

// - Texture making 
R_Handle __r_make_texture(U32 width_in_px, U32 height_in_px, Data_buffer opt_image_data_bytes);
R_Handle r_make_texture(U32 width, U32 height);
R_Handle r_load_texture_from_file(Str8 file_name); // TODO: Change the name
R_Handle r_load_texture_from_image(Image image);   // TODO: Change the name
void r_release_texture(R_Handle* texture);

// - Texure other stuff
Image r_image_from_texture(Arena* arena, R_Handle texture);
void r_export_texture(R_Handle texture, Str8 file_path);
void r_export_image(Image image, Str8 file_name);
void r_copy_into_texture_from_texture(R_Handle dest_texture, R_Handle src_texture, B32* out_opt_is_succ);
V2F32 r_get_handle_dims(R_Handle target);

// - Boring stuff with handles
R_Handle r_handle_zero();
B32 r_handle_match(R_Handle target, R_Handle other);

// - Misc
R_Program r_program_from_file(const WCHAR* shader_program_file, 
                              const char* v_shader_main_f_name, 
                              const char* p_shader_main_f_name, 
                              const D3D11_INPUT_ELEMENT_DESC* opt_desc_arr,
                              U32 desc_arr_count);
void r_clear_handle(R_Handle handle, V4F32 color);

///////////////////////////////////////////////////////////
// Private helpers
///////////////////////////////////////////////////////////

// - Extra handle checks
B32 __r_is_handle_valid_handle(R_Handle handle);
B32 __r_is_handle_valid_handle_chain(R_Handle handle);

// - Per vertex data describtions
const global 
D3D11_INPUT_ELEMENT_DESC __r_g_rect_program_input_assembler_element_desc[] = 
{
  // TODO: Have better names for 00 10 and all these UVs   
  { "RECT_00_COLOR",         0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, TypeFieldOffset(R_Rect_instance_data, color_00),            D3D11_INPUT_PER_INSTANCE_DATA, 1 },
  { "RECT_10_COLOR",         0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, TypeFieldOffset(R_Rect_instance_data, color_10),            D3D11_INPUT_PER_INSTANCE_DATA, 1 },
  { "RECT_01_COLOR",         0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, TypeFieldOffset(R_Rect_instance_data, color_01),            D3D11_INPUT_PER_INSTANCE_DATA, 1 },
  { "RECT_11_COLOR",         0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, TypeFieldOffset(R_Rect_instance_data, color_11),            D3D11_INPUT_PER_INSTANCE_DATA, 1 },
  { "RECT_ORIGIN_X",         0, DXGI_FORMAT_R32_FLOAT,          0, TypeFieldOffset(R_Rect_instance_data, origin_x),            D3D11_INPUT_PER_INSTANCE_DATA, 1 },
  { "RECT_ORIGIN_Y",         0, DXGI_FORMAT_R32_FLOAT,          0, TypeFieldOffset(R_Rect_instance_data, origin_y),            D3D11_INPUT_PER_INSTANCE_DATA, 1 },
  { "RECT_WIDTH",            0, DXGI_FORMAT_R32_FLOAT,          0, TypeFieldOffset(R_Rect_instance_data, width),               D3D11_INPUT_PER_INSTANCE_DATA, 1 },
  { "RECT_HEIGHT",           0, DXGI_FORMAT_R32_FLOAT,          0, TypeFieldOffset(R_Rect_instance_data, height),              D3D11_INPUT_PER_INSTANCE_DATA, 1 },
  { "RECT_00_CORNER_RADIUS", 0, DXGI_FORMAT_R32_FLOAT,          0, TypeFieldOffset(R_Rect_instance_data, corner_radius_00),    D3D11_INPUT_PER_INSTANCE_DATA, 1 },
  { "RECT_10_CORNER_RADIUS", 0, DXGI_FORMAT_R32_FLOAT,          0, TypeFieldOffset(R_Rect_instance_data, corner_radius_10),    D3D11_INPUT_PER_INSTANCE_DATA, 1 },
  { "RECT_01_CORNER_RADIUS", 0, DXGI_FORMAT_R32_FLOAT,          0, TypeFieldOffset(R_Rect_instance_data, corner_radius_01),    D3D11_INPUT_PER_INSTANCE_DATA, 1 },
  { "RECT_11_CORNER_RADIUS", 0, DXGI_FORMAT_R32_FLOAT,          0, TypeFieldOffset(R_Rect_instance_data, corner_radius_11),    D3D11_INPUT_PER_INSTANCE_DATA, 1 },
  { "RECT_BORDER_COLOR",     0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, TypeFieldOffset(R_Rect_instance_data, border_color),        D3D11_INPUT_PER_INSTANCE_DATA, 1 },
  { "RECT_BORNER_THICKNESS", 0, DXGI_FORMAT_R32_FLOAT,          0, TypeFieldOffset(R_Rect_instance_data, border_thickness),    D3D11_INPUT_PER_INSTANCE_DATA, 1 },
  { "SOFTNESS_INNER",        0, DXGI_FORMAT_R32_FLOAT,          0, TypeFieldOffset(R_Rect_instance_data, softness_inner),      D3D11_INPUT_PER_INSTANCE_DATA, 1 },
  { "SOFTNESS_OUTER",        0, DXGI_FORMAT_R32_FLOAT,          0, TypeFieldOffset(R_Rect_instance_data, softness_outer),      D3D11_INPUT_PER_INSTANCE_DATA, 1 },
  { "IS_TEXTURE",            0, DXGI_FORMAT_R8_UINT,            0, TypeFieldOffset(R_Rect_instance_data, is_texture),          D3D11_INPUT_PER_INSTANCE_DATA, 1 },
  { "TEXTURE_RECT_ORIGIN",   0, DXGI_FORMAT_R32G32_FLOAT,       0, TypeFieldOffset(R_Rect_instance_data, texture_rect_origin), D3D11_INPUT_PER_INSTANCE_DATA, 1 },
  { "TEXTURE_RECT_SIZE",     0, DXGI_FORMAT_R32G32_FLOAT,       0, TypeFieldOffset(R_Rect_instance_data, texture_rect_dims),   D3D11_INPUT_PER_INSTANCE_DATA, 1 },
};

D3D11_INPUT_ELEMENT_DESC __r_g_texture_program_input_assembler_element_desc[] = 
{
  { "TINT",             0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, TypeFieldOffset(R_Texture_instance_data, tint),             D3D11_INPUT_PER_INSTANCE_DATA, 1 },
  { "DEST_RECT_ORIGIN", 0, DXGI_FORMAT_R32G32_FLOAT,       0, TypeFieldOffset(R_Texture_instance_data, dest_rect_origin), D3D11_INPUT_PER_INSTANCE_DATA, 1 },
  { "DEST_RECT_SIZE",   0, DXGI_FORMAT_R32G32_FLOAT,       0, TypeFieldOffset(R_Texture_instance_data, dest_rect_size),   D3D11_INPUT_PER_INSTANCE_DATA, 1 },
  { "SRC_RECT_ORIGIN",  0, DXGI_FORMAT_R32G32_FLOAT,       0, TypeFieldOffset(R_Texture_instance_data, src_rect_origin),  D3D11_INPUT_PER_INSTANCE_DATA, 1 },
  { "SRC_RECT_SIZE",    0, DXGI_FORMAT_R32G32_FLOAT,       0, TypeFieldOffset(R_Texture_instance_data, src_rect_size),    D3D11_INPUT_PER_INSTANCE_DATA, 1 },
  { "SRC_TEXTURE_DIMS", 0, DXGI_FORMAT_R32G32_FLOAT,       0, TypeFieldOffset(R_Texture_instance_data, src_texture_dims), D3D11_INPUT_PER_INSTANCE_DATA, 1 },
};

struct __D3D_Draw_context {
  ID3D11RenderTargetView*   last_rtv;
  ID3D11BlendState*         last_blend_state;
  ID3D11RasterizerState*    last_rasterizer_state;
  D3D11_RECT                last_scissor_rect;
  B32                       has_scissor_rect;
  ID3D11ShaderResourceView* last_srv;
  ID3D11Buffer*             last_vertex_buffer;
  UINT                      last_vb_stride;
  UINT                      last_vb_offset;
  ID3D11InputLayout*        last_input_layout;
  ID3D11VertexShader*       last_v_shader;
  ID3D11PixelShader*        last_p_shader;
  ID3D11Buffer*             last_constant_buffer;
  ID3D11SamplerState*       last_sampler;
};

// TODO: Move these to the .cpp file

void __d3d_OMSetRenderTargets(__D3D_Draw_context* ctx, ID3D11RenderTargetView* rtv)
{
  D3D_State* d3d = r_get_state();
  if (ctx->last_rtv != rtv)
  {
    ctx->last_rtv = rtv;
    d3d->context->OMSetRenderTargets(1, &rtv, Null);
  }
}

void __d3d_OMSetBlendState(__D3D_Draw_context* ctx, ID3D11BlendState* blend_state)
{
  D3D_State* d3d = r_get_state();
  if (ctx->last_blend_state != blend_state)
  {
    ctx->last_blend_state = blend_state;
    d3d->context->OMSetBlendState(blend_state, Null, ~0U);
  }
}

void __d3d_RSSetState(__D3D_Draw_context* ctx, ID3D11RasterizerState* rasterizer_state)
{
  D3D_State* d3d = r_get_state();
  if (ctx->last_rasterizer_state != rasterizer_state)
  {
    ctx->last_rasterizer_state = rasterizer_state;
    d3d->context->RSSetState(rasterizer_state);
  }
}

void __d3d_RSSetScissorRect(__D3D_Draw_context* ctx, D3D11_RECT scissor_rect)
{
  D3D_State* d3d = r_get_state();
  if (!ctx->has_scissor_rect || memcmp(&ctx->last_scissor_rect, &scissor_rect, sizeof(D3D11_RECT)) != 0)
  {
    ctx->has_scissor_rect  = 1;
    ctx->last_scissor_rect = scissor_rect;
    d3d->context->RSSetScissorRects(1, &scissor_rect);
  }
}

void __d3d_SetShaderResource(__D3D_Draw_context* ctx, ID3D11ShaderResourceView* srv)
{
  D3D_State* d3d = r_get_state();
  if (ctx->last_srv != srv)
  {
    ctx->last_srv = srv;
    d3d->context->VSSetShaderResources(0, 1, &srv);
    d3d->context->PSSetShaderResources(0, 1, &srv);
  }
}

void __d3d_IASetVertexBuffer(__D3D_Draw_context* ctx, ID3D11Buffer* buffer, UINT stride, UINT offset)
{
  D3D_State* d3d = r_get_state();
  if (ctx->last_vertex_buffer != buffer || ctx->last_vb_stride != stride || ctx->last_vb_offset != offset)
  {
    ctx->last_vertex_buffer = buffer;
    ctx->last_vb_stride     = stride;
    ctx->last_vb_offset     = offset;
    d3d->context->IASetVertexBuffers(0, 1, &buffer, &stride, &offset);
  }
}

void __d3d_IASetInputLayout(__D3D_Draw_context* ctx, ID3D11InputLayout* input_layout)
{
  D3D_State* d3d = r_get_state();
  if (ctx->last_input_layout != input_layout)
  {
    ctx->last_input_layout = input_layout;
    d3d->context->IASetInputLayout(input_layout);
  }
}

void __d3d_VSSetShader(__D3D_Draw_context* ctx, ID3D11VertexShader* v_shader)
{
  D3D_State* d3d = r_get_state();
  if (ctx->last_v_shader != v_shader)
  {
    ctx->last_v_shader = v_shader;
    d3d->context->VSSetShader(v_shader, Null, Null);
  }
}

void __d3d_PSSetShader(__D3D_Draw_context* ctx, ID3D11PixelShader* p_shader)
{
  D3D_State* d3d = r_get_state();
  if (ctx->last_p_shader != p_shader)
  {
    ctx->last_p_shader = p_shader;
    d3d->context->PSSetShader(p_shader, Null, Null);
  }
}

void __d3d_SetConstantBuffer(__D3D_Draw_context* ctx, ID3D11Buffer* constant_buffer)
{
  D3D_State* d3d = r_get_state();
  if (ctx->last_constant_buffer != constant_buffer)
  {
    ctx->last_constant_buffer = constant_buffer;
    d3d->context->VSSetConstantBuffers(0, 1, &constant_buffer);
    d3d->context->PSSetConstantBuffers(0, 1, &constant_buffer);
  }
}

void __d3d_PSSetSampler(__D3D_Draw_context* ctx, ID3D11SamplerState* sampler)
{
  D3D_State* d3d = r_get_state();
  if (ctx->last_sampler != sampler)
  {
    ctx->last_sampler = sampler;
    d3d->context->PSSetSamplers(0, 1, &sampler);
  }
}

#endif