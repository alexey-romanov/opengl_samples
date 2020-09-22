#version 330 core

out vec4 o_frag_color;

uniform int itr;
uniform vec2 screenSize;
uniform vec2 offset;
uniform float zoom;

float n = 0.0;
float threshold = 100.0;

// copy-pasted
float mandelbrot(vec2 c) {
	vec2 z = vec2(0.0,0.0);
	for(int i = 0; i < itr; i++){
		vec2 znew;
		znew.x = (z.x * z.x) - (z.y * z.y) + c.x;
		znew.y = (2.0 * z.x * z.y) +c.y;
		z = znew;
		if((z.x * z.x) + (z.y * z.y) > threshold)break;
		n++;
	}
	return n / float(itr);
}

// copy-pasted
vec4 map_to_color(float t) {
    float r = 9.0 * (1.0 - t) * t * t * t;
    float g = 15.0 * (1.0 - t) * (1.0 - t) * t * t;
    float b = 8.5 * (1.0 - t) * (1.0 - t) * (1.0 - t) * t;

    return vec4(r, g, b, 1.0);
}

void main()
{
    vec2 coord = vec2(gl_FragCoord.xy);
    float t = mandelbrot(((coord - screenSize/2)/zoom) - offset);
    o_frag_color = map_to_color(float(t));
uniform vec3 u_color;
uniform float u_time;
uniform sampler2D u_tex;
    vec3 texture = texture(u_tex, v_out.color.xy).rgb;
    //o_frag_color = vec4(v_out.color.xy,0,1.0);

    //if ((int(gl_FragCoord.x / 30) % 2 == 0) ^^ (int(gl_FragCoord.y / 30) % 2 == 0))
    //  discard;

    o_frag_color = vec4(texture,1.0);

    //gl_FragDepth = 0;
}
