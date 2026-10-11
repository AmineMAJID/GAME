extends Control
## ============================================================
## FarmVale — HUD en jeu (style moderne & minimal)
## Barre du haut (monnaie, date/heure, météo), barre d'énergie,
## barre rapide (9 slots, touches 1-9), réticule, toasts, pause.
## ============================================================

@onready var money_label: Label = %MoneyLabel
@onready var coin_icon: Panel = %CoinIcon
@onready var season_label: Label = %SeasonLabel
@onready var time_label: Label = %TimeLabel
@onready var weather_label: Label = %WeatherLabel
@onready var weather_icon: Panel = %WeatherIcon
@onready var energy_bar: ProgressBar = %EnergyBar
@onready var hotbar: HBoxContainer = %Hotbar
@onready var toast_container: VBoxContainer = %ToastContainer
@onready var pause_panel: PanelContainer = %PausePanel
@onready var crosshair: ColorRect = %Crosshair

@onready var btn_resume: Button = %Reprendre
@onready var btn_quit_menu: Button = %QuitterMenu

## Objets de la barre rapide (phase 1 : outils de base)
var hotbar_items: Array[Dictionary] = [
	{"name": "Houe", "icon": "⛏", "color": Color("#8A5A33"), "count": 1},
	{"name": "Arrosoir", "icon": "🚿", "color": Color("#4A90D9"), "count": 1},
	{"name": "Hache", "icon": "🪓", "color": Color("#7A7A7A"), "count": 1},
	{"name": "Faux", "icon": "🌾", "color": Color("#C9A84C"), "count": 1},
	{"name": "Canne à pêche", "icon": "🎣", "color": Color("#5AA85A"), "count": 1},
	{"name": "Seau", "icon": "🪣", "color": Color("#8FB8D9"), "count": 1},
	{"name": "Tondeuse", "icon": "✂", "color": Color("#9AA5B1"), "count": 1},
	{"name": "Panier", "icon": "🧺", "color": Color("#B5834A"), "count": 1},
	{"name": "Main", "icon": "✋", "color": Color("#F2C9A0"), "count": 0},
]

var selected_slot: int = 0
var _slot_panels: Array[Panel] = []
var _paused: bool = false

# Icônes météo (couleur + libellé)
const WEATHER_ICONS := {
	"soleil": {"label": "Ensoleillé", "color": Color("#F2C14E")},
	"nuage": {"label": "Nuageux", "color": Color("#B8C4CC")},
	"pluie": {"label": "Pluie", "color": Color("#6A9AD9")},
	"orage": {"label": "Orage", "color": Color("#7A6AC9")},
	"neige": {"label": "Neige", "color": Color("#EAF4FA")},
}


func _ready() -> void:
	theme = UITheme.build()
	mouse_filter = Control.MOUSE_FILTER_IGNORE
	coin_icon.add_theme_stylebox_override("panel", UITheme.circle(Color("#F2C14E"), Color(1, 1, 1, 0.35), 2))
	_build_hotbar()
	set_money(GameManager.money)
	set_energy(100, 100)
	btn_resume.pressed.connect(_on_resume)
	btn_quit_menu.pressed.connect(_on_quit_menu)
	btn_resume.mouse_entered.connect(func(): GameManager.play_sfx("hover"))
	btn_quit_menu.mouse_entered.connect(func(): GameManager.play_sfx("hover"))
	GameManager.money_changed.connect(func(v): set_money(v))


# ------------------------------------------------------------
# Barre rapide (9 slots)
# ------------------------------------------------------------
func _build_hotbar() -> void:
	for i in 9:
		var idx := i  # copie : les lambdas capturent par référence
		var slot := Panel.new()
		slot.custom_minimum_size = Vector2(58, 58)
		slot.name = "Slot%d" % (i + 1)
		# style du slot
		var sb := StyleBoxFlat.new()
		sb.bg_color = Color(0.07, 0.10, 0.08, 0.78)
		sb.corner_radius_top_left = 10
		sb.corner_radius_top_right = 10
		sb.corner_radius_bottom_right = 10
		sb.corner_radius_bottom_left = 10
		sb.border_width_left = 2
		sb.border_width_top = 2
		sb.border_width_right = 2
		sb.border_width_bottom = 2
		sb.border_color = Color(1, 1, 1, 0.15)
		sb.content_margin_left = 4
		sb.content_margin_top = 4
		sb.content_margin_right = 4
		sb.content_margin_bottom = 4
		slot.add_theme_stylebox_override("panel", sb)
		slot.set_meta("base_style", sb)

		# pastille de couleur de l'objet
		var icon := Panel.new()
		icon.custom_minimum_size = Vector2(26, 26)
		icon.size_flags_horizontal = Control.SIZE_SHRINK_CENTER
		icon.size_flags_vertical = Control.SIZE_SHRINK_CENTER
		var icon_sb := UITheme.circle(hotbar_items[i]["color"], Color(1, 1, 1, 0.3), 2)
		icon.add_theme_stylebox_override("panel", icon_sb)
		slot.add_child(icon)

		# numéro de touche
		var key := Label.new()
		key.text = str(i + 1)
		key.position = Vector2(4, 2)
		key.add_theme_font_size_override("font_size", 11)
		key.add_theme_color_override("font_color", Color(1, 1, 1, 0.55))
		slot.add_child(key)

		# compte (pour les objets en quantité)
		var count := Label.new()
		count.name = "Count"
		count.anchor_left = 1.0
		count.anchor_top = 1.0
		count.offset_left = -20
		count.offset_top = -16
		count.offset_right = -2
		count.offset_bottom = -2
		count.horizontal_alignment = HORIZONTAL_ALIGNMENT_RIGHT
		count.add_theme_font_size_override("font_size", 13)
		count.add_theme_color_override("font_color", Color("#F2C14E"))
		slot.add_child(count)

		# infobulle avec le nom de l'objet
		slot.tooltip_text = "%s (touche %d)" % [hotbar_items[i]["name"], i + 1]
		slot.gui_input.connect(func(ev): _on_slot_input(ev, idx))
		hotbar.add_child(slot)
		_slot_panels.append(slot)
		_refresh_slot(i)
	_select_slot(0)  # sélection visuelle du premier slot


