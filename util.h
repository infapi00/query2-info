/*
 * Copyright © 2016 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */
#include <GL/glew.h>
#include <stdbool.h>

static const GLenum valid_targets[] = {
   GL_TEXTURE_1D,
   GL_TEXTURE_1D_ARRAY,
   GL_TEXTURE_2D,
   GL_TEXTURE_2D_ARRAY,
   GL_TEXTURE_3D,
   GL_TEXTURE_CUBE_MAP,
   GL_TEXTURE_CUBE_MAP_ARRAY,
   GL_TEXTURE_RECTANGLE,
   GL_TEXTURE_BUFFER,
   GL_RENDERBUFFER,
   GL_TEXTURE_2D_MULTISAMPLE,
   GL_TEXTURE_2D_MULTISAMPLE_ARRAY,
};

static const GLenum valid_pnames[] = {
   GL_SAMPLES,
   GL_NUM_SAMPLE_COUNTS,
   GL_INTERNALFORMAT_SUPPORTED,
   GL_INTERNALFORMAT_PREFERRED,
   GL_INTERNALFORMAT_RED_SIZE,
   GL_INTERNALFORMAT_GREEN_SIZE,
   GL_INTERNALFORMAT_BLUE_SIZE,
   GL_INTERNALFORMAT_ALPHA_SIZE,
   GL_INTERNALFORMAT_DEPTH_SIZE,
   GL_INTERNALFORMAT_STENCIL_SIZE,
   GL_INTERNALFORMAT_SHARED_SIZE,
   GL_INTERNALFORMAT_RED_TYPE,
   GL_INTERNALFORMAT_GREEN_TYPE,
   GL_INTERNALFORMAT_BLUE_TYPE,
   GL_INTERNALFORMAT_ALPHA_TYPE,
   GL_INTERNALFORMAT_DEPTH_TYPE,
   GL_INTERNALFORMAT_STENCIL_TYPE,
   GL_MAX_WIDTH,
   GL_MAX_HEIGHT,
   GL_MAX_DEPTH,
   GL_MAX_LAYERS,
   GL_MAX_COMBINED_DIMENSIONS,
   GL_COLOR_COMPONENTS,
   GL_DEPTH_COMPONENTS,
   GL_STENCIL_COMPONENTS,
   GL_COLOR_RENDERABLE,
   GL_DEPTH_RENDERABLE,
   GL_STENCIL_RENDERABLE,
   GL_FRAMEBUFFER_RENDERABLE,
   GL_FRAMEBUFFER_RENDERABLE_LAYERED,
   GL_FRAMEBUFFER_BLEND,
   GL_READ_PIXELS,
   GL_READ_PIXELS_FORMAT,
   GL_READ_PIXELS_TYPE,
   GL_TEXTURE_IMAGE_FORMAT,
   GL_TEXTURE_IMAGE_TYPE,
   GL_GET_TEXTURE_IMAGE_FORMAT,
   GL_GET_TEXTURE_IMAGE_TYPE,
   GL_MIPMAP,
   GL_MANUAL_GENERATE_MIPMAP,
   GL_AUTO_GENERATE_MIPMAP,
   GL_COLOR_ENCODING,
   GL_SRGB_READ,
   GL_SRGB_WRITE,
   GL_SRGB_DECODE_ARB,
   GL_FILTER,
   GL_VERTEX_TEXTURE,
   GL_TESS_CONTROL_TEXTURE,
   GL_TESS_EVALUATION_TEXTURE,
   GL_GEOMETRY_TEXTURE,
   GL_FRAGMENT_TEXTURE,
   GL_COMPUTE_TEXTURE,
   GL_TEXTURE_SHADOW,
   GL_TEXTURE_GATHER,
   GL_TEXTURE_GATHER_SHADOW,
   GL_SHADER_IMAGE_LOAD,
   GL_SHADER_IMAGE_STORE,
   GL_SHADER_IMAGE_ATOMIC,
   GL_IMAGE_TEXEL_SIZE,
   GL_IMAGE_COMPATIBILITY_CLASS,
   GL_IMAGE_PIXEL_FORMAT,
   GL_IMAGE_PIXEL_TYPE,
   GL_IMAGE_FORMAT_COMPATIBILITY_TYPE,
   GL_SIMULTANEOUS_TEXTURE_AND_DEPTH_TEST,
   GL_SIMULTANEOUS_TEXTURE_AND_STENCIL_TEST,
   GL_SIMULTANEOUS_TEXTURE_AND_DEPTH_WRITE,
   GL_SIMULTANEOUS_TEXTURE_AND_STENCIL_WRITE,
   GL_TEXTURE_COMPRESSED,
   GL_TEXTURE_COMPRESSED_BLOCK_WIDTH,
   GL_TEXTURE_COMPRESSED_BLOCK_HEIGHT,
   GL_TEXTURE_COMPRESSED_BLOCK_SIZE,
   GL_CLEAR_BUFFER,
   GL_TEXTURE_VIEW,
   GL_VIEW_COMPATIBILITY_CLASS
};

