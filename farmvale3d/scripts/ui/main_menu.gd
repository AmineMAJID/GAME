extends Control
## ============================================================
## FarmVale — Menu principal (HUD de démarrage)
## Titre animé, boutons modernes, panneau d'options fonctionnel,
## particules de feuilles qui tombent.
## ============================================================

@onready var btn_new: Button = %NouvellePartie
@onready var btn_continue: Button = %Continuer
@onready var btn_options: Button = %Options
@onready var btn_quit: Button = %Quitter
@onready var options_panel: PanelContainer = %OptionsPanel
@onready var version_label: Label = %Version
@onready var title_label: Label = %Titre
@onready var subtitle_label: Label = %SousTitre

@onready var slider_master: HSlider = %VolumeGeneral
@onready var slider_music: HSlider = %VolumeMusique
@onready var slider_sfx: HSlider = %VolumeSFX
@onready var check_fullscreen: CheckBox = %PleinEcran
@onready var btn_retour: Button = %Retour

var _leaves: GPUParticles2D


func _ready() -> void:
	theme = UITheme.build()
	Input.mouse_mode = Input.MOUSE_MODE_VISIBLE

	# Connexion des boutons
	btn_new.pressed.connect(_on_new_game)
	btn_continue.pressed.connect(_on_continue)
	btn_options.pressed.connect(_on_options)
	btn_quit.pressed.connect(_on_quit)
	btn_retour.pressed.connect(_on_close_options)
	btn_new.mouse_entered.connect(func(): GameManager.play_sfx("hover"))
	btn_continue.mouse_entered.connect(func(): GameManager.play_sfx("hover"))
	btn_options.mouse_entered.connect(func(): GameManager.play_sfx("hover"))
	btn_quit.mouse_entered.connect(func(): GameManager.play_sfx("hover"))

	# « Continuer » grisé s'il n'y a pas de sauvegarde
	btn_continue.disabled = not GameManager.has_save()

	# Panneau d'options : valeurs actuelles
	slider_master.value = GameManager.master_volume
	slider_music.value = GameManager.music_volume
	slider_sfx.value = GameManager.sfx_volume
	check_fullscreen.button_pressed = GameManager.fullscreen
	slider_master.value_changed.connect(func(v): GameManager.set_master_volume(v))
	slider_music.value_changed.connect(func(v): GameManager.set_music_volume(v))
	slider_sfx.value_changed.connect(func(v): GameManager.set_sfx_volume(v))
	check_fullscreen.toggled.connect(func(on): GameManager.set_fullscreen(on))

	version_label.text = "v0.1 — Démo : monde 3D & menu"

	_spawn_leaves()
	_animate_in()
	btn_new.grab_focus()


# ------------------------------------------------------------
# Particules de feuilles mortes qui tombent (2D)
# ------------------------------------------------------------
func _spawn_leaves() -> void:
	_leaves = GPUParticles2D.new()
	_leaves.name = "Feuilles"
	_leaves.position = Vector2(960, -40)  # départ en haut de l'écran (1920×1080)
	_leaves.amount = 26
	_leaves.lifetime = 7.0
	_leaves.preprocess = 1.5
	_leaves.speed_scale = 0.7
	_leaves.randomness = 0.4
	_leaves.visibility_rect = Rect2(-100, -100, size.x + 200, size.y + 200)

	var mat := ParticleProcessMaterial.new()
	mat.emission_shape = ParticleProcessMaterial.EMISSION_SHAPE_BOX
	mat.emission_box_extents = Vector3(size.x * 0.55, 10, 0)
	mat.direction = Vector3(0.15, 1, 0)
	mat.spread = 25.0
	mat.initial_velocity_min = 25.0
	mat.initial_velocity_max = 55.0
	mat.gravity = Vector3(10, 30, 0)
	mat.angular_velocity_min = -120.0
	mat.angular_velocity_max = 120.0
	mat.scale_min = 0.7
	mat.scale_max = 1.4
	mat.color = Color("#D9A441")
	_leaves.process_material = mat

	# GPUParticles2D n'a pas de « draw_passes » (réservé au 3D) : on lui donne
	# directement une texture de feuille. Chargement via Image (pas besoin d'import)
	var leaf_img := Image.new()
	if leaf_img.load("res://assets/textures/leaf.png") == OK:
		_leaves.texture = ImageTexture.create_from_image(leaf_img)
	add_child(_leaves)


# ------------------------------------------------------------
# Animation d'entrée (titre + boutons qui montent en fondu)
# ------------------------------------------------------------
func _animate_in() -> void:
	# Titre
	title_label.modulate.a = 0.0
	title_label.position.y += 26
	var tw := title_label.create_tween()
	tw.set_ease(Tween.EASE_OUT)
	tw.set_trans(Tween.TRANS_BACK)
	tw.tween_property(title_label, "modulate:a", 1.0, 0.7)
	tw.parallel().tween_property(title_label, "position:y", title_label.position.y - 26, 0.7)

	subtitle_label.modulate.a = 0.0
	var tw2 := subtitle_label.create_tween()
	tw2.tween_property(subtitle_label, "modulate:a", 1.0, 0.6).set_delay(0.25)

	# Boutons : apparition en cascade
	var delay := 0.45
	for btn in [btn_new, btn_continue, btn_options, btn_quit]:
		btn.modulate.a = 0.0
		btn.scale = Vector2(0.85, 0.85)
		btn.pivot_offset = btn.size / 2.0
		var b: Tween = btn.create_tween()
		b.set_ease(Tween.EASE_OUT)
		b.set_trans(Tween.TRANS_BACK)
		b.tween_property(btn, "modulate:a", 1.0, 0.45).set_delay(delay)
		b.parallel().tween_property(btn, "scale", Vector2.ONE, 0.5).set_delay(delay)
		delay += 0.09


# ------------------------------------------------------------
# Boutons
# ------------------------------------------------------------
func _on_new_game() -> void:
	GameManager.play_sfx("click")
	GameManager.new_game()


func _on_continue() -> void:
	GameManager.play_sfx("click")
	GameManager.continue_game()


func _on_options() -> void:
	GameManager.play_sfx("select")
	options_panel.visible = true
	btn_retour.grab_focus()


func _on_close_options() -> void:
	GameManager.play_sfx("click")
	options_panel.visible = false
	btn_options.grab_focus()


func _on_quit() -> void:
	GameManager.play_sfx("click")
	await get_tree().create_timer(0.25).timeout
	GameManager.quit_game()


func _unhandled_input(event: InputEvent) -> void:
	# Échap ferme le panneau d'options s'il est ouvert
	if event.is_action_pressed("ui_cancel") and options_panel.visible:
		_on_close_options()
