
in vec3 col;
in vec2 coord;
in vec2 zw;
in vec4 p;

uniform sampler2D atlas;
uniform sampler2D shadowmap0;
uniform sampler2D shadowmap1;
uniform sampler2D shadowmap2;
uniform sampler2D shadowmap3;
uniform sampler2D shadowmap4;
uniform sampler2D shadowmap5;
uniform sampler2D shadowmap6;
uniform sampler2D shadowmap7;
uniform vec3 fog_color;
layout(location=0) out vec4 frag_color0;
void main(){
    vec4 ShadowCoordPostW = p / p.w;
    float zmax=1.0;
    float zs=zmax/8.0;
    ShadowCoordPostW = ShadowCoordPostW * 0.5 + 0.5; 
    float z=ShadowCoordPostW.z;
    float z2=zmax;
    if(z<zs){
        z2=texture(shadowmap0,ShadowCoordPostW.xy).r/8.0;
    }
    
    else if(z<zs*2.0){
        z2=texture(shadowmap1,ShadowCoordPostW.xy).r/8.0+zs;
    }
    
    else if(z<zs*3.0){
        z2=texture(shadowmap2,ShadowCoordPostW.xy).r/8.0+zs*2.0;
    }
    
    else if(z<zs*4.0){
        z2=texture(shadowmap3,ShadowCoordPostW.xy).r/8.0+zs*3.0;
    }
    
    else if(z<zs*5.0){
        z2=texture(shadowmap4,ShadowCoordPostW.xy).r/8.0+zs*4.0;
    }
    
    else if(z<zs*6.0){
        z2=texture(shadowmap5,ShadowCoordPostW.xy).r/8.0+zs*5.0;
    }
    
    else if(z<zs*7.0){
        z2=texture(shadowmap6,ShadowCoordPostW.xy).r/8.0+zs*6.0;
    }
    
    else if(z<zs*8.0){
        z2=texture(shadowmap7,ShadowCoordPostW.xy).r/8.0+zs*7.0;
    }
    float shadow = float(z2>0.9999*ShadowCoordPostW.z);
	frag_color0=vec4(mix(texture(atlas,coord.xy).rgb*(shadow+col+0.15)*0.5,fog_color,max(0,zw.x/zw.y*0.5+0.5)*0.25), 1.0);
	//frag_color0=vec4(shadow,shadow,shadow,1.0);
}
