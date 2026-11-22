/************************************************************************
 *   insanemap GIMP plugin
 *
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

#include <glib.h>
#include <stdlib.h>
#include <string.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "main.h"
#include "InsaneBump.h"
#include "PluginConnectors.h"

/* Helper wrappers for GIMP 3 active layer selection & saving */
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


static void save_image_to_path(GimpImage *image, const gchar *path)
{
    if (path == NULL) return;
    GFile *file = g_file_new_for_path(path);
    gimp_file_save(GIMP_RUN_NONINTERACTIVE, image, file, NULL);
    g_object_unref(file);
}

GString *getFilename(const gchar *filename, const gchar *suffix)
{
    if (filename == NULL) return NULL;
    GString *newfilename = g_string_new(filename);
    gchar   *szStr       = newfilename->str;
    char    *szPtr       = strrchr((char *)szStr, '.');
    if (szPtr != NULL)
    {
        int nLocate = szPtr - szStr;
        g_string_insert(newfilename, nLocate, suffix);
    }
    else
    {
        g_string_append(newfilename, suffix);
        g_string_append(newfilename, ".png");
    }
    return newfilename;
}

void saveLastOperation(GimpImage *image_ID, const GString *filename)
{
    if (filename != NULL)
        save_image_to_path(image_ID, filename->str);
}

void set_progress_label_as_file(GString *file_name_given, const gchar *szSuffix)
{
    if (file_name_given == NULL) return;
    GString *string_temp = g_string_new(file_name_given->str);
    gchar   *szStr       = string_temp->str;
    char    *szPtr       = strrchr((char *)szStr, '\\');
    if (szPtr == NULL)
    {
        szPtr = strrchr((char *)szStr, '/');
    }
    if (szPtr != NULL)
    {
        szPtr++;
        GString *string_sub = g_string_new(szPtr);

        g_string_printf(string_temp, "Saving %s %s", szSuffix, string_sub->str);
        gtk_label_set_text(GTK_LABEL(progress_label), string_temp->str);
        g_string_free(string_sub, TRUE);
        g_string_free(string_temp, TRUE);
    }
    else
    {
        g_string_printf(string_temp, "Saving %s %s", szSuffix, file_name_given->str);
        gtk_label_set_text(GTK_LABEL(progress_label), string_temp->str);
        g_string_free(string_temp, TRUE);
    }
}

GimpLayer *specularEdgeWorker(GimpImage *image_ID, gint defin, gint defAO, gint32 bShowProgress)
{
    gfloat     secondRadius = defAO * 0.16f;
    GimpLayer *drawable_ID  = get_first_selected_layer(image_ID);
    GimpLayer *newlayer_ID  = gimp_layer_copy(drawable_ID);

    if (bShowProgress == TRUE) {
        gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress), 0.92);
    }

    gimp_image_insert_layer(image_ID, newlayer_ID, NULL, -1);
    set_single_selected_layer(image_ID, newlayer_ID);
    gimp_drawable_desaturate(GIMP_DRAWABLE(newlayer_ID), GIMP_DESATURATE_LIGHTNESS);

    drawable_ID = get_first_selected_layer(image_ID);
    newlayer_ID = gimp_layer_copy(drawable_ID);
    gimp_image_insert_layer(image_ID, newlayer_ID, NULL, -1);
    set_single_selected_layer(image_ID, newlayer_ID);
    gimp_drawable_desaturate(GIMP_DRAWABLE(newlayer_ID), GIMP_DESATURATE_LIGHTNESS);

    if (plug_in_dog_connector(image_ID, GIMP_DRAWABLE(newlayer_ID), 1.0, secondRadius, 1, 0) != 1) return NULL;
    if (bShowProgress == TRUE) {
        gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress), 0.94);
    }

    gimp_layer_set_mode(newlayer_ID, GIMP_LAYER_MODE_ADDITION);
    gimp_image_merge_down(image_ID, newlayer_ID, GIMP_EXPAND_AS_NECESSARY);
    drawable_ID = get_first_selected_layer(image_ID);
    gimp_drawable_brightness_contrast(GIMP_DRAWABLE(drawable_ID), 0.0, 64.0 / 127.0);
    if (bShowProgress == TRUE) {
        gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress), 0.96);
    }

    gimp_drawable_levels(GIMP_DRAWABLE(drawable_ID), GIMP_HISTOGRAM_VALUE,
                         (gdouble)defin / 255.0, 1.0, TRUE, 1.0, 0.0, 1.0, TRUE);

    return drawable_ID;
}

