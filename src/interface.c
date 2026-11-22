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

/*  Global variables  */
PlugInVals local_vals;
int update_preview = 0;
int preview_progress_reset = 1;
gint dialog_is_init = 0;
GtkWidget *g_gwRemoveLtgBtn = NULL;
GtkWidget *g_gwUpscaleBtn = NULL;
GtkWidget *g_gwTileBtn = NULL;
GtkWidget *g_gwEdgeBtn = NULL;
GtkWidget *g_gwSmoothBtn = NULL;
GtkWidget *g_gwNoiseBtn = NULL;
GtkWidget *g_gwInvertBtn = NULL;
GtkWidget *g_gwWidthSpin = NULL;
GtkWidget *g_gwSpecDefSpin = NULL;
GtkWidget *g_gwDepthSpin = NULL;
GtkWidget *g_gwLargeDSpin = NULL;
GtkWidget *g_gwMediumDSpin = NULL;
GtkWidget *g_gwSmallDSpin = NULL;
GtkWidget *g_gwShapeSpin = NULL;
GtkWidget *g_gwAOSpin = NULL;

/*  Local variables  */
static GtkWidget *dialog = NULL;
GtkWidget *progress = NULL;
GtkWidget *progress_label = NULL;
gint runme = 0;
int preview_progress_timer = 0;

/*  Private functions  */
static void do_cleanup(GtkWidget *widget, gpointer data)
{
   gtk_main_quit();
   _active = 'x';
}

static gboolean idle_callback(gpointer data)
{
   if ((update_preview) && (_active == 'x'))
   {
        preview_redraw();
   }
   if (preview_progress_reset == 0) {
       if (preview_progress_timer < 5) {
           preview_progress_timer++;
       } else {
           preview_progress_reset = 1;
           preview_progress_timer = 0;
           gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress), 0.0);
       }
   }
   return G_SOURCE_CONTINUE;
}

static void new_width_changed(GtkWidget *widget, gpointer data)
{
   local_vals.newWidth = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(widget));
   _active = 'x';
   update_preview = 1;
}

static void def_specular_changed(GtkWidget *widget, gpointer data)
{
   local_vals.defSpecular = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(widget));
   _active = 'x';
   update_preview = 1;
}

static void depth_changed(GtkWidget *widget, gpointer data)
{
   local_vals.Depth = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(widget));
   _active = 'x';
   update_preview = 1;
}

static void large_detail_size_changed(GtkWidget *widget, gpointer data)
{
   local_vals.LargeDetails = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(widget));
   _active = 'x';
   update_preview = 1;
}

static void medium_detail_intensity_changed(GtkWidget *widget, gpointer data)
{
   local_vals.MediumDetails = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(widget));
   _active = 'x';
   update_preview = 1;
}

static void small_detail_intensity_changed(GtkWidget *widget, gpointer data)
{
   local_vals.SmallDetails = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(widget));
   _active = 'x';
   update_preview = 1;
}

static void shape_recognition_changed(GtkWidget *widget, gpointer data)
{
   local_vals.ShapeRecog = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(widget));
   _active = 'x';
   update_preview = 1;
}

static void ao_changed(GtkWidget *widget, gpointer data)
{
   local_vals.ao = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(widget));
   _active = 'x';
   update_preview = 1;
}

static void insanebump_dialog_response(GtkWidget *widget, gint response_id,
                                      gpointer data)
{
   switch(response_id)
   {
      case GTK_RESPONSE_OK:
         runme = 2;
         if (render(local_vals.image_ID, &local_vals) == 0)
         {
             runme = 1;
         }
         removeAllLayersExceptMain();
         preview_progress_reset = 0;
         return;
      default:
         runme = 2;
         gtk_widget_destroy(widget);
         _active = 'x';
         break;
   }
}

static GtkWidget *CreateToggleButton(GtkWidget *grid, gint row, GCallback function, const gchar *szName, gint32 nVal)
{
    GtkWidget *btn = NULL;
    if (nVal == 1)
    {
       btn = gtk_toggle_button_new_with_label("Yes");
    }
    else
    {
       btn = gtk_toggle_button_new_with_label("No");
    }
    if (btn != NULL)
    {
        g_signal_connect(btn, "clicked", function, NULL);
        gimp_grid_attach_aligned(GTK_GRID(grid), 0, row, szName, 0.0, 0.5,
                                 btn, 1);
        gtk_widget_show(btn);
        if (nVal == 1)
        {
           gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(btn), TRUE);
        }
    }
    return btn;
}

