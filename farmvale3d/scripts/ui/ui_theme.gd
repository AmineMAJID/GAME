class_name UITheme
extends RefCounted
## ============================================================
## FarmVale — thème UI « moderne & stylisé »
## Paleta chaude (crème / or / vert profond), panneaux arrondis
## translucides, texte lisible avec ombre.
## ============================================================

const CREAM := Color("#F7F0DC")
const GOLD := Color("#F2C14E")
const GOLD_DIM := Color("#C99A3A")
const PANEL := Color(0.07, 0.10, 0.08, 0.78)
const PANEL_SOLID := Color("#16241A")
const PANEL_LIGHT := Color(0.12, 0.18, 0.13, 0.9)
const GREEN := Color("#8FBF6A")
const RED := Color("#E06A5A")
const WHITE := Color("#FFFFFF")
const SHADOW := Color(0, 0, 0, 0.45)


static func build() -> Theme:
	var theme := Theme.new()
	var font := load("res://assets/fonts/DejaVuSans-Bold.ttf") as Font
	if font:
		theme.default_font = font
	theme.default_font_size = 18

	# --- Label
	theme.set_color("font_color", "Label", CREAM)
	theme.set_color("font_shadow_color", "Label", SHADOW)
	theme.set_constant("shadow_offset_x", "Label", 1)
	theme.set_constant("shadow_offset_y", "Label", 2)
	theme.set_constant("shadow_outline_size", "Label", 0)

	# --- Boutons
	var normal := _panel(PANEL, 14, Color(1, 1, 1, 0.14), 2)
	var hover := _panel(PANEL_LIGHT, 14, GOLD, 3)
	var pressed := _panel(Color(0.10, 0.16, 0.11, 0.95), 14, GOLD_DIM, 3)
	var focus := _panel(PANEL_LIGHT, 14, GOLD, 3)
	var disabled := _panel(Color(0.08, 0.10, 0.08, 0.6), 14, Color(1, 1, 1, 0.08), 2)
	theme.set_stylebox("normal", "Button", normal)
	theme.set_stylebox("hover", "Button", hover)
	theme.set_stylebox("pressed", "Button", pressed)
	theme.set_stylebox("focus", "Button", focus)
	theme.set_stylebox("disabled", "Button", disabled)
	theme.set_color("font_color", "Button", CREAM)
	theme.set_color("font_hover_color", "Button", GOLD)
	theme.set_color("font_pressed_color", "Button", GOLD)
	theme.set_color("font_disabled_color", "Button", Color(1, 1, 1, 0.4))
	theme.set_color("font_focus_color", "Button", GOLD)
	theme.set_font_size("font_size", "Button", 22)

	# --- PanelContainer
	theme.set_stylebox("panel", "PanelContainer", _panel(PANEL, 18, Color(1, 1, 1, 0.12), 2))

	# --- Panel (utilisé pour les icônes rondes : pièce, météo, slots)
	theme.set_stylebox("panel", "Panel", _panel(PANEL, 10, Color(1, 1, 1, 0.12), 2))

	# --- ProgressBar (énergie)
	var bar_bg := StyleBoxFlat.new()
	bar_bg.bg_color = Color(0.05, 0.08, 0.06, 0.8)
	bar_bg.corner_radius_top_left = 7
	bar_bg.corner_radius_top_right = 7
	bar_bg.corner_radius_bottom_right = 7
	bar_bg.corner_radius_bottom_left = 7
	var bar_fill := StyleBoxFlat.new()
	bar_fill.bg_color = Color("#9CCB6A")
	bar_fill.corner_radius_top_left = 7
	bar_fill.corner_radius_top_right = 7
	bar_fill.corner_radius_bottom_right = 7
	bar_fill.corner_radius_bottom_left = 7
	theme.set_stylebox("background", "ProgressBar", bar_bg)
	theme.set_stylebox("fill", "ProgressBar", bar_fill)
	theme.set_color("font_color", "ProgressBar", CREAM)
	theme.set_font_size("font_size", "ProgressBar", 13)

	# --- HSlider (options)
	theme.set_constant("grabber_offset", "HSlider", 0)
	var slider_bg := StyleBoxFlat.new()
	slider_bg.bg_color = Color(0.05, 0.08, 0.06, 0.8)
	slider_bg.corner_radius_top_left = 6
	slider_bg.corner_radius_top_right = 6
	slider_bg.corner_radius_bottom_right = 6
	slider_bg.corner_radius_bottom_left = 6
	slider_bg.content_margin_top = 7
	slider_bg.content_margin_bottom = 7
	theme.set_stylebox("slider", "HSlider", slider_bg)
	theme.set_stylebox("grabber_area", "HSlider", _panel(GOLD, 8, GOLD_DIM, 2))
	theme.set_stylebox("grabber_area_highlight", "HSlider", _panel(GOLD, 8, GOLD, 3))
	theme.set_color("font_color", "HSlider", CREAM)

	# --- CheckBox (plein écran)
	theme.set_color("font_color", "CheckBox", CREAM)
	theme.set_color("font_hover_color", "CheckBox", GOLD)

	# --- Tooltip / toasts
	theme.set_stylebox("panel", "TooltipPanel", _panel(PANEL_SOLID, 10, GOLD_DIM, 2))

	return theme


## StyleBoxFlat arrondi avec bordure et ombre portée
static func _panel(color: Color, radius: int, border_color: Color, border: int) -> StyleBoxFlat:
	var sb := StyleBoxFlat.new()
	sb.bg_color = color
	sb.corner_radius_top_left = radius
	sb.corner_radius_top_right = radius
	sb.corner_radius_bottom_right = radius
	sb.corner_radius_bottom_left = radius
	sb.border_width_left = border
	sb.border_width_top = border
	sb.border_width_right = border
	sb.border_width_bottom = border
	sb.border_color = border_color
	sb.content_margin_left = 14
	sb.content_margin_top = 10
	sb.content_margin_right = 14
	sb.content_margin_bottom = 10
	sb.shadow_color = Color(0, 0, 0, 0.35)
	sb.shadow_size = 10
	sb.shadow_offset = Vector2(0, 3)
	return sb


## StyleBoxFlat « cercle » (icônes rondes : pièce, soleil, nuage…)
static func circle(color: Color, border_color: Color = Color(1, 1, 1, 0.25), border: int = 2) -> StyleBoxFlat:
	var sb := StyleBoxFlat.new()
	sb.bg_color = color
	sb.corner_radius_top_left = 1000
	sb.corner_radius_top_right = 1000
	sb.corner_radius_bottom_right = 1000
	sb.corner_radius_bottom_left = 1000
	sb.border_width_left = border
	sb.border_width_top = border
	sb.border_width_right = border
	sb.border_width_bottom = border
	sb.border_color = border_color
	sb.shadow_color = Color(0, 0, 0, 0.3)
	sb.shadow_size = 6
	return sb
