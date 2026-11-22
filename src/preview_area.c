/* GIMP Plug-in InsaneBump
 * Copyright (C) 2013  Derby Russsell <jdrussell51@gmail.com> (the "Author").
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHOR BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name of the Author of the
 * Software shall not be used in advertising or otherwise to promote the
 * sale, use or other dealings in this Software without prior written
 * authorization from the Author.
 */
#include "config.h"

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "main.h"
#include "interface.h"
#include "scale.h"
#include "InsaneBump.h"

#include "plugin-intl.h"
#include "PluginConnectors.h"

/*  Global Variables  */
gchar        _active = 'x';
PreviewDraws pDrawables;
GtkWidget   *g_gwNormalsFrame = NULL;

/*  Local variables  */
static GtkWidget *btn_n = NULL;
static GtkWidget *btn_ln = NULL;
static GtkWidget *btn_mn = NULL;
static GtkWidget *btn_hn = NULL;
static GtkWidget *btn_sn = NULL;
static GimpLayer *drawableBeginActiveLayer = NULL;
static gchar      gcNeedNormal = 'n';
static GtkWidget *gwNormalLabel = NULL;

static GimpLayer *get_first_selected_layer(GimpImage *image)
{
    GList     *layers = gimp_image_list_selected_layers(image);
    GimpLayer *layer  = NULL;

    if (layers != NULL)
    {
        layer = GIMP_LAYER(layers->data);
        g_list_free(layers);
    }
    return layer;
}

static void set_single_selected_layer(GimpImage *image, GimpLayer *layer)
{
    const GimpLayer *layers[2] = { layer, NULL };
    gimp_image_set_selected_layers(image, layers);
}

/*  Private functions  */
static void draw_preview_area_update(GtkWidget *preview, GimpDrawable *drawable) {
    if (is_3D_preview_active())
    {
        if ((local_vals.image_ID != NULL) && GIMP_IS_DRAWABLE(drawable))
        {
            gboolean      has_alpha = gimp_drawable_has_alpha(drawable);
            gint          bpp       = has_alpha ? 4 : 3;
            gint          w         = gimp_drawable_get_width(drawable);
            gint          h         = gimp_drawable_get_height(drawable);
            gint          rowbytes  = PREVIEW_SIZE * bpp;
            guchar       *tmp       = g_new(guchar, w * h * bpp);
            guchar       *dst       = g_new(guchar, PREVIEW_SIZE * PREVIEW_SIZE * bpp);
            GeglBuffer   *buf       = gimp_drawable_get_buffer(drawable);
            GeglRectangle rect      = { 0, 0, w, h };

            update_preview = 0;

            gegl_buffer_get(buf, &rect, 1.0,
                            babl_format(has_alpha ? "R'G'B'A u8" : "R'G'B' u8"),
                            tmp, GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);
            g_object_unref(buf);

            scale_pixels(dst, PREVIEW_SIZE, PREVIEW_SIZE, tmp, w, h, bpp);

            gimp_preview_area_draw(GIMP_PREVIEW_AREA(preview), 0, 0, PREVIEW_SIZE, PREVIEW_SIZE,
                                   has_alpha ? GIMP_RGBA_IMAGE : GIMP_RGB_IMAGE,
                                   dst, rowbytes);

            gtk_widget_queue_draw(preview);
            g_free(tmp);
            g_free(dst);
            _active = 'w';
        }
    }
}

