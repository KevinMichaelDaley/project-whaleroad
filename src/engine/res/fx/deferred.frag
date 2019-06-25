layout(location=0) out vec4 color;
uniform sampler2D atlas;
uniform sampler2D gbuffer;
uniform vec3 sun_color,sky_color,fog_color;
in vec2 uv;

void main(){
    
    vec4 g=texture(gbuffer,uv);
    float z=g.b*0.5+0.5;
    if(z>0.9999) color=vec4(sky_color,1.0);
    else{
    vec3 rgb=texture(atlas,g.rg).rgb*(mix(sky_color*0.05,sun_color,g.a));
    color=vec4(mix(rgb,fog_color,pow(z,156.0)),1.0);
    }
}
