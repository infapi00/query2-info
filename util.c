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

#include "util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <inttypes.h>  /* for PRIu64 macro */
#include "util-string.h"

/* Generic callback type, doing a cast of params to void*, to avoid
 * having two paths (32 and 64) for each check */
typedef void (*GetInternalformat)(GLenum target, GLenum internalformat,
                                  GLenum pname, GLsizei bufsize,
                                  void *params);

/* This struct is intended to abstract the fact that there are two
 * really similar methods, and two really similar params (just change
 * the type). All the castings and decision about which method should
 * be used would be done here, just to keep the code of the test
 * cleaner.
 */
struct _test_data {
   /* int instead of a bool to make easier iterate on the
    * possible values. */
   int testing64;
   int params_size;
   void *params;
   GetInternalformat callback;
};

/* Updates the callback and params based on current values of
 * testing64 and params_size */
static void
sync_test_data(test_data *data)
{
   if (data->params != NULL)
      free(data->params);

   if (data->testing64) {
      data->callback = (GetInternalformat) glGetInternalformati64v;
      data->params = malloc(sizeof(GLint64) * data->params_size);
   } else {
      data->callback = (GetInternalformat) glGetInternalformativ;
      data->params = malloc(sizeof(GLint) * data->params_size);
   }
}

test_data*
test_data_new(int testing64,
              int params_size)
{
   test_data *result;

   result = (test_data*) malloc(sizeof(test_data));
   result->testing64 = testing64;
   result->params_size = params_size;
   result->params = NULL;

   sync_test_data(result);

   return result;
}

/*
 * Frees @data, and sets its value to NULL.
 */
void
test_data_clear(test_data **data)
{
   test_data *_data = *data;

   if (_data == NULL)
      return;

   free(_data->params);
   _data->params = NULL;

   free(_data);
   *data = NULL;
}

void
test_data_execute(test_data *data,
                  const GLenum target,
                  const GLenum internalformat,
                  const GLenum pname)
{
   data->callback(target, internalformat, pname,
                  data->params_size, data->params);
}

void
test_data_set_testing64(test_data *data,
                        const int testing64)
{
   if (data->testing64 == testing64)
      return;

   data->testing64 = testing64;
   sync_test_data(data);
}

void
test_data_set_value_at_index(test_data *data,
                             const int index,
                             const GLint64 value)
{
   if (index > data->params_size || index < 0) {
      fprintf(stderr, "ERROR: invalid index while setting"
              " auxiliar test data\n");
      return;
   }

   if (data->testing64) {
      ((GLint64*)data->params)[index] = value;
   } else {
      ((GLint*)data->params)[index] = value;
   }
}

GLint64
test_data_value_at_index(const test_data *data,
                         const int index)
{
   if (index > data->params_size || index < 0) {
      fprintf(stderr, "ERROR: invalid index while retrieving"
              " data from auxiliar test data\n");
      return -1;
   }

   return data->testing64 ?
      ((GLint64*)data->params)[index] :
      ((GLint*)data->params)[index];
}

/*
 * Returns if @target/@internalformat is supported using
 * INTERNALFORMAT_SUPPORTED for @target and @internalformat.
 *
 * @data is only used to known if we are testing the 32-bit or the
 * 64-bit query, so the content of @data will not be modified due this
 * call.
 */
bool
test_data_check_supported(const test_data *data,
                          const GLenum target,
                          const GLenum internalformat)
{
   bool result;
   test_data *local_data = test_data_new(data->testing64, 1);

   test_data_execute(local_data, target, internalformat,
                     GL_INTERNALFORMAT_SUPPORTED);

   check_gl_error();
   result = test_data_value_at_index(local_data, 0) == GL_TRUE;

   test_data_clear(&local_data);

   return result;
}

/* There are cases where a pname is returning an already know GL enum
 * instead of a value. */
