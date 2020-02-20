Documentation Author: Niko Procopi

Drawing hitboxes around Entities

If we want to draw wireframe hitboxes, that have a solid color, 
then we need a new Vertex class and we need a new Pipe class.
PipeWire and VertexWire will be used for hitboxes, and we will
dynamically set the color of the hitbox with Uniforms. We also
need a new shader to render the hitboxes.

Shaders for hitboxes will be the simplest yet:
Draw geometry the same as PipelineColor in Vertex Shader
Return a solid color that you get from uniform in Pixel Shader

We also need a new Mesh constructor to handle this new Vertex type.
This new Mesh constructor will be identical to the Mesh constructor
that we used for VertexColor, except it is using sizeof(VertexWire)
instead of sizeof(VertexColor) when calculating array size.
There is probably a less repetitive way to do this, but I'm in a hurry.

VertexWire only has position, nothing else.
PipeWire uses VK_POLYGON_MODE_FILL in Rasterization, not MODE_WIRE,
because we are using LINE_STRIP topology. This boosts compatibility. 

If we were using TRIANGLE_STRIP and MODE_WIRE, then we would depend
on a device feature, just like how anisotropic filtering depends
on a feature being supported when we initialize the VkDevice

We wrote new shaders for the hitboxes to use the color from a uniform
	
We add an Entity function to create a descriptor for 
the wireframe entity, which will send color to the 
fragment shader, to set the color of the hitbox.
This is done in Entity::CreateDescriptorSetWire()

Since we are adding a color to the list of uniforms,
we need to do more than just "Update3D" for the hitbox,
so we add "UpdateColor" to Entity.cpp
However, instead of doing that, we built an automatic
"Update" system that updates every buffer that is initialized
for an entity (2D, 3D, Color, etc). This can be expanded
for more future types of Entities. In addition to 
matrixBufferCPU and spriteBufferCPU, now we have
colorBufferCPU for all wireframe hitbox entities

Reminder, set colorBufferCPU to nullptr in Entity constructor,
and remember to delete it in the destructor. This is needed
any time that new types of uniform buffers are added

Additonally, we no longer need to call "Update" for all 
entities in Scene.cpp, because now Demo.cpp will automatically
upate all Entities (only the ones being drawn) every frame. 

In the future, if someone wants to control which entities 
are updated at specific times (not every frame), then they 
can do a low-level optimization  by themselves

In Demo.cpp, just like previous tutorials, we need to 
add a new descriptor layout, and a new ApplyPipeline
function, so that the Scene can use our new wireframe effect

Scene has a new helper function to build a hitbox from an existing mesh.
That mesh is then used in an entity, the hitbox entity is given the
same position, rotation, and scale as the entity that it represents
(cat hitbox is given cat variables, and dog hitbox is given dog variables, etc)
