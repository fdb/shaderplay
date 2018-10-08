// Creation by Silexars
// https://www.shadertoy.com/view/XsXXDn
// http://www.pouet.net/prod.php?which=57245

#version 150

uniform vec2 u_resolution;
uniform vec2 u_mouse;
uniform float u_time;
out vec4 out_color;

#define t u_time
#define r u_resolution.xy

void main() {
    vec3 c;
    float l, z = t;
    for (int i = 0; i < 3; i++) {
        vec2 p = gl_FragCoord.xy / r;
        vec2 uv = p;
        p -= 0.5;
        p.x *= r.x / r.y;
        z += 0.7;
        l = length(p);
        uv += p / l * (sin(z) + 1.) * abs(sin(l * 9. - z * 2.));
        c[i] = 0.01 / length(abs(mod(uv, 1.0)- 0.5));

    }
    out_color = vec4(c / l, t);
}
