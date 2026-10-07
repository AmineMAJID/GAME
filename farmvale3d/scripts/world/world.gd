extends Node3D
## ============================================================
## FarmVale — scène du monde 3D
## Assemble le joueur, la caméra, le HUD, le système de temps et
## génère les props. Gère la pause (Échap) et la sauvegarde auto.
## ============================================================

@onready var time_system: TimeSystem = %TimeSystem
@onready var player: Player = %Player
@onready var hud: Control = %HUD
@onready var pond: MeshInstance3D = %Pond


func _ready() -> void:
	# curseur capturé pour contrôler la caméra (style jeux modernes)
	Input.mouse_mode = Input.MOUSE_MODE_CAPTURED
	# pose l'étang et le joueur sur le relief (miroir du shader de sol)
	pond.position = Vector3(24.0, Props.ground_height(24.0, -18.0) + 0.1, -18.0)
	pond.add_to_group("water")
	player.position = Vector3(0.0, Props.ground_height(0.0, 6.0) + 0.1, 6.0)
	# applique la monnaie sauvegardée
	GameManager.money_changed.connect(_on_money_changed)
	if hud and hud.has_method("set_money"):
		hud.call("set_money", GameManager.money)
	# toast de bienvenue
	if hud and hud.has_method("show_toast"):
		hud.call("show_toast", "Bienvenue à FarmVale ! ZQSD pour se déplacer, souris pour la caméra.", 6.0)


func _exit_tree() -> void:
	Input.mouse_mode = Input.MOUSE_MODE_VISIBLE


func _on_money_changed(value: int) -> void:
	if hud and hud.has_method("set_money"):
		hud.call("set_money", value)


func _unhandled_input(event: InputEvent) -> void:
	# Échap : pause (géré aussi par le HUD)
	if event.is_action_pressed("ui_cancel"):
		if hud and hud.has_method("toggle_pause"):
			hud.call("toggle_pause")
