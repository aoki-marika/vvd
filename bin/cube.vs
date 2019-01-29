attribute vec3 vertexPosition;
attribute vec3 vertexColour;

// output data for the fragment shader
varying vec3 fragmentColour;

uniform mat4 mvp;

void main()
{
    gl_Position = mvp * vec4(vertexPosition, 1);
    fragmentColour = vertexColour;
}
