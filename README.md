# image-filter-algorithms
Simple image filter application in C++ using OpenGL and FreeImage

# Dependencies

- X11 or other window manager
- FreeImage
- OpenGL (GLUT)

# Execution

    $ g++ Source.cpp -lGL -lglut -lfreeimage -lX11
    $ ./a.out <arg>
    
`<arg>` is an input image in the form of .TIF. If working with other types, use `imagemagick` or other image processor to convert to .TIF.

A CLI will appear giving keyboard controls.