static bool
pname_returns_enum(const GLenum pname)
{
   switch (pname) {
   case GL_NUM_SAMPLE_COUNTS:
   case GL_SAMPLES:
   case GL_INTERNALFORMAT_RED_SIZE:
   case GL_INTERNALFORMAT_GREEN_SIZE:
   case GL_INTERNALFORMAT_BLUE_SIZE:
   case GL_INTERNALFORMAT_ALPHA_SIZE:
   case GL_INTERNALFORMAT_DEPTH_SIZE:
   case GL_INTERNALFORMAT_STENCIL_SIZE:
   case GL_INTERNALFORMAT_SHARED_SIZE:
   case GL_MAX_WIDTH:
   case GL_MAX_HEIGHT:
   case GL_MAX_DEPTH:
   case GL_MAX_LAYERS:
   case GL_MAX_COMBINED_DIMENSIONS:
   case GL_IMAGE_TEXEL_SIZE:
   case GL_TEXTURE_COMPRESSED_BLOCK_WIDTH:
   case GL_TEXTURE_COMPRESSED_BLOCK_HEIGHT:
   case GL_TEXTURE_COMPRESSED_BLOCK_SIZE:
      return false;
   default:
      return true;
   }
}

/* Needed to complement _pname_returns_enum because GL_POINTS/GL_FALSE
 * and GL_LINES/GL_TRUE has the same value */
static bool
pname_returns_gl_boolean(const GLenum pname)
{
   switch(pname) {
   case GL_INTERNALFORMAT_SUPPORTED:
   case GL_COLOR_COMPONENTS:
   case GL_DEPTH_COMPONENTS:
   case GL_STENCIL_COMPONENTS:
   case GL_COLOR_RENDERABLE:
   case GL_DEPTH_RENDERABLE:
   case GL_STENCIL_RENDERABLE:
   case GL_MIPMAP:
   case GL_TEXTURE_COMPRESSED:
      return true;
   default:
      return false;
   }
}

/* Needed because GL_NONE has the same value that GL_FALSE and GL_POINTS */
static bool
pname_can_return_gl_none(const GLenum pname)
{
   switch(pname) {
   case GL_INTERNALFORMAT_PREFERRED:
   case GL_INTERNALFORMAT_RED_TYPE:
   case GL_INTERNALFORMAT_GREEN_TYPE:
   case GL_INTERNALFORMAT_BLUE_TYPE:
   case GL_INTERNALFORMAT_ALPHA_TYPE:
   case GL_INTERNALFORMAT_DEPTH_TYPE:
   case GL_INTERNALFORMAT_STENCIL_TYPE:
   case GL_FRAMEBUFFER_RENDERABLE:
   case GL_FRAMEBUFFER_RENDERABLE_LAYERED:
   case GL_FRAMEBUFFER_BLEND:
   case GL_READ_PIXELS:
   case GL_READ_PIXELS_FORMAT:
   case GL_READ_PIXELS_TYPE:
   case GL_TEXTURE_IMAGE_FORMAT:
   case GL_TEXTURE_IMAGE_TYPE:
   case GL_GET_TEXTURE_IMAGE_FORMAT:
   case GL_GET_TEXTURE_IMAGE_TYPE:
   case GL_MANUAL_GENERATE_MIPMAP:
   case GL_AUTO_GENERATE_MIPMAP:
   case GL_COLOR_ENCODING:
   case GL_SRGB_READ:
   case GL_SRGB_WRITE:
   case GL_SRGB_DECODE_ARB:
   case GL_FILTER:
   case GL_VERTEX_TEXTURE:
   case GL_TESS_CONTROL_TEXTURE:
   case GL_TESS_EVALUATION_TEXTURE:
   case GL_GEOMETRY_TEXTURE:
   case GL_FRAGMENT_TEXTURE:
   case GL_COMPUTE_TEXTURE:
   case GL_TEXTURE_SHADOW:
   case GL_TEXTURE_GATHER:
   case GL_TEXTURE_GATHER_SHADOW:
   case GL_SHADER_IMAGE_LOAD:
   case GL_SHADER_IMAGE_STORE:
   case GL_SHADER_IMAGE_ATOMIC:
   case GL_IMAGE_COMPATIBILITY_CLASS:
   case GL_IMAGE_PIXEL_FORMAT:
   case GL_IMAGE_PIXEL_TYPE:
   case GL_IMAGE_FORMAT_COMPATIBILITY_TYPE:
   case GL_SIMULTANEOUS_TEXTURE_AND_DEPTH_TEST:
   case GL_SIMULTANEOUS_TEXTURE_AND_STENCIL_TEST:
   case GL_SIMULTANEOUS_TEXTURE_AND_DEPTH_WRITE:
   case GL_SIMULTANEOUS_TEXTURE_AND_STENCIL_WRITE:
   case GL_CLEAR_BUFFER:
   case GL_TEXTURE_VIEW:
   case GL_VIEW_COMPATIBILITY_CLASS:
      return true;
   default:
      return false;
   }
}

