/*
 Copyright (C) Intel Corp.  2006.  All Rights Reserved.
 Intel funded Tungsten Graphics (http://www.tungstengraphics.com) to
 develop this 3D driver.
 
 Permission is hereby granted, free of charge, to any person obtaining
 a copy of this software and associated documentation files (the
 "Software"), to deal in the Software without restriction, including
 without limitation the rights to use, copy, modify, merge, publish,
 distribute, sublicense, and/or sell copies of the Software, and to
 permit persons to whom the Software is furnished to do so, subject to
 the following conditions:
 
 The above copyright notice and this permission notice (including the
 next paragraph) shall be included in all copies or substantial
 portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
 LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 
 **********************************************************************/
 /*
  * Authors:
  *   Keith Whitwell <keith@tungstengraphics.com>
  */

/* Code to layout images in a mipmap tree for i965.
 */

#include "intel_mipmap_tree.h"
#include "intel_tex_layout.h"
#include "intel_context.h"
#include "main/macros.h"

#define FILE_DEBUG_FLAG DEBUG_MIPTREE

GLboolean brw_miptree_layout(struct intel_context *intel,
			     struct intel_mipmap_tree *mt,
			     uint32_t tiling)
{
   /* XXX: these vary depending on image format: */
   /* GLint align_w = 4; */

   switch (mt->target) {
   case GL_TEXTURE_CUBE_MAP:
      if (intel->is_ironlake) {
          GLuint align_h = 2, align_w = 4;
          GLuint level;
          GLuint x = 0;
          GLuint y = 0;
          GLuint width = mt->width0;
          GLuint height = mt->height0;
          GLuint qpitch = 0;
          GLuint y_pitch = 0;

          mt->pitch = mt->width0;
          intel_get_texture_alignment_unit(mt->internal_format, &align_w, &align_h);
          y_pitch = ALIGN(height, align_h);

          if (mt->compressed) {
              mt->pitch = ALIGN(mt->width0, align_w);
          }

          if (mt->first_level != mt->last_level) {
              GLuint mip1_width;

              if (mt->compressed) {
                  mip1_width = ALIGN(minify(mt->width0), align_w)
                      + ALIGN(minify(minify(mt->width0)), align_w);
              } else {
                  mip1_width = ALIGN(minify(mt->width0), align_w)
                      + minify(minify(mt->width0));
              }

              if (mip1_width > mt->pitch) {
                  mt->pitch = mip1_width;
              }
          }

          mt->pitch = intel_miptree_pitch_align(intel, mt, tiling, mt->pitch);

          if (mt->compressed) {
              qpitch = (y_pitch + ALIGN(minify(y_pitch), align_h) + 11 * align_h) / 4;
              mt->total_height = (y_pitch + ALIGN(minify(y_pitch), align_h) + 11 * align_h) / 4 * 6;
          } else {
              qpitch = (y_pitch + ALIGN(minify(y_pitch), align_h) + 11 * align_h);
              mt->total_height = (y_pitch + ALIGN(minify(y_pitch), align_h) + 11 * align_h) * 6;
          }

          for (level = mt->first_level; level <= mt->last_level; level++) {
              GLuint img_height;
              GLuint nr_images = 6;
              GLuint q = 0;

              intel_miptree_set_level_info(mt, level, nr_images, x, y, width, 
                                           height, 1);

              for (q = 0; q < nr_images; q++)
                  intel_miptree_set_image_offset(mt, level, q,
						 x, y + q * qpitch);

              if (mt->compressed)
                  img_height = MAX2(1, height/4);
              else
                  img_height = ALIGN(height, align_h);

              if (level == mt->first_level + 1) {
                  x += ALIGN(width, align_w);
              }
              else {
                  y += img_height;
              }

              width  = minify(width);
              height = minify(height);
          }

          break;
      }

   case GL_TEXTURE_3D: {
      GLuint width  = mt->width0;
      GLuint height = mt->height0;
      GLuint depth = mt->depth0;
      GLuint pack_x_pitch, pack_x_nr;
      GLuint pack_y_pitch;
      GLuint level;
      GLuint align_h = 2;
      GLuint align_w = 4;

      mt->total_height = 0;
      intel_get_texture_alignment_unit(mt->internal_format, &align_w, &align_h);

      if (mt->compressed) {
          mt->pitch = ALIGN(width, align_w);
          pack_y_pitch = (height + 3) / 4;
      } else {
	 mt->pitch = intel_miptree_pitch_align (intel, mt, tiling, mt->width0);
	 pack_y_pitch = ALIGN(mt->height0, align_h);
      }

      pack_x_pitch = width;
      pack_x_nr = 1;

      for (level = mt->first_level ; level <= mt->last_level ; level++) {
	 GLuint nr_images = mt->target == GL_TEXTURE_3D ? depth : 6;
	 GLint x = 0;
	 GLint y = 0;
	 GLint q, j;

	 intel_miptree_set_level_info(mt, level, nr_images,
				      0, mt->total_height,
				      width, height, depth);

	 for (q = 0; q < nr_images;) {
	    for (j = 0; j < pack_x_nr && q < nr_images; j++, q++) {
	       intel_miptree_set_image_offset(mt, level, q, x, y);
	       x += pack_x_pitch;
	    }

	    x = 0;
	    y += pack_y_pitch;
	 }


	 mt->total_height += y;
	 width  = minify(width);
	 height = minify(height);
	 depth  = minify(depth);

	 if (mt->compressed) {
	    pack_y_pitch = (height + 3) / 4;

	    if (pack_x_pitch > ALIGN(width, align_w)) {
	       pack_x_pitch = ALIGN(width, align_w);
	       pack_x_nr <<= 1;
	    }
	 } else {
	    if (pack_x_pitch > 4) {
	       pack_x_pitch >>= 1;
	       pack_x_nr <<= 1;
	       assert(pack_x_pitch * pack_x_nr <= mt->pitch);
	    }

	    if (pack_y_pitch > 2) {
	       pack_y_pitch >>= 1;
	       pack_y_pitch = ALIGN(pack_y_pitch, align_h);
	    }
	 }

      }
      /* The 965's sampler lays cachelines out according to how accesses
       * in the texture surfaces run, so they may be "vertical" through
       * memory.  As a result, the docs say in Surface Padding Requirements:
       * Sampling Engine Surfaces that two extra rows of padding are required.
       * We don't know of similar requirements for pre-965, but given that
       * those docs are silent on padding requirements in general, let's play
       * it safe.
       */
      if (mt->target == GL_TEXTURE_CUBE_MAP)
	 mt->total_height += 2;
      break;
   }

   default:
      i945_miptree_layout_2d(intel, mt, tiling);
      break;
   }
   DBG("%s: %dx%dx%d - sz 0x%x\n", __FUNCTION__,
		mt->pitch,
		mt->total_height,
		mt->cpp,
		mt->pitch * mt->total_height * mt->cpp );

   return GL_TRUE;
}