/* From spec:
 *
 *  "INTERNALFORMAT_SUPPORTED:
 *  <skip>
 *
 * <internalformats> that must be supported (in GL 4.2 or later)
 *   include the following:
 *    - "sized internal formats" from Table 3.12, 3.13, and 3.15,
 *    - any specific "compressed internal format" from Table 3.14,
 *    - any "image unit format" from Table 3.21.
 *    - any generic "compressed internal format" from Table 3.14, if
 *      the implementation accepts it for any texture specification
 *      commands, and
 *    - unsized or base internal format, if the implementation accepts
 *      it for texture or image specification."
 */
static const GLenum valid_internalformats[] = {
   /* Base/unsized internal format (from Table 3.11) */
   GL_DEPTH_COMPONENT,
   GL_DEPTH_STENCIL,
   GL_RED,
   GL_RG,
   GL_RGB,
   GL_RGBA,
   /* Table 3.12 (Table 3.15 and 3.21 included here) */
   GL_R8,
   GL_R8_SNORM,
   GL_R16,
   GL_R16_SNORM,
   GL_RG8,
   GL_RG8_SNORM,
   GL_RG16,
   GL_RG16_SNORM,
   GL_R3_G3_B2,
   GL_RGB4,
   GL_RGB5,
   GL_RGB8,
   GL_RGB8_SNORM,
   GL_RGB10,
   GL_RGB12,
   GL_RGB16,
   GL_RGB16_SNORM,
   GL_RGBA2,
   GL_RGBA4,
   GL_RGB5_A1,
   GL_RGBA8,
   GL_RGBA8_SNORM,
   GL_RGB10_A2,
   GL_RGB10_A2UI,
   GL_RGBA12,
   GL_RGBA16,
   GL_RGBA16_SNORM,
   GL_SRGB8,
   GL_SRGB8_ALPHA8,
   GL_R16F,
   GL_RG16F,
   GL_RGB16F,
   GL_RGBA16F,
   GL_R32F,
   GL_RG32F,
   GL_RGB32F,
   GL_RGBA32F,
   GL_R11F_G11F_B10F,
   GL_RGB9_E5,
   GL_R8I,
   GL_R8UI,
   GL_R16I,
   GL_R16UI,
   GL_R32I,
   GL_R32UI,
   GL_RG8I,
   GL_RG16I,
   GL_RG16UI,
   GL_RG32I,
   GL_RG32UI,
   GL_RGB8I,
   GL_RGB8UI,
   GL_RGB16I,
   GL_RGB16UI,
   GL_RGB32I,
   GL_RGB32UI,
   GL_RGBA8I,
   GL_RGBA8UI,
   GL_RGBA16I,
   GL_RGBA16UI,
   GL_RGBA32I,
   GL_RGBA32UI,
   /* Table 3.13 */
   GL_DEPTH_COMPONENT16,
   GL_DEPTH_COMPONENT24,
   GL_DEPTH_COMPONENT32,
   GL_DEPTH_COMPONENT32F,
   GL_DEPTH24_STENCIL8,
   GL_DEPTH32F_STENCIL8,
   /* Table 3.14 (both specific and generic) */
   GL_COMPRESSED_RED,
   GL_COMPRESSED_RG,
   GL_COMPRESSED_RGB,
   GL_COMPRESSED_RGBA,
   GL_COMPRESSED_SRGB,
   GL_COMPRESSED_SRGB_ALPHA,
   GL_COMPRESSED_RED_RGTC1,
   GL_COMPRESSED_SIGNED_RED_RGTC1,
   GL_COMPRESSED_RG_RGTC2,
   GL_COMPRESSED_SIGNED_RG_RGTC2,
   GL_COMPRESSED_RGBA_BPTC_UNORM,
   GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM,
   GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT,
   GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT,
};

#define check_gl_error() check_ogl_error(__FILE__, __LINE__)
bool check_ogl_error(char *file,
                     int line);

typedef struct _test_data test_data;

test_data* test_data_new(int testing64,
                         int params_size);

void test_data_clear(test_data **data);

void test_data_execute(test_data *data,
                       const GLenum target,
                       const GLenum internalformat,
                       const GLenum pname);

void test_data_set_testing64(test_data *data,
                             const int testing64);

GLint64 test_data_value_at_index(const test_data *data,
                                 const int index);

void test_data_set_value_at_index(test_data *data,
                                  const int index,
                                  const GLint64 value);

bool test_data_check_supported(const test_data *data,
                               const GLenum target,
                               const GLenum internalformat);

void print_case(const GLenum target,
                const GLenum internalformat,
                const GLenum pname,
                const test_data *data);