gint32 specularSave(GString *sFileName, GimpImage *image_ID, GimpLayer *drawable_ID)
{
    if (sFileName != NULL) {
        set_progress_label_as_file(sFileName, "_s");
        save_image_to_path(image_ID, sFileName->str);
    }

    gimp_drawable_edit_clear(GIMP_DRAWABLE(drawable_ID));

    return 1;
}

GimpLayer *specularSmoothWorker(GimpImage *image_ID, gint defin, gint defAO, gint32 bShowProgress)
{
    gfloat     firstRadius = defAO * 0.16f;
    GimpLayer *drawable_ID = get_first_selected_layer(image_ID);
    GimpLayer *newlayer_ID = gimp_layer_copy(drawable_ID);

    gimp_image_insert_layer(image_ID, newlayer_ID, NULL, -1);
    set_single_selected_layer(image_ID, newlayer_ID);
    gimp_drawable_desaturate(GIMP_DRAWABLE(newlayer_ID), GIMP_DESATURATE_LIGHTNESS);

    if (bShowProgress == TRUE) {
        gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress), 0.92);
    }

    drawable_ID = get_first_selected_layer(image_ID);
    newlayer_ID = gimp_layer_copy(drawable_ID);
    gimp_image_insert_layer(image_ID, newlayer_ID, NULL, -1);
    set_single_selected_layer(image_ID, newlayer_ID);
    gimp_drawable_desaturate(GIMP_DRAWABLE(newlayer_ID), GIMP_DESATURATE_LIGHTNESS);

    if (plug_in_dog_connector(image_ID, GIMP_DRAWABLE(newlayer_ID), firstRadius, 1.0f, 1, 0) != 1) return NULL;
    if (bShowProgress == TRUE) {
        gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress), 0.94);
    }

    gimp_layer_set_mode(newlayer_ID, GIMP_LAYER_MODE_ADDITION);
    gimp_image_merge_down(image_ID, newlayer_ID, GIMP_EXPAND_AS_NECESSARY);

    drawable_ID = get_first_selected_layer(image_ID);
    gimp_drawable_brightness_contrast(GIMP_DRAWABLE(drawable_ID), 0.0, 64.0 / 127.0);
    gimp_drawable_levels(GIMP_DRAWABLE(drawable_ID), GIMP_HISTOGRAM_VALUE,
                         (gdouble)defin / 255.0, 1.0, TRUE, 1.0, 0.0, 1.0, TRUE);

    if (bShowProgress == TRUE) {
        gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress), 0.96);
    }

    gimp_image_raise_item_to_top(image_ID, GIMP_ITEM(drawable_ID));

    return drawable_ID;
}

gint32 specularDo(GimpImage *image_ID, const gchar *file_name, gint defin, gint defAO, gint32 edgeSpecular)
{
    GString   *file_name_temp = NULL;
    GimpLayer *drawable_ID    = NULL;

    if (file_name != NULL) {
        file_name_temp = getFilename(file_name, "_s");
    }

    if (edgeSpecular) {
        drawable_ID = specularEdgeWorker(image_ID, defin, defAO, TRUE);
    } else {
        drawable_ID = specularSmoothWorker(image_ID, defin, defAO, TRUE);
    }

    if (drawable_ID != NULL) {
        gint32 res = specularSave(file_name_temp, image_ID, drawable_ID);
        if (file_name_temp) g_string_free(file_name_temp, TRUE);
        return res;
    } else {
        if (file_name_temp) g_string_free(file_name_temp, TRUE);
        return 0;
    }
}

void removeGivenLayerFromImage(GimpImage *image_ID, GimpLayer *layer_ID)
{
    GList *layers          = gimp_image_list_layers(image_ID);
    guint  nNumberOfLayers = g_list_length(layers);

    if ((nNumberOfLayers > 1) && (layers != NULL)) {
        for (GList *l = layers; l != NULL; l = l->next) {
            if (l->data != layer_ID) {
                set_single_selected_layer(image_ID, GIMP_LAYER(l->data));
                break;
            }
        }

        if (gimp_layer_is_floating_sel(layer_ID))
        {
            gimp_floating_sel_remove(layer_ID);
        } else {
            if (gimp_image_remove_layer(image_ID, layer_ID) != TRUE) {
                gimp_message("Removing layer failed!");
            }
        }
    } else {
        gimp_message("Not removing layer!");
    }
    g_list_free(layers);
}

