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

#include "main/glheader.h"
#include "main/macros.h"
#include "main/enums.h"

#include "intel_batchbuffer.h"

#include "brw_defines.h"
#include "brw_context.h"
#include "brw_eu.h"
#include "brw_util.h"
#include "brw_state.h"
#include "brw_clip.h"


#define FRONT_UNFILLED_BIT  0x1
#define BACK_UNFILLED_BIT   0x2


static void compile_clip_prog( struct brw_context *brw,
			     struct brw_clip_prog_key *key )
{
   struct intel_context *intel = &brw->intel;
   struct brw_clip_compile c;
   const GLuint *program;
   GLuint program_size;
   GLuint delta;
   GLuint i;

   memset(&c, 0, sizeof(c));
   
   /* Begin the compilation:
    */
   brw_init_compile(brw, &c.func);

   c.func.single_program_flow = 1;

   c.key = *key;

   /* Need to locate the two positions present in vertex + header.
    * These are currently hardcoded:
    */
   c.header_position_offset = ATTR_SIZE;

   if (intel->is_ironlake)
       delta = 3 * REG_SIZE;
   else
       delta = REG_SIZE;

   for (i = 0; i < VERT_RESULT_MAX; i++)
      if (c.key.attrs & BITFIELD64_BIT(i)) {
	 c.offset[i] = delta;
	 delta += ATTR_SIZE;
      }

   c.nr_attrs = brw_count_bits(c.key.attrs);
   
   if (intel->is_ironlake)
       c.nr_regs = (c.nr_attrs + 1) / 2 + 3;  /* are vertices packed, or reg-aligned? */
   else
       c.nr_regs = (c.nr_attrs + 1) / 2 + 1;  /* are vertices packed, or reg-aligned? */

   c.nr_bytes = c.nr_regs * REG_SIZE;

   c.prog_data.clip_mode = c.key.clip_mode; /* XXX */

   /* For some reason the thread is spawned with only 4 channels
    * unmasked.  
    */
   brw_set_mask_control(&c.func, BRW_MASK_DISABLE);


   /* Would ideally have the option of producing a program which could
    * do all three:
    */
   switch (key->primitive) {
   case GL_TRIANGLES: 
      if (key->do_unfilled)
	 brw_emit_unfilled_clip( &c );
      else
	 brw_emit_tri_clip( &c );
      break;
   case GL_LINES:
      brw_emit_line_clip( &c );
      break;
   case GL_POINTS:
      brw_emit_point_clip( &c );
      break;
   default:
      assert(0);
      return;
   }

	 

   /* get the program
    */
   program = brw_get_program(&c.func, &program_size);

   /* Upload
    */
   dri_bo_unreference(brw->clip.prog_bo);
   brw->clip.prog_bo = brw_upload_cache_with_auxdata(&brw->cache,
						     BRW_CLIP_PROG,
						     &c.key, sizeof(c.key),
						     NULL, 0,
						     program, program_size,
						     &c.prog_data,
						     sizeof(c.prog_data),
						     &brw->clip.prog_data);
}

/* Calculate interpolants for triangle and line rasterization.
 */
