// ============================================================
// FarmVale — herbe stylisée (collines douces + vent dans l'herbe)
// Le déplacement des collines est reproduit en GDScript (Props)
// pour poser les props à la bonne hauteur.
// ============================================================
shader_type spatial;
render_mode diffuse_toon, specular_toon;

uniform vec4 grass_light : source_color = vec4(0.47, 0.70, 0.34, 1.0);
uniform vec4 grass_dark : source_color = vec4(0.29, 0.52, 0.23, 1.0);
uniform float hill_height = 0.55;
uniform float hill_scale = 0.016;
uniform float patch_scale = 0.12;
uniform float sway_speed = 1.1;
uniform float sway_strength = 0.035;

float hash21(vec2 p) {
	return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453123);
}

float vnoise(vec2 p) {
	vec2 i = floor(p);
	vec2 f = fract(p);
	f = f * f * (3.0 - 2.0 * f);
	float a = hash21(i);
	float b = hash21(i + vec2(1.0, 0.0));
	float c = hash21(i + vec2(0.0, 1.0));
	float d = hash21(i + vec2(1.0, 1.0));
	return mix(mix(a, b, f.x), mix(c, d, f.x), f.y);
}

// Hauteur du terrain (miroir GDScript de Props.ground_height)
float ground_height(vec2 p) {
	return vnoise(p * hill_scale) * hill_height
	     + vnoise(p * hill_scale * 4.0 + vec2(7.0, 3.0)) * hill_height * 0.3;
}

void vertex() {
	vec2 p = VERTEX.xz;
	float e = 0.4;
	float h = ground_height(p);
	float hx = ground_height(p + vec2(e, 0.0));
	float hz = ground_height(p + vec2(0.0, e));
	VERTEX.y += h;
	// normales recalculées pour un éclairage correct sur les collines
	NORMAL = normalize(vec3(h - hx, e, h - hz));
	// vent dans l'herbe
	float sway = sin(p.x * 1.2 + TIME * sway_speed) * cos(p.y * 1.05 + TIME * sway_speed * 0.85);
	VERTEX.x += sway * sway_strength;
	VERTEX.z += sway * sway_strength * 0.7;
}

void fragment() {
	// variation de couleur par « touffes » d'herbe
	float n = vnoise(UV * 300.0 * patch_scale + vec2(TIME * 0.008, 0.0));
	float n2 = vnoise(UV * 90.0 + vec2(0.0, TIME * 0.004));
	vec3 col = mix(grass_dark.rgb, grass_light.rgb, n * 0.75 + n2 * 0.25);
	ALBEDO = col;
	ROUGHNESS = 1.0;
	METALLIC = 0.0;
	SPECULAR = 0.04;
}
