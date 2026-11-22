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

#include <stdlib.h>
#include <string.h>

#include "PluginConnectors.h"

gint32 plug_in_colors_channel_mixer_connector(GimpImage *image_ID,
             GimpDrawable *drawable_ID, gint32 isMonochrome,
             gfloat rr_gain, gfloat rg_gain, gfloat rb_gain,
             gfloat gr_gain, gfloat gg_gain, gfloat gb_gain,
             gfloat br_gain, gfloat bg_gain, gfloat bb_gain)
{
    GimpDrawableFilter *filter;
    GimpDrawableFilterConfig *args;

    if (!GIMP_IS_DRAWABLE(drawable_ID)) return 0;

    filter = gimp_drawable_filter_new(drawable_ID, "gegl:channel-mixer", "Channel Mixer");
    if (!filter) return 0;

    args = gimp_drawable_filter_get_config(filter);
    g_object_set(args,
                 "monochrome",       (gboolean)(isMonochrome != 0),
                 "rr-gain",          (gdouble)rr_gain,
                 "rg-gain",          (gdouble)rg_gain,
                 "rb-gain",          (gdouble)rb_gain,
                 "gr-gain",          (gdouble)gr_gain,
                 "gg-gain",          (gdouble)gg_gain,
                 "gb-gain",          (gdouble)gb_gain,
                 "br-gain",          (gdouble)br_gain,
                 "bg-gain",          (gdouble)bg_gain,
                 "bb-gain",          (gdouble)bb_gain,
                 "preserve-luminosity", FALSE,
                 NULL);

    gimp_drawable_filter_update(filter);
    gimp_drawable_merge_filter(drawable_ID, filter);
    g_object_unref(filter);
    return 1;
}

gint32 plug_in_dog_connector(GimpImage *image_ID, GimpDrawable *newlayer_ID, gfloat inner, gfloat outer, gint32 isNormalize, gint32 isInvert)
{
    GimpDrawableFilter *filter;
    GimpDrawableFilterConfig *args;

    if (!GIMP_IS_DRAWABLE(newlayer_ID)) return 0;

    filter = gimp_drawable_filter_new(newlayer_ID, "gegl:difference-of-gaussians", "Difference of Gaussians");
    if (!filter) return 0;

    args = gimp_drawable_filter_get_config(filter);
    g_object_set(args,
                 "radius1", (gdouble)inner,
                 "radius2", (gdouble)outer,
                 NULL);

    gimp_drawable_filter_update(filter);
    gimp_drawable_merge_filter(newlayer_ID, filter);
    g_object_unref(filter);

    if (isNormalize)
        gimp_drawable_levels_stretch(newlayer_ID);
    if (isInvert)
        gimp_drawable_invert(newlayer_ID, FALSE);

    return 1;
}

gint32 plug_in_gauss_connector(GimpImage *image_ID, GimpDrawable *newlayer_ID, gfloat horizontal, gfloat vertical, gint32 isMethod)
{
    GimpDrawableFilter *filter;
    GimpDrawableFilterConfig *args;

    if (!GIMP_IS_DRAWABLE(newlayer_ID)) return 0;

    filter = gimp_drawable_filter_new(newlayer_ID, "gegl:gaussian-blur", "Gaussian Blur");
    if (!filter) return 0;

    args = gimp_drawable_filter_get_config(filter);
    g_object_set(args,
                 "std-dev-x", (gdouble)(horizontal / 3.0f),
                 "std-dev-y", (gdouble)(vertical / 3.0f),
                 NULL);

    gimp_drawable_filter_update(filter);
    gimp_drawable_merge_filter(newlayer_ID, filter);
    g_object_unref(filter);
    return 1;
}

gint32 plug_in_vinvert_connector(GimpImage *image_ID, GimpDrawable *newlayer_ID)
{
    GimpDrawableFilter *filter;

    if (!GIMP_IS_DRAWABLE(newlayer_ID)) return 0;

    filter = gimp_drawable_filter_new(newlayer_ID, "gegl:value-invert", "Value Invert");
    if (!filter) return 0;

    gimp_drawable_filter_update(filter);
    gimp_drawable_merge_filter(newlayer_ID, filter);
    g_object_unref(filter);
    return 1;
}

gint32 plug_in_rgb_noise_connector(GimpImage *image_ID, GimpDrawable *noiselayer_ID, gint32 independent,
             gint32 correlated, gfloat noise_1, gfloat noise_2, gfloat noise_3, gfloat noise_4)
{
    GimpDrawableFilter *filter;
    GimpDrawableFilterConfig *args;

    if (!GIMP_IS_DRAWABLE(noiselayer_ID)) return 0;

    filter = gimp_drawable_filter_new(noiselayer_ID, "gegl:noise-rgb", "RGB Noise");
    if (!filter) return 0;

    args = gimp_drawable_filter_get_config(filter);
    g_object_set(args,
                 "independent", (gboolean)(independent != 0),
                 "correlated",  (gboolean)(correlated != 0),
                 "red",         (gdouble)noise_1,
                 "green",       (gdouble)noise_2,
                 "blue",        (gdouble)noise_3,
                 "alpha",       (gdouble)noise_4,
                 NULL);

    gimp_drawable_filter_update(filter);
    gimp_drawable_merge_filter(noiselayer_ID, filter);
    g_object_unref(filter);
    return 1;
}

gint32 plug_in_make_seamless_connector(GimpImage *image_ID, GimpDrawable *diffuse_ID)
{
    GimpDrawableFilter *filter;

    if (!GIMP_IS_DRAWABLE(diffuse_ID)) return 0;

    filter = gimp_drawable_filter_new(diffuse_ID, "gegl:tile-seamless", "Tile Seamless");
    if (!filter) return 0;

    gimp_drawable_filter_update(filter);
    gimp_drawable_merge_filter(diffuse_ID, filter);
    g_object_unref(filter);
    return 1;
}

gint32 plug_in_blur_connector(GimpImage *image_ID, GimpDrawable *normalmap_ID)
{
    GimpDrawableFilter *filter;
    GimpDrawableFilterConfig *args;

    if (!GIMP_IS_DRAWABLE(normalmap_ID)) return 0;

    filter = gimp_drawable_filter_new(normalmap_ID, "gegl:gaussian-blur", "Blur");
    if (!filter) return 0;

    args = gimp_drawable_filter_get_config(filter);
    g_object_set(args,
                 "std-dev-x", 1.5,
                 "std-dev-y", 1.5,
                 NULL);

    gimp_drawable_filter_update(filter);
    gimp_drawable_merge_filter(normalmap_ID, filter);
    g_object_unref(filter);
    return 1;
}

gint32 plug_in_sharpen_connector(GimpImage *image_ID, GimpDrawable *normalmap_ID, gint32 percent)
{
    GimpDrawableFilter *filter;
    GimpDrawableFilterConfig *args;

    if (!GIMP_IS_DRAWABLE(normalmap_ID)) return 0;

    filter = gimp_drawable_filter_new(normalmap_ID, "gegl:unsharp-mask", "Sharpen");
    if (!filter) return 0;

    args = gimp_drawable_filter_get_config(filter);
    g_object_set(args,
                 "std-dev", 1.5,
                 "scale",   (gdouble)percent / 20.0,
                 NULL);

    gimp_drawable_filter_update(filter);
    gimp_drawable_merge_filter(normalmap_ID, filter);
    g_object_unref(filter);
    return 1;
}
