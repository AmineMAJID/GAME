class_name Props
extends Node3D
## ============================================================
## FarmVale — génération procédurale du monde 3D
## Champs, cultures, arbres, clôture, fleurs, nuages, papillons,
## oiseaux, particules de feuilles, lucioles, maison, étang, animaux.
## ============================================================

@export var world: Node3D

# --- Constantes miroir du shader ground.gdshader (pour poser les props)
const HILL_SCALE := 0.016
const HILL_HEIGHT := 0.55

# Tailles du monde
const WORLD_HALF := 100.0

# Palette « moderne & chaleureuse »
const CREAM := Color("#F5E9D0")
const WOOD := Color("#8A5A33")
const WOOD_DARK := Color("#5F3D20")
const TERRACOTTA := Color("#C1543C")
const LEAF := Color("#4E8E4E")
const LEAF_DARK := Color("#35703A")


func _ready() -> void:
	generate()


# ============================================================
# Bruit de terrain (miroir GDScript du shader — même fonction de hash)
# ============================================================
static func hash21(p: Vector2) -> float:
	var v := sin(p.dot(Vector2(127.1, 311.7))) * 43758.5453123
	return v - floor(v)  # fract() GLSL (fmod garderait le signe)


static func vnoise(p: Vector2) -> float:
	var i := p.floor()
	var f := p - i
	f = f * f * (Vector2(3.0, 3.0) - 2.0 * f)  # smoothstep, composante par composante
	var a := hash21(i)
	var b := hash21(i + Vector2(1, 0))
	var c := hash21(i + Vector2(0, 1))
	var d := hash21(i + Vector2(1, 1))
	return lerp(lerp(a, b, f.x), lerp(c, d, f.x), f.y)


## Hauteur du sol à une position monde (miroir du déplacement shader)
static func ground_height(x: float, z: float) -> float:
	var p := Vector2(x, z)
	return vnoise(p * HILL_SCALE) * HILL_HEIGHT + vnoise(p * HILL_SCALE * 4.0 + Vector2(7.0, 3.0)) * HILL_HEIGHT * 0.3


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


static func add_collision(parent: Node, shape: Shape3D, pos: Vector3) -> void:
	# Corps statique (layer « monde ») : empêche le joueur de traverser le prop
	var body := StaticBody3D.new()
	body.name = "Collision"
	body.collision_layer = 1
	body.collision_mask = 0
	body.position = pos
	var col := CollisionShape3D.new()
	col.shape = shape
	body.add_child(col)
	parent.add_child(body)


static var _mesh_counter := 0

static func add_mesh(parent: Node, mesh: Mesh, pos: Vector3, mat: Material, name: String = "") -> MeshInstance3D:
	var mi := MeshInstance3D.new()
	if name != "":
		_mesh_counter += 1
		mi.name = "%s_%d" % [name, _mesh_counter]  # nom unique (Godot renomme les doublons)
	mi.mesh = mesh
	mi.position = pos
	mi.material_override = mat
	mi.cast_shadow = GeometryInstance3D.SHADOW_CASTING_SETTING_ON
	parent.add_child(mi)
	return mi


# ============================================================
# Génération complète
# ============================================================
func generate() -> void:
	_spawn_house()
	_spawn_fields()
	_spawn_trees()
	_spawn_fence()
	_spawn_flowers()
	_spawn_clouds()
	_spawn_pond_details()
	_spawn_animals()
	_spawn_butterflies()
	_spawn_birds()
	_spawn_leaves_particles()
	_spawn_fireflies()


