class_name AnimalModel
extends RefCounted
## ============================================================
## FarmVale — construction procédurale des animaux en primitives
## (poule, vache, mouton) avec le matériau « toon ».
## ============================================================

static func toon(color: Color) -> ShaderMaterial:
	var shader := load("res://shaders/toon.gdshader") as Shader
	var mat := ShaderMaterial.new()
	mat.shader = shader
	mat.set_shader_parameter("albedo", color)
	return mat


static func part(parent: Node, mesh: Mesh, pos: Vector3, mat: Material, scale: Vector3 = Vector3.ONE) -> MeshInstance3D:
	var mi := MeshInstance3D.new()
	mi.mesh = mesh
	mi.position = pos
	mi.material_override = mat
	mi.scale = scale
	parent.add_child(mi)
	return mi


static func build(parent: Node3D, kind: String) -> void:
	match kind:
		"poule":
			_build_chicken(parent)
		"vache":
			_build_cow(parent)
		"mouton":
			_build_sheep(parent)


# ------------------------------------------------------------
# Poule
# ------------------------------------------------------------
static func _build_chicken(parent: Node3D) -> void:
	var white := toon(Color("#FAFAF5"))
	var orange := toon(Color("#E8912D"))
	var red := toon(Color("#C0392B"))
	var dark := toon(Color("#2A2118"))

	var body := SphereMesh.new()
	body.radius = 0.26
	part(parent, body, Vector3(0, 0.42, 0), white, Vector3(1.15, 0.9, 1.0))
	# tête
	var head := SphereMesh.new()
	head.radius = 0.16
	part(parent, head, Vector3(0, 0.74, 0.2), white)
	# bec
	var beak := BoxMesh.new()
	beak.size = Vector3(0.05, 0.05, 0.14)
	part(parent, beak, Vector3(0, 0.72, 0.34), orange)
	# crête
	var comb := BoxMesh.new()
	comb.size = Vector3(0.05, 0.09, 0.05)
	part(parent, comb, Vector3(0, 0.88, 0.16), red)
	# yeux
	var eye := SphereMesh.new()
	eye.radius = 0.025
	part(parent, eye, Vector3(-0.06, 0.78, 0.27), dark)
	part(parent, eye, Vector3(0.06, 0.78, 0.27), dark)
	# ailes
	var wing := SphereMesh.new()
	wing.radius = 0.14
	part(parent, wing, Vector3(-0.24, 0.44, 0), white, Vector3(0.5, 0.8, 1.0))
	part(parent, wing, Vector3(0.24, 0.44, 0), white, Vector3(0.5, 0.8, 1.0))
	# queue
	var tail := SphereMesh.new()
	tail.radius = 0.12
	part(parent, tail, Vector3(0, 0.5, -0.28), white, Vector3(0.8, 1.0, 0.6))
	# pattes
	var leg := CylinderMesh.new()
	leg.top_radius = 0.025
	leg.bottom_radius = 0.025
	leg.height = 0.16
	part(parent, leg, Vector3(-0.08, 0.1, 0.02), orange)
	part(parent, leg, Vector3(0.08, 0.1, 0.02), orange)


