attribute vec3 vertexPosition;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

void main()
{
    gl_Position = projection * view * model * vec4(vertexPosition, 1.0);
}
