layout(location=0) out vec4 color;
uniform sampler2D atlas;
uniform sampler2D gbuffer;
uniform vec4 frustum_corner0, frustum_corner1;
uniform vec3 sun_color,sky_color,fog_color;
in vec2 uv;
uniform sampler2D shadowmap0, shadowmap1,shadowmap2, shadowmap3;
float get_shadow(vec4 p){
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
                return shadow;
}
void main(){
    vec4 g=texture(gbuffer,uv);
    float z=0.5/g.b+0.5;
    vec4 p=mix(frustum_corner0, frustum_corner1,vec4(uv,z,1.0));
    vec3 rgb=texture(atlas,g.rg).rgb*(g.a*sky_color+get_shadow(p)*sun_color);
    color=vec4(mix(rgb,fog_color,pow(z,8.0)),1.0);
    
}
