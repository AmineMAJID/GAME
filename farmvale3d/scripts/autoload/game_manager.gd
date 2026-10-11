extends Node
## ============================================================
## FarmVale — GameManager (autoload)
## Gestion globale : transitions de scènes avec fondu, sauvegarde,
## réglages (volume / plein écran), monnaie et petits SFX synthétisés.
## ============================================================

signal money_changed(value: int)
signal settings_changed

const SAVE_PATH := "user://farmvale_save.json"
const SETTINGS_PATH := "user://farmvale_settings.json"

## Monnaie du joueur (persiste entre les sessions via la sauvegarde)
var money: int = 500:
	set = set_money

## Scène actuellement affichée
var current_scene_path: String = ""

## Réglages
var master_volume: float = 1.0
var music_volume: float = 0.75
var sfx_volume: float = 1.0
var fullscreen: bool = false

## Transition en cours (évite les doubles clics)
var _transitioning: bool = false


func _ready() -> void:
	_ensure_audio_buses()
	_load_settings()
	_apply_settings()


# ------------------------------------------------------------
# Audio : crée les bus Master / Music / SFX s'ils n'existent pas
# ------------------------------------------------------------
func _ensure_audio_buses() -> void:
	for bus_name in ["Master", "Music", "SFX"]:
		if AudioServer.get_bus_index(bus_name) == -1:
			AudioServer.add_bus()
			var idx := AudioServer.get_bus_count() - 1
			AudioServer.set_bus_name(idx, bus_name)
			if bus_name != "Master":
				AudioServer.set_bus_send(idx, "Master")


# ------------------------------------------------------------
# Réglages
# ------------------------------------------------------------
func _apply_settings() -> void:
	var master_idx := AudioServer.get_bus_index("Master")
	var music_idx := AudioServer.get_bus_index("Music")
	var sfx_idx := AudioServer.get_bus_index("SFX")
	if master_idx >= 0:
		AudioServer.set_bus_volume_db(master_idx, linear_to_db(master_volume))
		AudioServer.set_bus_mute(master_idx, master_volume <= 0.001)
	if music_idx >= 0:
		AudioServer.set_bus_volume_db(music_idx, linear_to_db(music_volume))
	if sfx_idx >= 0:
		AudioServer.set_bus_volume_db(sfx_idx, linear_to_db(sfx_volume))
	if fullscreen:
		DisplayServer.window_set_mode(DisplayServer.WINDOW_MODE_FULLSCREEN)
	else:
		DisplayServer.window_set_mode(DisplayServer.WINDOW_MODE_WINDOWED)
	settings_changed.emit()


func set_master_volume(v: float) -> void:
	master_volume = clampf(v, 0.0, 1.0)
	_apply_settings()
	save_settings()


func set_music_volume(v: float) -> void:
	music_volume = clampf(v, 0.0, 1.0)
	_apply_settings()
	save_settings()


func set_sfx_volume(v: float) -> void:
	sfx_volume = clampf(v, 0.0, 1.0)
	_apply_settings()
	save_settings()


func set_fullscreen(on: bool) -> void:
	fullscreen = on
	_apply_settings()
	save_settings()


func save_settings() -> void:
	var data := {
		"master_volume": master_volume,
		"music_volume": music_volume,
		"sfx_volume": sfx_volume,
		"fullscreen": fullscreen,
	}
	var f := FileAccess.open(SETTINGS_PATH, FileAccess.WRITE)
	if f:
		f.store_string(JSON.stringify(data))


func _load_settings() -> void:
	if not FileAccess.file_exists(SETTINGS_PATH):
		return
	var f := FileAccess.open(SETTINGS_PATH, FileAccess.READ)
	if f == null:
		return
	var data: Dictionary = JSON.parse_string(f.get_as_text())
	if data == null:
		return
	master_volume = data.get("master_volume", 1.0)
	music_volume = data.get("music_volume", 0.75)
	sfx_volume = data.get("sfx_volume", 1.0)
	fullscreen = data.get("fullscreen", false)


# ------------------------------------------------------------
# Monnaie
# ------------------------------------------------------------
func set_money(value: int) -> void:
	money = max(0, value)
	money_changed.emit(money)


func add_money(amount: int) -> void:
	set_money(money + amount)
	if amount > 0:
		play_sfx("coin")
	elif amount < 0:
		play_sfx("pay")


