layout(location=0) in vec2 pos;
layout(location=1) in vec2 tc;
out vec2 uv;
void main(){
    gl_Position = vec4(pos,0,1);
    uv=tc;
}
