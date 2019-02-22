uniform int lane;

void main()
{
    if (lane == 0)
    {
        // left analog
        gl_FragColor = vec4(0.07, 0.81, 1, 0.6);
    }
    else if (lane == 1)
    {
        // right analog
        gl_FragColor = vec4(0.78, 0.15, 0.59, 0.8);
    }
}
