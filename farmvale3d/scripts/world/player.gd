class_name Player
extends CharacterBody3D
## ============================================================
## FarmVale — le fermier (personnage 3D stylisé, construit en
## primitives avec un matériau « toon »). Déplacement au clavier,
## caméra relative, animation de marche procédurale.
## ============================================================

@export var speed: float = 4.6
@export var sprint_speed: float = 7.2
@export var turn_speed: float = 14.0
@export var camera_rig: CameraRig

@onready var model: Node3D = %Model

var walk_phase: float = 0.0
var _limb_leg_l: Node3D
var _limb_leg_r: Node3D
var _limb_arm_l: Node3D
var _limb_arm_r: Node3D
var _body: Node3D
var _head: Node3D


func _ready() -> void:
	_build_farmer()


func _physics_process(delta: float) -> void:
	# gravité
	velocity += get_gravity() * delta

	# direction relative à la caméra
	var dir := Input.get_vector("move_left", "move_right", "move_forward", "move_backward")
	var move := Vector3.ZERO
	if dir.length() > 0.01:
		# base de la caméra (nœud Yaw) → déplacement relatif à la vue
		var cam_basis: Basis = camera_rig.camera_basis() if camera_rig else global_transform.basis
		var forward := -cam_basis.z
		forward.y = 0.0
		forward = forward.normalized()
		var right := cam_basis.x
		right.y = 0.0
		right = right.normalized()
		move = forward * dir.y + right * dir.x
		if move.length() > 1.0:
			move = move.normalized()

	var current_speed := sprint_speed if Input.is_action_pressed("sprint") else speed
	velocity.x = move.x * current_speed
	velocity.z = move.z * current_speed

	move_and_slide()

	# rotation douce vers la direction du mouvement
	if move.length() > 0.05:
		var target_y := atan2(move.x, move.z)
		model.rotation.y = lerp_angle(model.rotation.y, target_y, turn_speed * delta)
		walk_phase += delta * current_speed * 2.4
	else:
		walk_phase = lerp(walk_phase, 0.0, delta * 8.0)

	_animate_walk(delta)


# ------------------------------------------------------------
# Animation de marche procédurale (jambes + bras + balancement)
# ------------------------------------------------------------
func _animate_walk(_delta: float) -> void:
	var moving := walk_phase > 0.01
	var swing := sin(walk_phase) * (0.65 if moving else 0.0)
	if _limb_leg_l:
		_limb_leg_l.rotation.x = swing
		_limb_leg_r.rotation.x = -swing
		if _limb_arm_l:
			_limb_arm_l.rotation.x = -swing * 0.8
			_limb_arm_r.rotation.x = swing * 0.8
	# balancement du corps
	var bob := absf(sin(walk_phase * 2.0)) * 0.035 if moving else 0.0
	model.position.y = bob
	if _head:
		_head.rotation.z = sin(walk_phase * 2.0) * 0.04 if moving else 0.0


# ------------------------------------------------------------
# Construction du fermier en primitives (toon)
# ------------------------------------------------------------
func _toon(color: Color) -> ShaderMaterial:
	var shader := load("res://shaders/toon.gdshader") as Shader
	var mat := ShaderMaterial.new()
	mat.shader = shader
	mat.set_shader_parameter("albedo", color)
	return mat


func _part(parent: Node, mesh: Mesh, pos: Vector3, rot: Vector3 = Vector3.ZERO, mat: Material = null) -> MeshInstance3D:
	var mi := MeshInstance3D.new()
	mi.mesh = mesh
	mi.position = pos
	mi.rotation = rot
	if mat:
		mi.material_override = mat
	parent.add_child(mi)
	return mi


