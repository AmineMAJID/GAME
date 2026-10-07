// ============================================================
// FarmVale — matériau « toon » (cel-shading) — le look stylisé
// des jeux modernes à succès. Éclairage par paliers + rim light.
// ============================================================
shader_type spatial;
render_mode diffuse_toon, specular_toon;

uniform vec4 albedo : source_color = vec4(1.0, 1.0, 1.0, 1.0);
uniform float roughness : hint_range(0.0, 1.0) = 0.9;
uniform float specular : hint_range(0.0, 1.0) = 0.25;
uniform float rim : hint_range(0.0, 1.0) = 0.45;
uniform float rim_tint : hint_range(0.0, 1.0) = 0.25;

void fragment() {
	ALBEDO = albedo.rgb;
	ROUGHNESS = roughness;
	METALLIC = 0.0;
	SPECULAR = specular;
	RIM = rim;
	RIM_TINT = rim_tint;
}
