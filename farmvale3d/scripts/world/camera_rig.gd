class_name CameraRig
extends Node3D
## ============================================================
## FarmVale — caméra 3e personne (style jeux de ferme modernes)
## Souris = orbite, molette = zoom, suivi fluide du joueur.
## ============================================================

@export var target: Node3D
@export var mouse_sensitivity: float = 0.0032
@export var min_pitch: float = -38.0
@export var max_pitch: float = 68.0
@export var min_zoom: float = 3.0
@export var max_zoom: float = 15.0
@export var follow_speed: float = 9.0

@onready var yaw_node: Node3D = %Yaw
@onready var pitch_node: Node3D = %Pitch
@onready var spring_arm: SpringArm3D = %SpringArm
@onready var camera: Camera3D = %Camera

var _yaw: float = 0.0
var _pitch: float = -18.0
var _zoom: float = 7.5


func _ready() -> void:
	if target == null:
		target = get_parent()
	if target:
		global_position = target.global_position + Vector3(0, 1.2, 0)
	_apply_rotation()


func _unhandled_input(event: InputEvent) -> void:
	if event is InputEventMouseMotion and Input.mouse_mode == Input.MOUSE_MODE_CAPTURED:
		_yaw -= event.relative.x * mouse_sensitivity
		_pitch -= event.relative.y * mouse_sensitivity * 0.75
		_pitch = clampf(_pitch, min_pitch, max_pitch)
		_apply_rotation()
	elif event is InputEventMouseButton:
		if event.pressed:
			if event.button_index == MOUSE_BUTTON_WHEEL_UP:
				_zoom = clampf(_zoom - 0.6, min_zoom, max_zoom)
			elif event.button_index == MOUSE_BUTTON_WHEEL_DOWN:
				_zoom = clampf(_zoom + 0.6, min_zoom, max_zoom)


func _apply_rotation() -> void:
	yaw_node.rotation.y = _yaw
	pitch_node.rotation.x = deg_to_rad(_pitch)


func _process(delta: float) -> void:
	if target == null:
		return
	# suivi fluide vers le joueur (hauteur ~ poitrine)
	var goal := target.global_position + Vector3(0, 1.15, 0)
	global_position = global_position.lerp(goal, 1.0 - exp(-follow_speed * delta))
	# zoom fluide
	spring_arm.spring_length = lerpf(spring_arm.spring_length, _zoom, 1.0 - exp(-10.0 * delta))


## Renvoie la direction « avant » de la caméra (pour le mouvement relatif)
func camera_basis() -> Basis:
	return yaw_node.global_transform.basis
