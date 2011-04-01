
/* Generated data (by glib-mkenums) */

#include "poppler-enums.h"


/* enumerations from "poppler-action.h" */
#include "poppler-action.h"
static const GEnumValue _poppler_action_type_values[] = {
  { POPPLER_ACTION_UNKNOWN, "POPPLER_ACTION_UNKNOWN", "unknown" },
  { POPPLER_ACTION_GOTO_DEST, "POPPLER_ACTION_GOTO_DEST", "goto-dest" },
  { POPPLER_ACTION_GOTO_REMOTE, "POPPLER_ACTION_GOTO_REMOTE", "goto-remote" },
  { POPPLER_ACTION_LAUNCH, "POPPLER_ACTION_LAUNCH", "launch" },
  { POPPLER_ACTION_URI, "POPPLER_ACTION_URI", "uri" },
  { POPPLER_ACTION_NAMED, "POPPLER_ACTION_NAMED", "named" },
  { POPPLER_ACTION_MOVIE, "POPPLER_ACTION_MOVIE", "movie" },
  { 0, NULL, NULL }
};

GType
poppler_action_type_get_type (void)
{
  static GType type = 0;

  if (G_UNLIKELY (type == 0))
    type = g_enum_register_static ("PopplerActionType", _poppler_action_type_values);

  return type;
}

static const GEnumValue _poppler_dest_type_values[] = {
  { POPPLER_DEST_UNKNOWN, "POPPLER_DEST_UNKNOWN", "unknown" },
  { POPPLER_DEST_XYZ, "POPPLER_DEST_XYZ", "xyz" },
  { POPPLER_DEST_FIT, "POPPLER_DEST_FIT", "fit" },
  { POPPLER_DEST_FITH, "POPPLER_DEST_FITH", "fith" },
  { POPPLER_DEST_FITV, "POPPLER_DEST_FITV", "fitv" },
  { POPPLER_DEST_FITR, "POPPLER_DEST_FITR", "fitr" },
  { POPPLER_DEST_FITB, "POPPLER_DEST_FITB", "fitb" },
  { POPPLER_DEST_FITBH, "POPPLER_DEST_FITBH", "fitbh" },
  { POPPLER_DEST_FITBV, "POPPLER_DEST_FITBV", "fitbv" },
  { 0, NULL, NULL }
};

GType
poppler_dest_type_get_type (void)
{
  static GType type = 0;

  if (G_UNLIKELY (type == 0))
    type = g_enum_register_static ("PopplerDestType", _poppler_dest_type_values);

  return type;
}


/* enumerations from "poppler-document.h" */
#include "poppler-document.h"
static const GEnumValue _poppler_page_layout_values[] = {
  { POPPLER_PAGE_LAYOUT_UNSET, "POPPLER_PAGE_LAYOUT_UNSET", "unset" },
  { POPPLER_PAGE_LAYOUT_SINGLE_PAGE, "POPPLER_PAGE_LAYOUT_SINGLE_PAGE", "single-page" },
  { POPPLER_PAGE_LAYOUT_ONE_COLUMN, "POPPLER_PAGE_LAYOUT_ONE_COLUMN", "one-column" },
  { POPPLER_PAGE_LAYOUT_TWO_COLUMN_LEFT, "POPPLER_PAGE_LAYOUT_TWO_COLUMN_LEFT", "two-column-left" },
  { POPPLER_PAGE_LAYOUT_TWO_COLUMN_RIGHT, "POPPLER_PAGE_LAYOUT_TWO_COLUMN_RIGHT", "two-column-right" },
  { POPPLER_PAGE_LAYOUT_TWO_PAGE_LEFT, "POPPLER_PAGE_LAYOUT_TWO_PAGE_LEFT", "two-page-left" },
  { POPPLER_PAGE_LAYOUT_TWO_PAGE_RIGHT, "POPPLER_PAGE_LAYOUT_TWO_PAGE_RIGHT", "two-page-right" },
  { 0, NULL, NULL }
};