static GtkWidget *CreateInteger255SpinButton(GtkWidget *grid, gint row, GCallback function, const gchar *szName, gint32 nVal, gint32 allow_neg)
{
   GtkAdjustment *adj = NULL;
   GtkWidget *spin = NULL;
   if (allow_neg)
   {
       adj = gtk_adjustment_new((gdouble)nVal, -1000, 1000, 1, 10, 0);
   }
   else
   {
       adj = gtk_adjustment_new((gdouble)nVal, 0, 1000, 1, 10, 0);
   }
   spin = gtk_spin_button_new(adj, 1, 0);
   gtk_widget_show(spin);
   gtk_spin_button_set_update_policy(GTK_SPIN_BUTTON(spin), GTK_UPDATE_IF_VALID);
   g_signal_connect(spin, "value-changed", function, NULL);
   gimp_grid_attach_aligned(GTK_GRID(grid), 0, row, szName, 0.0, 0.5,
                            spin, 1);

   return spin;
}

static void grid_attach_progress(GtkGrid     *grid,
                                 gint         column,
                                 gint         row,
                                 const gchar *label_text,
                                 gfloat       xalign,
                                 gfloat       yalign,
                                 GtkWidget   *widget,
                                 gint         colspan,
                                 gboolean     left_align)
{
    if (label_text)
    {
        progress_label = gtk_label_new_with_mnemonic(label_text);
        gtk_label_set_xalign(GTK_LABEL(progress_label), xalign);
        gtk_label_set_yalign(GTK_LABEL(progress_label), yalign);
        gtk_label_set_justify(GTK_LABEL(progress_label), GTK_JUSTIFY_LEFT);
        gtk_grid_attach(grid, progress_label, column, row, 1, 1);
        gtk_widget_show(progress_label);
    }

    if (left_align)
    {
        GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);

        gtk_box_pack_start(GTK_BOX(hbox), widget, FALSE, FALSE, 0);
        gtk_widget_show(widget);

        widget = hbox;
    }

    gtk_widget_set_hexpand(widget, TRUE);
    gtk_grid_attach(grid, widget, column, row + 1, colspan + 2, 1);
    gtk_widget_show(widget);
}

/*  Public functions  */