# ============================================================
# La maison (grange stylisée)
# ============================================================
func _spawn_house() -> void:
	var house := Node3D.new()
	house.name = "Maison"
	var hx := -18.0
	var hz := -14.0
	house.position = Vector3(hx, ground_height(hx, hz) - 0.02, hz)
	add_child(house)
	# collision : le joueur ne traverse pas la maison
	var house_col := BoxShape3D.new()
	house_col.size = Vector3(7.2, 3.4, 5.2)
	add_collision(house, house_col, Vector3(0, 1.7, 0))

	# murs
	var walls := BoxMesh.new()
	walls.size = Vector3(7.0, 3.4, 5.0)
	add_mesh(house, walls, Vector3(0, 1.7, 0), toon(CREAM), "Murs")
	# toit en tuiles : deux pentes inclinées qui se rejoignent au faîte
	var slope_angle := atan2(1.0, 3.5)  # ~16°
	var slope_len := sqrt(3.5 * 3.5 + 1.0 * 1.0)
	for side in [-1.0, 1.0]:
		var slope := BoxMesh.new()
		slope.size = Vector3(slope_len, 0.16, 5.4)
		var smi := add_mesh(house, slope, Vector3(side * 1.75, 3.9, 0), toon(TERRACOTTA), "Toit")
		smi.rotation.z = side * slope_angle  # inclinaison de la pente
	# cheminée + fumée
	var chimney := CylinderMesh.new()
	chimney.top_radius = 0.18
	chimney.bottom_radius = 0.22
	chimney.height = 0.8
	add_mesh(house, chimney, Vector3(2.2, 4.3, -1.2), toon(WOOD_DARK), "Cheminee")
	# porte
	var door := BoxMesh.new()
	door.size = Vector3(1.1, 2.0, 0.15)
	add_mesh(house, door, Vector3(0, 1.0, 2.55), toon(WOOD_DARK), "Porte")
	# fenêtres (émettent la nuit — groupe « window_glow »)
	var win_mat := StandardMaterial3D.new()
	win_mat.albedo_color = Color("#FFE9A8")
	win_mat.emission_enabled = true
	win_mat.emission = Color("#FFD76A")
	win_mat.emission_energy = 0.0
	for wx in [-2.0, 2.0]:
		var win := BoxMesh.new()
		win.size = Vector3(1.0, 0.9, 0.1)
		var wmi := add_mesh(house, win, Vector3(wx, 2.0, 2.55), win_mat, "Fenetre")
		wmi.add_to_group("window_glow")
	# lanterne près de la porte
	var lantern_mat := StandardMaterial3D.new()
	lantern_mat.albedo_color = Color("#FFE9A8")
	lantern_mat.emission_enabled = true
	lantern_mat.emission = Color("#FFC85A")
	lantern_mat.emission_energy = 2.5
	var lamp := SphereMesh.new()
	lamp.radius = 0.14
	var lamp_mi := add_mesh(house, lamp, Vector3(0.85, 1.6, 2.4), lantern_mat, "Lanterne")
	lamp_mi.add_to_group("window_glow")
	var lamp_post := CylinderMesh.new()
	lamp_post.top_radius = 0.04
	lamp_post.bottom_radius = 0.05
	lamp_post.height = 1.6
	add_mesh(house, lamp_post, Vector3(0.85, 0.8, 2.4), toon(WOOD_DARK), "PoteauLanterne")


