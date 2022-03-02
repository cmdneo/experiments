precision highp float;

#define k 100.
#define eps 10e-2
#define MAX_ITERS 100

vec3 light = normalize(vec3(0., -2., 1.));


float dist(vec3 dir) {
    vec3 sphere = vec3(0., 25. * sin(5. * iTime), 200.);
    float R = 130.;
    vec3 ray = vec3(0.);
    float d = 1000.;

    for (int i = 0; i < MAX_ITERS; i++) {
        d = distance(sphere, ray) - R;
        ray = ray + d * dir;
        if (d < eps)
            return -dot(light, normalize(ray - sphere));
    }
    
    return 0.0;
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = fragCoord - iResolution.xy / 2.;
    vec3 raydir = normalize(vec3(uv, k + 50. * sin(iTime)));
    
    float n = dist(raydir);
    

    vec3 col = vec3(n);
    // Output to screen
    fragColor = vec4(col,1.0);
}
