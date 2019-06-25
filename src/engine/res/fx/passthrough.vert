layout(location=0) in vec3 p;
uniform mat4 transform;
uniform int x0;
uniform int y0;
uniform int z0;
void main(){
    gl_Position=transform*vec4(p+vec3(float(x0),float(y0),float(z0)),1.0);
}
