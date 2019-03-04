attribute vec3 vertexPosition;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

uniform float speed;
uniform float position;

void main()
{
    gl_Position = projection * view * model * vec4(vertexPosition.x, (position * speed) + vertexPosition.y, vertexPosition.z, 1.0);
}
