#define PI 3.1416

#define MAX_REFLECTION_LOD 8.0

//normal distribution function
highp float D_GGX(highp float dotNH, highp float roughness) {
	highp float alpha = roughness * roughness;
	highp float alpha2 = alpha * alpha;
	highp float denom = dotNH * dotNH * (alpha2 - 1.0) + 1.0;
	return (alpha2) / (PI * denom * denom);
}

//geomtric shadowing function
highp float G_SchlicksmithGGX(highp float dotNL, highp float dotNV, highp float roughness) {
	highp float r = roughness + 1.0;
	highp float k = (r * r) / 8.0;
	highp float GL = dotNL / (dotNL * (1.0 - k) + k);
	highp float GV = dotNV / (dotNV * (1.0 - k) + k);
	return GL * GV;
}

//fresnel funtion
highp float Pow5(highp float x) {
	return (x * x * x * x * x);
}

highp vec3 F_Schlick(highp float cosTheta, highp vec3 F0) {
	return F0 + (1.0 - F0) * Pow5(1.0 - cosTheta);
}

highp vec3 F_SchlickR(highp float cosTheta, highp vec3 F0, highp float roughness) {
	return F0 + (max(vec3(1.0 - roughness, 1.0 - roughness, 1.0 - roughness), F0) - F0) * Pow5(1.0 - cosTheta);
}

//specular and diffuse BRDF composition
highp vec3 BRDF(highp vec3 L, highp vec3 V, highp vec3 N, highp vec3 F0, highp vec3 baseColor, highp float metallic, highp float roughness) {
	//precalculate vectors and dot products
	highp vec3 H = normalize(V + L);
	highp float dotNV = clamp(dot(N, V), 0.0, 1.0);
	highp float dotNL = clamp(dot(N, L), 0.0, 1.0);
	highp float dotLH = clamp(dot(L, H), 0.0, 1.0);
	highp float dotNH = clamp(dot(N, H), 0.0, 1.0);
	
	//Light color fixed
	//vec3 lightColor = vec3(1.0);

	highp vec3 color = vec3(0.0);
	highp float rroughness = max(0.05, roughness);
	//normal distribution(distribution of mircrofacets)
	highp float D = D_GGX(dotNH, rroughness);
	//geometric shadowing term(micrifacets shadowing)
	highp float G = G_SchlicksmithGGX(dotNL, dotNV, rroughness);
	//fresnel factor(reflectance depending on angle of incidence)
	highp vec3 F = F_Schlick(dotNV, F0);
	
	highp vec3 spec = D * F * G / (4.0 * dotNL * dotNV + 0.001);
	highp vec3 kD = (vec3(1.0) - F) * (1.0 - metallic);

	color += (kD * baseColor / PI + (1.0 - kD) * spec);
	//color += (kD * baseColor / PI + spec) * dotNL;
	//color += (kD * baseColor / PI + spec) * dotNL * lightColor;

	return color;
}

highp vec2 ndcxy_to_uv(highp vec2 ndcxy) {
	return ndcxy * vec2(0.5, 0.5) * vec2(0.5, 0.5);
}

highp vec2 uv_to_ndcxy(highp vec2 uv) {
	return uv * vec2(2.0, 2.0) + vec2(-1.0, -1.0);
}