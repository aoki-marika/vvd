uniform int state;

void main()
{
    if (state == 0)
        // default
        gl_FragColor = vec4(0.88, 0.58, 0.1, 0.7);
    else if (state == 1)
        // error
        gl_FragColor = vec4(0.88, 0.58, 0.1, 0.5);
    else if (state == 2)
        // critical
        gl_FragColor = vec4(0.88, 0.58, 0.1, 0.8);
}