# ============================================================
# Champs labourés + cultures
# ============================================================
func _spawn_fields() -> void:
	var fields := Node3D.new()
	fields.name = "Champs"
	add_child(fields)
	# terre labourée (terreau sombre)
	var soil_mat := StandardMaterial3D.new()
	soil_mat.albedo_color = Color("#6B4A2E")
	soil_mat.roughness = 1.0
	var soil_shader := load("res://shaders/toon.gdshader") as Shader
	var soil_toon := ShaderMaterial.new()
	soil_toon.shader = soil_shader
	soil_toon.set_shader_parameter("albedo", Color("#6B4A2E"))
	soil_toon.set_shader_parameter("rim", 0.15)

	# 2 planches de 4×3 parcelles
	var plots := [
		{"pos": Vector2(4.0, -6.0), "crops": ["blé", "fraise", "citrouille", "maïs"]},
		{"pos": Vector2(12.0, -13.0), "crops": ["tomate", "blé", "citrouille", "fraise"]},
	]
	var plot_idx := 0
	for plot in plots:
		plot_idx += 1
		var base := plot["pos"] as Vector2
		var crops: Array = plot["crops"]
		for px in 4:
			for pz in 3:
				var wx := base.x + float(px) * 2.0
				var wz := base.y + float(pz) * 2.0
				var gy := ground_height(wx, wz)
				# parcelle de terre
				var soil := BoxMesh.new()
				soil.size = Vector3(1.7, 0.14, 1.7)
				add_mesh(fields, soil, Vector3(wx, gy + 0.05, wz), soil_toon, "Parcelle")
				# collision : parcelle surélevée (on marche dessus)
				var soil_col := BoxShape3D.new()
				soil_col.size = Vector3(1.7, 0.14, 1.7)
				add_collision(fields, soil_col, Vector3(wx, gy + 0.05, wz))
				# sillons (lignes plus sombres)
				for s in 3:
					var furrow := BoxMesh.new()
					furrow.size = Vector3(1.5, 0.02, 0.12)
					add_mesh(fields, furrow, Vector3(wx, gy + 0.125, wz - 0.5 + float(s) * 0.5), toon(Color("#54371F")), "Sillon")
				# culture au centre de la parcelle
				var crop := Node3D.new()
				crop.name = "Culture_%d_%d_%d" % [plot_idx, px, pz]
				crop.position = Vector3(wx, gy + 0.08, wz)
				crop.scale = Vector3(0.9, 0.9, 0.9)
				fields.add_child(crop)
				CropModel.build(crop, String(crops[(px + pz) % crops.size()]))


# ============================================================
# Arbres stylisés
# ============================================================
func _spawn_trees() -> void:
	var trees := Node3D.new()
	trees.name = "Arbres"
	add_child(trees)
	var trunk_mat := toon(WOOD_DARK)
	# positions d'arbres (pourtour de la ferme)
	var positions: Array[Vector2] = [
		Vector2(-26, -26), Vector2(-30, -6), Vector2(-24, 14), Vector2(-8, 24),
		Vector2(8, 28), Vector2(24, 22), Vector2(30, 4), Vector2(26, -24),
		Vector2(6, -28), Vector2(-32, -20), Vector2(34, -10), Vector2(-14, -30),
	]
	for pos in positions:
		var wx := pos.x
		var wz := pos.y
		var gy := ground_height(wx, wz)
		var tree := Node3D.new()
		tree.name = "Arbre"
		# collision : tronc
		var trunk_col := CylinderShape3D.new()
		trunk_col.radius = 0.3
		trunk_col.height = 1.4
		add_collision(tree, trunk_col, Vector3(0, 0.7, 0))
		tree.position = Vector3(wx, gy - 0.02, wz)
		trees.add_child(tree)
		var scale_v := randf_range(0.9, 1.25)
		tree.scale = Vector3(scale_v, scale_v, scale_v)
		# tronc
		var trunk := CylinderMesh.new()
		trunk.top_radius = 0.14
		trunk.bottom_radius = 0.2
		trunk.height = 2.4
		add_mesh(tree, trunk, Vector3(0, 1.2, 0), trunk_mat, "Tronc")
		# feuillage (3 sphères, matériau « foliage » qui ondule)
		var leaf_mat := foliage(LEAF if randi() % 2 == 0 else LEAF_DARK)
		for i in 3:
			var blob := SphereMesh.new()
			blob.radius = randf_range(0.75, 1.05)
			var y := 2.6 + float(i) * 0.45
			var x := sin(float(i) * 2.4) * 0.35
			var z := cos(float(i) * 2.1) * 0.35
			add_mesh(tree, blob, Vector3(x, y, z), leaf_mat, "Feuillage")
		# fruits (pommes / baies rouges) sur certains arbres
		if randi() % 3 != 0:
			var fruit_mat := toon(Color("#D43A3A"))
			for f in 4:
				var a := randf_range(0.0, TAU)
				var fr := SphereMesh.new()
				fr.radius = 0.07
				add_mesh(tree, fr, Vector3(cos(a) * 0.7, 2.4 + randf_range(0.0, 1.2), sin(a) * 0.7), fruit_mat, "Fruit")


