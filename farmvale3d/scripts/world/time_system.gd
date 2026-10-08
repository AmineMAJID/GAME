class_name TimeSystem
extends Node
## ============================================================
## FarmVale — système de temps
## 1 heure de jeu = `hour_length` secondes réelles.
## Pilote : soleil (rotation + couleur + énergie), ciel (shader),
## environnement (ambient, brouillard), fenêtres qui brillent la nuit,
## lucioles, HUD.
## ============================================================

signal time_changed(hour: int, minute: int)
signal day_changed(day: int, season: String)
signal weather_changed(weather: String)

@export var hour_length: float = 15.0 ## secondes réelles pour 1 heure de jeu
@export var start_hour: int = 8
@export var auto_advance: bool = true

const MINUTES_PER_DAY := 1440

## 4 saisons × 28 jours
const SEASONS := ["Printemps", "Été", "Automne", "Hiver"]
const WEEKDAYS := ["Lundi", "Mardi", "Mercredi", "Jeudi", "Vendredi", "Samedi", "Dimanche"]

## Météo possible : soleil, nuage, pluie, orage, neige
var weather: String = "soleil"

var total_minutes: int = 0
var day: int = 1
var season_index: int = 0

@export var sun_pivot: Node3D
@export var sun: DirectionalLight3D
@export var world_environment: WorldEnvironment
@export var hud: Control

var _sky_material: ShaderMaterial
var _env: Environment
var _accum: float = 0.0
var _moon: DirectionalLight3D

# Palettes du ciel : [top, horizon, soleil, densité d'étoiles]
const SKY_NIGHT := [Color("#050814"), Color("#101A2E"), Color("#8090C0"), 1.0]
const SKY_DAWN := [Color("#2E4A7A"), Color("#F2A65A"), Color("#FFD9A0"), 0.15]
const SKY_DAY := [Color("#4A90D9"), Color("#BFE3F2"), Color("#FFF4D6"), 0.0]
const SKY_DUSK := [Color("#3A3A6A"), Color("#F28C5A"), Color("#FFC98A"), 0.1]


func _ready() -> void:
	if sun_pivot == null:
		sun_pivot = get_node_or_null("../SunPivot") as Node3D
	if sun == null:
		sun = get_node_or_null("../SunPivot/Sun") as DirectionalLight3D
	if world_environment == null:
		world_environment = get_node_or_null("../WorldEnvironment") as WorldEnvironment
	if hud == null:
		hud = get_node_or_null("../HUD") as Control
	if world_environment and world_environment.environment:
		_env = world_environment.environment
		if _env.sky and _env.sky.sky_material is ShaderMaterial:
			_sky_material = _env.sky.sky_material
	# lune (lumière bleutée très faible, opposée au soleil)
	_moon = DirectionalLight3D.new()
	_moon.name = "Moon"
	_moon.light_color = Color("#7C8FC9")
	_moon.light_energy = 0.0
	_moon.shadow_enabled = false
	add_child(_moon)
	total_minutes = start_hour * 60
	_roll_weather()
	# différé : le HUD (nœud frère) doit avoir résolu ses @onready avant le 1er update
	_apply_time.call_deferred()
	_update_hud.call_deferred()


func _process(delta: float) -> void:
	if not auto_advance:
		return
	_accum += delta * 60.0 / hour_length
	while _accum >= 1.0:
		_accum -= 1.0
		total_minutes += 1
		if total_minutes >= MINUTES_PER_DAY:
			total_minutes = 0
			_advance_day()
		_apply_time()
		_update_hud()
		time_changed.emit(hour(), minute())


# ------------------------------------------------------------
# Accesseurs
# ------------------------------------------------------------
func hour() -> int:
	return (total_minutes / 60) % 24


func minute() -> int:
	return total_minutes % 60


func season() -> String:
	return SEASONS[season_index]


func weekday() -> String:
	return WEEKDAYS[(day - 1) % 7]


## 0.0 = minuit, 0.25 = 6h (aube), 0.5 = midi, 0.75 = 18h (crépuscule)
func day_fraction() -> float:
	return float(total_minutes) / float(MINUTES_PER_DAY)


func is_night() -> bool:
	return hour() < 6 or hour() >= 20


func is_day() -> bool:
	return not is_night()


## -1 (minuit) → +1 (midi) : hauteur du soleil
func sun_height() -> float:
	return -cos(day_fraction() * TAU)