gint InsaneBumpDialog(GimpDrawable *drawable,
                      const gchar *szBinary,
                      PlugInVals *vals)
{
    GtkWidget *hbox;
    GtkWidget *grid;
    GtkWidget *content_area;

    if (_active != 'x') return 2;

    copyPlugInVals(vals, &local_vals);

    gimp_ui_init(szBinary);

    dialog = gimp_dialog_new(NAME_AND_VERSION, szBinary,
                             NULL, 0, gimp_standard_help_func, NULL,
                             "_Close", GTK_RESPONSE_CANCEL,
                             "_Execute", GTK_RESPONSE_OK,
                             NULL);

    g_signal_connect(dialog, "response",
                     G_CALLBACK(insanebump_dialog_response),
                     NULL);
    g_signal_connect(dialog, "destroy",
                     G_CALLBACK(do_cleanup),
                     NULL);

    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_container_set_border_width(GTK_CONTAINER(hbox), 8);
    content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    gtk_box_pack_start(GTK_BOX(content_area), hbox, TRUE, TRUE, 0);
    gtk_widget_show(hbox);

    CreateLeftPreviewFrames(hbox);
    CreateRightPreviewToggleButton(hbox);

    grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 8);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 8);
    gtk_widget_show(grid);
    gtk_box_pack_start(GTK_BOX(hbox), grid, TRUE, TRUE, 0);

    g_gwRemoveLtgBtn = CreateToggleButton(grid, 1, G_CALLBACK(remlightbtn_clicked), "Remove Lighting", vals->RemoveLighting);
    g_gwUpscaleBtn   = CreateToggleButton(grid, 2, G_CALLBACK(upscale_HD_clicked), "Upscale(HD)", vals->Resizie);
    g_gwTileBtn      = CreateToggleButton(grid, 3, G_CALLBACK(tile_clicked), "Tile", vals->Tile);
    g_gwWidthSpin    = CreateInteger255SpinButton(grid, 4, G_CALLBACK(new_width_changed), "New Width(Integer Times larger)", vals->newWidth, 0);
    g_gwEdgeBtn      = CreateToggleButton(grid, 5, G_CALLBACK(edge_enhancing_specular_clicked), "Edge Enhancing Specular", vals->EdgeSpecular);
    g_gwSpecDefSpin  = CreateInteger255SpinButton(grid, 6, G_CALLBACK(def_specular_changed), "Specular Definition(0-255)", vals->defSpecular, 0);
    g_gwDepthSpin    = CreateInteger255SpinButton(grid, 7, G_CALLBACK(depth_changed), "Depth(+/-)", vals->Depth, 1);
    g_gwLargeDSpin   = CreateInteger255SpinButton(grid, 8, G_CALLBACK(large_detail_size_changed), "Large Detail Size", vals->LargeDetails, 0);
    g_gwMediumDSpin  = CreateInteger255SpinButton(grid, 9, G_CALLBACK(medium_detail_intensity_changed), "Medium Detail Intensity(%)", vals->MediumDetails, 0);
    g_gwSmallDSpin   = CreateInteger255SpinButton(grid, 10, G_CALLBACK(small_detail_intensity_changed), "Small Detail Intensity(%)", vals->SmallDetails, 0);
    g_gwShapeSpin    = CreateInteger255SpinButton(grid, 11, G_CALLBACK(shape_recognition_changed), "Shape Recognition(%)", vals->ShapeRecog, 0);
    g_gwSmoothBtn    = CreateToggleButton(grid, 12, G_CALLBACK(smooth_step_clicked), "Smooth Step", vals->smoothstep);
    g_gwNoiseBtn     = CreateToggleButton(grid, 13, G_CALLBACK(noise_clicked), "Noise", vals->Noise);
    g_gwInvertBtn    = CreateToggleButton(grid, 14, G_CALLBACK(invert_height_map_clicked), "Invert Height Map", vals->invh);
    g_gwAOSpin       = CreateInteger255SpinButton(grid, 15, G_CALLBACK(ao_changed), "ao", vals->ao, 0);
    progress = gtk_progress_bar_new();
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress), 0.0);
    grid_attach_progress(GTK_GRID(grid), 0, 17, "Idle...", 0.0, 0.5,
    progress, 1, FALSE);
    gtk_widget_show(progress);
    gtk_widget_show(dialog);
    update_preview = 1;
    g_timeout_add(300, idle_callback, drawable);
    runme = 0;
    dialog_is_init = 1;
    _active = 'x';
    gtk_main();
    return runme;
    }
    void remlightbtn_clicked(GtkWidget *widget, gpointer data)
    {
    if (!dialog_is_init) return;
    if (local_vals.RemoveLighting == 0)
    {
    local_vals.RemoveLighting = 1;
    gtk_button_set_label(GTK_BUTTON(widget), "Yes");
    }
    else
    {
    local_vals.RemoveLighting = 0;
    gtk_button_set_label(GTK_BUTTON(widget), "No");
    }
    _active = 'x';
    update_preview = 1;
    }
    void upscale_HD_clicked(GtkWidget *widget, gpointer data)
    {
    if (!dialog_is_init) return;
    if (local_vals.Resizie == 0)
    {
    local_vals.Resizie = 1;
    gtk_button_set_label(GTK_BUTTON(widget), "Yes");
    }
    else
    {
    local_vals.Resizie = 0;
    gtk_button_set_label(GTK_BUTTON(widget), "No");
    }
    _active = 'x';
    update_preview = 1;
    }
    void tile_clicked(GtkWidget *widget, gpointer data)
    {
    if (!dialog_is_init) return;
    if (local_vals.Tile == 0)
    {
    local_vals.Tile = 1;
    gtk_button_set_label(GTK_BUTTON(widget), "Yes");
    }
    else
    {
    local_vals.Tile = 0;
    gtk_button_set_label(GTK_BUTTON(widget), "No");
    }
    _active = 'x';
    update_preview = 1;
    }
    void edge_enhancing_specular_clicked(GtkWidget *widget, gpointer data)
    {
    if (!dialog_is_init) return;
    if (local_vals.EdgeSpecular == 0)
    {
    local_vals.EdgeSpecular = 1;
    gtk_button_set_label(GTK_BUTTON(widget), "Yes");
    }
    else
    {
    local_vals.EdgeSpecular = 0;
    gtk_button_set_label(GTK_BUTTON(widget), "No");
    }
    _active = 'x';
    update_preview = 1;
    }
    void smooth_step_clicked(GtkWidget *widget, gpointer data)
    {
    if (!dialog_is_init) return;
    if (local_vals.smoothstep == 0)
    {
    local_vals.smoothstep = 1;
    gtk_button_set_label(GTK_BUTTON(widget), "Yes");
    }
    else
    {
    local_vals.smoothstep = 0;
    gtk_button_set_label(GTK_BUTTON(widget), "No");
    }
    _active = 'x';
    update_preview = 1;
    }
    void noise_clicked(GtkWidget *widget, gpointer data)
    {
    if (!dialog_is_init) return;
    if (local_vals.Noise == 0)
    {
    local_vals.Noise = 1;
    gtk_button_set_label(GTK_BUTTON(widget), "Yes");
    }
    else
    {
    local_vals.Noise = 0;
    gtk_button_set_label(GTK_BUTTON(widget), "No");
    }
    _active = 'x';
    update_preview = 1;
    }
    void invert_height_map_clicked(GtkWidget *widget, gpointer data)
    {
    if (!dialog_is_init) return;
    if (local_vals.invh == 0)
    {
    local_vals.invh = 1;
    gtk_button_set_label(GTK_BUTTON(widget), "Yes");
    }
    else
    {
    local_vals.invh = 0;
    gtk_button_set_label(GTK_BUTTON(widget), "No");
    }
    _active = 'x';
    update_preview = 1;
    }
	