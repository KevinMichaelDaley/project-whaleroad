in vec3 col;
in vec2 coord;
in vec4 v,vpos;
in vec4 p;

uniform sampler2D shadowMap[16];
uniform float SHADOW_CASCADES;
uniform mat4x4 light_view;
uniform sampler2D atlas;
uniform vec3 fog_color, sun_color;
layout(location=0) out vec4 frag_color0;
vec3 applyFog( vec3  rgb,      // original color of the pixel
               float dist)  // camera to point vector
{
    float c=0.7;
    return mix(fog_color, rgb, 1.0-pow(dist,32.0));
}
void main(){
                vec4 ShadowCoordPostW = p / p.w;
                float zmax=1.0;
                float zs=zmax/SHADOW_CASCADES;
                ShadowCoordPostW = ShadowCoordPostW * 0.5 + 0.5; 
                float z=ShadowCoordPostW.z;
                float z2=zmax;
                const float bias=1.0; 
                float shadow=1.0;
                z2=texture(shadowMap[0],ShadowCoordPostW.xy).r/SHADOW_CASCADES;
                shadow=min(shadow,float(z*bias<=z2));
                for(int i=1; i<SHADOW_CASCADES; ++i){
                    if(z>=zs*i){
                        z2=texture(shadowMap[i],ShadowCoordPostW.xy).r/SHADOW_CASCADES+zs*i;
                        shadow=min(shadow,float(z*bias<=z2));
                    }
                }   
	frag_color0=vec4(applyFog(texture(atlas,coord.xy).rgb*(mix( vec3(0.07,0.06,0.08),sun_color,shadow)+col), vpos.z/vpos.w*0.5+0.5),1.0);
	//frag_color0=vec4(col.r/2,col.r/2,col.r/2,1.0);
	//frag_color0=vec4(shadow,shadow,shadow,1.0);
}