# ------------------------------------------------------------
# Application du temps sur le monde
# ------------------------------------------------------------
func _apply_time() -> void:
	var frac := day_fraction()
	# Le DirectionalLight3D éclaire selon son axe +Z local.
	# x = frac * 360 - 90 : à midi (0.5) → x = 90° → le soleil éclaire vers le bas.
	if sun_pivot:
		sun_pivot.rotation_degrees.x = frac * 360.0 - 90.0
		if sun:
			var up := sun_height()
			# énergie : vive le jour, quasi éteinte la nuit
			sun.light_energy = clampf(0.06 + up * 1.15, 0.04, 1.25)
			# couleur : chaude à l'horizon, blanche à midi, bleutée la nuit
			var warm := Color("#FFB37A")
			var neutral := Color("#FFF3E0")
			var night := Color("#5A6FA8")
			var col: Color
			if up > 0.0:
				col = warm.lerp(neutral, clampf(up * 2.5, 0.0, 1.0))
			else:
				col = night.lerp(warm, clampf(-up * 2.0, 0.0, 1.0))
			sun.light_color = col
	# lune : opposée au soleil, faible
	if _moon:
		_moon.rotation_degrees.x = frac * 360.0 + 90.0
		_moon.light_energy = clampf(-sun_height() * 0.12, 0.0, 0.12)

	# --- Ciel (shader) : interpolation entre 4 palettes
	var pal := _sky_palette(frac)
	if _sky_material:
		_sky_material.set_shader_parameter("sky_top", pal[0])
		_sky_material.set_shader_parameter("sky_horizon", pal[1])
		_sky_material.set_shader_parameter("sun_color", pal[2])
		_sky_material.set_shader_parameter("star_density", pal[3])
		if sun:
			# « sun_direction » = direction VERS le soleil (le +Z local de la lampe
			# pointe dans le sens de la lumière, donc on inverse)
			var to_sun: Vector3 = -sun.global_transform.basis.z
			_sky_material.set_shader_parameter("sun_direction", to_sun)
			_sky_material.set_shader_parameter("sun_glow", 1.2 + clampf(sun_height(), 0.0, 1.0) * 0.8)
		if _moon:
			# lune à l'opposé du soleil
			var to_moon: Vector3 = -_moon.global_transform.basis.z
			_sky_material.set_shader_parameter("moon_direction", to_moon)
	# --- Environnement : ambient + brouillard
	if _env:
		var up2 := clampf(sun_height(), 0.0, 1.0)
		var amb_day := Color("#BFD3E6")
		var amb_night := Color("#26314E")
		_env.ambient_light_color = amb_night.lerp(amb_day, up2)
		_env.ambient_light_energy = lerpf(0.18, 0.55, up2)
		_env.fog_light_color = amb_night.lerp(Color("#AFC8DC"), up2)
		_env.fog_density = lerpf(0.0035, 0.0012, up2)
		_env.fog_sky_affect = lerpf(0.6, 0.25, up2)
	# --- Fenêtres qui brillent la nuit
	var night_factor := clampf(-sun_height(), 0.0, 1.0)
	for win in get_tree().get_nodes_in_group("window_glow"):
		if win is MeshInstance3D:
			var mat: Material = win.get_active_material(0)
			if mat is StandardMaterial3D:
				mat.emission_energy = lerpf(0.0, 3.5, night_factor * night_factor)
	# --- Lucioles (particules) : actives la nuit
	for flies in get_tree().get_nodes_in_group("fireflies"):
		if flies is GPUParticles3D:
			flies.emitting = night_factor > 0.5
	# --- Reflets du soleil sur l'eau (direction VERS le soleil)
	if sun:
		var to_sun: Vector3 = -sun.global_transform.basis.z
		for water in get_tree().get_nodes_in_group("water"):
			if water is MeshInstance3D:
				var wmat: Material = water.get_active_material(0)
				if wmat is ShaderMaterial:
					wmat.set_shader_parameter("sun_direction", to_sun)


## Interpolation circulaire entre les 4 palettes (minuit/aube/midi/crépuscule)
func _sky_palette(frac: float) -> Array:
	var anchors := [SKY_NIGHT, SKY_DAWN, SKY_DAY, SKY_DUSK]
	var points := [0.0, 0.25, 0.5, 0.75]
	# trouve le segment [i, i+1] qui contient frac (avec wrap)
	# (GDScript n'a pas de « for-else » : valeur par défaut = dernier segment)
	var idx := 3 # entre 0.75 et 1.0 (retour à minuit)
	for i in range(3):
		if frac >= points[i] and frac < points[i + 1]:
			idx = i
			break
	var a: float = points[idx]
	var b: float = points[(idx + 1) % 4] + (1.0 if idx == 3 else 0.0)
	var t := inverse_lerp(a, b, frac)
	var pa: Array = anchors[idx]
	var pb: Array = anchors[(idx + 1) % 4]
	return [
		pa[0].lerp(pb[0], t),
		pa[1].lerp(pb[1], t),
		pa[2].lerp(pb[2], t),
		lerpf(pa[3], pb[3], t),
	]


# ------------------------------------------------------------
# HUD
# ------------------------------------------------------------
func _update_hud() -> void:
	if hud and hud.has_method("set_time"):
		hud.call("set_time", hour(), minute(), day, season(), weekday())
	if hud and hud.has_method("set_weather"):
		hud.call("set_weather", weather)


# ------------------------------------------------------------
# Jours, saisons, météo
# ------------------------------------------------------------
func _advance_day() -> void:
	day += 1
	if day > 28:
		day = 1
		season_index = (season_index + 1) % 4
	_roll_weather()
	day_changed.emit(day, season())
	_update_hud()


## Tirage météo simple (sera enrichi avec les saisons plus tard)
func _roll_weather() -> void:
	var r := randf()
	match season_index:
		0: # printemps
			weather = "soleil" if r < 0.55 else ("nuage" if r < 0.8 else "pluie")
		1: # été
			weather = "soleil" if r < 0.7 else ("nuage" if r < 0.9 else "orage")
		2: # automne
			weather = "nuage" if r < 0.45 else ("pluie" if r < 0.75 else "soleil")
		3: # hiver
			weather = "neige" if r < 0.5 else ("nuage" if r < 0.8 else "soleil")
	weather_changed.emit(weather)


## Avance le temps manuellement (utile pour les tests)
func advance_minutes(minutes: int) -> void:
	for i in minutes:
		total_minutes += 1
		if total_minutes >= MINUTES_PER_DAY:
			total_minutes = 0
			_advance_day()
	_apply_time()
	_update_hud()
	time_changed.emit(hour(), minute())