GType
poppler_page_layout_get_type (void)
{
  static GType type = 0;

  if (G_UNLIKELY (type == 0))
    type = g_enum_register_static ("PopplerPageLayout", _poppler_page_layout_values);

  return type;
}

static const GEnumValue _poppler_page_mode_values[] = {
  { POPPLER_PAGE_MODE_UNSET, "POPPLER_PAGE_MODE_UNSET", "unset" },
  { POPPLER_PAGE_MODE_NONE, "POPPLER_PAGE_MODE_NONE", "none" },
  { POPPLER_PAGE_MODE_USE_OUTLINES, "POPPLER_PAGE_MODE_USE_OUTLINES", "use-outlines" },
  { POPPLER_PAGE_MODE_USE_THUMBS, "POPPLER_PAGE_MODE_USE_THUMBS", "use-thumbs" },
  { POPPLER_PAGE_MODE_FULL_SCREEN, "POPPLER_PAGE_MODE_FULL_SCREEN", "full-screen" },
  { POPPLER_PAGE_MODE_USE_OC, "POPPLER_PAGE_MODE_USE_OC", "use-oc" },
  { POPPLER_PAGE_MODE_USE_ATTACHMENTS, "POPPLER_PAGE_MODE_USE_ATTACHMENTS", "use-attachments" },
  { 0, NULL, NULL }
};

GType
poppler_page_mode_get_type (void)
{
  static GType type = 0;

  if (G_UNLIKELY (type == 0))
    type = g_enum_register_static ("PopplerPageMode", _poppler_page_mode_values);

  return type;
}

static const GEnumValue _poppler_font_type_values[] = {
  { POPPLER_FONT_TYPE_UNKNOWN, "POPPLER_FONT_TYPE_UNKNOWN", "unknown" },
  { POPPLER_FONT_TYPE_TYPE1, "POPPLER_FONT_TYPE_TYPE1", "type1" },
  { POPPLER_FONT_TYPE_TYPE1C, "POPPLER_FONT_TYPE_TYPE1C", "type1c" },
  { POPPLER_FONT_TYPE_TYPE3, "POPPLER_FONT_TYPE_TYPE3", "type3" },
  { POPPLER_FONT_TYPE_TRUETYPE, "POPPLER_FONT_TYPE_TRUETYPE", "truetype" },
  { POPPLER_FONT_TYPE_CID_TYPE0, "POPPLER_FONT_TYPE_CID_TYPE0", "cid-type0" },
  { POPPLER_FONT_TYPE_CID_TYPE0C, "POPPLER_FONT_TYPE_CID_TYPE0C", "cid-type0c" },
  { POPPLER_FONT_TYPE_CID_TYPE2, "POPPLER_FONT_TYPE_CID_TYPE2", "cid-type2" },
  { 0, NULL, NULL }
};

GType
poppler_font_type_get_type (void)
{
  static GType type = 0;

  if (G_UNLIKELY (type == 0))
    type = g_enum_register_static ("PopplerFontType", _poppler_font_type_values);

  return type;
}

static const GFlagsValue _poppler_viewer_preferences_values[] = {
  { POPPLER_VIEWER_PREFERENCES_UNSET, "POPPLER_VIEWER_PREFERENCES_UNSET", "unset" },
  { POPPLER_VIEWER_PREFERENCES_HIDE_TOOLBAR, "POPPLER_VIEWER_PREFERENCES_HIDE_TOOLBAR", "hide-toolbar" },
  { POPPLER_VIEWER_PREFERENCES_HIDE_MENUBAR, "POPPLER_VIEWER_PREFERENCES_HIDE_MENUBAR", "hide-menubar" },
  { POPPLER_VIEWER_PREFERENCES_HIDE_WINDOWUI, "POPPLER_VIEWER_PREFERENCES_HIDE_WINDOWUI", "hide-windowui" },
  { POPPLER_VIEWER_PREFERENCES_FIT_WINDOW, "POPPLER_VIEWER_PREFERENCES_FIT_WINDOW", "fit-window" },
  { POPPLER_VIEWER_PREFERENCES_CENTER_WINDOW, "POPPLER_VIEWER_PREFERENCES_CENTER_WINDOW", "center-window" },
  { POPPLER_VIEWER_PREFERENCES_DISPLAY_DOC_TITLE, "POPPLER_VIEWER_PREFERENCES_DISPLAY_DOC_TITLE", "display-doc-title" },
  { POPPLER_VIEWER_PREFERENCES_DIRECTION_RTL, "POPPLER_VIEWER_PREFERENCES_DIRECTION_RTL", "direction-rtl" },
  { 0, NULL, NULL }
};

