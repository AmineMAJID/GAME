// ============================================================
// FarmVale — eau stylisée (vagues + fresnel + reflets de soleil)
// ============================================================
shader_type spatial;
render_mode cull_disabled;

uniform vec4 shallow_color : source_color = vec4(0.36, 0.68, 0.72, 0.72);
uniform vec4 deep_color : source_color = vec4(0.10, 0.32, 0.48, 0.88);
uniform vec3 sun_direction = vec3(0.0, -1.0, 0.0);
uniform float wave_speed = 0.55;
uniform float wave_scale = 1.3;
uniform float wave_height = 0.09;

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

void vertex() {
	float t = TIME * wave_speed;
	vec2 p = VERTEX.xz;
	float w = sin(p.x * wave_scale + t) * cos(p.y * wave_scale * 0.9 + t * 1.2);
	w += sin(p.x * wave_scale * 2.3 + t * 1.7) * 0.35;
	w += vnoise(p * 0.8 + t * 0.1) * 0.4;
	VERTEX.y += w * wave_height;
}

void fragment() {
	float t = TIME * wave_speed;
	vec2 p = UV * 7.0;
	// normales animées (double ondulation)
	vec3 n1 = vec3(sin(p.x * 2.0 + t), 0.55, sin(p.y * 2.3 + t * 1.3));
	vec3 n2 = vec3(sin(p.y * 1.7 - t * 0.8), 0.45, sin(p.x * 1.4 - t));
	NORMAL = normalize(mix(n1, n2, 0.5));

	// fresnel : plus transparent au centre, plus réfléchissant sur les bords
	float fres = pow(1.0 - max(dot(NORMAL, VIEW), 0.0), 3.0);
	vec3 col = mix(deep_color.rgb, shallow_color.rgb, fres * 0.65 + 0.35);
	ALBEDO = col;
	ALPHA = mix(deep_color.a, shallow_color.a, fres);
	ROUGHNESS = 0.12;
	METALLIC = 0.15;
	SPECULAR = 0.7;

	// reflets du soleil (paillettes)
	vec3 sun = normalize(sun_direction);
	float sparkle = pow(max(dot(reflect(-sun, NORMAL), VIEW), 0.0), 48.0);
	sparkle += pow(max(dot(reflect(-sun, NORMAL), VIEW), 0.0), 8.0) * 0.15;
	EMISSION = vec3(1.0, 0.96, 0.85) * sparkle * 1.4 * clamp(-sun.y, 0.0, 1.0);
}