# ============================================================
# Clôture du pâturage
# ============================================================
func _spawn_fence() -> void:
	var fence := Node3D.new()
	fence.name = "Cloture"
	add_child(fence)
	var post_mat := toon(WOOD)
	var rail_mat := toon(WOOD)
	# rectangle du pâturage
	var x0 := 8.0
	var x1 := 22.0
	var z0 := 4.0
	var z1 := 16.0
	var step := 2.0
	# les 4 côtés posent poteaux + lisses (les coins sont inclus)
	_spawn_fence_side(fence, Vector2(x0, z0), Vector2(x1, z0), step, rail_mat)
	_spawn_fence_side(fence, Vector2(x1, z0), Vector2(x1, z1), step, rail_mat)
	_spawn_fence_side(fence, Vector2(x1, z1), Vector2(x0, z1), step, rail_mat)
	_spawn_fence_side(fence, Vector2(x0, z1), Vector2(x0, z0), step, rail_mat)


func _spawn_fence_side(fence: Node3D, a: Vector2, b: Vector2, step: float, mat: Material) -> void:
	var dir := (b - a).normalized()
	var len := a.distance_to(b)
	var n := int(round(len / step))
	for i in range(1, n + 1):
		var t := float(i) * step
		var p := a + dir * t
		var gy := ground_height(p.x, p.y)
		# poteau
		var post := CylinderMesh.new()
		post.top_radius = 0.07
		post.bottom_radius = 0.09
		post.height = 1.0
		add_mesh(fence, post, Vector3(p.x, gy + 0.5, p.y), mat, "Poteau")
		# collision : poteau
		var post_col := CylinderShape3D.new()
		post_col.radius = 0.12
		post_col.height = 1.0
		add_collision(fence, post_col, Vector3(p.x, gy + 0.5, p.y))
		# 2 lisses horizontales
		var rail_len := step
		for h in [0.32, 0.66]:
			var rail := BoxMesh.new()
			rail.size = Vector3(rail_len, 0.07, 0.07)
			var rmi := add_mesh(fence, rail, Vector3(p.x + dir.x * step * 0.5, gy + h, p.y + dir.y * step * 0.5), mat, "Lis")
			rmi.rotation.y = atan2(-dir.y, dir.x)  # aligne l'axe X sur la direction


# ============================================================
# Fleurs (MultiMesh — 150 instances colorées)
# ============================================================
func _spawn_flowers() -> void:
	var flowers := MultiMeshInstance3D.new()
	flowers.name = "Fleurs"
	add_child(flowers)

	# maillage d'une fleur : tige + pétales (couleurs par instance)
	var flower_mesh := _build_flower_mesh()
	var mm := MultiMesh.new()
	mm.transform_format = MultiMesh.TRANSFORM_3D
	mm.use_colors = true
	mm.mesh = flower_mesh
	mm.instance_count = 150
	flowers.multimesh = mm

	var palette := [Color("#FFF6FA"), Color("#FFD34D"), Color("#F28AB0"), Color("#B58AE8"), Color("#FF8A5A"), Color("#FFE9A8")]
	var rng := RandomNumberGenerator.new()
	rng.seed = 42
	for i in 150:
		# position aléatoire dans un anneau autour du centre
		var a := rng.randf_range(0.0, TAU)
		var r := rng.randf_range(6.0, 46.0)
		var wx := cos(a) * r
		var wz := sin(a) * r
		# évite l'étang, la maison et les champs
		if Vector2(wx, wz).distance_to(Vector2(24, -18)) < 9.0:
			continue
		if Vector2(wx, wz).distance_to(Vector2(-18, -14)) < 10.0:
			continue
		if wx > 2.0 and wx < 16.0 and wz > -16.0 and wz < -2.0:
			continue
		var gy := ground_height(wx, wz)
		var s := rng.randf_range(0.7, 1.3)
		var xform := Transform3D(Basis.IDENTITY, Vector3(wx, gy - 0.02, wz))
		xform = xform.scaled(Vector3(s, s, s))
		xform = xform.rotated(Vector3.UP, rng.randf_range(0.0, TAU))
		mm.set_instance_transform(i, xform)
		mm.set_instance_color(i, palette[rng.randi_range(0, palette.size() - 1)])
	flowers.cast_shadow = GeometryInstance3D.SHADOW_CASTING_SETTING_OFF