func _build_farmer() -> void:
	var skin := _toon(Color("#F2C9A0"))
	var overalls := _toon(Color("#3E6BB5"))
	var shirt := _toon(Color("#F0E6D2"))
	var hat := _toon(Color("#D9A441"))
	var shoes := _toon(Color("#5A3A22"))
	var hair := _toon(Color("#4A2E1B"))

	# --- Jambes (pivots à la hanche pour l'animation)
	_limb_leg_l = Node3D.new()
	_limb_leg_l.name = "JambeG"
	_limb_leg_l.position = Vector3(-0.13, 0.62, 0)
	model.add_child(_limb_leg_l)
	var leg_l := CapsuleMesh.new()
	leg_l.radius = 0.085
	leg_l.height = 0.42
	_part(_limb_leg_l, leg_l, Vector3(0, -0.28, 0))
	_limb_leg_r = Node3D.new()
	_limb_leg_r.name = "JambeD"
	_limb_leg_r.position = Vector3(0.13, 0.62, 0)
	model.add_child(_limb_leg_r)
	var leg_r := CapsuleMesh.new()
	leg_r.radius = 0.085
	leg_r.height = 0.42
	_part(_limb_leg_r, leg_r, Vector3(0, -0.28, 0))
	# chaussures
	_part(_limb_leg_l, _shoe(), Vector3(0, -0.5, 0.03), Vector3.ZERO, shoes)
	_part(_limb_leg_r, _shoe(), Vector3(0, -0.5, 0.03), Vector3.ZERO, shoes)

	# --- Corps (torse)
	_body = Node3D.new()
	_body.name = "Corps"
	_body.position = Vector3(0, 0.95, 0)
	model.add_child(_body)
	var torso := CapsuleMesh.new()
	torso.radius = 0.21
	torso.height = 0.52
	_part(_body, torso, Vector3.ZERO, Vector3.ZERO, shirt)
	# salopette (devant du torso)
	var apron := BoxMesh.new()
	apron.size = Vector3(0.30, 0.42, 0.10)
	_part(_body, apron, Vector3(0, -0.02, 0.14), Vector3.ZERO, overalls)
	# bretelles
	var strap_l := BoxMesh.new()
	strap_l.size = Vector3(0.06, 0.34, 0.05)
	_part(_body, strap_l, Vector3(-0.11, 0.16, 0.10), Vector3.ZERO, overalls)
	var strap_r := BoxMesh.new()
	strap_r.size = Vector3(0.06, 0.34, 0.05)
	_part(_body, strap_r, Vector3(0.11, 0.16, 0.10), Vector3.ZERO, overalls)

	# --- Bras (pivots à l'épaule)
	_limb_arm_l = Node3D.new()
	_limb_arm_l.name = "BrasG"
	_limb_arm_l.position = Vector3(-0.27, 1.22, 0)
	model.add_child(_limb_arm_l)
	var arm_l := CapsuleMesh.new()
	arm_l.radius = 0.065
	arm_l.height = 0.42
	_part(_limb_arm_l, arm_l, Vector3(0, -0.26, 0), Vector3.ZERO, shirt)
	_limb_arm_r = Node3D.new()
	_limb_arm_r.name = "BrasD"
	_limb_arm_r.position = Vector3(0.27, 1.22, 0)
	model.add_child(_limb_arm_r)
	var arm_r := CapsuleMesh.new()
	arm_r.radius = 0.065
	arm_r.height = 0.42
	_part(_limb_arm_r, arm_r, Vector3(0, -0.26, 0), Vector3.ZERO, shirt)
	# mains
	_part(_limb_arm_l, _hand(), Vector3(0, -0.5, 0), Vector3.ZERO, skin)
	_part(_limb_arm_r, _hand(), Vector3(0, -0.5, 0), Vector3.ZERO, skin)

	# --- Tête
	_head = Node3D.new()
	_head.name = "Tete"
	_head.position = Vector3(0, 1.52, 0)
	model.add_child(_head)
	var head := SphereMesh.new()
	head.radius = 0.165
	_part(_head, head, Vector3.ZERO, Vector3.ZERO, skin)
	# cheveux (calotte)
	var hair_mesh := SphereMesh.new()
	hair_mesh.radius = 0.175
	var hair_mi := _part(_head, hair_mesh, Vector3(0, 0.03, -0.01), Vector3.ZERO, hair)
	hair_mi.scale = Vector3(1.0, 0.72, 1.0)
	# yeux
	var eye_mat := _toon(Color("#2A2118"))
	_part(_head, _eye(), Vector3(-0.065, 0.02, 0.135), Vector3.ZERO, eye_mat)
	_part(_head, _eye(), Vector3(0.065, 0.02, 0.135), Vector3.ZERO, eye_mat)
	# sourire (petit box sombre)
	var smile := BoxMesh.new()
	smile.size = Vector3(0.09, 0.025, 0.02)
	_part(_head, smile, Vector3(0, -0.055, 0.145), Vector3.ZERO, _toon(Color("#5A3A2A")))

	# --- Chapeau de paille
	var hat_brim := CylinderMesh.new()
	hat_brim.top_radius = 0.27
	hat_brim.bottom_radius = 0.27
	hat_brim.height = 0.035
	_part(_head, hat_brim, Vector3(0, 0.16, 0), Vector3.ZERO, hat)
	var hat_top := CylinderMesh.new()
	hat_top.top_radius = 0.13
	hat_top.bottom_radius = 0.17
	hat_top.height = 0.16
	_part(_head, hat_top, Vector3(0, 0.26, 0), Vector3.ZERO, hat)
	# ruban du chapeau
	var band := CylinderMesh.new()
	band.top_radius = 0.155
	band.bottom_radius = 0.175
	band.height = 0.035
	_part(_head, band, Vector3(0, 0.185, 0), Vector3.ZERO, _toon(Color("#B0503C")))


func _shoe() -> Mesh:
	var m := BoxMesh.new()
	m.size = Vector3(0.11, 0.09, 0.2)
	return m


func _hand() -> Mesh:
	var m := SphereMesh.new()
	m.radius = 0.06
	return m


func _eye() -> Mesh:
	var m := SphereMesh.new()
	m.radius = 0.022
	return m