func _refresh_slot(i: int) -> void:
	var slot := _slot_panels[i]
	var count: Label = slot.get_node("Count")
	var c: int = hotbar_items[i]["count"]
	count.text = str(c) if c > 1 else ""
	var sb: StyleBoxFlat = slot.get_meta("base_style")
	if i == selected_slot:
		sb.bg_color = Color(0.16, 0.24, 0.15, 0.95)
		sb.border_color = Color("#F2C14E")
	else:
		sb.bg_color = Color(0.07, 0.10, 0.08, 0.78)
		sb.border_color = Color(1, 1, 1, 0.15)
	slot.add_theme_stylebox_override("panel", sb)


func _select_slot(i: int) -> void:
	selected_slot = clampi(i, 0, 8)
	for j in _slot_panels.size():
		_refresh_slot(j)
	GameManager.play_sfx("select")


func _on_slot_input(event: InputEvent, index: int) -> void:
	if event is InputEventMouseButton and event.pressed and event.button_index == MOUSE_BUTTON_LEFT:
		_select_slot(index)


func _unhandled_input(event: InputEvent) -> void:
	for i in 9:
		if event.is_action_pressed("hotbar_%d" % (i + 1)):
			_select_slot(i)


# ------------------------------------------------------------
# Mise à jour des infos
# ------------------------------------------------------------
func set_money(value: int) -> void:
	money_label.text = "%d" % value


func set_time(hour: int, minute: int, day: int, season: String, weekday: String) -> void:
	season_label.text = "%s — Jour %d" % [season, day]
	time_label.text = "%s · %02d:%02d" % [weekday, hour, minute]


func set_weather(weather: String) -> void:
	var info: Dictionary = WEATHER_ICONS.get(weather, WEATHER_ICONS["soleil"])
	weather_label.text = info["label"]
	var icon_sb := UITheme.circle(info["color"], Color(1, 1, 1, 0.3), 2)
	weather_icon.add_theme_stylebox_override("panel", icon_sb)


func set_energy(value: float, max_value: float) -> void:
	energy_bar.max_value = max_value
	energy_bar.value = value


# ------------------------------------------------------------
# Toasts (notifications temporaires en haut à droite)
# ------------------------------------------------------------
func show_toast(text: String, duration: float = 3.5) -> void:
	var toast := PanelContainer.new()
	var sb := StyleBoxFlat.new()
	sb.bg_color = Color(0.07, 0.10, 0.08, 0.9)
	sb.corner_radius_top_left = 10
	sb.corner_radius_top_right = 10
	sb.corner_radius_bottom_right = 10
	sb.corner_radius_bottom_left = 10
	sb.border_width_left = 3
	sb.border_width_top = 3
	sb.border_width_right = 3
	sb.border_width_bottom = 3
	sb.border_color = Color("#F2C14E")
	sb.content_margin_left = 12
	sb.content_margin_top = 8
	sb.content_margin_right = 12
	sb.content_margin_bottom = 8
	toast.add_theme_stylebox_override("panel", sb)
	var label := Label.new()
	label.text = text
	label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	toast.add_child(label)
	toast.modulate.a = 0.0
	toast_container.add_child(toast)
	var tw := toast.create_tween()
	tw.tween_property(toast, "modulate:a", 1.0, 0.3)
	tw.tween_interval(duration)
	tw.tween_property(toast, "modulate:a", 0.0, 0.4)
	tw.tween_callback(toast.queue_free)


# ------------------------------------------------------------
# Pause (Échap)
# ------------------------------------------------------------
func toggle_pause() -> void:
	_paused = not _paused
	pause_panel.visible = _paused
	get_tree().paused = _paused
	crosshair.visible = not _paused
	if _paused:
		Input.mouse_mode = Input.MOUSE_MODE_VISIBLE
		GameManager.play_sfx("pause")
		btn_resume.grab_focus()
	else:
		Input.mouse_mode = Input.MOUSE_MODE_CAPTURED


func _on_resume() -> void:
	GameManager.play_sfx("click")
	toggle_pause()


func _on_quit_menu() -> void:
	GameManager.play_sfx("click")
	get_tree().paused = false
	GameManager.quit_to_menu()