## Maillage d'une fleur simple (tige + 5 pétales + cœur) en ArrayMesh
func _build_flower_mesh() -> ArrayMesh:
	var st := SurfaceTool.new()
	st.begin(Mesh.PRIMITIVE_TRIANGLES)
	# tige (2 triangles)
	st.set_color(Color("#3E7E3E"))
	st.add_vertex(Vector3(-0.015, 0.0, 0.0))
	st.add_vertex(Vector3(0.015, 0.0, 0.0))
	st.add_vertex(Vector3(0.015, 0.28, 0.0))
	st.add_vertex(Vector3(-0.015, 0.0, 0.0))
	st.add_vertex(Vector3(0.015, 0.28, 0.0))
	st.add_vertex(Vector3(-0.015, 0.28, 0.0))
	# pétales (5 quads autour du cœur) — couleurs blanches, teintées par l'instance
	st.set_color(Color.WHITE)
	for i in 5:
		var a := float(i) * TAU / 5.0
		var dir := Vector3(cos(a), 0.0, sin(a))
		var center := Vector3(0.0, 0.34, 0.0)
		var p1 := center + dir * 0.05
		var p2 := center + dir * 0.16
		var up := Vector3(0.0, 0.06, 0.0)
		# quad (2 triangles)
		st.add_vertex(p1 - up)
		st.add_vertex(p2 - up)
		st.add_vertex(p2 + up)
		st.add_vertex(p1 - up)
		st.add_vertex(p2 + up)
		st.add_vertex(p1 + up)
	# cœur (petit disque)
	st.set_color(Color("#E8B93A"))
	for i in 8:
		var a0 := float(i) * TAU / 8.0
		var a1 := float(i + 1) * TAU / 8.0
		st.add_vertex(Vector3(0.0, 0.36, 0.0))
		st.add_vertex(Vector3(cos(a0) * 0.05, 0.36, sin(a0) * 0.05))
		st.add_vertex(Vector3(cos(a1) * 0.05, 0.36, sin(a1) * 0.05))
	st.generate_normals()
	var mesh := st.commit()
	# matériau « toon » qui utilise les couleurs de sommet/instance
	var shader := Shader.new()
	shader.code = """
shader_type spatial;
render_mode diffuse_toon, cull_disabled;
uniform float sway_strength = 0.04;
void vertex() {
	float s = sin(TIME * 2.0 + VERTEX.x * 6.0) * sway_strength;
	VERTEX.x += s;
	VERTEX.z += s * 0.6;
}
void fragment() {
	// COLOR = couleur de sommet × couleur d'instance (MultiMesh)
	ALBEDO = COLOR.rgb;
	ROUGHNESS = 0.9;
}
"""
	var mat := ShaderMaterial.new()
	mat.shader = shader
	mesh.surface_set_material(0, mat)
	return mesh


# ============================================================
# Nuages (sphères blanches qui dérivent)
# ============================================================
func _spawn_clouds() -> void:
	var clouds := Node3D.new()
	clouds.name = "Nuages"
	add_child(clouds)
	var cloud_mat := toon(Color("#FFFFFF"))
	cloud_mat.set_shader_parameter("rim", 0.6)
	cloud_mat.set_shader_parameter("specular", 0.1)
	var rng := RandomNumberGenerator.new()
	rng.seed = 7
	for i in 6:
		var cloud := Node3D.new()
		cloud.name = "Nuage"
		var wx := rng.randf_range(-70.0, 70.0)
		var wz := rng.randf_range(-70.0, 70.0)
		var y := rng.randf_range(16.0, 24.0)
		cloud.position = Vector3(wx, y, wz)
		clouds.add_child(cloud)
		for b in 4:
			var puff := SphereMesh.new()
			puff.radius = rng.randf_range(1.2, 2.0)
			add_mesh(cloud, puff, Vector3((b - 1.5) * 1.6 + rng.randf_range(-0.3, 0.3), rng.randf_range(-0.3, 0.4), rng.randf_range(-0.6, 0.6)), cloud_mat, "NuageBis")
		# les nuages se déplacent dans _process
		cloud.set_meta("speed", rng.randf_range(1.5, 4.0))
		cloud.add_to_group("cloud")


