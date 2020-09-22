#version 330 core

out vec4 o_frag_color;

uniform int itr;
uniform vec2 screenSize;
uniform vec2 offset;
uniform float zoom;
uniform sampler2D u_tex;

float threshold = 100.0;

// copy-pasted
float mandelbrot(vec2 c) {
    float n = 0.0;
	vec2 z = vec2(0.0,0.0);
	for(int i = 0; i < itr; i++){
		vec2 znew;
		znew.x = (z.x * z.x) - (z.y * z.y) + c.x;
		znew.y = (2.0 * z.x * z.y) +c.y;
		z = znew;
		if((z.x * z.x) + (z.y * z.y) > threshold)break;
		n++;
	}
	return n;
}

// copy-pasted
vec4 map_to_color(float n) {
    vec3 textur = texture(u_tex, vec2(n, n)).rgb;
    return vec4(textur.r, textur.g, textur.b, 1.0);
}

void main()
{
    vec2 coord = vec2(gl_FragCoord.xy);
    float n = mandelbrot(((coord - screenSize/2)/zoom) - offset);
    o_frag_color = map_to_color(n);
}
