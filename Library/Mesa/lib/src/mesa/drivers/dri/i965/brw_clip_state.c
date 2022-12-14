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

#include "brw_context.h"
#include "brw_state.h"
#include "brw_defines.h"

struct brw_clip_unit_key {
   unsigned int total_grf;
   unsigned int urb_entry_read_length;
   unsigned int curb_entry_read_length;
   unsigned int clip_mode;

   unsigned int curbe_offset;

   unsigned int nr_urb_entries, urb_size;

   GLboolean depth_clamp;
};

static void
clip_unit_populate_key(struct brw_context *brw, struct brw_clip_unit_key *key)
{
   GLcontext *ctx = &brw->intel.ctx;
   memset(key, 0, sizeof(*key));

   /* CACHE_NEW_CLIP_PROG */
   key->total_grf = brw->clip.prog_data->total_grf;
   key->urb_entry_read_length = brw->clip.prog_data->urb_read_length;
   key->curb_entry_read_length = brw->clip.prog_data->curb_read_length;
   key->clip_mode = brw->clip.prog_data->clip_mode;

   /* BRW_NEW_CURBE_OFFSETS */
   key->curbe_offset = brw->curbe.clip_start;

   /* BRW_NEW_URB_FENCE */
   key->nr_urb_entries = brw->urb.nr_clip_entries;
   key->urb_size = brw->urb.vsize;

   /* _NEW_TRANSOFORM */
   key->depth_clamp = ctx->Transform.DepthClamp;
}

static dri_bo *
clip_unit_create_from_key(struct brw_context *brw,
			  struct brw_clip_unit_key *key)
{
   struct intel_context *intel = &brw->intel;
   struct brw_clip_unit_state clip;
   dri_bo *bo;

   memset(&clip, 0, sizeof(clip));

   clip.thread0.grf_reg_count = ALIGN(key->total_grf, 16) / 16 - 1;
   /* reloc */
   clip.thread0.kernel_start_pointer = brw->clip.prog_bo->offset >> 6;

   clip.thread1.floating_point_mode = BRW_FLOATING_POINT_NON_IEEE_754;
   clip.thread1.single_program_flow = 1;

   clip.thread3.urb_entry_read_length = key->urb_entry_read_length;
   clip.thread3.const_urb_entry_read_length = key->curb_entry_read_length;
   clip.thread3.const_urb_entry_read_offset = key->curbe_offset * 2;
   clip.thread3.dispatch_grf_start_reg = 1;
   clip.thread3.urb_entry_read_offset = 0;

   clip.thread4.nr_urb_entries = key->nr_urb_entries;
   clip.thread4.urb_entry_allocation_size = key->urb_size - 1;
   /* If we have enough clip URB entries to run two threads, do so.
    */
   if (key->nr_urb_entries >= 10) {
      /* Half of the URB entries go to each thread, and it has to be an
       * even number.
       */
      assert(key->nr_urb_entries % 2 == 0);
      
      /* Although up to 16 concurrent Clip threads are allowed on IGDNG, 
       * only 2 threads can output VUEs at a time.
       */
      if (intel->is_ironlake)
         clip.thread4.max_threads = 16 - 1;        
      else
         clip.thread4.max_threads = 2 - 1;
   } else {
      assert(key->nr_urb_entries >= 5);
      clip.thread4.max_threads = 1 - 1;
   }

   if (INTEL_DEBUG & DEBUG_SINGLE_THREAD)
      clip.thread4.max_threads = 0;

   if (INTEL_DEBUG & DEBUG_STATS)
      clip.thread4.stats_enable = 1;

   clip.clip5.userclip_enable_flags = 0x7f;
   clip.clip5.userclip_must_clip = 1;
   clip.clip5.guard_band_enable = 0;
   if (!key->depth_clamp)
      clip.clip5.viewport_z_clip_enable = 1;
   clip.clip5.viewport_xy_clip_enable = 1;
   clip.clip5.vertex_position_space = BRW_CLIP_NDCSPACE;
   clip.clip5.api_mode = BRW_CLIP_API_OGL;
   clip.clip5.clip_mode = key->clip_mode;

   if (intel->is_g4x)
      clip.clip5.negative_w_clip_test = 1;

   clip.clip6.clipper_viewport_state_ptr = 0;
   clip.viewport_xmin = -1;
   clip.viewport_xmax = 1;
   clip.viewport_ymin = -1;
   clip.viewport_ymax = 1;

   bo = brw_upload_cache(&brw->cache, BRW_CLIP_UNIT,
			 key, sizeof(*key),
			 &brw->clip.prog_bo, 1,
			 &clip, sizeof(clip));

   /* Emit clip program relocation */
   assert(brw->clip.prog_bo);
   dri_bo_emit_reloc(bo,
		     I915_GEM_DOMAIN_INSTRUCTION,
		     0,
		     clip.thread0.grf_reg_count << 1,
		     offsetof(struct brw_clip_unit_state, thread0),
		     brw->clip.prog_bo);

   return bo;
}

static void upload_clip_unit( struct brw_context *brw )
{
   struct brw_clip_unit_key key;

   clip_unit_populate_key(brw, &key);

   dri_bo_unreference(brw->clip.state_bo);
   brw->clip.state_bo = brw_search_cache(&brw->cache, BRW_CLIP_UNIT,
					 &key, sizeof(key),
					 &brw->clip.prog_bo, 1,
					 NULL);
   if (brw->clip.state_bo == NULL) {
      brw->clip.state_bo = clip_unit_create_from_key(brw, &key);
   }
}

const struct brw_tracked_state brw_clip_unit = {
   .dirty = {
      .mesa  = _NEW_TRANSFORM,
      .brw   = (BRW_NEW_CURBE_OFFSETS |
		BRW_NEW_URB_FENCE),
      .cache = CACHE_NEW_CLIP_PROG
   },
   .prepare = upload_clip_unit,
};