func _process(delta: float) -> void:
	# dérive des nuages
	for cloud in get_tree().get_nodes_in_group("cloud"):
		var c := cloud as Node3D
		c.position.x += delta * c.get_meta("speed", 2.0)
		if c.position.x > 80.0:
			c.position.x = -80.0
	# papillons : trajectoire sinusoïdale + battement d'ailes
	for bf_node in get_tree().get_nodes_in_group("butterfly"):
		var bf := bf_node as Node3D
		var phase: float = bf.get_meta("phase", 0.0) + delta * 1.6
		bf.set_meta("phase", phase)
		var center: Vector3 = bf.get_meta("center", bf.position)
		bf.position = center + Vector3(sin(phase * 1.3) * 3.0, sin(phase * 2.1) * 0.5, cos(phase) * 3.0)
		bf.rotation.y = phase
		var flap := sin(phase * 22.0) * 0.7
		var wl := bf.get_meta("wing_l") as MeshInstance3D
		var wr := bf.get_meta("wing_r") as MeshInstance3D
		if wl:
			wl.rotation.z = flap
		if wr:
			wr.rotation.z = -flap
	# oiseaux : vol circulaire + battement d'ailes
	for bird_node in get_tree().get_nodes_in_group("bird"):
		var bird := bird_node as Node3D
		var phase: float = bird.get_meta("phase", 0.0) + delta * 0.5
		bird.set_meta("phase", phase)
		var radius := 26.0
		bird.position = Vector3(cos(phase) * radius, 12.0 + sin(phase * 0.7) * 1.5, sin(phase) * radius)
		bird.rotation.y = -phase + PI / 2.0
		var flap := sin(phase * 18.0) * 0.5
		for child in bird.get_children():
			if child is MeshInstance3D:
				child.rotation.z = flap if child.name == "AileG" else -flap


# ============================================================
# Étang : nénuphars + roseaux
# ============================================================
func _spawn_pond_details() -> void:
	var pond := Node3D.new()
	pond.name = "EtangDetails"
	add_child(pond)
	var px := 24.0
	var pz := -18.0
	var gy := ground_height(px, pz)
	var lily_mat := toon(Color("#5AA85A"))
	var reed_mat := toon(Color("#6E8E3E"))
	var rng := RandomNumberGenerator.new()
	rng.seed = 11
	# nénuphars
	for i in 8:
		var a := rng.randf_range(0.0, TAU)
		var r := rng.randf_range(1.0, 5.5)
		var lx := px + cos(a) * r
		var lz := pz + sin(a) * r
		var lily := CylinderMesh.new()
		lily.top_radius = 0.35
		lily.bottom_radius = 0.35
		lily.height = 0.05
		add_mesh(pond, lily, Vector3(lx, gy + 0.12, lz), lily_mat, "Nenuphar")
	# roseaux autour
	for i in 12:
		var a := rng.randf_range(0.0, TAU)
		var r := rng.randf_range(6.2, 7.4)
		var rx := px + cos(a) * r
		var rz := pz + sin(a) * r
		var gy2 := ground_height(rx, rz)
		var reed := CylinderMesh.new()
		reed.top_radius = 0.03
		reed.bottom_radius = 0.045
		reed.height = rng.randf_range(0.7, 1.3)
		var rmi := add_mesh(pond, reed, Vector3(rx, gy2 + reed.height / 2.0, rz), reed_mat, "Roseau")
		rmi.rotation.z = rng.randf_range(-0.15, 0.15)
		rmi.rotation.x = rng.randf_range(-0.15, 0.15)


