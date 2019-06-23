
layout(location=0) out vec4 fragcolor;
in vec4 gbuffer;
in float w;
void main(){
    fragcolor=gbuffer/vec4(1,1,w,1);
}
