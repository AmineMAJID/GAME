class_name CropModel
extends RefCounted
## ============================================================
## FarmVale — cultures 3D stylisées (blé, fraise, citrouille)
## construites en primitives. Le feuillage utilise le shader
## « foliage » (vent + translucidité).
## ============================================================

static func toon(color: Color) -> ShaderMaterial:
	var shader := load("res://shaders/toon.gdshader") as Shader
	var mat := ShaderMaterial.new()
	mat.shader = shader
	mat.set_shader_parameter("albedo", color)
	return mat


static func foliage(color: Color) -> ShaderMaterial:
	var shader := load("res://shaders/foliage.gdshader") as Shader
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


## Construit une culture. `kind` : blé, fraise, citrouille, maïs, tomate…
static func build(parent: Node3D, kind: String) -> void:
	match kind:
		"blé":
			_build_wheat(parent)
		"fraise":
			_build_strawberry(parent)
		"citrouille":
			_build_pumpkin(parent)
		"maïs":
			_build_corn(parent)
		"tomate":
			_build_tomato(parent)
		_:
			_build_wheat(parent)


# ------------------------------------------------------------
# Blé : tiges dorées + gerbe
# ------------------------------------------------------------
static func _build_wheat(parent: Node3D) -> void:
	var stalk_mat := toon(Color("#C9A84C"))
	var grain_mat := toon(Color("#E8C96A"))
	for i in 3:
		var stalk := CylinderMesh.new()
		stalk.top_radius = 0.015
		stalk.bottom_radius = 0.02
		stalk.height = 0.5
		var x := (i - 1) * 0.06
		part(parent, stalk, Vector3(x, 0.25, 0), stalk_mat)
		# gerbe au sommet
		for g in 4:
			var grain := SphereMesh.new()
			grain.radius = 0.035
			part(parent, grain, Vector3(x + sin(g * 1.7) * 0.03, 0.48 + g * 0.045, cos(g * 1.3) * 0.03), grain_mat)


# ------------------------------------------------------------
# Fraise : touffe verte + fraises rouges
# ------------------------------------------------------------
static func _build_strawberry(parent: Node3D) -> void:
	var leaf_mat := foliage(Color("#3E8E4E"))
	var berry_mat := toon(Color("#D43A2F"))
	# touffe
	var mound := SphereMesh.new()
	mound.radius = 0.2
	part(parent, mound, Vector3(0, 0.1, 0), leaf_mat, Vector3(1.4, 0.7, 1.4))
	for i in 3:
		var leaf := SphereMesh.new()
		leaf.radius = 0.09
		part(parent, leaf, Vector3((i - 1) * 0.1, 0.16, 0), leaf_mat, Vector3(1.2, 0.7, 1.2))
	# fraises
	for i in 5:
		var a := float(i) * TAU / 5.0
		var berry := SphereMesh.new()
		berry.radius = 0.05
		part(parent, berry, Vector3(cos(a) * 0.09, 0.2, sin(a) * 0.09), berry_mat, Vector3(1.0, 0.85, 1.0))
	# petites fleurs blanches
	var flower_mat := toon(Color("#FFF8EE"))
	var f := SphereMesh.new()
	f.radius = 0.03
	part(parent, f, Vector3(0.12, 0.24, -0.05), flower_mat)


# ------------------------------------------------------------
# Citrouille
# ------------------------------------------------------------
static func _build_pumpkin(parent: Node3D) -> void:
	var pumpkin_mat := toon(Color("#D97B2B"))
	var stem_mat := toon(Color("#5A4A2A"))
	var leaf_mat := foliage(Color("#4E8E3E"))
	var body := SphereMesh.new()
	body.radius = 0.26
	part(parent, body, Vector3(0, 0.24, 0), pumpkin_mat, Vector3(1.15, 0.85, 1.15))
	# côtes de la citrouille
	for i in 2:
		var rib := TorusMesh.new()
		rib.inner_radius = 0.24
		rib.outer_radius = 0.27
		var rib_mi := part(parent, rib, Vector3(0, 0.24, 0), pumpkin_mat, Vector3(1.15, 0.85, 1.15))
		rib_mi.rotation.x = PI / 2.0
		rib_mi.rotation.y = i * PI / 2.0
	# tige
	var stem := CylinderMesh.new()
	stem.top_radius = 0.03
	stem.bottom_radius = 0.045
	stem.height = 0.14
	part(parent, stem, Vector3(0, 0.5, 0), stem_mat, Vector3(1, 1, 1))
	# feuilles
	var leaf := SphereMesh.new()
	leaf.radius = 0.12
	part(parent, leaf, Vector3(-0.14, 0.42, 0.02), leaf_mat, Vector3(1.2, 0.5, 1.2))
	part(parent, leaf, Vector3(0.13, 0.4, -0.04), leaf_mat, Vector3(1.1, 0.5, 1.1))
	# vrille
	var curl := TorusMesh.new()
	curl.inner_radius = 0.03
	curl.outer_radius = 0.05
	part(parent, curl, Vector3(0.08, 0.56, 0), stem_mat, Vector3(0.6, 0.6, 0.6))


# ------------------------------------------------------------
# Maïs
# ------------------------------------------------------------
static func _build_corn(parent: Node3D) -> void:
	var stalk_mat := foliage(Color("#4E9E4E"))
	var cob_mat := toon(Color("#E8C84A"))
	var stalk := CylinderMesh.new()
	stalk.top_radius = 0.03
	stalk.bottom_radius = 0.04
	stalk.height = 0.75
	part(parent, stalk, Vector3(0, 0.375, 0), stalk_mat)
	# épi
	var cob := CylinderMesh.new()
	cob.top_radius = 0.06
	cob.bottom_radius = 0.09
	cob.height = 0.28
	part(parent, cob, Vector3(0, 0.72, 0), cob_mat, Vector3(1, 1, 1))
	# soies
	var silk := CylinderMesh.new()
	silk.top_radius = 0.02
	silk.bottom_radius = 0.05
	silk.height = 0.14
	part(parent, silk, Vector3(0, 0.9, 0), toon(Color("#F2E2B0")))
	# feuilles
	var leaf := SphereMesh.new()
	leaf.radius = 0.16
	part(parent, leaf, Vector3(-0.12, 0.4, 0), stalk_mat, Vector3(1.4, 0.4, 1.2))
	part(parent, leaf, Vector3(0.12, 0.3, 0), stalk_mat, Vector3(1.3, 0.4, 1.1))


# ------------------------------------------------------------
# Tomate
# ------------------------------------------------------------
static func _build_tomato(parent: Node3D) -> void:
	var vine_mat := foliage(Color("#3E7E3E"))
	var tomato_mat := toon(Color("#D43A2F"))
	var mound := SphereMesh.new()
	mound.radius = 0.22
	part(parent, mound, Vector3(0, 0.12, 0), vine_mat, Vector3(1.4, 0.8, 1.4))
	for i in 4:
		var a := float(i) * TAU / 4.0 + 0.4
		var t := SphereMesh.new()
		t.radius = 0.07
		part(parent, t, Vector3(cos(a) * 0.11, 0.22, sin(a) * 0.11), tomato_mat, Vector3(1, 0.9, 1))
	# petite étoile verte sur chaque tomate
	var calyx := CylinderMesh.new()
	calyx.top_radius = 0.02
	calyx.bottom_radius = 0.035
	calyx.height = 0.03
	part(parent, calyx, Vector3(0.11, 0.3, 0), vine_mat)