# ------------------------------------------------------------
# Transitions de scènes (fondu au noir)
# ------------------------------------------------------------
func change_scene(path: String) -> void:
	if _transitioning:
		return
	_transitioning = true
	current_scene_path = path
	var fade_layer := CanvasLayer.new()
	fade_layer.layer = 100
	var fade := ColorRect.new()
	fade.color = Color(0, 0, 0, 0)
	fade.set_anchors_preset(Control.PRESET_FULL_RECT)
	fade_layer.add_child(fade)
	get_tree().root.add_child(fade_layer)
	var tw := fade.create_tween()
	tw.tween_property(fade, "color:a", 1.0, 0.35)
	await tw.finished
	get_tree().change_scene_to_file(path)
	# fondu inverse sur la nouvelle scène
	var tw2 := fade.create_tween()
	tw2.tween_property(fade, "color:a", 0.0, 0.4)
	await tw2.finished
	fade_layer.queue_free()
	_transitioning = false


# ------------------------------------------------------------
# Nouvelle partie / continuer
# ------------------------------------------------------------
func new_game() -> void:
	money = 500
	change_scene("res://scenes/world/world.tscn")


func has_save() -> bool:
	return FileAccess.file_exists(SAVE_PATH)


func continue_game() -> void:
	if has_save():
		change_scene("res://scenes/world/world.tscn")


func quit_to_menu() -> void:
	change_scene("res://scenes/ui/main_menu.tscn")


func quit_game() -> void:
	get_tree().quit()


# ------------------------------------------------------------
# Sauvegarde (JSON simple)
# ------------------------------------------------------------
func save_game(state: Dictionary) -> void:
	state["money"] = money
	var f := FileAccess.open(SAVE_PATH, FileAccess.WRITE)
	if f:
		f.store_string(JSON.stringify(state))


func load_game() -> Dictionary:
	if not has_save():
		return {}
	var f := FileAccess.open(SAVE_PATH, FileAccess.READ)
	if f == null:
		return {}
	var parsed: Variant = JSON.parse_string(f.get_as_text())
	if parsed == null or typeof(parsed) != TYPE_DICTIONARY:
		return {}
	var data: Dictionary = parsed
	money = int(data.get("money", 500))
	return data


# ------------------------------------------------------------
# SFX synthétisés (aucun fichier requis)
# ------------------------------------------------------------
func play_sfx(name: String) -> void:
	var sample_rate := 44100
	# (fréquence de départ, fréquence d'arrivée, durée, decay, volume)
	var params := {
		"click": [1150.0, 1150.0, 0.07, 40.0, 0.25],
		"hover": [880.0, 880.0, 0.045, 60.0, 0.12],
		"coin": [1318.0, 1975.0, 0.18, 12.0, 0.3],
		"pay": [520.0, 390.0, 0.12, 18.0, 0.25],
		"select": [740.0, 990.0, 0.09, 30.0, 0.22],
		"pause": [500.0, 500.0, 0.12, 20.0, 0.2],
	}
	var p: Array = params.get(name, params["click"])
	var f0: float = p[0]
	var f1: float = p[1]
	var duration: float = p[2]
	var decay: float = p[3]
	var vol: float = p[4]
	var gen := AudioStreamGenerator.new()
	gen.mix_rate = sample_rate
	gen.buffer_length = duration + 0.1  # AVANT play() : le tampon est lu à la création de la lecture
	var player := AudioStreamPlayer.new()
	player.bus = "SFX"
	player.stream = gen
	add_child(player)
	player.play()
	var playback := player.get_stream_playback() as AudioStreamGeneratorPlayback
	if playback == null:
		player.queue_free()
		return
	var total := int(sample_rate * duration)
	var phase := 0.0
	for i in total:
		var t := float(i) / sample_rate
		var freq := lerpf(f0, f1, t / duration)
		var env := exp(-t * decay)
		var s := sin(phase) * env * vol
		phase = fmod(phase + freq * TAU / sample_rate, TAU)
		playback.push_frame(Vector2(s, s))
	# laisse le son se terminer proprement
	var timer := get_tree().create_timer(duration + 0.08)
	timer.timeout.connect(func():
		if is_instance_valid(player):
			player.stop()
			player.queue_free()
	)
