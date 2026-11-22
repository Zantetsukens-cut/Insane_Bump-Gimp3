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

#include <string.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "main.h"
#include "interface.h"
#include "InsaneBump.h"

#include "plugin-intl.h"

/*  Constants  */
#define PLUG_IN_PROC           "plug-in-insanebump"
#define PLUG_IN_BINARY         "gimp-plugin-insanebump"
#define PLUG_IN_LOCALDIR       "<Image>/Filters/Map"
#define PLUG_IN_AUTHOR_LINEONE "Derby Russell <jdrussell51@gmail.com>"
#define PLUG_IN_AUTHOR_LINETWO "Copyright 2013 Derby Russell"

/*  GObject Plug-in Boilerplate  */
typedef struct _InsaneBump InsaneBump;

struct _InsaneBump
{
    GimpPlugIn parent_instance;
};

#define INSANE_BUMP_TYPE (insane_bump_get_type ())
G_DECLARE_FINAL_TYPE (InsaneBump, insane_bump, INSANE_BUMP, PLUG_IN, GimpPlugIn)

static GList          *insane_bump_query_procedures (GimpPlugIn           *plug_in);
static GimpProcedure  *insane_bump_create_procedure (GimpPlugIn           *plug_in,
                                                     const gchar          *name);
static GimpValueArray *insane_bump_run              (GimpProcedure        *procedure,
                                                     GimpRunMode           run_mode,
                                                     GimpImage            *image,
                                                     GimpDrawable        **drawables,
                                                     GimpProcedureConfig  *config,
                                                     gpointer              run_data);

G_DEFINE_TYPE (InsaneBump, insane_bump, GIMP_TYPE_PLUG_IN)

GIMP_MAIN (INSANE_BUMP_TYPE)

/*  Global / Local variables  */
NormalmapVals nmapvals =
{
    0,
    0.0,
    1.0,
    0,
    0,
    ALPHA_NONE,
    CONVERT_NONE,
    DUDV_NONE,
    0,
    0,
    0,
    0.0,
    0
};

const PlugInVals default_vals =
{
    0,
    0,
    0,
    2,
    1,
    64,
    20.00f,
    3,
    50,
    50,
    50,
    1,
    0,
    0,
    50,
    NULL,
    1
};

static PlugInVals vals;

static void
insane_bump_class_init (InsaneBumpClass *klass)
{
    GimpPlugInClass *plug_in_class = GIMP_PLUG_IN_CLASS (klass);

    plug_in_class->query_procedures = insane_bump_query_procedures;
    plug_in_class->create_procedure = insane_bump_create_procedure;
    plug_in_class->set_i18n         = NULL;
}

static void
insane_bump_init (InsaneBump *insane_bump)
{
}

static GList *
insane_bump_query_procedures (GimpPlugIn *plug_in)
{
    return g_list_append (NULL, g_strdup (PLUG_IN_PROC));
}

