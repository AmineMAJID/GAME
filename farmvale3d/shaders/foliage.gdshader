// ============================================================
// FarmVale — feuillage (toon + vent + translucidité)
// S'inspire du tutoriel officiel « Making trees » de Godot.
// ============================================================
shader_type spatial;
render_mode diffuse_toon, specular_toon, cull_disabled, world_vertex_coords;

uniform vec4 albedo : source_color = vec4(0.32, 0.62, 0.30, 1.0);
uniform float sway_speed = 1.3;
uniform float sway_strength = 0.055;
uniform vec4 transmission : source_color = vec4(0.45, 0.75, 0.35, 1.0);

void vertex() {
	// le haut de la branche bouge plus que le bas
	float strength = 0.35 + 0.65 * sin(VERTEX.y * 2.5 + 1.7);
	float t = TIME * sway_speed;
	VERTEX.x += sin(VERTEX.z * 1.7 + t) * sway_strength * strength;
	VERTEX.z += sin(VERTEX.x * 1.3 + t * 1.21) * sway_strength * strength;
	VERTEX.y += sin(VERTEX.x * 0.9 + t * 0.8) * sway_strength * 0.4 * strength;
}

void fragment() {
	ALBEDO = albedo.rgb;
	ROUGHNESS = 0.95;
	METALLIC = 0.0;
	SPECULAR = 0.12;
	RIM = 0.4;
	RIM_TINT = 0.3;
	// lumière qui traverse les feuilles (subsurface scattering)
	SSS_TRANSMITTANCE_COLOR = transmission;  // vec4 (couleur × alpha)
}