static void preview_diffuse_only(GimpImage *image_ID) {
    GimpLayer *noiselayer_ID      = NULL;
    GimpLayer *drawable_ID        = NULL;
    GimpLayer *drawableDiffuse_ID = NULL;

    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress), 0.0);
    gtk_label_set_text(GTK_LABEL(progress_label), "Begin");

    drawableBeginActiveLayer = get_first_selected_layer(image_ID);
    drawable_ID = gimp_layer_copy(drawableBeginActiveLayer);
    gimp_image_insert_layer(image_ID, drawable_ID, NULL, -1);
    set_single_selected_layer(image_ID, drawable_ID);
    gimp_item_set_visible(GIMP_ITEM(drawableBeginActiveLayer), FALSE);

    if (local_vals.Noise)
    {
        noiselayer_ID = gimp_layer_copy(drawable_ID);

        gimp_image_insert_layer(image_ID, noiselayer_ID, NULL, -1);
        set_single_selected_layer(image_ID, noiselayer_ID);
        gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress), 0.03);
        gtk_label_set_text(GTK_LABEL(progress_label), "Noise");

        if (plug_in_rgb_noise_connector(image_ID, GIMP_DRAWABLE(noiselayer_ID), 1, 1, 0.20f, 0.20f, 0.20f, 0.0f) != 1) return;
        gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress), 0.04);

        gimp_layer_set_mode(noiselayer_ID, GIMP_LAYER_MODE_HSV_VALUE);
        gimp_image_merge_down(image_ID, noiselayer_ID, GIMP_EXPAND_AS_NECESSARY);

        gtk_label_set_text(GTK_LABEL(gwNormalLabel), "Noise affects every redraw!");
    } else {
        gtk_label_set_text(GTK_LABEL(gwNormalLabel), "");
        gtk_label_set_text(GTK_LABEL(progress_label), "Noise added");
        gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress), 0.04);
    }

    if (local_vals.RemoveLighting)
    {
        gtk_label_set_text(GTK_LABEL(progress_label), "Remove Shading");
        removeShadingPreview(image_ID, local_vals.Noise);
        gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress), 0.05);
    } else {
        gtk_label_set_text(GTK_LABEL(progress_label), "Stretch");
        gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress), 0.05);
    }

    drawableDiffuse_ID = get_first_selected_layer(image_ID);
    gimp_drawable_levels_stretch(GIMP_DRAWABLE(drawableDiffuse_ID));
    if (local_vals.Tile)
    {
        gtk_label_set_text(GTK_LABEL(progress_label), "Making Seamless");
        if (plug_in_make_seamless_connector(image_ID, GIMP_DRAWABLE(drawableDiffuse_ID)) != 1) return;
        gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress), 0.07);
    } else {
        gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress), 0.07);
    }

    pDrawables.drawable_d = GIMP_DRAWABLE(drawableDiffuse_ID);
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress), 0.1);
}

static void preview_alt_normal(GimpImage *image_ID) {
    GimpLayer *mergedLayer_ID   = NULL;
    GimpLayer *normalmap_ID     = NULL;
    gfloat     wsize            = (gfloat)gimp_image_get_width(image_ID);
    gfloat     hsize            = (gfloat)gimp_image_get_width(image_ID);
    GimpLayer *drawable_temp_ID = get_first_selected_layer(image_ID);
    GimpLayer *diffuse_ID       = gimp_layer_copy(drawable_temp_ID);

    gimp_image_insert_layer(image_ID, diffuse_ID, NULL, -1);
    set_single_selected_layer(image_ID, diffuse_ID);
    gtk_label_set_text(GTK_LABEL(progress_label), "Smoothing");

    blur(image_ID, diffuse_ID, wsize, hsize, local_vals.LargeDetails, 0, local_vals.ao);
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress), 0.15);

    normalmap_ID = get_first_selected_layer(image_ID);
    if (local_vals.smoothstep)
    {
        gtk_label_set_text(GTK_LABEL(progress_label), "Smoothing");
        if (plug_in_blur_connector(image_ID, GIMP_DRAWABLE(normalmap_ID)) != 1) return;
        gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress), 0.20);
    }
    else
    {
        gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress), 0.20);
    }

    if (local_vals.invh)
    {
        if (plug_in_vinvert_connector(image_ID, GIMP_DRAWABLE(normalmap_ID)) != 1) return;
        gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress), 0.25);
    }
    else
    {
        gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress), 0.25);
    }

    pDrawables.drawable_p = GIMP_DRAWABLE(normalmap_ID);
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress), 0.30);

    gtk_label_set_text(GTK_LABEL(progress_label), "Base Mapping");
    doBaseMap(image_ID, diffuse_ID, local_vals.Depth, local_vals.LargeDetails, local_vals.ao);
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress), 0.35);
    normalmap_ID = get_first_selected_layer(image_ID);
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress), 0.40);

    if (gcNeedNormal == 'l') {
        pDrawables.drawable_n = GIMP_DRAWABLE(normalmap_ID);
    }

    shapeRecognise(image_ID, normalmap_ID, local_vals.ShapeRecog, local_vals.ao);
    if (local_vals.smoothstep)
    {
        normalmap_ID = get_first_selected_layer(image_ID);
        gtk_label_set_text(GTK_LABEL(progress_label), "Smoothing");
        if (plug_in_blur_connector(image_ID, GIMP_DRAWABLE(normalmap_ID)) != 1) return;
        gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress), 0.45);
    }
    else
    {
        gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress), 0.45);
    }

    if (gcNeedNormal == 's') {
        pDrawables.drawable_n = GIMP_DRAWABLE(normalmap_ID);
    }
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress), 0.50);
    gtk_label_set_text(GTK_LABEL(progress_label), "Sharpen");

    sharpen(image_ID, diffuse_ID, local_vals.Depth, 0, local_vals.SmallDetails);
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress), 0.55);
    normalmap_ID = get_first_selected_layer(image_ID);

    gtk_label_set_text(GTK_LABEL(progress_label), "Sharpen more");
	gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress), 0.65);
