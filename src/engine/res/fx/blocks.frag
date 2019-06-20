in vec3 col;
in vec2 coord;
in vec4 v;
in vec4 p;
in float shadow;

uniform mat4x4 light_view;
uniform sampler2D atlas;
uniform vec3 fog_color;
layout(location=0) out vec4 frag_color0;
vec3 applyFog( vec3  rgb,      // original color of the pixel
               float dist, // camera to point distance
               vec3  rayOri,   // camera position
               vec3  rayDir, float col )  // camera to point vector
{
    float c=0.7;
    float b=0.2;
    float fogAmount = c * exp(-rayOri.y*b) * (1.0-exp( -dist*rayDir.y*b ))/rayDir.y;
    
    return mix( rgb, fog_color, 0.7 );
}
void main(){
	frag_color0=vec4(applyFog(texture(atlas,coord.xy).rgb*(mix( vec3(0.15,0.16,0.18),vec3(1.0,0.9,0.7),shadow)+0.3*col), length(v), vec3(0,0,p.z+v.z), normalize(v.xyz), 0.0 ),1.0);
	//frag_color0=vec4(col.r/2,col.r/2,col.r/2,1.0);
}
