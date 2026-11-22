/************************************************************************
 *   insanemap GIMP plugin
 *
 *   Copyright (C) 2002-2008 Shawn Kirst <skirst@insightbb.com>
 *   Copyright (C) 2013 Omar Emad
 *   Copyright (C) 2013 Derby Russell <jdrussell51@gmail.com>
 *
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU General Public
 *   License as published by the Free Software Foundation; either
 *   version 2 of the License, or (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *   General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; see the file COPYING.  If not, write to
 *   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 *   Boston, MA 02111-1307, USA.
 ************************************************************************/

#ifndef PLUG_IN_CONNECTORS_HEADER_GUARD
#define PLUG_IN_CONNECTORS_HEADER_GUARD

#include <glib/gstdio.h>
#include <libgimp/gimp.h>

gint32 plug_in_colors_channel_mixer_connector(GimpImage *image_ID,
             GimpDrawable *drawable_ID, gint32 isMonochrome,
             gfloat rr_gain, gfloat rg_gain, gfloat rb_gain,
             gfloat gr_gain, gfloat gg_gain, gfloat gb_gain,
             gfloat br_gain, gfloat bg_gain, gfloat bb_gain);
gint32 plug_in_dog_connector(GimpImage *image_ID, GimpDrawable *newlayer_ID, gfloat inner, gfloat outer, gint32 isNormalize, gint32 isInvert);
gint32 plug_in_gauss_connector(GimpImage *image_ID, GimpDrawable *newlayer_ID, gfloat horizontal, gfloat vertical, gint32 isMethod);
gint32 plug_in_vinvert_connector(GimpImage *image_ID, GimpDrawable *newlayer_ID);
gint32 plug_in_rgb_noise_connector(GimpImage *image_ID, GimpDrawable *noiselayer_ID, gint32 independent,
             gint32 correlated, gfloat noise_1, gfloat noise_2, gfloat noise_3, gfloat noise_4);
gint32 plug_in_make_seamless_connector(GimpImage *image_ID, GimpDrawable *diffuse_ID);
gint32 plug_in_blur_connector(GimpImage *image_ID, GimpDrawable *normalmap_ID);
gint32 plug_in_sharpen_connector(GimpImage *image_ID, GimpDrawable *normalmap_ID, gint32 percent);

#endif /** PLUG_IN_CONNECTORS_HEADER_GUARD */