if (plug_in_sharpen_connector(image_ID, GIMP_DRAWABLE(normalmap_ID), 20) != 1) return;
if (gcNeedNormal == 'h') {
pDrawables.drawable_n = GIMP_DRAWABLE(normalmap_ID);
}
gtk_label_set_text(GTK_LABEL(progress_label), "Sharpen again");
sharpen(image_ID, diffuse_ID, local_vals.Depth, 6, local_vals.MediumDetails);
gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress), 0.70);
normalmap_ID = get_first_selected_layer(image_ID);
gtk_label_set_text(GTK_LABEL(progress_label), "Smoothing");
if (plug_in_blur_connector(image_ID, GIMP_DRAWABLE(normalmap_ID)) != 1) return;
gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress), 0.75);
if (gcNeedNormal == 'm') {
pDrawables.drawable_n = GIMP_DRAWABLE(normalmap_ID);
}
gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress), 0.78);
gimp_item_set_visible(GIMP_ITEM(diffuse_ID), FALSE);
mergedLayer_ID = gimp_layer_new_from_visible(image_ID, image_ID, "temp");
gimp_image_insert_layer(image_ID, mergedLayer_ID, NULL, -1);
set_single_selected_layer(image_ID, mergedLayer_ID);
}
static void preview_normal_only(GimpImage *image_ID) {
GimpLayer *drawableNormal_ID = NULL;
preview_diffuse_only(image_ID);
preview_alt_normal(image_ID);
drawableNormal_ID = get_first_selected_layer(image_ID);
nmapvals.filter        = 0;
nmapvals.minz          = 0.0f;
nmapvals.scale         = 1.0f;
nmapvals.wrap          = 0;
nmapvals.height_source = 0;
nmapvals.alpha         = 0;
nmapvals.conversion    = 8;
nmapvals.dudv          = 0;
nmapvals.xinvert       = 0;
nmapvals.yinvert       = 0;
nmapvals.swapRGB       = 0;
nmapvals.contrast      = 0.0f;
nmapvals.alphamap_id   = GIMP_DRAWABLE(drawableNormal_ID);
gtk_label_set_text(GTK_LABEL(progress_label), "Normal map");
normalmap(GIMP_DRAWABLE(drawableNormal_ID), FALSE);
gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress), 0.80);
if (gcNeedNormal == 'n') {
pDrawables.drawable_n = GIMP_DRAWABLE(drawableNormal_ID);
}
gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress), 0.85);
}
static void preview_ambient_occlusion_only(GimpImage *image_ID) {
GimpLayer *drawable_ID   = NULL;
GimpLayer *drawableAO_ID = NULL;
gtk_label_set_text(GTK_LABEL(progress_label), "Mixing Colors");
preview_normal_only(image_ID);
drawable_ID   = get_first_selected_layer(image_ID);
drawableAO_ID = gimp_layer_copy(drawable_ID);
gimp_image_insert_layer(image_ID, drawableAO_ID, NULL, -1);
set_single_selected_layer(image_ID, drawableAO_ID);
if (plug_in_colors_channel_mixer_connector(image_ID, GIMP_DRAWABLE(drawableAO_ID), 0, -1.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f) != 1) return;
gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress), 0.87);
gimp_drawable_desaturate(GIMP_DRAWABLE(drawableAO_ID), GIMP_DESATURATE_LIGHTNESS);
gimp_drawable_levels_stretch(GIMP_DRAWABLE(drawableAO_ID));
pDrawables.drawable_ao = GIMP_DRAWABLE(drawableAO_ID);
gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress), 0.90);
}
static void preview_specular_only(GimpImage *image_ID) {
GimpLayer *drawableSpecular_ID = NULL;
gint32     nResult             = 0;
preview_ambient_occlusion_only(image_ID);
if (local_vals.EdgeSpecular)
{
gtk_label_set_text(GTK_LABEL(progress_label), "Specular Edging");
drawableSpecular_ID = specularEdgeWorker(image_ID, local_vals.defSpecular, local_vals.ao, FALSE);
if (drawableSpecular_ID == NULL) {
gimp_message("Specular Edge Worker returned NULL!");
nResult = 0;
} else nResult = 1;
}
else
{
gtk_label_set_text(GTK_LABEL(progress_label), "Specular Smoothing");
drawableSpecular_ID = specularSmoothWorker(image_ID, local_vals.defSpecular, local_vals.ao, FALSE);
if (drawableSpecular_ID == NULL) {
gimp_message("Specular Smooth Worker returned NULL!");
nResult = 0;
} else nResult = 1;
}
gtk_label_set_text(GTK_LABEL(progress_label), "Idle...");
gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress), 1.0);
if (nResult == 1) {
pDrawables.drawable_s = GIMP_DRAWABLE(drawableSpecular_ID);
} else {
pDrawables.drawable_s = NULL;
}
}
/*  Public functions  */
void init_drawables(void)
{
pDrawables.drawable_d  = NULL;
pDrawables.drawable_ao = NULL;
pDrawables.drawable_s  = NULL;
pDrawables.drawable_n  = NULL;
pDrawables.drawable_p  = NULL;
pDrawables.preview_d   = NULL;
pDrawables.preview_ao  = NULL;
pDrawables.preview_s   = NULL;
pDrawables.preview_n   = NULL;
pDrawables.preview_p   = NULL;
btn_n  = NULL;
btn_ln = NULL;
btn_mn = NULL;
btn_hn = NULL;
btn_sn = NULL;
drawableBeginActiveLayer = NULL;
gcNeedNormal = 'n';
_active = 'x';
}
void preview_redraw(void)
{
if (!dialog_is_init) return;
if (is_3D_preview_active()) return;
if (local_vals.prev == 1)
{
_active = 't';
pDrawables.drawable_d  = NULL;
pDrawables.drawable_ao = NULL;
pDrawables.drawable_s  = NULL;
pDrawables.drawable_n  = NULL;
pDrawables.drawable_p  = NULL;
preview_specular_only(local_vals.image_ID);
if (pDrawables.drawable_d != NULL) {
_active = 'd';
draw_preview_area_update(pDrawables.preview_d, pDrawables.drawable_d);
} else {
gimp_preview_area_fill(GIMP_PREVIEW_AREA(pDrawables.preview_d), 0, 0, PREVIEW_SIZE, PREVIEW_SIZE, 0xF2, 0xF1, 0xF0);
_active = 'd';
}
if (pDrawables.drawable_ao != NULL) {
_active = 'a';
draw_preview_area_update(pDrawables.preview_ao, pDrawables.drawable_ao);
} else {
gimp_preview_area_fill(GIMP_PREVIEW_AREA(pDrawables.preview_ao), 0, 0, PREVIEW_SIZE, PREVIEW_SIZE, 0xF2, 0xF1, 0xF0);
_active = 'a';
}
if (pDrawables.drawable_s != NULL) {
_active = 's';
draw_preview_area_update(pDrawables.preview_s, pDrawables.drawable_s);
} else {
gimp_preview_area_fill(GIMP_PREVIEW_AREA(pDrawables.preview_s), 0, 0, PREVIEW_SIZE, PREVIEW_SIZE, 0xF2, 0xF1, 0xF0);
_active = 's';
}
if (pDrawables.drawable_p != NULL) {
_active = 'p';
draw_preview_area_update(pDrawables.preview_p, pDrawables.drawable_p);
} else {
gimp_preview_area_fill(GIMP_PREVIEW_AREA(pDrawables.preview_p), 0, 0, PREVIEW_SIZE, PREVIEW_SIZE, 0xF2, 0xF1, 0xF0);
_active = 'p';
}
if (pDrawables.drawable_n != NULL) {
_active = 'n';
draw_preview_area_update(pDrawables.preview_n, pDrawables.drawable_n);
} else {
gimp_preview_area_fill(GIMP_PREVIEW_AREA(pDrawables.preview_n), 0, 0, PREVIEW_SIZE, PREVIEW_SIZE, 0xF2, 0xF1, 0xF0);
}
removeAllLayersExceptMain();
preview_progress_reset = 0;
}
}
void preview_clicked(GtkWidget *widget, gpointer data)
{
if (!dialog_is_init) return;
if (local_vals.prev == 0)
{
local_vals.prev = 1;
gtk_button_set_label(GTK_BUTTON(widget), "Preview On");
preview_redraw();
}
else
{
local_vals.prev = 0;
gtk_button_set_label(GTK_BUTTON(widget), "Preview Off");
gimp_preview_area_fill(GIMP_PREVIEW_AREA(pDrawables.preview_d), 0, 0, PREVIEW_SIZE, PREVIEW_SIZE, 0xF2, 0xF1, 0xF0);
gimp_preview_area_fill(GIMP_PREVIEW_AREA(pDrawables.preview_ao), 0, 0, PREVIEW_SIZE, PREVIEW_SIZE, 0xF2, 0xF1, 0xF0);
gimp_preview_area_fill(GIMP_PREVIEW_AREA(pDrawables.preview_s), 0, 0, PREVIEW_SIZE, PREVIEW_SIZE, 0xF2, 0xF1, 0xF0);
gimp_preview_area_fill(GIMP_PREVIEW_AREA(pDrawables.preview_p), 0, 0, PREVIEW_SIZE, PREVIEW_SIZE, 0xF2, 0xF1, 0xF0);
gimp_preview_area_fill(GIMP_PREVIEW_AREA(pDrawables.preview_n), 0, 0, PREVIEW_SIZE, PREVIEW_SIZE, 0xF2, 0xF1, 0xF0);
_active = 'x';
return;
}
}
void restore_defaults_clicked(GtkWidget *widget, gpointer data)
{
PlugInVals pivTemp;
if (!dialog_is_init) return;
copyPlugInVals(&local_vals, &pivTemp);
copyPlugInVals(&default_vals, &local_vals);
if (pivTemp.RemoveLighting != local_vals.RemoveLighting) {
gtk_button_clicked(GTK_BUTTON(g_gwRemoveLtgBtn));
remlightbtn_clicked(g_gwRemoveLtgBtn, NULL);
}
if (pivTemp.Resizie != local_vals.Resizie) {
gtk_button_clicked(GTK_BUTTON(g_gwUpscaleBtn));
upscale_HD_clicked(g_gwUpscaleBtn, NULL);
}
if (pivTemp.Tile != local_vals.Tile) {
gtk_button_clicked(GTK_BUTTON(g_gwTileBtn));
tile_clicked(g_gwTileBtn, NULL);
}
if (pivTemp.EdgeSpecular != local_vals.EdgeSpecular) {
gtk_button_clicked(GTK_BUTTON(g_gwEdgeBtn));
edge_enhancing_specular_clicked(g_gwEdgeBtn, NULL);
}
if (pivTemp.smoothstep != local_vals.smoothstep) {
gtk_button_clicked(GTK_BUTTON(g_gwSmoothBtn));
smooth_step_clicked(g_gwSmoothBtn, NULL);
}
if (pivTemp.Noise != local_vals.Noise) {
gtk_button_clicked(GTK_BUTTON(g_gwNoiseBtn));
noise_clicked(g_gwNoiseBtn, NULL);
}
if (pivTemp.invh != local_vals.invh) {
gtk_button_clicked(GTK_BUTTON(g_gwInvertBtn));
invert_height_map_clicked(g_gwInvertBtn, NULL);
}
if (pivTemp.Depth != local_vals.Depth) {
GtkAdjustment *gaTemp = gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON(g_gwDepthSpin));
gtk_adjustment_set_value(GTK_ADJUSTMENT(gaTemp), (gdouble)local_vals.Depth);
}
if (pivTemp.LargeDetails != local_vals.LargeDetails) {
GtkAdjustment *gaTemp = gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON(g_gwLargeDSpin));
gtk_adjustment_set_value(GTK_ADJUSTMENT(gaTemp), (gdouble)local_vals.LargeDetails);
}
if (pivTemp.MediumDetails != local_vals.MediumDetails) {
GtkAdjustment *gaTemp = gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON(g_gwMediumDSpin));
gtk_adjustment_set_value(GTK_ADJUSTMENT(gaTemp), (gdouble)local_vals.MediumDetails);
}
if (pivTemp.SmallDetails != local_vals.SmallDetails) {
GtkAdjustment *gaTemp = gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON(g_gwSmallDSpin));
gtk_adjustment_set_value(GTK_ADJUSTMENT(gaTemp), (gdouble)local_vals.SmallDetails);
}
if (pivTemp.ShapeRecog != local_vals.ShapeRecog) {
GtkAdjustment *gaTemp = gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON(g_gwShapeSpin));
gtk_adjustment_set_value(GTK_ADJUSTMENT(gaTemp), (gdouble)local_vals.ShapeRecog);
}
if (pivTemp.ao != local_vals.ao) {
GtkAdjustment *gaTemp = gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON(g_gwAOSpin));
gtk_adjustment_set_value(GTK_ADJUSTMENT(gaTemp), (gdouble)local_vals.ao);
}
if (pivTemp.defSpecular != local_vals.defSpecular) {
GtkAdjustment *gaTemp = gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON(g_gwSpecDefSpin));
gtk_adjustment_set_value(GTK_ADJUSTMENT(gaTemp), (gdouble)local_vals.defSpecular);
}
if (pivTemp.newWidth != local_vals.newWidth) {
GtkAdjustment *gaTemp = gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON(g_gwWidthSpin));
gtk_adjustment_set_value(GTK_ADJUSTMENT(gaTemp), (gdouble)local_vals.newWidth);
}
copyPlugInVals(&default_vals, &local_vals);
local_vals.image_ID = pivTemp.image_ID;
local_vals.prev = pivTemp.prev;
if (local_vals.prev == 1)
{
preview_redraw();
}
}
void preview_clicked_normal(GtkWidget *widget, gpointer data)
{
if (!dialog_is_init) return;
if (local_vals.prev == 0) return;
if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)) == TRUE)
{
GtkWidget *gLabelWidge = gtk_frame_get_label_widget(GTK_FRAME(g_gwNormalsFrame));
gtk_label_set_text(GTK_LABEL(gLabelWidge), "Normal");
gcNeedNormal = 'n';
preview_redraw();
}
}
void preview_clicked_lownormal(GtkWidget *widget, gpointer data)
{
if (!dialog_is_init) return;
if (local_vals.prev == 0) return;
if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)) == TRUE)
{
GtkWidget *gLabelWidge = gtk_frame_get_label_widget(GTK_FRAME(g_gwNormalsFrame));
gtk_label_set_text(GTK_LABEL(gLabelWidge), "Low Normal");
gcNeedNormal = 'l';
preview_redraw();
}
}
void preview_clicked_mediumnormal(GtkWidget *widget, gpointer data)
{
if (!dialog_is_init) return;
if (local_vals.prev == 0) return;
if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)) == TRUE)
{
GtkWidget *gLabelWidge = gtk_frame_get_label_widget(GTK_FRAME(g_gwNormalsFrame));
gtk_label_set_text(GTK_LABEL(gLabelWidge), "Medium Normal");
gcNeedNormal = 'm';
preview_redraw();
}
}
void preview_clicked_highnormal(GtkWidget *widget, gpointer data)
{
if (!dialog_is_init) return;
if (local_vals.prev == 0) return;
if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)) == TRUE)
{
GtkWidget *gLabelWidge = gtk_frame_get_label_widget(GTK_FRAME(g_gwNormalsFrame));
gtk_label_set_text(GTK_LABEL(gLabelWidge), "High Normal");
gcNeedNormal = 'h';
preview_redraw();
}
}
void preview_clicked_supernormal(GtkWidget *widget, gpointer data)
{
if (!dialog_is_init) return;
if (local_vals.prev == 0) return;
if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)) == TRUE)
{
GtkWidget *gLabelWidge = gtk_frame_get_label_widget(GTK_FRAME(g_gwNormalsFrame));
gtk_label_set_text(GTK_LABEL(gLabelWidge), "Super Normal");
gcNeedNormal = 's';
preview_redraw();
}
}
void removeAllLayersExceptMain(void) {
GList *layers = NULL;
if (drawableBeginActiveLayer == NULL) return;
set_single_selected_layer(local_vals.image_ID, drawableBeginActiveLayer);
layers = gimp_image_list_layers(local_vals.image_ID);
for (GList *l = layers; l != NULL; l = l->next) {
GimpLayer *layer = GIMP_LAYER(l->data);
if (layer != drawableBeginActiveLayer) {
if (gimp_layer_is_floating_sel(layer))
{
gimp_floating_sel_remove(layer);
} else {
gimp_image_remove_layer(local_vals.image_ID, layer);
}
}
}
g_list_free(layers);
gimp_item_set_visible(GIMP_ITEM(drawableBeginActiveLayer), TRUE);
}
int is_3D_preview_active(void)
{
if (_active == 'w') {
return FALSE;
}
else if (_active != 'x') {
return TRUE;
}
return FALSE;
}
static GtkWidget *CreateOnePreviewFrame(GtkWidget *vbox, GtkWidget **preview, const gchar *szName)
{
GtkWidget *frame = NULL;
GtkWidget *labelFrame = NULL;
GtkWidget *abox = NULL;
if (vbox == NULL) return NULL;
labelFrame = gtk_frame_new(szName);
gtk_box_pack_start(GTK_BOX(vbox), labelFrame, FALSE, FALSE, 0);
gtk_widget_show(labelFrame);
abox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
gtk_widget_set_halign(abox, GTK_ALIGN_CENTER);
gtk_widget_set_valign(abox, GTK_ALIGN_CENTER);
gtk_container_set_border_width(GTK_CONTAINER(abox), 4);
gtk_container_add(GTK_CONTAINER(labelFrame), abox);
gtk_widget_show(abox);
frame = gtk_frame_new(NULL);
gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_IN);
gtk_container_add(GTK_CONTAINER(abox), frame);
gtk_widget_show(frame);
*preview = gimp_preview_area_new();
gimp_preview_area_set_max_size(GIMP_PREVIEW_AREA(*preview), PREVIEW_SIZE, PREVIEW_SIZE);
gtk_widget_set_size_request(*preview, PREVIEW_SIZE, PREVIEW_SIZE);
gtk_container_add(GTK_CONTAINER(frame), *preview);
gtk_widget_show(*preview);
return labelFrame;
}
void CreateLeftPreviewFrames(GtkWidget *hbox)
{
GtkWidget *vbox = NULL;
GtkWidget *btn = NULL;
if (hbox == NULL) return;
vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
gtk_box_pack_start(GTK_BOX(hbox), vbox, TRUE, TRUE, 0);
gtk_widget_show(vbox);
CreateOnePreviewFrame(vbox, &pDrawables.preview_d, "Diffuse");
CreateOnePreviewFrame(vbox, &pDrawables.preview_ao, "Occlusion");
CreateOnePreviewFrame(vbox, &pDrawables.preview_s, "Specular");
btn = gtk_button_new_with_label("Restore Defaults");
if (btn != NULL)
{
g_signal_connect(btn, "clicked",
G_CALLBACK(restore_defaults_clicked), NULL);
gtk_box_pack_start(GTK_BOX(vbox), btn, FALSE, FALSE, 0);
gtk_widget_show(btn);
}
}
void CreateRightPreviewToggleButton(GtkWidget *hbox)
{
GtkWidget *vbox = NULL;
GtkWidget *btn = NULL;
if (hbox == NULL) return;
vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
gtk_box_pack_start(GTK_BOX(hbox), vbox, TRUE, TRUE, 0);
gtk_widget_show(vbox);
CreateOnePreviewFrame(vbox, &pDrawables.preview_p, "Displacement");
g_gwNormalsFrame = CreateOnePreviewFrame(vbox, &pDrawables.preview_n, "Normal");
if (local_vals.prev == 1) {
btn = gtk_toggle_button_new_with_label("Preview On");
} else {
btn = gtk_toggle_button_new_with_label("Preview Off");
}
if (btn != NULL)
{
g_signal_connect(btn, "clicked",
G_CALLBACK(preview_clicked), NULL);
gtk_box_pack_start(GTK_BOX(vbox), btn, FALSE, FALSE, 0);
gtk_widget_show(btn);
if (local_vals.prev == 1)
{
gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(btn), TRUE);
}
}
btn_n = gtk_radio_button_new_with_label_from_widget(NULL, "Normal");
if (btn_n != NULL)
{
g_signal_connect(btn_n, "clicked",
G_CALLBACK(preview_clicked_normal), NULL);
gtk_box_pack_start(GTK_BOX(vbox), btn_n, FALSE, FALSE, 0);
gtk_widget_show(btn_n);
}
btn_ln = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(btn_n), "low normal");
if (btn_ln != NULL)
{
g_signal_connect(btn_ln, "clicked",
G_CALLBACK(preview_clicked_lownormal), NULL);
gtk_box_pack_start(GTK_BOX(vbox), btn_ln, FALSE, FALSE, 0);
gtk_widget_show(btn_ln);
}
btn_mn = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(btn_n), "medium normal");
if (btn_mn != NULL)
{
g_signal_connect(btn_mn, "clicked",
G_CALLBACK(preview_clicked_mediumnormal), NULL);
gtk_box_pack_start(GTK_BOX(vbox), btn_mn, FALSE, FALSE, 0);
gtk_widget_show(btn_mn);
}
btn_hn = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(btn_n), "high normal");
if (btn_hn != NULL)
{
g_signal_connect(btn_hn, "clicked",
G_CALLBACK(preview_clicked_highnormal), NULL);
gtk_box_pack_start(GTK_BOX(vbox), btn_hn, FALSE, FALSE, 0);
gtk_widget_show(btn_hn);
}
btn_sn = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(btn_n), "super normal");
if (btn_sn != NULL)
{
g_signal_connect(btn_sn, "clicked",
G_CALLBACK(preview_clicked_supernormal), NULL);
gtk_box_pack_start(GTK_BOX(vbox), btn_sn, FALSE, FALSE, 0);
gtk_widget_show(btn_sn);
}
gwNormalLabel = gtk_label_new("");
gtk_box_pack_start(GTK_BOX(vbox), gwNormalLabel, FALSE, FALSE, 0);
gtk_widget_show(gwNormalLabel);
}