static GimpProcedure *
insane_bump_create_procedure (GimpPlugIn  *plug_in,
                              const gchar *name)
{
    GimpProcedure *procedure = NULL;

    if (strcmp (name, PLUG_IN_PROC) == 0)
    {
        procedure = gimp_image_procedure_new (plug_in, name,
                                              GIMP_PDB_PROC_TYPE_PLUGIN,
                                              insane_bump_run, NULL, NULL);

        gimp_procedure_set_image_types (procedure, "RGB*, GRAY*, INDEXED*");
        gimp_procedure_set_sensitivity_mask (procedure,
                                             GIMP_PROCEDURE_SENSITIVE_DRAWABLE);

        gimp_procedure_set_menu_label (procedure, N_("_InsaneBump..."));
        gimp_procedure_add_menu_path (procedure, PLUG_IN_LOCALDIR);

        gimp_procedure_set_documentation (procedure,
                                          N_("Produce images needed for Graphical Rendering"),
                                          "This plug-in automatically produces specular, "
                                          "normal and other images from current image.",
                                          name);
        gimp_procedure_set_attribution (procedure,
                                        PLUG_IN_AUTHOR_LINEONE,
                                        PLUG_IN_AUTHOR_LINETWO,
                                        "2013 - 2026 (v3.0.0)");

        gimp_procedure_add_boolean_argument (procedure, "remove-lighting",
                                             "Remove Lighting",
                                             "Remove Lighting",
                                             FALSE, G_PARAM_READWRITE);
        gimp_procedure_add_boolean_argument (procedure, "resizie",
                                             "Upscale(HD)",
                                             "Upscale(HD)",
                                             FALSE, G_PARAM_READWRITE);
        gimp_procedure_add_boolean_argument (procedure, "tile",
                                             "Tile",
                                             "Tile",
                                             FALSE, G_PARAM_READWRITE);
        gimp_procedure_add_int_argument (procedure, "new-width",
                                         "New Width",
                                         "New Width (Integer Times larger)",
                                         1, 16, 2, G_PARAM_READWRITE);
        gimp_procedure_add_boolean_argument (procedure, "edge-specular",
                                             "Edge Enhancing Specular",
                                             "Edge Enhancing Specular",
                                             TRUE, G_PARAM_READWRITE);
        gimp_procedure_add_int_argument (procedure, "def-specular",
                                         "Specular Definition",
                                         "Specular Definition (0-255)",
                                         0, 255, 64, G_PARAM_READWRITE);
        gimp_procedure_add_double_argument (procedure, "depth",
                                            "Depth",
                                            "Depth (+/-)",
                                            -1000.0, 1000.0, 20.0, G_PARAM_READWRITE);
        gimp_procedure_add_int_argument (procedure, "large-details",
                                         "Large Detail Size",
                                         "Large Detail Size",
                                         0, 1000, 3, G_PARAM_READWRITE);
        gimp_procedure_add_int_argument (procedure, "medium-details",
                                         "Medium Detail Intensity",
                                         "Medium Detail Intensity (%)",
                                         0, 1000, 50, G_PARAM_READWRITE);
        gimp_procedure_add_int_argument (procedure, "small-details",
                                         "Small Detail Intensity",
                                         "Small Detail Intensity (%)",
                                         0, 1000, 50, G_PARAM_READWRITE);
        gimp_procedure_add_int_argument (procedure, "shape-recog",
                                         "Shape Recognition",
                                         "Shape Recognition (%)",
                                         0, 1000, 50, G_PARAM_READWRITE);
        gimp_procedure_add_boolean_argument (procedure, "smoothstep",
                                             "Smooth Step",
                                             "Smooth Step",
                                             TRUE, G_PARAM_READWRITE);
        gimp_procedure_add_boolean_argument (procedure, "noise",
                                             "Noise",
                                             "Noise",
                                             FALSE, G_PARAM_READWRITE);
        gimp_procedure_add_boolean_argument (procedure, "invh",
                                             "Invert Height Map",
                                             "Invert Height Map",
                                             FALSE, G_PARAM_READWRITE);
        gimp_procedure_add_int_argument (procedure, "ao",
                                         "AO",
                                         "Ambient Occlusion",
                                         0, 1000, 50, G_PARAM_READWRITE);
        gimp_procedure_add_boolean_argument (procedure, "prev",
                                             "Preview",
                                             "Enable Preview",
                                             TRUE, G_PARAM_READWRITE);
    }

    return procedure;
}