/* wrapper for GL_SAMPLE_COUNTS */
static GLint64
get_num_sample_counts(const GLenum target,
                      const GLenum internalformat)
{
   GLint64 result = -1;
   test_data *local_data = test_data_new(0, 1);

   test_data_execute(local_data, target, internalformat,
                     GL_NUM_SAMPLE_COUNTS);

   if (!check_gl_error())
      result = -1;
   else
      result = test_data_value_at_index(local_data, 0);

   test_data_clear(&local_data);

   return result;
}

static const char*
get_value_enum_name(const GLenum pname,
                    const GLint64 value)
{
   if (pname_returns_gl_boolean(pname))
      return value ? "GL_TRUE" : "GL_FALSE";
   else if (pname_can_return_gl_none(pname) && value == 0)
      return "GL_NONE";
   else
      return util_get_gl_enum_name(value);
}
/*
 * Returns the number of values that a given pname returns. For
 * example, for the case of GL_SAMPLES, it returns as many sample
 * counts as the valure returned by GL_SAMPLE_COUNTS
 */
static unsigned
pname_value_count(const GLenum pname,
                  const GLenum target,
                  const GLenum internalformat)
{
   switch (pname) {
   case GL_SAMPLES:
      return get_num_sample_counts(target, internalformat);
   default:
      return 1;
   }
}

/*
 * Prints the info of a case for a given pname, in a csv format. In order to
 * get that the value is included on "", as some queries returns more than one
 * value (ex: GL_SAMPLES).
 * @target, @internalformat, @pname are the parameters othe the given query.
 * @data contains the outcome of the query already being executed, so the given value.
 *
 */
void
print_case(const GLenum target,
           const GLenum internalformat,
           const GLenum pname,
           const test_data *data)
{
   fprintf(stdout, "%s, %s, %s, %s, ",
           data->testing64 ? "64 bit" : "32 bit",
           util_get_gl_enum_name(pname),
           util_get_gl_enum_name(target),
           util_get_gl_enum_name(internalformat));

   if (pname_returns_enum(pname)) {
      fprintf(stdout, "\"%s\"\n",
              get_value_enum_name(pname, test_data_value_at_index(data, 0)));
   } else {
      int count = pname_value_count(pname, target, internalformat);
      int i = 0;

      fprintf(stdout, "\"");
      for (i = 0; i < count - 1; i++) {
         fprintf(stdout, "%" PRIi64 ",", test_data_value_at_index(data, i));
      }
      fprintf(stdout, "%" PRIi64 "\"\n", test_data_value_at_index(data, i));
   }
}


/*
 * Checks for OpenGL error if one ocurred, and prints it. Retuns true if any
 * error found;
 */
bool
check_ogl_error (char *file,
                 int line)
{
   GLenum gl_err;
   bool result = false;

   gl_err = glGetError();
   while (gl_err != GL_NO_ERROR) {
      result = true;
      fprintf(stderr,"gl_error in file %s @ line %d: %s\n",
              file, line, gluErrorString(gl_err));

      gl_err = glGetError();
   }

   return result;
}
