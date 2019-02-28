uniform int state;

void main()
{
    if (state == 0)
        // default
        gl_FragColor = vec4(0.8, 0.8, 0.8, 1);
    else if (state == 1)
        // error
        gl_FragColor = vec4(0.5, 0.5, 0.5, 1);
    else if (state == 2)
        // critical
        gl_FragColor = vec4(1, 1, 1, 1);
}