static void
load_vals_from_config (GimpProcedureConfig *config, PlugInVals *v)
{
    gboolean remove_lighting, resizie, tile, edge_specular, smoothstep, noise, invh, prev;
    gdouble  depth;

    g_object_get (config,
	"remove-lighting", &remove_lighting,
"resizie",         &resizie,
"tile",            &tile,
"new-width",       &v->newWidth,
"edge-specular",   &edge_specular,
"def-specular",    &v->defSpecular,
"depth",           &depth,
"large-details",   &v->LargeDetails,
"medium-details",  &v->MediumDetails,
"small-details",   &v->SmallDetails,
"shape-recog",     &v->ShapeRecog,
"smoothstep",      &smoothstep,
"noise",           &noise,
"invh",            &invh,
"ao",              &v->ao,
"prev",            &prev,
NULL);
v->RemoveLighting = remove_lighting ? 1 : 0;
v->Resizie        = resizie         ? 1 : 0;
v->Tile           = tile            ? 1 : 0;
v->EdgeSpecular   = edge_specular   ? 1 : 0;
v->Depth          = (gfloat) depth;
v->smoothstep     = smoothstep      ? 1 : 0;
v->Noise          = noise           ? 1 : 0;
v->invh           = invh            ? 1 : 0;
v->prev           = prev            ? 1 : 0;
}
static void
save_vals_to_config (GimpProcedureConfig *config, const PlugInVals *v)
{
g_object_set (config,
"remove-lighting", (gboolean) (v->RemoveLighting != 0),
"resizie",         (gboolean) (v->Resizie != 0),
"tile",            (gboolean) (v->Tile != 0),
"new-width",       v->newWidth,
"edge-specular",   (gboolean) (v->EdgeSpecular != 0),
"def-specular",    v->defSpecular,
"depth",           (gdouble) v->Depth,
"large-details",   v->LargeDetails,
"medium-details",  v->MediumDetails,
"small-details",   v->SmallDetails,
"shape-recog",     v->ShapeRecog,
"smoothstep",      (gboolean) (v->smoothstep != 0),
"noise",           (gboolean) (v->Noise != 0),
"invh",            (gboolean) (v->invh != 0),
"ao",              v->ao,
"prev",            (gboolean) (v->prev != 0),
NULL);
}
static GimpValueArray *
insane_bump_run (GimpProcedure        *procedure,
GimpRunMode           run_mode,
GimpImage            *image,
GimpDrawable        **drawables,
GimpProcedureConfig  *config,
gpointer              run_data)
{
GimpDrawable *drawable = NULL;
gint          nDlgResponse = -5;
init_drawables ();
bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
#ifdef HAVE_BIND_TEXTDOMAIN_CODESET
bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
#endif
textdomain (GETTEXT_PACKAGE);
    if (drawables == NULL || drawables[0] == NULL)
    {
        return gimp_procedure_new_return_values (procedure,
                                                 GIMP_PDB_CALLING_ERROR,
                                                 NULL);
    }

    drawable = drawables[0];

copyPlugInVals (&default_vals, &vals);
load_vals_from_config (config, &vals);
vals.image_ID = image;
switch (run_mode)
{
case GIMP_RUN_INTERACTIVE:
gimp_ui_init (PLUG_IN_BINARY);
nDlgResponse = InsaneBumpDialog (drawable, PLUG_IN_BINARY, &vals);
switch (nDlgResponse)
{
case 0:
return gimp_procedure_new_return_values (procedure,
GIMP_PDB_EXECUTION_ERROR,
NULL);
case 1:
copyPlugInVals (&local_vals, &vals);
save_vals_to_config (config, &vals);
/* fall through */
default:
gimp_displays_flush ();
return gimp_procedure_new_return_values (procedure,
GIMP_PDB_SUCCESS,
NULL);
}
break;
case GIMP_RUN_NONINTERACTIVE:
case GIMP_RUN_WITH_LAST_VALS:
if (render (image, &vals) != 0)
{
return gimp_procedure_new_return_values (procedure,
GIMP_PDB_EXECUTION_ERROR,
NULL);
}
if (run_mode != GIMP_RUN_NONINTERACTIVE)
gimp_displays_flush ();
break;
default:
break;
}
return gimp_procedure_new_return_values (procedure,
GIMP_PDB_SUCCESS,
NULL);
}
void copyPlugInVals(const PlugInVals *some_vals_source, PlugInVals *some_vals_destination)
{
if (some_vals_source == NULL) return;
if (some_vals_destination == NULL) return;
some_vals_destination->RemoveLighting = some_vals_source->RemoveLighting;
some_vals_destination->Resizie        = some_vals_source->Resizie;
some_vals_destination->Tile           = some_vals_source->Tile;
some_vals_destination->newWidth       = some_vals_source->newWidth;
some_vals_destination->EdgeSpecular   = some_vals_source->EdgeSpecular;
some_vals_destination->defSpecular    = some_vals_source->defSpecular;
some_vals_destination->Depth          = some_vals_source->Depth;
some_vals_destination->LargeDetails   = some_vals_source->LargeDetails;
some_vals_destination->MediumDetails  = some_vals_source->MediumDetails;
some_vals_destination->SmallDetails   = some_vals_source->SmallDetails;
some_vals_destination->ShapeRecog     = some_vals_source->ShapeRecog;
some_vals_destination->smoothstep     = some_vals_source->smoothstep;
some_vals_destination->Noise          = some_vals_source->Noise;
some_vals_destination->invh           = some_vals_source->invh;
some_vals_destination->ao             = some_vals_source->ao;
some_vals_destination->prev           = some_vals_source->prev;
some_vals_destination->image_ID       = some_vals_source->image_ID;
}
