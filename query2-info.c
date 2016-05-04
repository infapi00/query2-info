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

/**
 *
 * This program prints the outcome of calling glGetInternalformati*v
 * with all the possible combinations of pname/target/internalformat.
 *
 * Command line optios:
 *  -pname <pname>: Prints info for only that pname (numeric value).
 *  -b:             Prints info using (b)oth 32 and 64 bit queries. By default
 *                  it only uses the 64-bit one.
 *  -f:             Prints info (f)iltering out the unsupported internalformat.
 *  -h:             Prints help.
 *
 * Note that the filtering option is based on internalformat being supported
 * or not, not on the combination of pname/target/internalformat being
 * supported or not. In practice, is filter out based on the value returned by
 * the pname GL_INTERNALFORMAT_SUPPORTED.
 *
 * Most of the code is based on the piglit tests for the extension
 * ARB_internalformat_query2
 *
 * Author: Alejandro Piñeiro Iglesias <apinheiro@igalia.com>
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <inttypes.h>  /* for PRIu64 macro */

#include <GL/glew.h>

#include "glut_wrap.h"
#include "util.h"

#define WINDOW_WIDTH    640
#define WINDOW_HEIGHT   480

#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))

int filter_supported = 0;
int only_64bit_query = 1;
int just_one_pname = 0;
GLenum global_pname = 0;

static void
init(int argc, char *argv[])
{
   glutInit(&argc, argv);
   glutInitWindowPosition(100, 0);
   /* FIXME: check for freeglut and use initcontexprofile */
   glutInitWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);
   glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE);

   if (glutCreateWindow(argv[0]) == GL_FALSE) {
      fprintf(stderr, "Error creating glut window.\n");
      exit(1);
   }

   glewExperimental = GL_TRUE;
   GLenum err = glewInit();
   if (GLEW_OK != err) {
      fprintf(stderr, "Error calling glewInit(): %s\n", glewGetErrorString(err));
      exit(1);
   }
}

static bool
check_pname(const GLenum pname)
{
   int i;
   for (i = 0; i < ARRAY_SIZE(valid_pnames); i++) {
     if (pname == valid_pnames[i])
       return true;
   }
   return false;
}

static void
print_usage(void)
{
   printf("Usage: query2-info [-a] [-f] [-h] [-pname <pname>]\n");
   printf("\t-pname <pname>: Prints info for only that pname (numeric value).\n");
   printf("\t-b: Prints info using (b)oth 32 and 64 bit queries. "
          "By default it only uses the 64-bit one.\n");
   printf("\t-f: Prints info (f)iltering out the unsupported internalformat.\n");
   printf("\t\tNOTE: the filtering is based on internalformat being supported"
          " or not,\n\t\tnot on the combination of pname/target/internalformat being "
          "supported or not.\n");
   printf("\t-h: This information.\n");
}

static void
parse_args(int argc, char **argv)
{
   int i;

   for (i = 1; i < argc; i++) {
      if (strcmp(argv[i], "-pname") == 0 && i + 1 < argc) {
         global_pname = atoi(argv[i + 1]);
         if (!check_pname(global_pname)) {
            printf("Value `%i' is not a valid <pname> for "
                   "GetInternalformati*v.\n", global_pname);
            print_usage();
            exit(0);
         }
         just_one_pname = true;
         i++;
      } else if (strcmp(argv[i], "-f") == 0) {
         filter_supported = true;
      } else if (strcmp(argv[i], "-b") == 0) {
         only_64bit_query = 0;
      } else if (strcmp(argv[i], "-h") == 0) {
         print_usage();
         exit(0);
      } else {
         printf("Unknown option `%s'\n", argv[i]);
         print_usage();
         exit(0);
      }
   }
}

/*
 * Print all the values for a given pname.
 */
static void
print_pname_values(const GLenum *targets, unsigned num_targets,
                   const GLenum *internalformats, unsigned num_internalformats,
                   const GLenum pname,
                   test_data *data)
{
   unsigned i;
   unsigned j;

   for (i = 0; i < num_targets; i++) {
      for (j = 0; j < num_internalformats; j++) {
         bool filter;

         filter = filter_supported ?
            !test_data_check_supported(data, targets[i], internalformats[j]) :
            false;

         if (filter)
            continue;

         /* Some queries will not modify params if unsupported. Use -1 as
          * reference value. */
         test_data_set_value_at_index(data, 0, -1);
         test_data_execute(data, targets[i], internalformats[j], pname);

         check_gl_error();

         print_case(targets[i], internalformats[j],
                    pname, data);
      }
   }

}

int
main(int argc,
     char *argv[])
{
   test_data *data;
   GLenum pname;
   int testing64;

   parse_args(argc, argv);

   init(argc, argv);

   if (!glewIsSupported("GL_ARB_internalformat_query2")) {
      printf("GL_ARB_internalformat_query2 extension not found\n");
      exit(1);
   }

   /* Note that we need to create test_data after initialization, as
    * glGetInternalformat*v methods are not available until we call glewInit */
   data = test_data_new(0, 64);
   for (unsigned i = 0; i < ARRAY_SIZE(valid_pnames); i++) {
      pname = valid_pnames[i];

      /* Not really the optimal, but do their work */
      if (just_one_pname && global_pname != pname)
         continue;

      for (testing64 = only_64bit_query; testing64 <= 1; testing64++) {
         test_data_set_testing64(data, testing64);
         print_pname_values(valid_targets, ARRAY_SIZE(valid_targets),
                            valid_internalformats, ARRAY_SIZE(valid_internalformats),
                            pname, data);
      }
   }

   test_data_clear(&data);

   return 0;
}
