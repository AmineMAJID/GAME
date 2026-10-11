extends Node
## ============================================================
## FarmVale — test automatique headless du projet.
## Lancé par : godot --headless --path farmvale3d res://scenes/tools/self_test.tscn
## Vérifie : input map, menu, monde 3D, props, mouvement, temps,
## HUD, sauvegarde, transition de scène. Quitte avec un code 0/1.
## ============================================================

var _checks: int = 0
var _failures: int = 0


func _ready() -> void:
	print("=== FarmVale — self-test headless ===")
	# la racine est encore en cours de setup : on attend une frame avant d'ajouter des scènes
	await get_tree().process_frame
	await _run_tests()
	print("=== Résultat : %d vérifications, %d échec(s) ===" % [_checks, _failures])
	get_tree().quit(1 if _failures > 0 else 0)


func _check(label: String, condition: bool, detail: String = "") -> void:
	_checks += 1
	if condition:
		print("  [OK] %s" % label)
	else:
		_failures += 1
		print("  [ECHEC] %s %s" % [label, detail])


func _run_tests() -> void:
	# --- 1. Input map (définie dans project.godot)
	for action in ["move_forward", "move_backward", "move_left", "move_right", "sprint",
				"interact", "hotbar_1", "hotbar_9", "camera_zoom_in", "camera_zoom_out"]:
		_check("input map : action « %s »" % action, InputMap.has_action(action))

	# --- 2. Menu principal
	var menu_scene := load("res://scenes/ui/main_menu.tscn") as PackedScene
	_check("menu : scène chargeable", menu_scene != null)
	var menu := menu_scene.instantiate()
	get_tree().root.add_child(menu)
	await get_tree().process_frame
	var titre := menu.get_node_or_null("%Titre") as Label
	_check("menu : titre « FarmVale »", titre != null and titre.text == "FarmVale")
	var btn_new := menu.get_node_or_null("%NouvellePartie") as Button
	var btn_continue := menu.get_node_or_null("%Continuer") as Button
	var btn_options := menu.get_node_or_null("%Options") as Button
	var btn_quit := menu.get_node_or_null("%Quitter") as Button
	_check("menu : 4 boutons présents", btn_new != null and btn_continue != null and btn_options != null and btn_quit != null)
	_check("menu : « Continuer » désactivé sans sauvegarde", btn_continue.disabled == (not GameManager.has_save()))
	var options_panel := menu.get_node_or_null("%OptionsPanel") as PanelContainer
	_check("menu : panneau options caché au départ", options_panel != null and not options_panel.visible)
	# panneau options fonctionnel
	btn_options.pressed.emit()
	await get_tree().process_frame
	_check("menu : panneau options s'ouvre", options_panel.visible)
	GameManager.set_fullscreen(false)
	_check("menu : réglage plein écran appliqué", GameManager.fullscreen == false)
	menu.queue_free()
	await get_tree().process_frame

	# --- 3. Monde 3D
	var world_scene := load("res://scenes/world/world.tscn") as PackedScene
	_check("monde : scène chargeable", world_scene != null)
	var world := world_scene.instantiate()
	get_tree().root.add_child(world)
	await get_tree().process_frame
	await get_tree().process_frame

	var sun := world.get_node_or_null("SunPivot/Sun") as DirectionalLight3D
	_check("monde : soleil (DirectionalLight3D)", sun != null and sun.shadow_enabled)
	var env_node := world.get_node_or_null("WorldEnvironment") as WorldEnvironment
	var sky_res := env_node.environment.sky if (env_node != null and env_node.environment != null) else null
	_check("monde : WorldEnvironment avec ciel", sky_res != null)
	var ground := world.get_node_or_null("Ground") as MeshInstance3D
	_check("monde : sol (PlaneMesh + shader herbe)", ground != null and ground.mesh is PlaneMesh)
	var pond := world.get_node_or_null("Pond") as MeshInstance3D
	_check("monde : étang (eau shader)", pond != null and pond.material_override is ShaderMaterial)
	var player := world.get_node_or_null("Player") as Player
	_check("monde : joueur (CharacterBody3D)", player != null)
	var cam_rig := world.get_node_or_null("CameraRig") as CameraRig
	_check("monde : caméra 3e personne", cam_rig != null and cam_rig.get_node_or_null("Yaw/Pitch/SpringArm/Camera") is Camera3D)
	var hud := world.get_node_or_null("HUD") as Control
	_check("monde : HUD présent", hud != null)
	var time_sys := world.get_node_or_null("TimeSystem") as TimeSystem
	_check("monde : système de temps", time_sys != null)

	# props générés
	var props_node := world.get_node_or_null("Props") as Props
	_check("monde : générateur de props", props_node != null)
	var parcelle_count := 0
	var culture_count := 0
	var arbre_count := 0
	var poteau_count := 0
	if props_node:
		for child in props_node.get_children():
			if child.name == "Champs":
				for c in child.get_children():
					if c.name.begins_with("Parcelle"):
						parcelle_count += 1
					elif c.name.begins_with("Culture"):
						culture_count += 1
			elif child.name == "Arbres":
				arbre_count += child.get_child_count()
			elif child.name == "Cloture":
				for c in child.get_children():
					if c.name.begins_with("Poteau"):
						poteau_count += 1
	var animals := 0
	if props_node:
		for child in props_node.get_children():
			if child.name == "Animaux":
				animals += child.get_child_count()
	_check("monde : 24 parcelles labourées", parcelle_count == 24, "trouvé=%d" % parcelle_count)
	_check("monde : 24 cultures 3D", culture_count == 24, "trouvé=%d" % culture_count)
	_check("monde : 12 arbres", arbre_count == 12, "trouvé=%d" % arbre_count)
	_check("monde : clôture (poteaux)", poteau_count >= 24, "trouvé=%d" % poteau_count)
	_check("monde : 6 animaux", animals == 6, "trouvé=%d" % animals)
	var fleurs := world.get_node_or_null("Props/Fleurs") as MultiMeshInstance3D
	_check("monde : 150 fleurs (MultiMesh)", fleurs != null and fleurs.multimesh.instance_count == 150)

	# --- 4. Mouvement du joueur (input simulé)
	if player:
		var start_pos := player.global_position
		Input.action_press("move_forward")
		for i in 30:
			await get_tree().physics_frame
		Input.action_release("move_forward")
		var moved := player.global_position.distance_to(start_pos)
		_check("joueur : se déplace avec Z/W", moved > 0.5, "distance=%.2f" % moved)

		# le joueur tient sur le sol (collision du terrain — régression « chute à travers »)
		for i in 90:
			await get_tree().physics_frame
		_check("joueur : reste sur le sol (collision)",
			player.is_on_floor() and player.position.y > -1.0,
			"y=%.2f on_floor=%s" % [player.position.y, player.is_on_floor()])
		var sol_col := world.get_node_or_null("SolCollision")
		_check("monde : collision du sol (StaticBody3D)", sol_col != null)
		var animaux_sol := true
		var animaux := world.get_node_or_null("Props/Animaux")
		if animaux:
			for a in animaux.get_children():
				if a.position.y < -1.0:
					animaux_sol = false
		_check("animaux : restent sur le sol", animaux_sol)
		var maison_col := world.get_node_or_null("Props/Maison/Collision")
		_check("monde : collision de la maison", maison_col != null)

	# --- 5. Système de temps (cycle jour/nuit)
	if time_sys:
		var sun_rot_before := sun.rotation_degrees.x if sun else 0.0
		time_sys.advance_minutes(120) # +2 heures
		var sky_mat := sky_res.sky_material as ShaderMaterial if sky_res != null else null
		_check("temps : avance de 2h", time_sys.hour() == 10, "heure=%d" % time_sys.hour())
		_check("temps : soleil a tourné", sun != null and not is_equal_approx(sun.global_rotation_degrees.x, sun_rot_before))
		_check("temps : ciel shader actif", sky_mat != null and sky_mat.shader != null)
		# nuit : étoiles + fenêtres allumées
		time_sys.advance_minutes(14 * 60) # → 00:00 (minuit)
		_check("temps : minuit atteint", time_sys.hour() == 0, "heure=%d" % time_sys.hour())
		_check("temps : étoiles la nuit", sky_mat.get_shader_parameter("star_density") > 0.5)
		_check("temps : soleil quasi éteint la nuit", sun.light_energy < 0.2, "énergie=%.2f" % sun.light_energy)
		# retour au jour (midi)
		time_sys.advance_minutes(12 * 60) # → 12h
		_check("temps : midi (énergie soleil)", sun.light_energy > 0.8, "énergie=%.2f" % sun.light_energy)
		_check("temps : pas d'étoiles le jour", sky_mat.get_shader_parameter("star_density") < 0.1)
		# le disque de soleil doit apparaître DANS le ciel (uniform = vers le soleil)
		var sun_dir: Vector3 = sky_mat.get_shader_parameter("sun_direction")
		_check("temps : soleil vers le haut à midi", sun_dir.y > 0.9, "sun_dir=%s" % sun_dir)
		var moon_dir: Vector3 = sky_mat.get_shader_parameter("moon_direction")
		_check("temps : lune opposée au soleil", moon_dir.y < -0.9, "moon_dir=%s" % moon_dir)

	# --- 6. HUD
	if hud:
		var hud_script := hud.get_script() as GDScript
		hud.set_money(1234)
		var money_label := hud.get_node_or_null("%MoneyLabel") as Label
		_check("hud : monnaie affichée", money_label != null and money_label.text == "1234")
		hud.call("set_time", 9, 30, 5, "Été", "Vendredi")
		var season_label := hud.get_node_or_null("%SeasonLabel") as Label
		var time_label := hud.get_node_or_null("%TimeLabel") as Label
		_check("hud : saison/jour", season_label.text == "Été — Jour 5")
		_check("hud : heure", time_label.text == "Vendredi · 09:30")
		hud.call("set_weather", "pluie")
		var weather_label := hud.get_node_or_null("%WeatherLabel") as Label
		_check("hud : météo", weather_label.text == "Pluie")
		hud.call("show_toast", "Test toast", 0.2)
		await get_tree().process_frame
		var toasts := hud.get_node_or_null("%ToastContainer") as VBoxContainer
		_check("hud : toast créé", toasts.get_child_count() >= 1)
		# pause
		hud.call("toggle_pause")
		_check("hud : pause active", get_tree().paused)
		hud.call("toggle_pause")
		_check("hud : reprise", not get_tree().paused)

	# --- 7. Sauvegarde / chargement
	GameManager.money = 777
	GameManager.save_game({"day": 3})  # save_game écrit toujours la monnaie courante
	var loaded := GameManager.load_game()
	_check("sauvegarde : aller-retour JSON", int(loaded.get("money", 0)) == 777 and int(loaded.get("day", 0)) == 3)
	_check("sauvegarde : « Continuer » disponible", GameManager.has_save())

	# --- 8. Génération procédurale déterministe (terrain)
	_check("terrain : hauteur cohérente", abs(Props.ground_height(0.0, 0.0) - Props.ground_height(0.0, 0.0)) < 0.0001)
	var h1 := Props.ground_height(10.0, 10.0)
	var h2 := Props.ground_height(-30.0, 22.0)
	_check("terrain : collines douces", h1 >= -0.2 and h1 <= 0.75 and h2 >= -0.2 and h2 <= 0.75, "h1=%.2f h2=%.2f" % [h1, h2])

	# --- 9. Transition menu → monde via GameManager
	# (vérifiée sans attendre le changement effectif : ce nœud serait libéré)
	GameManager.money = 500
	GameManager.new_game()
	_check("transition : nouvelle partie cible le monde",
		GameManager.current_scene_path == "res://scenes/world/world.tscn",
		"chemin=%s" % GameManager.current_scene_path)
