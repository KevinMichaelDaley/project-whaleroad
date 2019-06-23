layout(location=0) in vec2 pos;
layout(location=1) in vec2 tc;
void main(){
    gl_Position = vec4(pos,1,1);
}