static void upload_clip_prog(struct brw_context *brw)
{
   struct intel_context *intel = &brw->intel;
   GLcontext *ctx = &intel->ctx;
   struct brw_clip_prog_key key;

   memset(&key, 0, sizeof(key));

   /* Populate the key:
    */
   /* BRW_NEW_REDUCED_PRIMITIVE */
   key.primitive = brw->intel.reduced_primitive;
   /* CACHE_NEW_VS_PROG */
   key.attrs = brw->vs.prog_data->outputs_written;
   /* _NEW_LIGHT */
   key.do_flat_shading = (ctx->Light.ShadeModel == GL_FLAT);
   key.pv_first = (ctx->Light.ProvokingVertex == GL_FIRST_VERTEX_CONVENTION);
   /* _NEW_TRANSFORM */
   key.nr_userclip = brw_count_bits(ctx->Transform.ClipPlanesEnabled);

   if (intel->is_ironlake)
       key.clip_mode = BRW_CLIPMODE_KERNEL_CLIP;
   else
       key.clip_mode = BRW_CLIPMODE_NORMAL;

   /* _NEW_POLYGON */
   if (key.primitive == GL_TRIANGLES) {
      if (ctx->Polygon.CullFlag &&
	  ctx->Polygon.CullFaceMode == GL_FRONT_AND_BACK)
	 key.clip_mode = BRW_CLIPMODE_REJECT_ALL;
      else {
	 GLuint fill_front = CLIP_CULL;
	 GLuint fill_back = CLIP_CULL;
	 GLuint offset_front = 0;
	 GLuint offset_back = 0;

	 if (!ctx->Polygon.CullFlag ||
	     ctx->Polygon.CullFaceMode != GL_FRONT) {
	    switch (ctx->Polygon.FrontMode) {
	    case GL_FILL: 
	       fill_front = CLIP_FILL; 
	       offset_front = 0;
	       break;
	    case GL_LINE:
	       fill_front = CLIP_LINE;
	       offset_front = ctx->Polygon.OffsetLine;
	       break;
	    case GL_POINT:
	       fill_front = CLIP_POINT;
	       offset_front = ctx->Polygon.OffsetPoint;
	       break;
	    }
	 }

	 if (!ctx->Polygon.CullFlag ||
	     ctx->Polygon.CullFaceMode != GL_BACK) {
	    switch (ctx->Polygon.BackMode) {
	    case GL_FILL: 
	       fill_back = CLIP_FILL; 
	       offset_back = 0;
	       break;
	    case GL_LINE:
	       fill_back = CLIP_LINE;
	       offset_back = ctx->Polygon.OffsetLine;
	       break;
	    case GL_POINT:
	       fill_back = CLIP_POINT;
	       offset_back = ctx->Polygon.OffsetPoint;
	       break;
	    }
	 }

	 if (ctx->Polygon.BackMode != GL_FILL ||
	     ctx->Polygon.FrontMode != GL_FILL) {
	    key.do_unfilled = 1;

	    /* Most cases the fixed function units will handle.  Cases where
	     * one or more polygon faces are unfilled will require help:
	     */
	    key.clip_mode = BRW_CLIPMODE_CLIP_NON_REJECTED;

	    if (offset_back || offset_front) {
	       /* _NEW_POLYGON, _NEW_BUFFERS */
	       key.offset_units = ctx->Polygon.OffsetUnits * brw->intel.polygon_offset_scale;
	       key.offset_factor = ctx->Polygon.OffsetFactor * ctx->DrawBuffer->_MRD;
	    }

	    switch (ctx->Polygon.FrontFace) {
	    case GL_CCW:
	       key.fill_ccw = fill_front;
	       key.fill_cw = fill_back;
	       key.offset_ccw = offset_front;
	       key.offset_cw = offset_back;
	       if (ctx->Light.Model.TwoSide &&
		   key.fill_cw != CLIP_CULL) 
		  key.copy_bfc_cw = 1;
	       break;
	    case GL_CW:
	       key.fill_cw = fill_front;
	       key.fill_ccw = fill_back;
	       key.offset_cw = offset_front;
	       key.offset_ccw = offset_back;
	       if (ctx->Light.Model.TwoSide &&
		   key.fill_ccw != CLIP_CULL) 
		  key.copy_bfc_ccw = 1;
	       break;
	    }
	 }
      }
   }

   dri_bo_unreference(brw->clip.prog_bo);
   brw->clip.prog_bo = brw_search_cache(&brw->cache, BRW_CLIP_PROG,
					&key, sizeof(key),
					NULL, 0,
					&brw->clip.prog_data);
   if (brw->clip.prog_bo == NULL)
      compile_clip_prog( brw, &key );
}


const struct brw_tracked_state brw_clip_prog = {
   .dirty = {
      .mesa  = (_NEW_LIGHT | 
		_NEW_TRANSFORM |
		_NEW_POLYGON | 
		_NEW_BUFFERS),
      .brw   = (BRW_NEW_REDUCED_PRIMITIVE),
      .cache = CACHE_NEW_VS_PROG
   },
   .prepare = upload_clip_prog
};
