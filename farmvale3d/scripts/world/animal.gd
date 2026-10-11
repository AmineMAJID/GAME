class_name Animal
extends CharacterBody3D
## ============================================================
## FarmVale — animal du pâturage (IA simple : se promène, se
## repose, recommence). Le modèle 3D est construit par AnimalModel.
## ============================================================

@export var kind: String = "poule"
@export var wander_radius: float = 5.0
@export var move_speed: float = 1.3
@export var home_position: Vector3

@onready var model: Node3D = get_node("Model")

var _target: Vector3
var _state: String = "idle"
var _timer: float = 0.0
var _bob_phase: float = 0.0


func _ready() -> void:
	home_position = global_position
	AnimalModel.build(model, kind)
	_pick_target()
	_timer = randf_range(0.5, 2.5)


func _physics_process(delta: float) -> void:
	velocity += get_gravity() * delta

	match _state:
		"idle":
			velocity.x = 0.0
			velocity.z = 0.0
			_timer -= delta
			if _timer <= 0.0:
				_state = "walk"
				_pick_target()
		"walk":
			var to := _target - global_position
			to.y = 0.0
			if to.length() < 0.4:
				_state = "idle"
				_timer = randf_range(1.0, 3.5)
			else:
				var dir := to.normalized()
				velocity.x = dir.x * move_speed
				velocity.z = dir.z * move_speed
				model.rotation.y = lerp_angle(model.rotation.y, atan2(dir.x, dir.z), delta * 6.0)

	move_and_slide()

	# petit balancement en marchant
	_bob_phase += delta * (8.0 if _state == "walk" else 2.0)
	model.position.y = abs(sin(_bob_phase)) * 0.025


func _pick_target() -> void:
	var angle := randf_range(0.0, TAU)
	var radius := randf_range(1.0, wander_radius)
	_target = home_position + Vector3(cos(angle) * radius, 0.0, sin(angle) * radius)
	_target.y = global_position.y
