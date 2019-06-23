
layout(location=0) out vec4 fragcolor;
in vec4 gbuffer;
void main(){
    fragcolor=gbuffer;
}