# ------------------------------------------------------------
# Vache
# ------------------------------------------------------------
static func _build_cow(parent: Node3D) -> void:
	var white := toon(Color("#FAFAF5"))
	var spot := toon(Color("#3A3A3A"))
	var pink := toon(Color("#E8A0A0"))
	var horn := toon(Color("#E8D8B0"))
	var dark := toon(Color("#2A2118"))

	# corps
	var body := BoxMesh.new()
	body.size = Vector3(0.95, 0.52, 0.55)
	part(parent, body, Vector3(0, 0.72, 0), white)
	# taches
	var s1 := SphereMesh.new()
	s1.radius = 0.16
	part(parent, s1, Vector3(-0.15, 0.78, 0.1), spot, Vector3(1.0, 0.7, 0.9))
	var s2 := SphereMesh.new()
	s2.radius = 0.12
	part(parent, s2, Vector3(0.22, 0.66, -0.12), spot, Vector3(0.9, 0.7, 0.8))
	# tête
	var head := BoxMesh.new()
	head.size = Vector3(0.4, 0.36, 0.42)
	part(parent, head, Vector3(0, 0.95, 0.42), white)
	# museau
	var muzzle := BoxMesh.new()
	muzzle.size = Vector3(0.26, 0.16, 0.16)
	part(parent, muzzle, Vector3(0, 0.82, 0.6), pink)
	# yeux
	var eye := SphereMesh.new()
	eye.radius = 0.035
	part(parent, eye, Vector3(-0.11, 1.0, 0.55), dark)
	part(parent, eye, Vector3(0.11, 1.0, 0.55), dark)
	# cornes
	var horn_mesh := CylinderMesh.new()
	horn_mesh.top_radius = 0.02
	horn_mesh.bottom_radius = 0.045
	horn_mesh.height = 0.18
	part(parent, horn_mesh, Vector3(-0.14, 1.2, 0.4), horn, Vector3.ONE)
	part(parent, horn_mesh, Vector3(0.14, 1.2, 0.4), horn, Vector3.ONE)
	# oreilles
	var ear := BoxMesh.new()
	ear.size = Vector3(0.16, 0.06, 0.1)
	part(parent, ear, Vector3(-0.24, 1.05, 0.35), white, Vector3(1, 1, 1))
	part(parent, ear, Vector3(0.24, 1.05, 0.35), white, Vector3(1, 1, 1))
	# pattes
	var leg := CylinderMesh.new()
	leg.top_radius = 0.06
	leg.bottom_radius = 0.06
	leg.height = 0.5
	part(parent, leg, Vector3(-0.32, 0.25, 0.2), white)
	part(parent, leg, Vector3(0.32, 0.25, 0.2), white)
	part(parent, leg, Vector3(-0.32, 0.25, -0.2), white)
	part(parent, leg, Vector3(0.32, 0.25, -0.2), white)
	# pis
	var udder := SphereMesh.new()
	udder.radius = 0.12
	part(parent, udder, Vector3(0, 0.48, 0.1), pink, Vector3(1.2, 0.7, 1.0))
	# queue
	var tail := CylinderMesh.new()
	tail.top_radius = 0.03
	tail.bottom_radius = 0.03
	tail.height = 0.55
	var tail_mi := part(parent, tail, Vector3(-0.48, 0.85, 0), dark)
	tail_mi.rotation.z = 0.5


# ------------------------------------------------------------
# Mouton
# ------------------------------------------------------------
static func _build_sheep(parent: Node3D) -> void:
	var wool := toon(Color("#F2F0E8"))
	var face := toon(Color("#4A4038"))
	var dark := toon(Color("#1E1A16"))

	# laine (grappe de sphères)
	for offset in [Vector3(0, 0.62, 0), Vector3(-0.2, 0.58, 0.08), Vector3(0.2, 0.58, 0.08),
					Vector3(0, 0.52, -0.12), Vector3(-0.14, 0.48, 0.16), Vector3(0.14, 0.48, 0.16)]:
		var puff := SphereMesh.new()
		puff.radius = 0.24
		part(parent, puff, offset, wool, Vector3(1.15, 0.95, 1.1))
	# tête
	var head := SphereMesh.new()
	head.radius = 0.17
	part(parent, head, Vector3(0, 0.58, 0.32), face)
	# yeux
	var eye := SphereMesh.new()
	eye.radius = 0.03
	part(parent, eye, Vector3(-0.07, 0.62, 0.42), dark)
	part(parent, eye, Vector3(0.07, 0.62, 0.42), dark)
	# oreilles
	var ear := BoxMesh.new()
	ear.size = Vector3(0.12, 0.05, 0.08)
	part(parent, ear, Vector3(-0.16, 0.66, 0.3), face)
	part(parent, ear, Vector3(0.16, 0.66, 0.3), face)
	# pattes
	var leg := CylinderMesh.new()
	leg.top_radius = 0.05
	leg.bottom_radius = 0.05
	leg.height = 0.32
	part(parent, leg, Vector3(-0.16, 0.16, 0.14), dark)
	part(parent, leg, Vector3(0.16, 0.16, 0.14), dark)
	part(parent, leg, Vector3(-0.16, 0.16, -0.14), dark)
	part(parent, leg, Vector3(0.16, 0.16, -0.14), dark)