GType
poppler_viewer_preferences_get_type (void)
{
  static GType type = 0;

  if (G_UNLIKELY (type == 0))
    type = g_flags_register_static ("PopplerViewerPreferences", _poppler_viewer_preferences_values);

  return type;
}

static const GFlagsValue _poppler_permissions_values[] = {
  { POPPLER_PERMISSIONS_OK_TO_PRINT, "POPPLER_PERMISSIONS_OK_TO_PRINT", "ok-to-print" },
  { POPPLER_PERMISSIONS_OK_TO_MODIFY, "POPPLER_PERMISSIONS_OK_TO_MODIFY", "ok-to-modify" },
  { POPPLER_PERMISSIONS_OK_TO_COPY, "POPPLER_PERMISSIONS_OK_TO_COPY", "ok-to-copy" },
  { POPPLER_PERMISSIONS_OK_TO_ADD_NOTES, "POPPLER_PERMISSIONS_OK_TO_ADD_NOTES", "ok-to-add-notes" },
  { POPPLER_PERMISSIONS_FULL, "POPPLER_PERMISSIONS_FULL", "full" },
  { 0, NULL, NULL }
};

GType
poppler_permissions_get_type (void)
{
  static GType type = 0;

  if (G_UNLIKELY (type == 0))
    type = g_flags_register_static ("PopplerPermissions", _poppler_permissions_values);

  return type;
}


/* enumerations from "poppler.h" */
#include "poppler.h"
static const GEnumValue _poppler_error_values[] = {
  { POPPLER_ERROR_INVALID, "POPPLER_ERROR_INVALID", "invalid" },
  { POPPLER_ERROR_ENCRYPTED, "POPPLER_ERROR_ENCRYPTED", "encrypted" },
  { 0, NULL, NULL }
};

GType
poppler_error_get_type (void)
{
  static GType type = 0;

  if (G_UNLIKELY (type == 0))
    type = g_enum_register_static ("PopplerError", _poppler_error_values);

  return type;
}

static const GEnumValue _poppler_orientation_values[] = {
  { POPPLER_ORIENTATION_PORTRAIT, "POPPLER_ORIENTATION_PORTRAIT", "portrait" },
  { POPPLER_ORIENTATION_LANDSCAPE, "POPPLER_ORIENTATION_LANDSCAPE", "landscape" },
  { POPPLER_ORIENTATION_UPSIDEDOWN, "POPPLER_ORIENTATION_UPSIDEDOWN", "upsidedown" },
  { POPPLER_ORIENTATION_SEASCAPE, "POPPLER_ORIENTATION_SEASCAPE", "seascape" },
  { 0, NULL, NULL }
};

GType
poppler_orientation_get_type (void)
{
  static GType type = 0;

  if (G_UNLIKELY (type == 0))
    type = g_enum_register_static ("PopplerOrientation", _poppler_orientation_values);

  return type;
}

static const GEnumValue _poppler_backend_values[] = {
  { POPPLER_BACKEND_UNKNOWN, "POPPLER_BACKEND_UNKNOWN", "unknown" },
  { POPPLER_BACKEND_SPLASH, "POPPLER_BACKEND_SPLASH", "splash" },
  { POPPLER_BACKEND_CAIRO, "POPPLER_BACKEND_CAIRO", "cairo" },
  { 0, NULL, NULL }
};

GType
poppler_backend_get_type (void)
{
  static GType type = 0;

  if (G_UNLIKELY (type == 0))
    type = g_enum_register_static ("PopplerBackend", _poppler_backend_values);

  return type;
}


/* Generated data ends here */