# ============================================================
# Animaux du pâturage
# ============================================================
func _spawn_animals() -> void:
	var animals := Node3D.new()
	animals.name = "Animaux"
	add_child(animals)
	var kinds := ["poule", "poule", "poule", "vache", "mouton", "mouton"]
	var rng := RandomNumberGenerator.new()
	rng.seed = 23
	for i in kinds.size():
		var animal := Animal.new()
		animal.name = "Animal_%s_%d" % [kinds[i], i]
		animal.kind = kinds[i]
		animal.collision_layer = 4    # layer « animaux »
		animal.collision_mask = 1 | 2  # monde + joueur
		var a := rng.randf_range(0.0, TAU)
		var r := rng.randf_range(1.0, 6.0)
		var wx := 15.0 + cos(a) * r
		var wz := 10.0 + sin(a) * r
		animal.position = Vector3(wx, ground_height(wx, wz), wz)
		animal.wander_radius = 5.0
		animal.move_speed = 1.4 if kinds[i] == "poule" else 0.9
		# le modèle DOIT exister avant l'ajout à l'arbre (résolution @onready)
		var model := Node3D.new()
		model.name = "Model"
		animal.add_child(model)
		# collision : petite capsule
		var col := CollisionShape3D.new()
		var shape := CapsuleShape3D.new()
		shape.radius = 0.3
		shape.height = 0.7
		col.shape = shape
		col.position = Vector3(0, 0.35, 0)
		animal.add_child(col)
		animals.add_child(animal)


# ============================================================
# Papillons (suivent des trajectoires sinusoïdales)
# ============================================================
func _spawn_butterflies() -> void:
	var butterflies := Node3D.new()
	butterflies.name = "Papillons"
	add_child(butterflies)
	var wing_mat_a := toon(Color("#F28A3C"))
	var wing_mat_b := toon(Color("#B58AE8"))
	var rng := RandomNumberGenerator.new()
	rng.seed = 31
	for i in 6:
		var bf := Node3D.new()
		bf.name = "Papillon"
		bf.position = Vector3(rng.randf_range(-20.0, 30.0), rng.randf_range(0.8, 2.0), rng.randf_range(-25.0, 20.0))
		bf.set_meta("phase", rng.randf_range(0.0, TAU))
		bf.set_meta("center", bf.position)
		bf.set_meta("color", wing_mat_a if i % 2 == 0 else wing_mat_b)
		butterflies.add_child(bf)
		var wing := BoxMesh.new()
		wing.size = Vector3(0.14, 0.02, 0.1)
		var wl := add_mesh(bf, wing, Vector3(-0.07, 0, 0), bf.get_meta("color"), "AileG")
		var wr := add_mesh(bf, wing, Vector3(0.07, 0, 0), bf.get_meta("color"), "AileD")
		bf.set_meta("wing_l", wl)
		bf.set_meta("wing_r", wr)
		bf.add_to_group("butterfly")


# ============================================================
# Oiseaux (volent en cercle)
# ============================================================
func _spawn_birds() -> void:
	var birds := Node3D.new()
	birds.name = "Oiseaux"
	add_child(birds)
	var wing_mat := toon(Color("#3A3A44"))
	for i in 3:
		var bird := Node3D.new()
		bird.name = "Oiseau"
		bird.position = Vector3(0, 12.0 + float(i) * 2.0, 0)
		bird.set_meta("phase", float(i) * TAU / 3.0)
		birds.add_child(bird)
		var wing := BoxMesh.new()
		wing.size = Vector3(0.22, 0.02, 0.06)
		add_mesh(bird, wing, Vector3(-0.11, 0, 0), wing_mat, "AileG")
		add_mesh(bird, wing, Vector3(0.11, 0, 0), wing_mat, "AileD")
		bird.add_to_group("bird")


