layout(location=0) in vec2 pos;
layout(location=1) in vec2 tc;
out vec2 uv;

uniform vec4 frustum_corner0, frustum_corner1, frustum_corner2, frustum_corner3;
out vec4 p1,p2;
uniform mat4 transform;
void main(){
    p1=transform*vec4(pos,-1.0,0.0);
    p2=transform*vec4(pos,1.0,0.0);
    gl_Position = vec4(pos,0,1);
    uv=tc;
}
