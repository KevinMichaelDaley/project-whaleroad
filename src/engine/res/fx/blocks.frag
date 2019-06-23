in vec3 col;
in vec2 coord;
in vec4 v,vpos;
in vec4 p;

uniform sampler2D shadowmap0;
uniform sampler2D shadowmap1;
uniform sampler2D shadowmap2;
uniform sampler2D shadowmap3;
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
                float SHADOW_CASCADES=4.0;
                float zs=zmax/SHADOW_CASCADES;
                ShadowCoordPostW = ShadowCoordPostW * 0.5 + 0.5; 
                float z=ShadowCoordPostW.z;
                float z2=zmax;
                const float bias=0.99; 
                float shadow=1.0;
                z2=texture(shadowmap0,ShadowCoordPostW.xy).r/SHADOW_CASCADES;
                shadow=min(shadow,float(z*bias<=z2));
                if(z>zs){
                    z2=texture(shadowmap1,ShadowCoordPostW.xy).r/SHADOW_CASCADES+zs;
                    shadow=min(shadow,float(z*bias<=z2));
                }
                if(z>zs*2.0){
                    z2=texture(shadowmap2,ShadowCoordPostW.xy).r/SHADOW_CASCADES+zs*2.0;
                    shadow=min(shadow,float(z*bias<=z2));
                }
                if(z>zs*3.0){
                    z2=texture(shadowmap3,ShadowCoordPostW.xy).r/SHADOW_CASCADES+zs*3.0;
                    shadow=min(shadow,float(z*bias<=z2));
                }
	frag_color0=vec4(applyFog(texture(atlas,coord.xy).rgb*(mix( vec3(0.17,0.16,0.18),sun_color,shadow)+0.3*col), vpos.z/vpos.w*0.5+0.5),1.0);
	//frag_color0=vec4(col.r/2,col.r/2,col.r/2,1.0);
	//frag_color0=vec4(shadow,shadow,shadow,1.0);
}