# ============================================================
# Particules : feuilles mortes + lucioles
# ============================================================
func _spawn_leaves_particles() -> void:
	var leaves := GPUParticles3D.new()
	leaves.name = "FeuillesMortes"
	leaves.amount = 80
	leaves.lifetime = 7.0
	leaves.preprocess = 2.0
	leaves.emitting = true
	leaves.position = Vector3(-10, 6, -10)
	leaves.visibility_aabb = AABB(Vector3(-40, -10, -40), Vector3(80, 20, 80))
	var mat := ParticleProcessMaterial.new()
	mat.emission_shape = ParticleProcessMaterial.EMISSION_SHAPE_BOX
	mat.emission_box_extents = Vector3(35, 8, 35)
	mat.direction = Vector3(0.2, -1, 0.1)
	mat.spread = 20.0
	mat.initial_velocity_min = 0.4
	mat.initial_velocity_max = 1.2
	mat.gravity = Vector3(0, -0.6, 0)
	mat.angular_velocity_min = -90.0
	mat.angular_velocity_max = 90.0
	mat.scale_min = 0.6
	mat.scale_max = 1.2
	var gradient := Gradient.new()
	gradient.add_point(0.0, Color("#7BAF5A"))
	gradient.add_point(0.5, Color("#D9A441"))
	gradient.add_point(1.0, Color("#A8642A"))
	var grad_tex := GradientTexture1D.new()
	grad_tex.gradient = gradient
	mat.color_ramp = grad_tex
	leaves.process_material = mat
	# maillage d'une feuille
	var quad := QuadMesh.new()
	quad.size = Vector2(0.09, 0.09)
	var leaf_shader := Shader.new()
	leaf_shader.code = """
shader_type spatial;
render_mode diffuse_toon, cull_disabled;
void vertex() {
	// oriente la feuille pour qu'elle flotte à plat
	NORMAL = vec3(0.0, 1.0, 0.0);
}
void fragment() {
	// COLOR vient du dégradé de la particule
	ALBEDO = COLOR.rgb;
	ROUGHNESS = 0.9;
}
"""
	var leaf_mat := ShaderMaterial.new()
	leaf_mat.shader = leaf_shader
	quad.material = leaf_mat
	leaves.draw_passes = 1
	leaves.draw_pass_1 = quad
	add_child(leaves)


func _spawn_fireflies() -> void:
	var flies := GPUParticles3D.new()
	flies.name = "Lucioles"
	flies.add_to_group("fireflies")
	flies.amount = 60
	flies.lifetime = 4.0
	flies.preprocess = 1.0
	flies.emitting = false # allumées la nuit par le TimeSystem
	flies.position = Vector3(24, 1.0, -18)
	flies.visibility_aabb = AABB(Vector3(-15, -2, -15), Vector3(30, 5, 30))
	var mat := ParticleProcessMaterial.new()
	mat.emission_shape = ParticleProcessMaterial.EMISSION_SHAPE_BOX
	mat.emission_box_extents = Vector3(12, 2.5, 12)
	mat.direction = Vector3(0, 0.3, 0)
	mat.spread = 180.0
	mat.initial_velocity_min = 0.1
	mat.initial_velocity_max = 0.4
	mat.gravity = Vector3(0, 0, 0)
	mat.scale_min = 0.5
	mat.scale_max = 1.0
	flies.process_material = mat
	var orb := SphereMesh.new()
	orb.radius = 0.045
	var orb_shader := Shader.new()
	orb_shader.code = """
shader_type spatial;
render_mode unshaded, cull_disabled;
void fragment() {
	float blink = 0.55 + 0.45 * sin(TIME * 4.0 + UV.x * 40.0);
	ALBEDO = vec3(1.0, 0.95, 0.4) * blink;
	EMISSION = vec3(1.0, 0.9, 0.35) * blink * 2.5;
	ROUGHNESS = 1.0;
}
"""
	var orb_mat := ShaderMaterial.new()
	orb_mat.shader = orb_shader
	orb.material = orb_mat
	flies.draw_passes = 1
	flies.draw_pass_1 = orb
	add_child(flies)
