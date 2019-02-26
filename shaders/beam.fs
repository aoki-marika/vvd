uniform int judgement;
uniform float alpha;

void main()
{
    if (judgement == 1)
        // critical
        gl_FragColor = vec4(1.0, 1.0, 0.0, 0.5 * alpha);
    else if (judgement == 2)
        // near
        gl_FragColor = vec4(1.0, 0.0, 1.0, 0.5 * alpha);
    else if (judgement == 3)
        // error
        gl_FragColor = vec4(0.0, 0.5, 1.0, 0.5 * alpha);
}
