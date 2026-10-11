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
	_build_ground_collision()
	# applique la monnaie sauvegardée
	GameManager.money_changed.connect(_on_money_changed)
	if hud and hud.has_method("set_money"):
		hud.call("set_money", GameManager.money)
	# toast de bienvenue
	if hud and hud.has_method("show_toast"):
		hud.call("show_toast", "Bienvenue à FarmVale ! ZQSD pour se déplacer, souris pour la caméra.", 6.0)


func _build_ground_collision() -> void:
	# Corps statique sous le sol VISUEL : maillage aplati sur les mêmes collines
	# que le shader (Props.ground_height) pour que joueur et animaux tiennent.
	var plane := PlaneMesh.new()
	plane.size = Vector2(220.0, 220.0)
	plane.subdivide_width = 110
	plane.subdivide_depth = 110
	var arrays := plane.surface_get_arrays(0)
	var verts: PackedVector3Array = arrays[Mesh.ARRAY_VERTEX]
	for i in verts.size():
		verts[i].y = Props.ground_height(verts[i].x, verts[i].z)
	var col_mesh := ArrayMesh.new()
	col_mesh.add_surface_from_arrays(Mesh.PRIMITIVE_TRIANGLES, arrays)
	var body := StaticBody3D.new()
	body.name = "SolCollision"
	body.collision_layer = 1  # layer « monde »
	body.collision_mask = 0
	var shape := CollisionShape3D.new()
	shape.shape = col_mesh.create_trimesh_shape()
	body.add_child(shape)
	add_child(body)


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
