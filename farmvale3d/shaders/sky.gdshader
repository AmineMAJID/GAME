// ============================================================
// FarmVale — ciel stylisé (shader_type sky)
// Dégradé jour/nuit piloté par le TimeSystem : soleil, lune, étoiles.
// ============================================================
shader_type sky;

uniform vec4 sky_top : source_color = vec4(0.29, 0.56, 0.85, 1.0);
uniform vec4 sky_horizon : source_color = vec4(0.75, 0.89, 0.95, 1.0);
uniform vec4 ground_tint : source_color = vec4(0.30, 0.42, 0.28, 1.0);
uniform vec4 sun_color : source_color = vec4(1.0, 0.95, 0.8, 1.0);
uniform vec3 sun_direction = vec3(0.0, -1.0, 0.0);
uniform float sun_size : hint_range(0.005, 0.2) = 0.035;
uniform float sun_glow : hint_range(0.0, 6.0) = 1.6;
uniform vec4 moon_color : source_color = vec4(0.92, 0.95, 1.0, 1.0);
uniform vec3 moon_direction = vec3(0.0, 1.0, 0.0);
uniform float star_density : hint_range(0.0, 1.0) = 0.0;
uniform float exposure : hint_range(0.2, 2.0) = 1.0;

float hash21(vec2 p) {
	return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453123);
}

void sky() {
	vec3 dir = normalize(EYEDIR);
	float up = clamp(dir.y, -1.0, 1.0);

	// --- Dégradé vertical du ciel
	vec3 col = mix(sky_horizon.rgb, sky_top.rgb, pow(max(up, 0.0), 0.55));

	// --- Halo autour de l'horizon (lumière chaude)
	col += sky_horizon.rgb * pow(1.0 - abs(up), 4.0) * 0.35;

	// --- Soleil (disque + halo)
	float sun_amount = max(dot(dir, normalize(sun_direction)), 0.0);
	float disc = smoothstep(1.0 - sun_size, 1.0 - sun_size * 0.55, sun_amount);
	float halo = exp(-acos(clamp(sun_amount, 0.0, 1.0)) * 5.0) * sun_glow * 0.35;
	float sun_visibility = clamp(-sun_direction.y + 0.15, 0.0, 1.0);
	col += sun_color.rgb * (disc * 2.2 + halo) * sun_visibility;

	// --- Lune
	float moon_amount = max(dot(dir, normalize(moon_direction)), 0.0);
	float moon_disc = smoothstep(1.0 - 0.028, 1.0 - 0.017, moon_amount);
	float moon_visibility = clamp(sun_direction.y + 0.1, 0.0, 1.0);
	col += moon_color.rgb * moon_disc * moon_visibility * 0.9;
	// halo lunaire
	col += moon_color.rgb * exp(-acos(clamp(moon_amount, 0.0, 1.0)) * 8.0) * 0.12 * moon_visibility;

	// --- Étoiles (nuit uniquement)
	if (star_density > 0.01 && up > 0.0) {
		vec2 suv = dir.xz / max(up, 0.08) * 3.0;
		float star = step(0.9965, hash21(floor(suv * 140.0)));
		star *= step(0.5, hash21(floor(suv * 140.0) + 7.3));
		col += vec3(star) * star_density * smoothstep(0.0, 0.35, up) * 0.9;
	}

	// --- Sol sous l'horizon
	if (up < 0.0) {
		col = mix(ground_tint.rgb, col, smoothstep(-0.08, 0.0, up));
	}

	COLOR = col * exposure;
}
