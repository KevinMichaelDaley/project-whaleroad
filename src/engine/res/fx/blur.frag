uniform sampler2D tex;
in vec3 uv;
layout(location=0) out vec3 blurred;

const float pi = 3.1415926;
const int samples = 15;
const float sigma = float(samples) * 0.25;
const float s = 2 * sigma * sigma; 
float gauss(vec2 i)
{
    
    return exp(-(i.x * i.x + i.y * i.y) / s) / (pi * s);
}
vec3 gaussianBlur(vec2 uv, vec2 scale)
{
    vec3 pixel = vec3(0.0);
    float weightSum = 0.0;
    float weight;
    vec2 offset;

    for(int i = -samples / 2; i < samples / 2; i++)
    {
        for(int j = -samples / 2; j < samples / 2; j++)
        {
            offset = vec2(i, j);
            weight = gauss(offset);
            pixel += texture(tex, uv + scale * offset).rgb * weight;
            weightSum += weight;
        }
    }
    return pixel / weightSum;
}
void main(){
    blurred=gaussianBlur(tex,uv,1.0);
}
