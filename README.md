# CS330
Classwork submitted for SNHU CS-330

Below are questions asked in reflection of the project:
```
    How do I approach designing software?
        What new design skills has your work on the project helped you to craft?
        What design process did you follow for your project work?
        How could tactics from your design approach be applied in future work?
    How do I approach developing programs?
        What new development strategies did you use while working on your 3D scene?
        How did iteration factor into your development?
        How has your approach to developing code evolved throughout the milestones, which led you to the project’s completion?
    How can computer science help me in reaching my goals?
        How do computational graphics and visualizations give you new knowledge and skills that can be applied in your future educational pathway?
        How do computational graphics and visualizations give you new knowledge and skills that can be applied in your future professional pathway?
```

My approach to designing software is to create modularity and simplicity. I often dislike using pre-existing libraries, as they may do more than is required, and be quite a bit more complex than what is necessary. OpenGL is an example of such a library that does exactly what is required with an appropriate amount of complexity that would take an extraordinary effort to research and reimplement, and is thus worthy of being used in a project. While creating functions inside the existing source code provided by the course, I ensured that I kept the style consistent, making an effort to name things similarly to the predefined variables and functions, and making comments that mimic what was already present. There is nothing as frustrating as working with an incoherent library. To the end of modularity, previously mentioned, the provided code does a good job of this, separating the jobs of the ViewManager, SceneManager, ShaderManager, etc., and I merely kept in style for each of these separate tasks to ensure this modularity was not ruined.

THe milestones are organized in a logical way: each new feature is a separate milestone for the course. Focusing on more than one complex feature per-iteration has a greater potential to introduce bugs and undefined behaviour, as the behaviour of the new feature may not be as thoroughly tested or given attention. Separating the addition of shapes, textures, lighting into different iterations was a preferable way to develop.

I am quite fond of this class and of computer graphics in general. I have been interested in quite some time in trying to learn how video graphics drivers work and interface between an Operating System and the hardware cards. Rendering anything to a display, even just text, is an amazing achievement of modern technology. If anything, I hope my career involves writing software that is close to metal. I enjoy instructing hardware to produce results that are observable in life, such as rendering images to a display. Although I already had knowledge of how buffers are used to pass information between the CPU and GPU, I wish there was more content in the class regarding how this is implemented or what processes are occuring in the hardware when the shader programs are used.
