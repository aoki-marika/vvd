uniform int lane;

void main()
{
    if (lane == 0)
        // notes
        gl_FragColor = vec4(0.2, 0.2, 0.2, 1);
    else if (lane == 1)
        // left analog
        gl_FragColor = vec4(0.23, 0.25, 0.32, 1);
    else if (lane == 2)
        // right analog
        gl_FragColor = vec4(0.3, 0.22, 0.27, 1);
}


