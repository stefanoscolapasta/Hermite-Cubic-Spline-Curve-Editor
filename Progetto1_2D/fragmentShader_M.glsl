#version 330 core
in vec4 ourColor;
out vec4 FragColor;  
 
uniform int sceltaFS;
uniform float time;
uniform vec2 res;
uniform vec2 mouse;
const float cutoff = 2048.0;
vec2 c = (mouse - 0.5) * 2.5;
float cx = 0.25 - c.x;
float cy = -c.y;
float ca = sqrt(cx * cx + cy * cy);
float zoom = exp(cos(time * 0.2) * 4.0 - 2.0);
vec2 zoomTo = vec2(-0.5 - sign(cy) * sqrt((ca + cx) * 0.5), -sign(cy) * sign(cy) * sqrt((ca - cx) * 0.5));
#define PI 3.1415
#define BALLS 50
#define MINSIZE .05
#define MAXSIZE .065

vec3 hsv2rgb(vec3 c)
{
	vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
	vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
	return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

float rand(vec2 co) {
	return fract(sin(dot(co, vec2(12.9898, 78.233))) * 43758.5453);
}

float invLerp(float a, float b, float t) {
	return (t - a) / (b - a);
}

float circle(vec2 c, float r, vec2 p) {
	
	if (length(c-p) < r) { return 1.0; }
	return 0.0;
}

float m(vec2 p) {
	p *= 16.;
	return mod(floor(p.x) + floor(p.y), 2.);
}

float round(float a) {
	if (a < 0.5)
		return 0.;
	return 1.;
}

vec3 frctl(vec2 a) {
	a = a * 6.;
	a = vec2(a.x - 3.0, a.y - 1.6);

	a = a * zoom + zoomTo;

	vec2 z = a;

	float r = 1.0, g = 1.0, b = 1.0, g2 = 1.0, b2 = 1.0;
	float i2 = 512.0;
	for (float i = 0.0; i < 512.0; i++) {
		vec2 zold = z;
		z = vec2(z.x * z.x - z.y * z.y, z.y * z.x + z.x * z.y);
		z = vec2(z.x + c.x, z.y + c.y);
		if ((z.x * z.x + z.y * z.y) > cutoff * cutoff) {
			i2 = i;
			r = i / 256.0;
			g = z.x;
			b = z.y;
			g2 = zold.x;
			b2 = zold.y;
			break;
		}
	}
	r = pow(r, 0.5);
	float q1 = log(log(g * g + b * b) / (2.0 * log(cutoff))) / log(2.0);
	float d1 = sqrt(-sin(time * 5.0 + atan(b, g)) * 0.5 + 0.5);
	float q2 = log(log(g2 * g2 + b2 * b2) / (2.0 * log(cutoff))) / log(2.0);
	float d2 = sqrt(-sin(time * 5.0 + atan(b2, g2)) * 0.5 + 0.5);
	float blend = -q2 / (q1 - q2);
	blend = ((6.0 * blend - 15.0) * blend + 10.0) * blend * blend * blend;
	float e1 = pow(blend, 2.0) * d1;
	float e2 = pow(1.0 - blend, 2.0) * d2;
	float d = max(e1, e2);

	return vec3(d, d * d * d * d, r);
}

void main()
{

   if (sceltaFS==0)
    {
        FragColor = vec4(ourColor);
    }

    else if (sceltaFS==1)
    {
	   vec2 position = (gl_FragCoord.xy / res.xx);
	   vec3 color = frctl(position);

	   FragColor = vec4(color, 1.0);
    } 
    
     else if (sceltaFS==2){

	   vec2 yToXRatio = vec2(res.y / res.x, 1.0);

	   vec2 position = ((gl_FragCoord.xy / res.xy) - vec2(.5)) / yToXRatio;
	   vec2 mousePos = (mouse - vec2(.5)) / yToXRatio;

	   float val = .0;
	   vec3 col = vec3(0);

	   for (int i = 0; i < BALLS; i++) {
		   vec2 ballPosInit = (vec2(rand(vec2(i, 14.6324)), rand(vec2(23.7644, i))) - vec2(.5)) / yToXRatio;
		   vec2 ballVel = vec2(rand(vec2(i, 11.3654)), rand(vec2(14.5324, i))) * 2. - 1.;
		   float ballRad = mix(MINSIZE, MAXSIZE, rand(vec2(i, 43.5463)));
		   vec3 ballCol = hsv2rgb(vec3(rand(vec2(i, 24.4452)), 1, 1));
		   vec2 ballDisp = sin(ballVel * time) / 8.;
		   vec2 ballPos = ballPosInit + ballDisp;
		   vec2 ballToMouse = mousePos - ballPos;
		   float dispFactor = 1. / (1. + pow(2., (length(ballToMouse) - .25) * 20.));
		   //float dispFactor = 1. - 1./(pow(length(ballToMouse) / 1., 2.)+1.);
		   //float dispFactor = step(length(ballToMouse), .3);
		   ballPos += ballToMouse * dispFactor;
		   float len = length(position - ballPos);
		   float valAdd = ballRad / len;
		   val += valAdd;
		   col += ballCol * valAdd;
	   }

	   float on = step(1.0 * pow(float(BALLS), .5), val) - step(1.1 * pow(float(BALLS), .5), val);
	   col = col / val;

	   FragColor = vec4(col * on, 1.0);

	}

}