attribute vec3 vertexPosition;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

uniform float speed;

void main()
{
    gl_Position = projection * view * model * vec4(vertexPosition.x, vertexPosition.y * speed, vertexPosition.z, 1.0);
}