void removeShading(GimpImage *image_ID)
{
    GimpLayer *drawable_ID = get_first_selected_layer(image_ID);
    GimpLayer *newlayer_ID = gimp_layer_copy(drawable_ID);

    gimp_image_insert_layer(image_ID, newlayer_ID, NULL, -1);
    set_single_selected_layer(image_ID, newlayer_ID);

    if (plug_in_gauss_connector(image_ID, GIMP_DRAWABLE(newlayer_ID), 20.0f, 20.0f, 0) != 1) return;
    if (plug_in_vinvert_connector(image_ID, GIMP_DRAWABLE(newlayer_ID)) != 1) return;

    gimp_layer_set_mode(newlayer_ID, GIMP_LAYER_MODE_OVERLAY);
    gimp_image_merge_visible_layers(image_ID, GIMP_EXPAND_AS_NECESSARY);
}

void removeShadingPreview(GimpImage *image_ID, gint32 noise_val)
{
    GimpLayer *mergedLayer_ID = NULL;
    GimpLayer *drawable_ID    = get_first_selected_layer(image_ID);
    GimpLayer *newlayer_ID    = gimp_layer_copy(drawable_ID);

    gimp_image_insert_layer(image_ID, newlayer_ID, NULL, -1);
    set_single_selected_layer(image_ID, newlayer_ID);
	if (plug_in_gauss_connector(image_ID, GIMP_DRAWABLE(newlayer_ID), 20.0f, 20.0f, 0) != 1) return;
if (plug_in_vinvert_connector(image_ID, GIMP_DRAWABLE(newlayer_ID)) != 1) return;
gimp_layer_set_mode(newlayer_ID, GIMP_LAYER_MODE_OVERLAY);
mergedLayer_ID = gimp_layer_new_from_visible(image_ID, image_ID, "temp_shadow");
gimp_image_insert_layer(image_ID, mergedLayer_ID, NULL, -1);
set_single_selected_layer(image_ID, mergedLayer_ID);
}
void blur(GimpImage *image_ID, GimpLayer *diffuse_ID, gfloat width, gfloat height, gint32 passes, gint32 normal, gint defAO)
{
GimpLayer *drawable_ID     = NULL;
gint32     i               = 0;
gfloat     fMultiplier     = defAO / 1000.0f;
GimpLayer *desatdiffuse_ID = gimp_layer_copy(diffuse_ID);
gimp_image_insert_layer(image_ID, desatdiffuse_ID, NULL, -1);
set_single_selected_layer(image_ID, desatdiffuse_ID);
if (normal == 0)
{
gimp_drawable_desaturate(GIMP_DRAWABLE(desatdiffuse_ID), GIMP_DESATURATE_LIGHTNESS);
}
for (i = 0; i < passes; i++)
{
drawable_ID = get_first_selected_layer(image_ID);
GimpLayer *newlayer_ID = gimp_layer_copy(drawable_ID);
gimp_image_insert_layer(image_ID, newlayer_ID, NULL, -1);
set_single_selected_layer(image_ID, newlayer_ID);
if (plug_in_gauss_connector(image_ID, GIMP_DRAWABLE(newlayer_ID), width * fMultiplier, height * fMultiplier, 0) != 1) return;
gimp_layer_set_mode(newlayer_ID, GIMP_LAYER_MODE_OVERLAY);
gimp_image_merge_down(image_ID, newlayer_ID, GIMP_EXPAND_AS_NECESSARY);
}
drawable_ID = get_first_selected_layer(image_ID);
if (normal == 1)
{
gfloat fScalar = defAO / 50.0f;
nmapvals.filter        = 0;
nmapvals.minz          = 0.0f;
nmapvals.scale         = fScalar;
nmapvals.wrap          = 0;
nmapvals.height_source = 0;
nmapvals.alpha         = 0;
nmapvals.conversion    = 8;
nmapvals.dudv          = 0;
nmapvals.xinvert       = 0;
nmapvals.yinvert       = 0;
nmapvals.swapRGB       = 0;
nmapvals.contrast      = 0.0f;
nmapvals.alphamap_id   = GIMP_DRAWABLE(drawable_ID);
normalmap(GIMP_DRAWABLE(drawable_ID), FALSE);
gimp_layer_set_mode(drawable_ID, GIMP_LAYER_MODE_OVERLAY);
gimp_image_merge_down(image_ID, drawable_ID, GIMP_EXPAND_AS_NECESSARY);
}
}
void sharpen(GimpImage *image_ID, GimpLayer *diffuse, gfloat depth, gint32 filterSize, gdouble strength)
{
GimpLayer *sharpnormal_ID = gimp_layer_copy(diffuse);
gimp_image_insert_layer(image_ID, sharpnormal_ID, NULL, -1);
set_single_selected_layer(image_ID, sharpnormal_ID);
nmapvals.filter        = filterSize;
nmapvals.minz          = 0.0f;
nmapvals.scale         = depth;
nmapvals.wrap          = 0;
nmapvals.height_source = 0;
nmapvals.alpha         = 0;
nmapvals.conversion    = 0;
nmapvals.dudv          = 0;
nmapvals.xinvert       = 0;
nmapvals.yinvert       = 1;
nmapvals.swapRGB       = 0;
nmapvals.contrast      = 0.0f;
nmapvals.alphamap_id   = GIMP_DRAWABLE(sharpnormal_ID);
normalmap(GIMP_DRAWABLE(sharpnormal_ID), FALSE);
set_single_selected_layer(image_ID, sharpnormal_ID);
gimp_layer_set_mode(sharpnormal_ID, GIMP_LAYER_MODE_OVERLAY);
gimp_layer_set_opacity(sharpnormal_ID, strength);
gimp_image_raise_item_to_top(image_ID, GIMP_ITEM(sharpnormal_ID));
}
void shapeRecognise(GimpImage *image_ID, GimpLayer *normalmap_ID, gdouble strength, gint defAO)
{
gfloat     fGaussFactor  = defAO * 0.40f;
gfloat     fScalar       = defAO / 50.0f;
GimpLayer *blurnormal_ID = gimp_layer_copy(normalmap_ID);
gimp_image_insert_layer(image_ID, blurnormal_ID, NULL, -1);
set_single_selected_layer(image_ID, blurnormal_ID);
if (plug_in_gauss_connector(image_ID, GIMP_DRAWABLE(blurnormal_ID), fGaussFactor, fGaussFactor, 0) != 1) return;
if (plug_in_colors_channel_mixer_connector(image_ID, GIMP_DRAWABLE(blurnormal_ID), 0, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, -1.0f) != 1) return;
nmapvals.filter        = 0;
nmapvals.minz          = 0.0f;
nmapvals.scale         = fScalar;
nmapvals.wrap          = 0;
nmapvals.height_source = 0;
nmapvals.alpha         = 0;
nmapvals.conversion    = 8;
nmapvals.dudv          = 0;
nmapvals.xinvert       = 0;
nmapvals.yinvert       = 0;
nmapvals.swapRGB       = 0;
nmapvals.contrast      = 0.0f;
nmapvals.alphamap_id   = GIMP_DRAWABLE(blurnormal_ID);
normalmap(GIMP_DRAWABLE(blurnormal_ID), FALSE);
if (plug_in_colors_channel_mixer_connector(image_ID, GIMP_DRAWABLE(blurnormal_ID), 0, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, -1.0f) != 1) return;
gimp_layer_set_mode(blurnormal_ID, GIMP_LAYER_MODE_OVERLAY);
gimp_layer_set_opacity(blurnormal_ID, strength);
}
void doBaseMap(GimpImage *image_ID, GimpLayer *diffuse_ID, gfloat Depth, gint32 passes, gint defAO)
{
gfloat fGausianFactor = defAO * 0.06f;
gfloat fScalar        = defAO / 50.0f;
gint32 i              = 0;
for (i = 0; i < passes; i++)
{
GimpLayer *newlayer_ID = gimp_layer_copy(diffuse_ID);
gfloat     ok          = (gfloat)(i + 1) * (gfloat)(i + 1);
gimp_image_insert_layer(image_ID, newlayer_ID, NULL, -1);
set_single_selected_layer(image_ID, newlayer_ID);
if (plug_in_gauss_connector(image_ID, GIMP_DRAWABLE(newlayer_ID), ok * fGausianFactor, ok * fGausianFactor, 0) != 1) return;
nmapvals.filter        = 5;
nmapvals.minz          = 0.0f;
nmapvals.scale         = Depth * ok;
nmapvals.wrap          = 0;
nmapvals.height_source = 0;
nmapvals.alpha         = 0;
nmapvals.conversion    = 0;
nmapvals.dudv          = 0;
nmapvals.xinvert       = 0;
nmapvals.yinvert       = 1;
nmapvals.swapRGB       = 0;
nmapvals.contrast      = 0.0f;
nmapvals.alphamap_id   = GIMP_DRAWABLE(newlayer_ID);
normalmap(GIMP_DRAWABLE(newlayer_ID), FALSE);
if (i > 0)
{
gimp_layer_set_mode(newlayer_ID, GIMP_LAYER_MODE_OVERLAY);
gimp_image_merge_down(image_ID, newlayer_ID, GIMP_EXPAND_AS_NECESSARY);
newlayer_ID = get_first_selected_layer(image_ID);
nmapvals.filter        = 0;
nmapvals.minz          = 0.0f;
nmapvals.scale         = fScalar;
nmapvals.wrap          = 0;
nmapvals.height_source = 0;
nmapvals.alpha         = 0;
nmapvals.conversion    = 8;
nmapvals.dudv          = 0;
nmapvals.xinvert       = 0;
nmapvals.yinvert       = 0;
nmapvals.swapRGB       = 0;
nmapvals.contrast      = 0.0f;
nmapvals.alphamap_id   = GIMP_DRAWABLE(newlayer_ID);
normalmap(GIMP_DRAWABLE(newlayer_ID), FALSE);
}
}
}
gint32 render(GimpImage *img_ID, PlugInVals *vals)
{
GFile     *img_file       = gimp_image_get_file(img_ID);
gchar     *file_name      = img_file ? g_file_get_path(img_file) : NULL;
GimpImage *image_ID       = img_ID;
gint       size           = gimp_image_get_width(image_ID);
gfloat     wsize          = 0.0f;
gfloat     hsize          = 0.0f;
GimpLayer *diffuse_ID     = NULL;
GimpLayer *normalmap_ID   = NULL;
GString   *file_name_temp = NULL;
GimpLayer *drawable_ID    = NULL;
gint32     nResult        = 0;
gfloat     fScalar        = 0.0f;
if (img_file) g_object_unref(img_file);
gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress), 0.0);
gtk_label_set_text(GTK_LABEL(progress_label), "Begin");
if (vals->Resizie)
{
gboolean hiresimg = gimp_image_resize(image_ID, (gint)(vals->newWidth * size), (gint)(vals->newWidth * size), 0, 0);
gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress), 0.01);
gtk_label_set_text(GTK_LABEL(progress_label), "Resizing");
if (hiresimg)
{
drawable_ID = get_first_selected_layer(image_ID);
gimp_layer_scale(drawable_ID, (gint)(vals->newWidth * size), (gint)(vals->newWidth * size), FALSE);
gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress), 0.02);
}
}
else
{
drawable_ID = get_first_selected_layer(image_ID);
gtk_label_set_text(GTK_LABEL(progress_label), "Stretch");
gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress), 0.03);
}
if (vals->Noise) {
gfloat     fGausianNoiseFactor = vals->ao * 0.0040f;
GimpLayer *noiselayer_ID       = gimp_layer_copy(drawable_ID);
gimp_image_insert_layer(image_ID, noiselayer_ID, NULL, -1);
set_single_selected_layer(image_ID, noiselayer_ID);
gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress), 0.03);
gtk_label_set_text(GTK_LABEL(progress_label), "Noise");
if (plug_in_rgb_noise_connector(image_ID, GIMP_DRAWABLE(noiselayer_ID), 1, 1, fGausianNoiseFactor, fGausianNoiseFactor, fGausianNoiseFactor, 0.0f) != 1)
{
g_free(file_name);
return 0;
}
gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress), 0.04);
gimp_layer_set_mode(noiselayer_ID, GIMP_LAYER_MODE_MULTIPLY);
gimp_image_merge_down(image_ID, noiselayer_ID, GIMP_EXPAND_AS_NECESSARY);
}
else
{
gtk_label_set_text(GTK_LABEL(progress_label), "Noise added");
gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress), 0.04);
}
if (vals->RemoveLighting)
{
gtk_label_set_text(GTK_LABEL(progress_label), "Remove Shading");
removeShading(image_ID);
gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress), 0.05);
}
else
{
gtk_label_set_text(GTK_LABEL(progress_label), "Stretch");
gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress), 0.05);
}
diffuse_ID = get_first_selected_layer(image_ID);
gimp_drawable_levels_stretch(GIMP_DRAWABLE(diffuse_ID));
file_name_temp = getFilename(file_name, "_d");
if (vals->Tile)
{
gtk_label_set_text(GTK_LABEL(progress_label), "Making Seamless");
if (plug_in_make_seamless_connector(image_ID, GIMP_DRAWABLE(diffuse_ID)) != 1)
{
if (file_name_temp) g_string_free(file_name_temp, TRUE);
g_free(file_name);
return 0;
}
set_progress_label_as_file(file_name_temp, "_d");
gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress), 0.07);
}
else
{
set_progress_label_as_file(file_name_temp, "_d");
gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress), 0.07);
}
saveLastOperation(image_ID, file_name_temp);
if (file_name_temp) g_string_free(file_name_temp, TRUE);
file_name_temp = NULL;
gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress), 0.1);
wsize = (gfloat)gimp_image_get_width(image_ID);
hsize = (gfloat)gimp_image_get_width(image_ID);
gtk_label_set_text(GTK_LABEL(progress_label), "Smoothing");
blur(image_ID, diffuse_ID, wsize, hsize, vals->LargeDetails, 0, vals->ao);
gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress), 0.15);
file_name_temp = getFilename(file_name, "_h");
normalmap_ID   = get_first_selected_layer(image_ID);
if (vals->smoothstep)
{
gtk_label_set_text(GTK_LABEL(progress_label), "Smoothing");
if (plug_in_blur_connector(image_ID, GIMP_DRAWABLE(normalmap_ID)) != 1)
{
if (file_name_temp) g_string_free(file_name_temp, TRUE);
g_free(file_name);
return 0;
}
gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress), 0.20);
}
else
{
set_progress_label_as_file(file_name_temp, "_h");
gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress), 0.20);
}
if (vals->invh)
{
if (plug_in_vinvert_connector(image_ID, GIMP_DRAWABLE(normalmap_ID)) != 1)
{
if (file_name_temp) g_string_free(file_name_temp, TRUE);
g_free(file_name);
return 0;
}
gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress), 0.25);
}
else
{
set_progress_label_as_file(file_name_temp, "_h");
gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress), 0.25);
}
saveLastOperation(image_ID, file_name_temp);
if (file_name_temp) g_string_free(file_name_temp, TRUE);
file_name_temp = NULL;
gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress), 0.30);
gtk_label_set_text(GTK_LABEL(progress_label), "Base Mapping");
doBaseMap(image_ID, diffuse_ID, vals->Depth, vals->LargeDetails, vals->ao);
gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress), 0.35);
normalmap_ID = get_first_selected_layer(image_ID);
file_name_temp = getFilename(file_name, "_ln");
set_progress_label_as_file(file_name_temp, "_ln");
saveLastOperation(image_ID, file_name_temp);
if (file_name_temp) g_string_free(file_name_temp, TRUE);
file_name_temp = NULL;
gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress), 0.40);
file_name_temp = getFilename(file_name, "_sn");
shapeRecognise(image_ID, normalmap_ID, vals->ShapeRecog, vals->ao);
if (vals->smoothstep)
{
normalmap_ID = get_first_selected_layer(image_ID);
gtk_label_set_text(GTK_LABEL(progress_label), "Smoothing");
if (plug_in_blur_connector(image_ID, GIMP_DRAWABLE(normalmap_ID)) != 1)
{
if (file_name_temp) g_string_free(file_name_temp, TRUE);
g_free(file_name);
return 0;
}
gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress), 0.45);
}
else
{
set_progress_label_as_file(file_name_temp, "_sn");
gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress), 0.45);
}
saveLastOperation(image_ID, file_name_temp);
if (file_name_temp) g_string_free(file_name_temp, TRUE);
file_name_temp = NULL;
gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress), 0.50);
gtk_label_set_text(GTK_LABEL(progress_label), "Sharpen");
sharpen(image_ID, diffuse_ID, vals->Depth, 0, vals->SmallDetails);
gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress), 0.55);
normalmap_ID = get_first_selected_layer(image_ID);
gtk_label_set_text(GTK_LABEL(progress_label), "Sharpen more");
if (plug_in_sharpen_connector(image_ID, GIMP_DRAWABLE(normalmap_ID), 20) != 1)
{
g_free(file_name);
return 0;
}
gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress), 0.60);
file_name_temp = getFilename(file_name, "_hn");
set_progress_label_as_file(file_name_temp, "_hn");
saveLastOperation(image_ID, file_name_temp);
if (file_name_temp) g_string_free(file_name_temp, TRUE);
file_name_temp = NULL;
gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress), 0.65);
gtk_label_set_text(GTK_LABEL(progress_label), "Sharpen again");
sharpen(image_ID, diffuse_ID, vals->Depth, 6, vals->MediumDetails);
gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress), 0.70);
normalmap_ID = get_first_selected_layer(image_ID);
gtk_label_set_text(GTK_LABEL(progress_label), "Smoothing");
if (plug_in_blur_connector(image_ID, GIMP_DRAWABLE(normalmap_ID)) != 1)
{
g_free(file_name);
return 0;
}
gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress), 0.75);
file_name_temp = getFilename(file_name, "_mn");
set_progress_label_as_file(file_name_temp, "_mn");
saveLastOperation(image_ID, file_name_temp);
if (file_name_temp) g_string_free(file_name_temp, TRUE);
file_name_temp = NULL;
gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress), 0.78);
gimp_item_set_visible(GIMP_ITEM(diffuse_ID), FALSE);
gimp_image_merge_visible_layers(image_ID, GIMP_EXPAND_AS_NECESSARY);
drawable_ID = get_first_selected_layer(image_ID);
fScalar                = vals->ao / 50.0f;
nmapvals.filter        = 0;
nmapvals.minz          = 0.0f;
nmapvals.scale         = fScalar;
nmapvals.wrap          = 0;
nmapvals.height_source = 0;
nmapvals.alpha         = 0;
nmapvals.conversion    = 8;
nmapvals.dudv          = 0;
nmapvals.xinvert       = 0;
nmapvals.yinvert       = 0;
nmapvals.swapRGB       = 0;
nmapvals.contrast      = 0.0f;
nmapvals.alphamap_id   = GIMP_DRAWABLE(drawable_ID);
gtk_label_set_text(GTK_LABEL(progress_label), "Normal map");
normalmap(GIMP_DRAWABLE(drawable_ID), FALSE);
gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress), 0.80);
file_name_temp = getFilename(file_name, "_n");
set_progress_label_as_file(file_name_temp, "_n");
saveLastOperation(image_ID, file_name_temp);
if (file_name_temp) g_string_free(file_name_temp, TRUE);
file_name_temp = NULL;
gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress), 0.85);
gtk_label_set_text(GTK_LABEL(progress_label), "Mixing Colors");
if (plug_in_colors_channel_mixer_connector(image_ID, GIMP_DRAWABLE(drawable_ID), 0, -1.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f) != 1)
{
g_free(file_name);
return 0;
}
gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress), 0.87);
gimp_drawable_desaturate(GIMP_DRAWABLE(drawable_ID), GIMP_DESATURATE_LIGHTNESS);
gimp_drawable_levels_stretch(GIMP_DRAWABLE(drawable_ID));
file_name_temp = getFilename(file_name, "_a");
set_progress_label_as_file(file_name_temp, "_a");
saveLastOperation(image_ID, file_name_temp);
if (file_name_temp) g_string_free(file_name_temp, TRUE);
file_name_temp = NULL;
gimp_item_set_visible(GIMP_ITEM(diffuse_ID), TRUE);
set_single_selected_layer(image_ID, diffuse_ID);
gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress), 0.90);
if (vals->EdgeSpecular)
{
gtk_label_set_text(GTK_LABEL(progress_label), "Specular Edging");
}
else
{
gtk_label_set_text(GTK_LABEL(progress_label), "Specular Smoothing");
}
nResult = specularDo(image_ID, file_name, vals->defSpecular, vals->ao, vals->EdgeSpecular);
gtk_label_set_text(GTK_LABEL(progress_label), "Idle...");
gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress), 1.0);
g_free(file_name);
return nResult;
}