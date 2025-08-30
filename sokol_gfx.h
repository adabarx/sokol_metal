#if defined(SOKOL_IMPL) && !defined(SOKOL_GFX_IMPL)
#define SOKOL_GFX_IMPL
#endif
#ifndef SOKOL_GFX_INCLUDED
/*
    sokol_gfx.h -- simple 3D API wrapper

    Project URL: https://github.com/floooh/sokol

    Example code: https://github.com/floooh/sokol-samples

    Do this:
        #define SOKOL_IMPL or
        #define SOKOL_GFX_IMPL
    before you include this file in *one* C or C++ file to create the
    implementation.

    In the same place define one of the following to select the rendering
    backend:
        #define SOKOL_GLCORE
        #define SOKOL_GLES3
        #define SOKOL_D3D11
        #define SOKOL_METAL
        #define SOKOL_WGPU
        #define SOKOL_DUMMY_BACKEND

    I.e. for the desktop GL it should look like this:

    #include ...
    #include ...
    #define SOKOL_IMPL
    #define SOKOL_GLCORE
    #include "sokol_gfx.h"

    The dummy backend replaces the platform-specific backend code with empty
    stub functions. This is useful for writing tests that need to run on the
    command line.

    Optionally provide the following defines with your own implementations:

    SOKOL_ASSERT(c)             - your own assert macro (default: assert(c))
    SOKOL_UNREACHABLE()         - a guard macro for unreachable code (default: assert(false))
    SOKOL_GFX_API_DECL          - public function declaration prefix (default: extern)
    SOKOL_API_DECL              - same as SOKOL_GFX_API_DECL
    SOKOL_API_IMPL              - public function implementation prefix (default: -)
    SOKOL_TRACE_HOOKS           - enable trace hook callbacks (search below for TRACE HOOKS)
    SOKOL_EXTERNAL_GL_LOADER    - indicates that you're using your own GL loader, in this case
                                  sokol_gfx.h will not include any platform GL headers and disable
                                  the integrated Win32 GL loader

    If sokol_gfx.h is compiled as a DLL, define the following before
    including the declaration or implementation:

    SOKOL_DLL

    On Windows, SOKOL_DLL will define SOKOL_GFX_API_DECL as __declspec(dllexport)
    or __declspec(dllimport) as needed.

    If you want to compile without deprecated structs and functions,
    define:

    SOKOL_NO_DEPRECATED

    Optionally define the following to force debug checks and validations
    even in release mode:

    SOKOL_DEBUG - by default this is defined if _DEBUG is defined

    Link with the following system libraries (note that sokol_app.h has
    additional linker requirements):

    - on macOS/iOS with Metal: Metal
    - on macOS with GL: OpenGL
    - on iOS with GL: OpenGLES
    - on Linux with EGL: GL or GLESv2
    - on Linux with GLX: GL
    - on Android: GLESv3, log, android
    - on Windows with the MSVC or Clang toolchains: no action needed, libs are defined in-source via pragma-comment-lib
    - on Windows with MINGW/MSYS2 gcc: compile with '-mwin32' so that _WIN32 is defined
        - with the D3D11 backend: -ld3d11

    On macOS and iOS, the implementation must be compiled as Objective-C.

    On Emscripten:
        - for WebGL2: add the linker option `-s USE_WEBGL2=1`
        - for WebGPU: compile and link with `--use-port=emdawnwebgpu`
          (for more exotic situations, read: https://dawn.googlesource.com/dawn/+/refs/heads/main/src/emdawnwebgpu/pkg/README.md)

    sokol_gfx DOES NOT:
    ===================
    - create a window, swapchain or the 3D-API context/device, you must do this
      before sokol_gfx is initialized, and pass any required information
      (like 3D device pointers) to the sokol_gfx initialization call

    - present the rendered frame, how this is done exactly usually depends
      on how the window and 3D-API context/device was created

    - provide a unified shader language, instead 3D-API-specific shader
      source-code or shader-bytecode must be provided (for the "official"
      offline shader cross-compiler / code-generator, see here:
      https://github.com/floooh/sokol-tools/blob/master/docs/sokol-shdc.md)


    STEP BY STEP
    ============
    --- to initialize sokol_gfx, after creating a window and a 3D-API
        context/device, call:

            sg_setup(const sg_desc*)

        Depending on the selected 3D backend, sokol-gfx requires some
        information about its runtime environment, like a GPU device pointer,
        default swapchain pixel formats and so on. If you are using sokol_app.h
        for the window system glue, you can use a helper function provided in
        the sokol_glue.h header:

            #include "sokol_gfx.h"
            #include "sokol_app.h"
            #include "sokol_glue.h"
            //...
            sg_setup(&(sg_desc){
                .environment = sglue_environment(),
            });

        To get any logging output for errors and from the validation layer, you
        need to provide a logging callback. Easiest way is through sokol_log.h:

            #include "sokol_log.h"
            //...
            sg_setup(&(sg_desc){
                //...
                .logger.func = slog_func,
            });

    --- create resource objects (buffers, images, views, samplers, shaders
        and pipeline objects)

            sg_buffer sg_make_buffer(const sg_buffer_desc*)
            sg_image sg_make_image(const sg_image_desc*)
            sg_view sg_make_view(const sg_view_desc*)
            sg_sampler sg_make_sampler(const sg_sampler_desc*)
            sg_shader sg_make_shader(const sg_shader_desc*)
            sg_pipeline sg_make_pipeline(const sg_pipeline_desc*)

    --- start a render- or compute-pass:

            sg_begin_pass(const sg_pass* pass);

        Typically, render passes render into an externally provided swapchain which
        presents the rendering result on the display. Such a 'swapchain pass'
        is started like this:

            sg_begin_pass(&(sg_pass){ .action = { ... }, .swapchain = sglue_swapchain() })

        ...where .action is an sg_pass_action struct containing actions to be performed
        at the start and end of a render pass (such as clearing the render surfaces to
        a specific color), and .swapchain is an sg_swapchain struct with all the required
        information to render into the swapchain's surfaces.

        To start an 'offscreen render pass' into sokol-gfx image objects, populate
        the sg_pass.attachments nested struct with attachment view objects
        (1..4 color-attachment-views for to render into, a depth-stencil-attachment-view
        to provide the depth-stencil-buffer, and optionally 1..4 resolve-attachment-views
        for an MSAA-resolve operation:

            sg_begin_pass(&(sg_pass){
                .action = { ... },
                .attachments = {
                    .colors[0] = color_attachment_view,
                    .resolves[0] = optional_resolve_attachment_view,
                    .depth_stencil = depth_stencil_attachment_view,
                },
            });

        To start a compute-pass, just set the .compute item to true:

            sg_begin_pass(&(sg_pass){ .compute = true });

    --- set the pipeline state for the next draw call with:

            sg_apply_pipeline(sg_pipeline pip)

    --- fill an sg_bindings struct with the resource bindings for the next
        draw- or dispatch-call (0..N vertex buffers, 0 or 1 index buffer, 0..N views,
        0..N samplers), and call

            sg_apply_bindings(const sg_bindings* bindings)

        ...to update the resource bindings. Note that in a compute pass, no vertex-
        or index-buffer bindings can be used, and in render passes, no storage-image bindings
        are allowed. Those restrictions will be checked by the sokol-gfx validation layer.

    --- optionally update shader uniform data with:

            sg_apply_uniforms(int ub_slot, const sg_range* data)

        Read the section 'UNIFORM DATA LAYOUT' to learn about the expected memory layout
        of the uniform data passed into sg_apply_uniforms().

    --- kick off a draw call with:

            sg_draw(int base_element, int num_elements, int num_instances)

        The sg_draw() function unifies all the different ways to render primitives
        in a single call (indexed vs non-indexed rendering, and instanced vs non-instanced
        rendering). In case of indexed rendering, base_element and num_element specify
        indices in the currently bound index buffer. In case of non-indexed rendering
        base_element and num_elements specify vertices in the currently bound
        vertex-buffer(s). To perform instanced rendering, the rendering pipeline
        must be setup for instancing (see sg_pipeline_desc below), a separate vertex buffer
        containing per-instance data must be bound, and the num_instances parameter
        must be > 1.

    --- ...or kick of a dispatch call to invoke a compute shader workload:

            sg_dispatch(int num_groups_x, int num_groups_y, int num_groups_z)

        The dispatch args define the number of 'compute workgroups' processed
        by the currently applied compute shader.

    --- finish the current pass with:

            sg_end_pass()

    --- when done with the current frame, call

            sg_commit()

    --- at the end of your program, shutdown sokol_gfx with:

            sg_shutdown()

    --- if you need to destroy resources before sg_shutdown(), call:

            sg_destroy_buffer(sg_buffer buf)
            sg_destroy_image(sg_image img)
            sg_destroy_sampler(sg_sampler smp)
            sg_destroy_shader(sg_shader shd)
            sg_destroy_pipeline(sg_pipeline pip)
            sg_destroy_view(sg_view view)

    --- to set a new viewport rectangle, call:

            sg_apply_viewport(int x, int y, int width, int height, bool origin_top_left)

        ...or if you want to specify the viewport rectangle with float values:

            sg_apply_viewportf(float x, float y, float width, float height, bool origin_top_left)

    --- to set a new scissor rect, call:

            sg_apply_scissor_rect(int x, int y, int width, int height, bool origin_top_left)

        ...or with float values:

            sg_apply_scissor_rectf(float x, float y, float width, float height, bool origin_top_left)

        Both sg_apply_viewport() and sg_apply_scissor_rect() must be called
        inside a rendering pass (e.g. not in a compute pass, or outside a pass)

        Note that sg_begin_pass() will reset both the viewport and scissor
        rectangles to cover the entire framebuffer.

    --- to update (overwrite) the content of buffer and image resources, call:

            sg_update_buffer(sg_buffer buf, const sg_range* data)
            sg_update_image(sg_image img, const sg_image_data* data)

        Buffers and images to be updated must have been created with
        sg_buffer_desc.usage.dynamic_update or .stream_update.

        Only one update per frame is allowed for buffer and image resources when
        using the sg_update_*() functions. The rationale is to have a simple
        protection from the CPU scribbling over data the GPU is currently
        using, or the CPU having to wait for the GPU

        Buffer and image updates can be partial, as long as a rendering
        operation only references the valid (updated) data in the
        buffer or image.

    --- to append a chunk of data to a buffer resource, call:

            int sg_append_buffer(sg_buffer buf, const sg_range* data)

        The difference to sg_update_buffer() is that sg_append_buffer()
        can be called multiple times per frame to append new data to the
        buffer piece by piece, optionally interleaved with draw calls referencing
        the previously written data.

        sg_append_buffer() returns a byte offset to the start of the
        written data, this offset can be assigned to
        sg_bindings.vertex_buffer_offsets[n] or
        sg_bindings.index_buffer_offset

        Code example:

        for (...) {
            const void* data = ...;
            const int num_bytes = ...;
            int offset = sg_append_buffer(buf, &(sg_range) { .ptr=data, .size=num_bytes });
            bindings.vertex_buffer_offsets[0] = offset;
            sg_apply_pipeline(pip);
            sg_apply_bindings(&bindings);
            sg_apply_uniforms(...);
            sg_draw(...);
        }

        A buffer to be used with sg_append_buffer() must have been created
        with sg_buffer_desc.usage.dynamic_update or .stream_update.

        If the application appends more data to the buffer then fits into
        the buffer, the buffer will go into the "overflow" state for the
        rest of the frame.

        Any draw calls attempting to render an overflown buffer will be
        silently dropped (in debug mode this will also result in a
        validation error).

        You can also check manually if a buffer is in overflow-state by calling

            bool sg_query_buffer_overflow(sg_buffer buf)

        You can manually check to see if an overflow would occur before adding
        any data to a buffer by calling

            bool sg_query_buffer_will_overflow(sg_buffer buf, size_t size)

        NOTE: Due to restrictions in underlying 3D-APIs, appended chunks of
        data will be 4-byte aligned in the destination buffer. This means
        that there will be gaps in index buffers containing 16-bit indices
        when the number of indices in a call to sg_append_buffer() is
        odd. This isn't a problem when each call to sg_append_buffer()
        is associated with one draw call, but will be problematic when
        a single indexed draw call spans several appended chunks of indices.

    --- to check at runtime for optional features, limits and pixelformat support,
        call:

            sg_features sg_query_features()
            sg_limits sg_query_limits()
            sg_pixelformat_info sg_query_pixelformat(sg_pixel_format fmt)

    --- if you need to call into the underlying 3D-API directly, you must call:

            sg_reset_state_cache()

        ...before calling sokol_gfx functions again

    --- you can inspect the original sg_desc structure handed to sg_setup()
        by calling sg_query_desc(). This will return an sg_desc struct with
        the default values patched in instead of any zero-initialized values

    --- you can get a desc struct matching the creation attributes of a
        specific resource object via:

            sg_buffer_desc sg_query_buffer_desc(sg_buffer buf)
            sg_image_desc sg_query_image_desc(sg_image img)
            sg_sampler_desc sg_query_sampler_desc(sg_sampler smp)
            sg_shader_desc sq_query_shader_desc(sg_shader shd)
            sg_pipeline_desc sg_query_pipeline_desc(sg_pipeline pip)
            sg_view_desc sg_query_view_desc(sg_view view)

        ...but NOTE that the returned desc structs may be incomplete, only
        creation attributes that are kept around internally after resource
        creation will be filled in, and in some cases (like shaders) that's
        very little. Any missing attributes will be set to zero. The returned
        desc structs might still be useful as partial blueprint for creating
        similar resources if filled up with the missing attributes.

        Calling the query-desc functions on an invalid resource will return
        completely zeroed structs (it makes sense to check  the resource state
        with sg_query_*_state() first)

    --- you can query the default resource creation parameters through the functions

            sg_buffer_desc sg_query_buffer_defaults(const sg_buffer_desc* desc)
            sg_image_desc sg_query_image_defaults(const sg_image_desc* desc)
            sg_sampler_desc sg_query_sampler_defaults(const sg_sampler_desc* desc)
            sg_shader_desc sg_query_shader_defaults(const sg_shader_desc* desc)
            sg_pipeline_desc sg_query_pipeline_defaults(const sg_pipeline_desc* desc)
            sg_view_desc sg_query_view_defaults(const sg_view_desc* desc)

        These functions take a pointer to a desc structure which may contain
        zero-initialized items for default values. These zero-init values
        will be replaced with their concrete values in the returned desc
        struct.

    --- you can inspect various internal resource runtime values via:

            sg_buffer_info sg_query_buffer_info(sg_buffer buf)
            sg_image_info sg_query_image_info(sg_image img)
            sg_sampler_info sg_query_sampler_info(sg_sampler smp)
            sg_shader_info sg_query_shader_info(sg_shader shd)
            sg_pipeline_info sg_query_pipeline_info(sg_pipeline pip)
            sg_view_info sg_query_view_info(sg_view view)

        ...please note that the returned info-structs are tied quite closely
        to sokol_gfx.h internals, and may change more often than other
        public API functions and structs.

    -- you can query the type/flavour and parent resource of a view:

            sg_view_type sg_query_view_type(sg_view view)
            sg_image sg_query_view_image(sg_view view)
            sg_buffer sg_query_view_buffer(sg_view view)

    --- you can query frame stats and control stats collection via:

            sg_query_frame_stats()
            sg_enable_frame_stats()
            sg_disable_frame_stats()
            sg_frame_stats_enabled()

    --- you can ask at runtime what backend sokol_gfx.h has been compiled for:

            sg_backend sg_query_backend(void)

    --- call the following helper functions to compute the number of
        bytes in a texture row or surface for a specific pixel format.
        These functions might be helpful when preparing image data for consumption
        by sg_make_image() or sg_update_image():

            int sg_query_row_pitch(sg_pixel_format fmt, int width, int int row_align_bytes);
            int sg_query_surface_pitch(sg_pixel_format fmt, int width, int height, int row_align_bytes);

        Width and height are generally in number pixels, but note that 'row' has different meaning
        for uncompressed vs compressed pixel formats: for uncompressed formats, a row is identical
        with a single line if pixels, while in compressed formats, one row is a line of *compression blocks*.

        This is why calling sg_query_surface_pitch() for a compressed pixel format and height
        N, N+1, N+2, ... may return the same result.

        The row_align_bytes parameter is for added flexibility. For image data that goes into
        the sg_make_image() or sg_update_image() this should generally be 1, because these
        functions take tightly packed image data as input no matter what alignment restrictions
        exist in the backend 3D APIs.

    ON INITIALIZATION:
    ==================
    When calling sg_setup(), a pointer to an sg_desc struct must be provided
    which contains initialization options. These options provide two types
    of information to sokol-gfx:

        (1) upper bounds and limits needed to allocate various internal
            data structures:
                - the max number of resources of each type that can
                  be alive at the same time, this is used for allocating
                  internal pools
                - the max overall size of uniform data that can be
                  updated per frame, including a worst-case alignment
                  per uniform update (this worst-case alignment is 256 bytes)
                - the max size of all dynamic resource updates (sg_update_buffer,
                  sg_append_buffer and sg_update_image) per frame
                - the max number of compute-dispatch calls in a compute pass
            Not all of those limit values are used by all backends, but it is
            good practice to provide them none-the-less.

        (2) 3D backend "environment information" in a nested sg_environment struct:
            - pointers to backend-specific context- or device-objects (for instance
              the D3D11, WebGPU or Metal device objects)
            - defaults for external swapchain pixel formats and sample counts,
              these will be used as default values in image and pipeline objects,
              and the sg_swapchain struct passed into sg_begin_pass()
            Usually you provide a complete sg_environment struct through
            a helper function, as an example look at the sglue_environment()
            function in the sokol_glue.h header.

    See the documentation block of the sg_desc struct below for more information.


    ON RENDER PASSES
    ================
    Relevant samples:
        - https://floooh.github.io/sokol-html5/offscreen-sapp.html
        - https://floooh.github.io/sokol-html5/offscreen-msaa-sapp.html
        - https://floooh.github.io/sokol-html5/mrt-sapp.html
        - https://floooh.github.io/sokol-html5/mrt-pixelformats-sapp.html

    A render pass groups rendering commands into a set of render target images
    (called 'render pass attachments'). Render target images can be used in subsequent
    passes as textures (it is invalid to use the same image both as render target
    and as texture in the same pass).

    The following sokol-gfx functions must only be called inside a render-pass:

        sg_apply_viewport[f]
        sg_apply_scissor_rect[f]
        sg_draw

    The following function may be called inside a render- or compute-pass, but
    not outside a pass:

        sg_apply_pipeline
        sg_apply_bindings
        sg_apply_uniforms

    A frame must have at least one 'swapchain render pass' which renders into an
    externally provided swapchain provided as an sg_swapchain struct to the
    sg_begin_pass() function. If you use sokol_gfx.h together with sokol_app.h,
    just call the sglue_swapchain() helper function in sokol_glue.h to
    provide the swapchain information. Otherwise the following information
    must be provided:

        - the color pixel-format of the swapchain's render surface
        - an optional depth/stencil pixel format if the swapchain
          has a depth/stencil buffer
        - an optional sample-count for MSAA rendering
        - NOTE: the above three values can be zero-initialized, in that
          case the defaults from the sg_environment struct will be used that
          had been passed to the sg_setup() function.
        - a number of backend specific objects:
            - GL/GLES3: just a GL framebuffer handle
            - D3D11:
                - an ID3D11RenderTargetView for the rendering surface
                - if MSAA is used, an ID3D11RenderTargetView as
                  MSAA resolve-target
                - an optional ID3D11DepthStencilView for the
                  depth/stencil buffer
            - WebGPU
                - a WGPUTextureView object for the rendering surface
                - if MSAA is used, a WGPUTextureView object as MSAA resolve target
                - an optional WGPUTextureView for the
            - Metal (NOTE that the roles of provided surfaces is slightly
              different in Metal than in D3D11 or WebGPU, notably, the
              CAMetalDrawable is either rendered to directly, or serves
              as MSAA resolve target):
                - a CAMetalDrawable object which is either rendered
                  into directly, or in case of MSAA rendering, serves
                  as MSAA-resolve-target
                - if MSAA is used, an multisampled MTLTexture where
                  rendering goes into
                - an optional MTLTexture for the depth/stencil buffer

    It's recommended that you create a helper function which returns an
    initialized sg_swapchain struct by value. This can then be directly plugged
    into the sg_begin_pass function like this:

        sg_begin_pass(&(sg_pass){ .swapchain = sglue_swapchain() });

    As an example for such a helper function check out the function sglue_swapchain()
    in the sokol_glue.h header.

    For offscreen render passes, the render target images used in a render pass
    must be provided as sg_view objects specialized for the specific pass-attachment
    types:

        - color-attachment-views for color-rendering
        - depth-stencil-attachment-views for the depth-stencil-buffer surface
        - resolve-attachment-views for MSAA-resolve operations

    For a simple offscreen scenario with one color-, one depth-stencil-render
    target and without multisampling, setting up the required image-
    and view-objects looks like this:

    First create two render target images, one with a color pixel format,
    and one with the depth- or depth-stencil pixel format. Both images
    must have the same dimensions. Also not the usage flags:

        const sg_image color_img = sg_make_image(&(sg_image_desc){
            .usage.color_attachment = true,
            .width = 256,
            .height = 256,
            .pixel_format = SG_PIXELFORMAT_RGBA8,
            .sample_count = 1,
        });
        const sg_image depth_img = sg_make_image(&(sg_image_desc){
            .usage.depth_stencil_attachment = true,
            .width = 256,
            .height = 256,
            .pixel_format = SG_PIXELFORMAT_DEPTH,
            .sample_count = 1,
        });

    NOTE: when creating render target images, have in mind that some default values
    are aligned with the default environment attributes in the sg_environment struct
    that was passed into the sg_setup() call:

        - the default value for sg_image_desc.pixel_format is taken from
          sg_environment.defaults.color_format
        - the default value for sg_image_desc.sample_count is taken from
          sg_environment.defaults.sample_count
        - the default value for sg_image_desc.num_mipmaps is always 1

    Next, create two view objects, one color-attachment-view and one
    depth-stencil-attachment view:

        const sg_view color_att_view = sg_make_view(&(sg_view_desc){
            .color_attachment.image = color_img,
        });
        const sg_view depth_att_view = sg_make_view(&(sg_view_desc){
            .depth_stencil_attachment.image = depth_img,
        });

    You'll typically also want to create a texture-view on the color image
    to sample the color attachment image as texture in a later pass:

        const sg_view tex_view = sg_make_view(&(sg_view_desc){
            .texture.image = color_img,
        });

    The attachment-view objects are then passed into the sg_begin_pass function in
    place of the nested swapchain struct:

        sg_begin_pass(&(sg_pass){
            .attachments = {
                .colors[0] = color_att_view,
                .depth_stencil = depth_att_view,
            },
        });

    ...in a later pass when you want to sample the color attachment image as
    texture, use the texture view in the sg_apply_bindings() call:

        sg_apply_bindings(&(sg_bindings){
            .vertex_buffers[0] = ...,
            .index_buffer = ...,
            .views[VIEW_tex] = tex_view,
            .samplers[SMP_smp] = smp,
        });

    Swapchain and offscreen passes form dependency trees with a swapchain
    pass at the root, offscreen passes as nodes, and attachment images as
    dependencies between passes.

    sg_pass_action structs are used to define actions that should happen at the
    start and end of render passes (such as clearing pass attachments to a
    specific color or depth-value, or performing an MSAA resolve operation at
    the end of a pass).

    A typical sg_pass_action object which clears the color attachment to black
    might look like this:

        const sg_pass_action = {
            .colors[0] = {
                .load_action = SG_LOADACTION_CLEAR,
                .clear_value = { 0.0f, 0.0f, 0.0f, 1.0f }
            }
        };

    This omits the defaults for the color attachment store action, and
    the depth-stencil-attachments actions. The same pass action with the
    defaults explicitly filled in would look like this:

        const sg_pass_action pass_action = {
            .colors[0] = {
                .load_action = SG_LOADACTION_CLEAR,
                .store_action = SG_STOREACTION_STORE,
                .clear_value = { 0.0f, 0.0f, 0.0f, 1.0f }
            },
            .depth = = {
                .load_action = SG_LOADACTION_CLEAR,
                .store_action = SG_STOREACTION_DONTCARE,
                .clear_value = 1.0f,
            },
            .stencil = {
                .load_action = SG_LOADACTION_CLEAR,
                .store_action = SG_STOREACTION_DONTCARE,
                .clear_value = 0
            }
        };

    With the sg_pass object and sg_pass_action struct in place everything
    is ready now for the actual render pass:

    Using such this prepared sg_pass_action in a swapchain pass looks like
    this:

        sg_begin_pass(&(sg_pass){
            .action = pass_action,
            .swapchain = sglue_swapchain()
        });
        ...
        sg_end_pass();

    ...of alternatively in one offscreen pass:

        sg_begin_pass(&(sg_pass){
            .action = pass_action,
            .attachments = {
                .colors[0] = color_att_view,
                .depth_stencil = ds_att_view,
            },
        });
        ...
        sg_end_pass();

    Offscreen rendering can also go into a mipmap, or a slice/face of
    a cube-, array- or 3d-image (which some restrictions, for instance
    it's not possible to create a 3D image with a depth/stencil pixel format,
    these exceptions are generally caught by the sokol-gfx validation layer).

    The mipmap/slice selection is baked into the attachment-view objects, for
    instance to create a color-attachment-view for rendering into mip-level
    2 and slice 3 of an array texture:

        const sg_view color_att_view = sg_make_view(&(sg_view_desc){
            .color_attachment = {
                .image = color_img,
                .mip_level = 2,
                .slice = 3,
            },
        });

    If MSAA offscreen rendering is desired, the multi-sample rendering result
    must be 'resolved' into a separate 'resolve image', before that image can
    be used as texture.

    Setting up MSAA offscreen 3D rendering requires three image objects
    (one color-attachment image with a sample count > 1), a resolve-attachment
    image with a sample count of 1, and a depth-stencil-attachment image
    with the same sample count as the color-attachment image:

        const sg_image color_img = sg_make_image(&(sg_image_desc){
            .usage.color_attachment = true,
            .width = 256,
            .height = 256,
            .pixel_format = SG_PIXELFORMAT_RGBA8,
            .sample_count = 4,
        });
        const sg_image resolve_img = sg_make_image(&(sg_image_desc){
            .usage.resolve_attachment = true,
            .width = 256,
            .height = 256,
            .pixel_format = SG_PIXELFORMAT_RGBA8,
            .sample_count = 1,
        });
        const sg_image depth_img = sg_make_image(&(sg_image_desc){
            .usage.depth_stencil_attachment = true,
            .width = 256,
            .height = 256,
            .pixel_format = SG_PIXELFORMAT_DEPTH,
            .sample_count = 4,
        });

    Next you'll need the corresponding attachment-view objects:

        const sg_view color_att_view = sg_make_view(&(sg_view_desc){
            .color_attachment.image = color_img,
        });
        const sg_view resolve_att_view = sg_make_view(&(sg_view_desc){
            .resolve_attachment.image = resolve_img,
        });
        const sg_view depth_att_view = sg_make_view(&(sg_view_desc){
            .depth_stencil_attachment.image = depth_img,
        });

    To sample the rendered image as a texture in a later pass you'll also
    need a texture-view on the resolve-attachment-image (not the color-attachment-image!):

        const sg_view tex_view = sg_make_view(&(sg_view_desc){
            .texture.image = resolve_img,
        });

    Next start the render pass with all attachment-views, as soon as a
    resolve-attachment-view is provided, an MSAA resolve operation will happen
    at the end of the pass. Also note that the content of the MSAA color-attachemnt-image
    doesn't need to be preserved, since it's only needed until the MSAA-resolve
    at the end of the pass, so the .store_action should be set to "don't care":

        sg_begin_pass(&(sg_pass){
            .attachments = {
                .colors[0] = color_att_view,
                .resolves[0] = resolve_att_view,
                .depth_stencil = depth_att_view,
            },
            .action = {
                .colors[0] = {
                    .load_action = SG_LOADACTION_CLEAR,
                    .store_action = SG_STOREACTION_DONTCARE,
                    .clear_value = { 0.0f, 0.0f, 0.0f, 1.0f },
                }
            },
        });

    ...in a later pass, use the texture-view that had been created on the
    resolve-image to use the rendering result as texture:

        sg_apply_bindings(&(sg_bindings){
            .vertex_buffers[0] = ...,
            .index_buffer = ...,
            .views[VIEW_tex] = tex_view,
            .samplers[SMP_smp] = smp,
        });

    ON COMPUTE PASSES
    =================
    Compute passes are used to update the content of storage buffers and
    storage images by running compute shader code on
    the GPU. Updating storage resources with a compute shader will almost always
    be more efficient than computing the same data on the CPU and then uploading
    it via `sg_update_buffer()` or `sg_update_image()`.

    NOTE: compute passes are only supported on the following platforms and
    backends:

        - macOS and iOS with Metal
        - Windows with D3D11 and OpenGL
        - Linux with OpenGL or GLES3.1+
        - Web with WebGPU
        - Android with GLES3.1+

    ...this means compute shaders can't be used on the following platform/backend
    combos (the same restrictions apply to using storage buffers without compute
    shaders):

        - macOS with GL
        - iOS with GLES3
        - Web with WebGL2

    A compute pass is started with:

        sg_begin_pass(&(sg_pass){ .compute = true });

    ...and finished with a regular:

        sg_end_pass();

    Typically the following functions will be called inside a compute pass:

        sg_apply_pipeline()
        sg_apply_bindings()
        sg_apply_uniforms()
        sg_dispatch()

    The following functions are disallowed inside a compute pass
    and will cause validation layer errors:

        sg_apply_viewport[f]()
        sg_apply_scissor_rect[f]()
        sg_draw()

    Only special 'compute shaders' and 'compute pipelines' can be used in
    compute passes. A compute shader only has a compute-function instead
    of a vertex- and fragment-function pair, and it doesn't accept vertex-
    and index-buffers as bindings, only storage-buffer-views (readable
    and writable), storage-image-views (read/write or writeonly) and
    texture-views (read-only).

    A compute pipeline is created by providing a compute shader object,
    setting the .compute creation parameter to true and not defining any
    'render state':

        sg_pipeline pip = sg_make_pipeline(&(sg_pipeline_desc){
            .compute = true,
            .shader = compute_shader,
        });

    The sg_apply_bindings and sg_apply_uniforms calls are the same as in
    render passes, with the exception that no vertex- and index-buffers
    can be bound in the sg_apply_bindings call.

    Finally to kick off a compute workload, call sg_dispatch with the
    number of workgroups in the x, y and z-dimension:

        sg_dispatch(int num_groups_x, int num_groups_y, int num_groups_z)

    Also see the following compute-shader samples:

        - https://floooh.github.io/sokol-webgpu/instancing-compute-sapp.html
        - https://floooh.github.io/sokol-webgpu/computeboids-sapp.html
        - https://floooh.github.io/sokol-webgpu/imageblur-sapp.html


    ON SHADER CREATION
    ==================
    sokol-gfx doesn't come with an integrated shader cross-compiler, instead
    backend-specific shader sources or binary blobs need to be provided when
    creating a shader object, along with reflection information about the
    shader resource binding interface needed to bind sokol-gfx resources to the
    proper shader inputs.

    The easiest way to provide all this shader creation data is to use the
    sokol-shdc shader compiler tool to compile shaders from a common
    GLSL syntax into backend-specific sources or binary blobs, along with
    shader interface information and uniform blocks and storage buffer array items
    mapped to C structs.

    To create a shader using a C header which has been code-generated by sokol-shdc:

        // include the C header code-generated by sokol-shdc:
        #include "myshader.glsl.h"
        ...

        // create shader using a code-generated helper function from the C header:
        sg_shader shd = sg_make_shader(myshader_shader_desc(sg_query_backend()));

    The samples in the 'sapp' subdirectory of the sokol-samples project
    also use the sokol-shdc approach:

        https://github.com/floooh/sokol-samples/tree/master/sapp

    If you're planning to use sokol-shdc, you can stop reading here, instead
    continue with the sokol-shdc documentation:

        https://github.com/floooh/sokol-tools/blob/master/docs/sokol-shdc.md

    To create shaders with backend-specific shader code or binary blobs,
    the sg_make_shader() function requires the following information:

    - Shader code or shader binary blobs for the vertex- and fragment-, or the
      compute-shader-stage:
        - for the desktop GL backend, source code can be provided in '#version 410' or
          '#version 430', version 430 is required when using storage buffers and
          compute shaders support, but note that this is not available on macOS
        - for the GLES3 backend, source code must be provided in '#version 300 es' or
          '#version 310 es' syntax (version 310 is required for storage buffer and
          compute shader support, but note that this is not supported on WebGL2)
        - for the D3D11 backend, shaders can be provided as source or binary
          blobs, the source code should be in HLSL4.0 (for compatibility with old
          low-end GPUs) or preferably in HLSL5.0 syntax, note that when
          shader source code is provided for the D3D11 backend, sokol-gfx will
          dynamically load 'd3dcompiler_47.dll'
        - for the Metal backends, shaders can be provided as source or binary blobs, the
          MSL version should be in 'metal-1.1' (other versions may work but are not tested)
        - for the WebGPU backend, shaders must be provided as WGSL source code
        - optionally the following shader-code related attributes can be provided:
            - an entry function name (only on D3D11 or Metal, but not OpenGL)
            - on D3D11 only, a compilation target (default is "vs_4_0" and "ps_4_0")

    - Information about the input vertex attributes used by the vertex shader,
      most of that backend-specific:
        - An optional 'base type' (float, signed-/unsigned-int) for each vertex
          attribute. When provided, this used by the validation layer to check
          that the CPU-side input vertex format is compatible with the input
          vertex declaration of the vertex shader.
        - Metal: no location information needed since vertex attributes are always bound
          by their attribute location defined in the shader via '[[attribute(N)]]'
        - WebGPU: no location information needed since vertex attributes are always
          bound by their attribute location defined in the shader via `@location(N)`
        - GLSL: vertex attribute names can be optionally provided, in that case their
          location will be looked up by name, otherwise, the vertex attribute location
          can be defined with 'layout(location = N)'
        - D3D11: a 'semantic name' and 'semantic index' must be provided for each vertex
          attribute, e.g. if the vertex attribute is defined as 'TEXCOORD1' in the shader,
          the semantic name would be 'TEXCOORD', and the semantic index would be '1'

      NOTE that vertex attributes currently must not have gaps. This requirement
      may be relaxed in the future.

    - Specifically for Metal compute shaders, the 'number of threads per threadgroup'
      must be provided. Normally this is extracted by sokol-shdc from the GLSL
      shader source code. For instance the following statement in the input
      GLSL:

        layout(local_size_x=64, local_size_y=1, local_size_z=1) in;

      ...will be communicated to the sokol-gfx Metal backend in the
      code-generated sg_shader_desc struct:

        (sg_shader_desc){
            .mtl_threads_per_threadgroup = { .x = 64, .y = 1, .z = 1 },
        }

    - Information about each uniform block binding used in the shader:
        - the shader stage of the uniform block (vertex, fragment or compute)
        - the size of the uniform block in number of bytes
        - a memory layout hint (currently 'native' or 'std140') where 'native' defines a
          backend-specific memory layout which shouldn't be used for cross-platform code.
          Only std140 guarantees a backend-agnostic memory layout.
        - a backend-specific bind slot:
            - D3D11/HLSL: the buffer register N (`register(bN)`) where N is 0..7
            - Metal/MSL: the buffer bind slot N (`[[buffer(N)]]`) where N is 0..7
            - WebGPU: the binding N in `@group(0) @binding(N)` where N is 0..15
        - For GLSL only: a description of the internal uniform block layout, which maps
          member types and their offsets on the CPU side to uniform variable names
          in the GLSL shader
        - please also NOTE the documentation sections about UNIFORM DATA LAYOUT
          and CROSS-BACKEND COMMON UNIFORM DATA LAYOUT below!

    - A description of each resource binding (texture-, storage-buffer-
      and storage-image-bindings) which directly map to the sg_bindings.view[]
      array slots.

      Each resource binding slot comes in three flavours:

        1. Texture bindings with the following properties:
            - the shader stage of the texture (vertex, fragment or compute)
            - the expected image type:
                - SG_IMAGETYPE_2D
                - SG_IMAGETYPE_CUBE
                - SG_IMAGETYPE_3D
                - SG_IMAGETYPE_ARRAY
            - the expected 'image sample type':
                - SG_IMAGESAMPLETYPE_FLOAT
                - SG_IMAGESAMPLETYPE_DEPTH
                - SG_IMAGESAMPLETYPE_SINT
                - SG_IMAGESAMPLETYPE_UINT
                - SG_IMAGESAMPLETYPE_UNFILTERABLE_FLOAT
            - a flag whether the texture is expected to be multisampled
            - a backend-specific bind slot:
                - D3D11/HLSL: the texture register N (`register(tN)`) where N is 0..23
                (in HLSL, readonly storage buffers and texture share the same bind space)
                - Metal/MSL: the texture bind slot N (`[[texture(N)]]`) where N is 0..19
                (the bind slot must not collide with storage image bindings on the same stage)
                - WebGPU/WGSL: the binding N in `@group(0) @binding(N)` where N is 0..127

        2. Storage buffer bindings with the following properties:
            - the shader stage of the storage buffer
            - a boolean 'readonly' flag, this is used for validation and hazard
            tracking in some 3D backends. Note that in render passes, only
            readonly storage buffer bindings are allowed. In compute passes, any
            read/write storage buffer binding is assumed to be written to by the
            compute shader.
            - a backend-specific bind slot:
                - D3D11/HLSL:
                    - for readonly storage buffer bindings: the texture register N
                    (`register(tN)`) where N is 0..23 (in HLSL, readonly storage
                    buffers and textures share the same bind space for
                    'shader resource views')
                    - for read/write storage buffer buffer bindings: the UAV register N
                    (`register(uN)`) where N is 0..11 (in HLSL, readwrite storage
                    buffers use their own bind space for 'unordered access views')
                - Metal/MSL: the buffer bind slot N (`[[buffer(N)]]`) where N is 8..15
                - WebGPU/WGSL: the binding N in `@group(0) @binding(N)` where N is 0..127
                - GL/GLSL: the buffer binding N in `layout(binding=N)` where N is 0..7
            - note that storage buffer bindings are not supported on all backends
            and platforms

        3. Storage image bindings with the following properties:
            - the shader stage (*must* be compute)
            - the expected image type:
                - SG_IMAGETYPE_2D
                - SG_IMAGETYPE_CUBE
                - SG_IMAGETYPE_3D
                - SG_IMAGETYPE_ARRAY
            - the 'access pixel format', this is currently limited to:
                - SG_PIXELFORMAT_RGBA8
                - SG_PIXELFORMAT_RGBA8SN/UI/SI
                - SG_PIXELFORMAT_RGBA16UI/SI/F
                - SG_PIXELFORMAT_R32UIUI/SI/F
                - SG_PIXELFORMAT_RG32UI/SI/F
                - SG_PIXELFORMAT_RGBA32UI/SI/F
            - the access type (readwrite or writeonly)
            - a backend-specific bind slot:
                - D3D11/HLSL: the UAV register N (`register(uN)` where N is 0..11, the
                bind slot must not collide with UAV storage buffer bindings
                - Metal/MSL: the texture bind slot N (`[[texture(N)]])` where N is 0..19,
                the bind slot must not collide with other texture bindings on the same
                stage
                - WebGPU/WGSL: the binding N in `@group(1) @binding(N)` where N is 0..127
                - GL/GLSL: the buffer binding N in `layout(binding=N)` where N is 0..3
            - note that storage image bindings are not supported on all backends and platforms

    - A description of each sampler used in the shader:
        - the shader stage of the sampler (vertex, fragment or compute)
        - the expected sampler type:
            - SG_SAMPLERTYPE_FILTERING,
            - SG_SAMPLERTYPE_NONFILTERING,
            - SG_SAMPLERTYPE_COMPARISON,
        - a backend-specific bind slot:
            - D3D11/HLSL: the sampler register N (`register(sN)`) where N is 0..15
            - Metal/MSL: the sampler bind slot N (`[[sampler(N)]]`) where N is 0..15
            - WebGPU/WGSL: the binding N in `@group(0) @binding(N)` where N is 0..127

    - An array of 'texture-sampler-pairs' used by the shader to sample textures,
      for D3D11, Metal and WebGPU this is used for validation purposes to check
      whether the texture and sampler are compatible with each other (especially
      WebGPU is very picky about combining the correct
      texture-sample-type with the correct sampler-type). For GLSL an
      additional 'combined-image-sampler name' must be provided because 'OpenGL
      style GLSL' cannot handle separate texture and sampler objects, but still
      groups them into a traditional GLSL 'sampler object'.

    Compatibility rules for image-sample-type vs sampler-type are as follows:

        - SG_IMAGESAMPLETYPE_FLOAT => (SG_SAMPLERTYPE_FILTERING or SG_SAMPLERTYPE_NONFILTERING)
        - SG_IMAGESAMPLETYPE_UNFILTERABLE_FLOAT => SG_SAMPLERTYPE_NONFILTERING
        - SG_IMAGESAMPLETYPE_SINT => SG_SAMPLERTYPE_NONFILTERING
        - SG_IMAGESAMPLETYPE_UINT => SG_SAMPLERTYPE_NONFILTERING
        - SG_IMAGESAMPLETYPE_DEPTH => SG_SAMPLERTYPE_COMPARISON

    Backend-specific bindslot ranges (not relevant when using sokol-shdc):

        - D3D11/HLSL:
            - separate bindslot space per shader stage
            - uniform block bindings (as cbuffer): `register(b0..b7)`
            - texture- and readonly storage buffer bindings: `register(t0..t23)`
            - read/write storage buffer and storage image bindings: `register(u0..u11)`
            - samplers: `register(s0..s15)`
        - Metal/MSL:
            - separate bindslot space per shader stage
            - uniform blocks: `[[buffer(0..7)]]`
            - storage buffers: `[[buffer(8..15)]]`
            - textures and storage image bindings: `[[texture(0..19)]]`
            - samplers: `[[sampler(0..15)]]`
        - WebGPU/WGSL:
            - common bindslot space across shader stages
            - uniform blocks: `@group(0) @binding(0..15)`
            - textures, storage-images, storage-buffers and sampler: `@group(1) @binding(0..127)`
        - GL/GLSL:
            - uniforms and image-samplers are bound by name
            - storage buffer bindings: `layout(std430, binding=0..7)` (common
              bindslot space across shader stages)
            - storage image bindings: `layout(binding=0..3, [access_format])`

    For example code of how to create backend-specific shader objects,
    please refer to the following samples:

        - for D3D11:    https://github.com/floooh/sokol-samples/tree/master/d3d11
        - for Metal:    https://github.com/floooh/sokol-samples/tree/master/metal
        - for OpenGL:   https://github.com/floooh/sokol-samples/tree/master/glfw
        - for GLES3:    https://github.com/floooh/sokol-samples/tree/master/html5
        - for WebGPU:   https://github.com/floooh/sokol-samples/tree/master/wgpu


    ON SG_IMAGESAMPLETYPE_UNFILTERABLE_FLOAT AND SG_SAMPLERTYPE_NONFILTERING
    ========================================================================
    The WebGPU backend introduces the concept of 'unfilterable-float' textures,
    which can only be combined with 'nonfiltering' samplers (this is a restriction
    specific to WebGPU, but since the same sokol-gfx code should work across
    all backend, the sokol-gfx validation layer also enforces this restriction
    - the alternative would be undefined behaviour in some backend APIs on
    some devices).

    The background is that some mobile devices (most notably iOS devices) can
    not perform linear filtering when sampling textures with certain pixel
    formats, most notable the 32F formats:

        - SG_PIXELFORMAT_R32F
        - SG_PIXELFORMAT_RG32F
        - SG_PIXELFORMAT_RGBA32F

    The information of whether a shader is going to be used with such an
    unfilterable-float texture must already be provided in the sg_shader_desc
    struct when creating the shader (see the above section "ON SHADER CREATION").

    If you are using the sokol-shdc shader compiler, the information whether a
    texture/sampler binding expects an 'unfilterable-float/nonfiltering'
    texture/sampler combination cannot be inferred from the shader source
    alone, you'll need to provide this hint via annotation-tags. For instance
    here is an example from the ozz-skin-sapp.c sample shader which samples an
    RGBA32F texture with skinning matrices in the vertex shader:

    ```glsl
    @image_sample_type joint_tex unfilterable_float
    uniform texture2D joint_tex;
    @sampler_type smp nonfiltering
    uniform sampler smp;
    ```

    This will result in SG_IMAGESAMPLETYPE_UNFILTERABLE_FLOAT and
    SG_SAMPLERTYPE_NONFILTERING being written to the code-generated
    sg_shader_desc struct.


    ON VERTEX FORMATS
    =================
    Sokol-gfx implements the same strict mapping rules from CPU-side
    vertex component formats to GPU-side vertex input data types:

    - float and packed normalized CPU-side formats must be used as
      floating point base type in the vertex shader
    - packed signed-integer CPU-side formats must be used as signed
      integer base type in the vertex shader
    - packed unsigned-integer CPU-side formats must be used as unsigned
      integer base type in the vertex shader

    These mapping rules are enforced by the sokol-gfx validation layer,
    but only when sufficient reflection information is provided in
    `sg_shader_desc.attrs[].base_type`. This is the case when sokol-shdc
    is used, otherwise the default base_type will be SG_SHADERATTRBASETYPE_UNDEFINED
    which causes the sokol-gfx validation check to be skipped (of course you
    can also provide the per-attribute base type information manually when
    not using sokol-shdc).

    The detailed mapping rules from SG_VERTEXFORMAT_* to GLSL data types
    are as follows:

    - FLOAT[*] => float, vec*
    - BYTE4N => vec* (scaled to -1.0 .. +1.0)
    - UBYTE4N => vec* (scaled to 0.0 .. +1.0)
    - SHORT[*]N => vec* (scaled to -1.0 .. +1.0)
    - USHORT[*]N => vec* (scaled to 0.0 .. +1.0)
    - INT[*] => int, ivec*
    - UINT[*] => uint, uvec*
    - BYTE4 => int*
    - UBYTE4 => uint*
    - SHORT[*] => int*
    - USHORT[*] => uint*

    NOTE that sokol-gfx only provides vertex formats with sizes of a multiple
    of 4 (e.g. BYTE4N but not BYTE2N). This is because vertex components must
    be 4-byte aligned anyway.


    UNIFORM DATA LAYOUT:
    ====================
    NOTE: if you use the sokol-shdc shader compiler tool, you don't need to worry
    about the following details.

    The data that's passed into the sg_apply_uniforms() function must adhere to
    specific layout rules so that the GPU shader finds the uniform block
    items at the right offset.

    For the D3D11 and Metal backends, sokol-gfx only cares about the size of uniform
    blocks, but not about the internal layout. The data will just be copied into
    a uniform/constant buffer in a single operation and it's up you to arrange the
    CPU-side layout so that it matches the GPU side layout. This also means that with
    the D3D11 and Metal backends you are not limited to a 'cross-platform' subset
    of uniform variable types.

    If you ever only use one of the D3D11, Metal *or* WebGPU backend, you can stop reading here.

    For the GL backends, the internal layout of uniform blocks matters though,
    and you are limited to a small number of uniform variable types. This is
    because sokol-gfx must be able to locate the uniform block members in order
    to upload them to the GPU with glUniformXXX() calls.

    To describe the uniform block layout to sokol-gfx, the following information
    must be passed to the sg_make_shader() call in the sg_shader_desc struct:

        - a hint about the used packing rule (either SG_UNIFORMLAYOUT_NATIVE or
          SG_UNIFORMLAYOUT_STD140)
        - a list of the uniform block members types in the correct order they
          appear on the CPU side

    For example if the GLSL shader has the following uniform declarations:

        uniform mat4 mvp;
        uniform vec2 offset0;
        uniform vec2 offset1;
        uniform vec2 offset2;

    ...and on the CPU side, there's a similar C struct:

        typedef struct {
            float mvp[16];
            float offset0[2];
            float offset1[2];
            float offset2[2];
        } params_t;

    ...the uniform block description in the sg_shader_desc must look like this:

        sg_shader_desc desc = {
            .vs.uniform_blocks[0] = {
                .size = sizeof(params_t),
                .layout = SG_UNIFORMLAYOUT_NATIVE,  // this is the default and can be omitted
                .uniforms = {
                    // order must be the same as in 'params_t':
                    [0] = { .name = "mvp", .type = SG_UNIFORMTYPE_MAT4 },
                    [1] = { .name = "offset0", .type = SG_UNIFORMTYPE_VEC2 },
                    [2] = { .name = "offset1", .type = SG_UNIFORMTYPE_VEC2 },
                    [3] = { .name = "offset2", .type = SG_UNIFORMTYPE_VEC2 },
                }
            }
        };

    With this information sokol-gfx can now compute the correct offsets of the data items
    within the uniform block struct.

    The SG_UNIFORMLAYOUT_NATIVE packing rule works fine if only the GL backends are used,
    but for proper D3D11/Metal/GL a subset of the std140 layout must be used which is
    described in the next section:


    CROSS-BACKEND COMMON UNIFORM DATA LAYOUT
    ========================================
    For cross-platform / cross-3D-backend code it is important that the same uniform block
    layout on the CPU side can be used for all sokol-gfx backends. To achieve this,
    a common subset of the std140 layout must be used:

    - The uniform block layout hint in sg_shader_desc must be explicitly set to
      SG_UNIFORMLAYOUT_STD140.
    - Only the following GLSL uniform types can be used (with their associated sokol-gfx enums):
        - float => SG_UNIFORMTYPE_FLOAT
        - vec2  => SG_UNIFORMTYPE_FLOAT2
        - vec3  => SG_UNIFORMTYPE_FLOAT3
        - vec4  => SG_UNIFORMTYPE_FLOAT4
        - int   => SG_UNIFORMTYPE_INT
        - ivec2 => SG_UNIFORMTYPE_INT2
        - ivec3 => SG_UNIFORMTYPE_INT3
        - ivec4 => SG_UNIFORMTYPE_INT4
        - mat4  => SG_UNIFORMTYPE_MAT4
    - Alignment for those types must be as follows (in bytes):
        - float => 4
        - vec2  => 8
        - vec3  => 16
        - vec4  => 16
        - int   => 4
        - ivec2 => 8
        - ivec3 => 16
        - ivec4 => 16
        - mat4  => 16
    - Arrays are only allowed for the following types: vec4, int4, mat4.

    Note that the HLSL cbuffer layout rules are slightly different from the
    std140 layout rules, this means that the cbuffer declarations in HLSL code
    must be tweaked so that the layout is compatible with std140.

    The by far easiest way to tackle the common uniform block layout problem is
    to use the sokol-shdc shader cross-compiler tool!


    ON STORAGE BUFFERS
    ==================
    The two main purpose of storage buffers are:

        - to be populated by compute shaders with dynamically generated data
        - for providing random-access data to all shader stages

    Storage buffers can be used to pass large amounts of random access structured
    data from the CPU side to the shaders. They are similar to data textures, but are
    more convenient to use both on the CPU and shader side since they can be accessed
    in shaders as as a 1-dimensional array of struct items.

    Storage buffers are *NOT* supported on the following platform/backend combos:

    - macOS+GL (because storage buffers require GL 4.3, while macOS only goes up to GL 4.1)
    - platforms which only support a GLES3.0 context (WebGL2 and iOS)

    To use storage buffers, the following steps are required:

        - write a shader which uses storage buffers (vertex- and fragment-shaders
          can only read from storage buffers, while compute-shaders can both read
          and write storage buffers)
        - create one or more storage buffers via sg_make_buffer() with the
          `.usage.storage_buffer = true`
        - when creating a shader via sg_make_shader(), populate the sg_shader_desc
          struct with binding info (when using sokol-shdc, this step will be taken care
          of automatically)
            - which storage buffer bind slots on the vertex-, fragment- or compute-stage
              are occupied
            - whether the storage buffer on that bind slot is readonly (readonly
              bindings are required for vertex- and fragment-shaders, and in compute
              shaders the readonly flag is used to control hazard tracking in some
              3D backends)

        - when calling sg_apply_bindings(), apply the matching bind slots with the previously
          created storage buffers
        - ...and that's it.

    For more details, see the following backend-agnostic sokol samples:

    - simple vertex pulling from a storage buffer:
        - C code: https://github.com/floooh/sokol-samples/blob/master/sapp/vertexpull-sapp.c
        - shader: https://github.com/floooh/sokol-samples/blob/master/sapp/vertexpull-sapp.glsl
    - instanced rendering via storage buffers (vertex- and instance-pulling):
        - C code: https://github.com/floooh/sokol-samples/blob/master/sapp/instancing-pull-sapp.c
        - shader: https://github.com/floooh/sokol-samples/blob/master/sapp/instancing-pull-sapp.glsl
    - storage buffers both on the vertex- and fragment-stage:
        - C code: https://github.com/floooh/sokol-samples/blob/master/sapp/sbuftex-sapp.c
        - shader: https://github.com/floooh/sokol-samples/blob/master/sapp/sbuftex-sapp.glsl
    - the Ozz animation sample rewritten to pull all rendering data from storage buffers:
        - C code: https://github.com/floooh/sokol-samples/blob/master/sapp/ozz-storagebuffer-sapp.cc
        - shader: https://github.com/floooh/sokol-samples/blob/master/sapp/ozz-storagebuffer-sapp.glsl
    - the instancing sample modified to use compute shaders:
        - C code: https://github.com/floooh/sokol-samples/blob/master/sapp/instancing-compute-sapp.c
        - shader: https://github.com/floooh/sokol-samples/blob/master/sapp/instancing-compute-sapp.glsl
    - the Compute Boids sample ported to sokol-gfx:
        - C code: https://github.com/floooh/sokol-samples/blob/master/sapp/computeboids-sapp.c
        - shader: https://github.com/floooh/sokol-samples/blob/master/sapp/computeboids-sapp.glsl

    ...also see the following backend-specific vertex pulling samples (those also don't use sokol-shdc):

    - D3D11: https://github.com/floooh/sokol-samples/blob/master/d3d11/vertexpulling-d3d11.c
    - desktop GL: https://github.com/floooh/sokol-samples/blob/master/glfw/vertexpulling-glfw.c
    - Metal: https://github.com/floooh/sokol-samples/blob/master/metal/vertexpulling-metal.c
    - WebGPU: https://github.com/floooh/sokol-samples/blob/master/wgpu/vertexpulling-wgpu.c

    ...and the backend specific compute shader samples:

    - D3D11: https://github.com/floooh/sokol-samples/blob/master/d3d11/instancing-compute-d3d11.c
    - desktop GL: https://github.com/floooh/sokol-samples/blob/master/glfw/instancing-compute-glfw.c
    - Metal: https://github.com/floooh/sokol-samples/blob/master/metal/instancing-compute-metal.c
    - WebGPU: https://github.com/floooh/sokol-samples/blob/master/wgpu/instancing-compute-wgpu.c

    Storage buffer shader authoring caveats when using sokol-shdc:

        - declare a read-only storage buffer interface block with `layout(binding=N) readonly buffer [name] { ... }`
          (where 'N' is the index in `sg_bindings.storage_buffers[N]`)
        - ...or a read/write storage buffer interface block with `layout(binding=N) buffer [name] { ... }`
        - declare a struct which describes a single array item in the storage buffer interface block
        - only put a single flexible array member into the storage buffer interface block

    E.g. a complete example in 'sokol-shdc GLSL':

        ```glsl
        @vs
        // declare a struct:
        struct sb_vertex {
            vec3 pos;
            vec4 color;
        }
        // declare a buffer interface block with a single flexible struct array:
        layout(binding=0) readonly buffer vertices {
            sb_vertex vtx[];
        }
        // in the shader function, access the storage buffer like this:
        void main() {
            vec3 pos = vtx[gl_VertexIndex].pos;
            ...
        }
        @end
        ```

    In a compute shader you can read and write the same item in the same
    storage buffer (but you'll have to be careful for random access since
    many threads of the same compute function run in parallel):

        @cs
        struct sb_item {
            vec3 pos;
            vec3 vel;
        }
        layout(binding=0) buffer items_ssbo {
            sb_item items[];
        }
        layout(local_size_x=64, local_size_y=1, local_size_z=1) in;
        void main() {
            uint idx = gl_GlobalInvocationID.x;
            vec3 pos = items[idx].pos;
            ...
            items[idx].pos = pos;
        }
        @end

    Backend-specific storage-buffer caveats (not relevant when using sokol-shdc):

        D3D11:
            - storage buffers are created as 'raw' Byte Address Buffers
              (https://learn.microsoft.com/en-us/windows/win32/direct3d11/overviews-direct3d-11-resources-intro#raw-views-of-buffers)
            - in HLSL, use a ByteAddressBuffer for readonly access of the buffer content:
              (https://learn.microsoft.com/en-us/windows/win32/direct3dhlsl/sm5-object-byteaddressbuffer)
            - ...or RWByteAddressBuffer for read/write access:
              (https://learn.microsoft.com/en-us/windows/win32/direct3dhlsl/sm5-object-rwbyteaddressbuffer)
            - readonly-storage buffers and textures are both bound as 'shader-resource-view' and
              share the same bind slots (declared as `register(tN)` in HLSL), where N must be in the range 0..23)
            - read/write storage buffers and storage images are bound as 'unordered-access-view'
              (declared as `register(uN)` in HLSL where N is in the range 0..11)

        Metal:
            - in Metal there is no internal difference between vertex-, uniform- and
              storage-buffers, all are bound to the same 'buffer bind slots' with the
              following reserved ranges:
                - vertex shader stage:
                    - uniform buffers: slots 0..7
                    - storage buffers: slots 8..15
                    - vertex buffers: slots 15..23
                - fragment shader stage:
                    - uniform buffers: slots 0..7
                    - storage buffers: slots 8..15
            - this means in MSL, storage buffer bindings start at [[buffer(8)]] both in
              the vertex and fragment stage

        GL:
            - the GL backend doesn't use name-lookup to find storage buffer bindings, this
              means you must annotate buffers with `layout(std430, binding=N)` in GLSL
            - ...where N is 0..7 in the vertex shader, and 8..15 in the fragment shader

        WebGPU:
            - in WGSL, textures, samplers and storage buffers all use a shared
              bindspace across all shader stages on bindgroup 1:

              `@group(1) @binding(0..127)

    ON STORAGE IMAGES:
    ==================
    To write pixel data to texture objects in compute shaders, first an image
    object must be created with `storage_image usage`:

        sg_image storage_image = sg_make_image(&(sg_image_desc){
            .usage.storage_image = true,
            },
            .width = ...,
            .height = ...,
            .pixel_format = ...,
        });

    Next a storage-image-view object is required which also allows to pick
    a specific mip-level or slice for the compute-shader to access:

        sg_view simg_view = sg_make_view(&(sg_view_desc){
            .storage_image = {
                .image = storage_image,
                .mip_level = ...,
                .slice = ...
            },
        });

    Finally 'bind' the storage-image-view via a regular sg_apply_bindings() call
    inside a compute pass:

        sg_begin_pass(&(sg_pass){ .compute = true });
        sg_apply_pipeline(...);
        sg_apply_bindings(&(sg_bindings){
            .views[VIEW_simg] = simg_view,
        });
        sg_dispatch(...);
        sg_end_pass();

    Currently, storage images can only be used with `readwrite` or `writeonly` access in
    shaders. For readonly access use a regular texture binding instead.

    For an example of using storage images in compute shaders see imageblur-sapp:

        - C code: https://github.com/floooh/sokol-samples/blob/master/sapp/imageblur-sapp.c
        - shader: https://github.com/floooh/sokol-samples/blob/master/sapp/imageblur-sapp.glsl

    TRACE HOOKS:
    ============
    sokol_gfx.h optionally allows to install "trace hook" callbacks for
    each public API functions. When a public API function is called, and
    a trace hook callback has been installed for this function, the
    callback will be invoked with the parameters and result of the function.
    This is useful for things like debugging- and profiling-tools, or
    keeping track of resource creation and destruction.

    To use the trace hook feature:

    --- Define SOKOL_TRACE_HOOKS before including the implementation.

    --- Setup an sg_trace_hooks structure with your callback function
        pointers (keep all function pointers you're not interested
        in zero-initialized), optionally set the user_data member
        in the sg_trace_hooks struct.

    --- Install the trace hooks by calling sg_install_trace_hooks(),
        the return value of this function is another sg_trace_hooks
        struct which contains the previously set of trace hooks.
        You should keep this struct around, and call those previous
        functions pointers from your own trace callbacks for proper
        chaining.

    As an example of how trace hooks are used, have a look at the
    imgui/sokol_gfx_imgui.h header which implements a realtime
    debugging UI for sokol_gfx.h on top of Dear ImGui.


    MEMORY ALLOCATION OVERRIDE
    ==========================
    You can override the memory allocation functions at initialization time
    like this:

        void* my_alloc(size_t size, void* user_data) {
            return malloc(size);
        }

        void my_free(void* ptr, void* user_data) {
            free(ptr);
        }

        ...
            sg_setup(&(sg_desc){
                // ...
                .allocator = {
                    .alloc_fn = my_alloc,
                    .free_fn = my_free,
                    .user_data = ...,
                }
            });
        ...

    If no overrides are provided, malloc and free will be used.

    This only affects memory allocation calls done by sokol_gfx.h
    itself though, not any allocations in OS libraries.


    ERROR REPORTING AND LOGGING
    ===========================
    To get any logging information at all you need to provide a logging callback in the setup call
    the easiest way is to use sokol_log.h:

        #include "sokol_log.h"

        sg_setup(&(sg_desc){ .logger.func = slog_func });

    To override logging with your own callback, first write a logging function like this:

        void my_log(const char* tag,                // e.g. 'sg'
                    uint32_t log_level,             // 0=panic, 1=error, 2=warn, 3=info
                    uint32_t log_item_id,           // SG_LOGITEM_*
                    const char* message_or_null,    // a message string, may be nullptr in release mode
                    uint32_t line_nr,               // line number in sokol_gfx.h
                    const char* filename_or_null,   // source filename, may be nullptr in release mode
                    void* user_data)
        {
            ...
        }

    ...and then setup sokol-gfx like this:

        sg_setup(&(sg_desc){
            .logger = {
                .func = my_log,
                .user_data = my_user_data,
            }
        });

    The provided logging function must be reentrant (e.g. be callable from
    different threads).

    If you don't want to provide your own custom logger it is highly recommended to use
    the standard logger in sokol_log.h instead, otherwise you won't see any warnings or
    errors.


    COMMIT LISTENERS
    ================
    It's possible to hook callback functions into sokol-gfx which are called from
    inside sg_commit() in unspecified order. This is mainly useful for libraries
    that build on top of sokol_gfx.h to be notified about the end/start of a frame.

    To add a commit listener, call:

        static void my_commit_listener(void* user_data) {
            ...
        }

        bool success = sg_add_commit_listener((sg_commit_listener){
            .func = my_commit_listener,
            .user_data = ...,
        });

    The function returns false if the internal array of commit listeners is full,
    or the same commit listener had already been added.

    If the function returns true, my_commit_listener() will be called each frame
    from inside sg_commit().

    By default, 1024 distinct commit listeners can be added, but this number
    can be tweaked in the sg_setup() call:

        sg_setup(&(sg_desc){
            .max_commit_listeners = 2048,
        });

    An sg_commit_listener item is equal to another if both the function
    pointer and user_data field are equal.

    To remove a commit listener:

        bool success = sg_remove_commit_listener((sg_commit_listener){
            .func = my_commit_listener,
            .user_data = ...,
        });

    ...where the .func and .user_data field are equal to a previous
    sg_add_commit_listener() call. The function returns true if the commit
    listener item was found and removed, and false otherwise.


    RESOURCE CREATION AND DESTRUCTION IN DETAIL
    ===========================================
    The 'vanilla' way to create resource objects is with the 'make functions':

        sg_buffer sg_make_buffer(const sg_buffer_desc* desc)
        sg_image sg_make_image(const sg_image_desc* desc)
        sg_sampler sg_make_sampler(const sg_sampler_desc* desc)
        sg_shader sg_make_shader(const sg_shader_desc* desc)
        sg_pipeline sg_make_pipeline(const sg_pipeline_desc* desc)
        sg_view sg_make_view(const sg_view_desc* desc)

    This will result in one of three cases:

        1. The returned handle is invalid. This happens when there are no more
           free slots in the resource pool for this resource type. An invalid
           handle is associated with the INVALID resource state, for instance:

                sg_buffer buf = sg_make_buffer(...)
                if (sg_query_buffer_state(buf) == SG_RESOURCESTATE_INVALID) {
                    // buffer pool is exhausted
                }

        2. The returned handle is valid, but creating the underlying resource
           has failed for some reason. This results in a resource object in the
           FAILED state. The reason *why* resource creation has failed differ
           by resource type. Look for log messages with more details. A failed
           resource state can be checked with:

                sg_buffer buf = sg_make_buffer(...)
                if (sg_query_buffer_state(buf) == SG_RESOURCESTATE_FAILED) {
                    // creating the resource has failed
                }

        3. And finally, if everything goes right, the returned resource is
           in resource state VALID and ready to use. This can be checked
           with:

                sg_buffer buf = sg_make_buffer(...)
                if (sg_query_buffer_state(buf) == SG_RESOURCESTATE_VALID) {
                    // creating the resource has failed
                }

    When calling the 'make functions', the created resource goes through a number
    of states:

        - INITIAL: the resource slot associated with the new resource is currently
          free (technically, there is no resource yet, just an empty pool slot)
        - ALLOC: a handle for the new resource has been allocated, this just means
          a pool slot has been reserved.
        - VALID or FAILED: in VALID state any 3D API backend resource objects have
          been successfully created, otherwise if anything went wrong, the resource
          will be in FAILED state.

    Sometimes it makes sense to first grab a handle, but initialize the
    underlying resource at a later time. For instance when loading data
    asynchronously from a slow data source, you may know what buffers and
    textures are needed at an early stage of the loading process, but actually
    loading the buffer or texture content can only be completed at a later time.

    For such situations, sokol-gfx resource objects can be created in two steps.
    You can allocate a handle upfront with one of the 'alloc functions':

        sg_buffer sg_alloc_buffer(void)
        sg_image sg_alloc_image(void)
        sg_sampler sg_alloc_sampler(void)
        sg_shader sg_alloc_shader(void)
        sg_pipeline sg_alloc_pipeline(void)
        sg_view sg_alloc_view(void)

    This will return a handle with the underlying resource object in the
    ALLOC state:

        sg_image img = sg_alloc_image();
        if (sg_query_image_state(img) == SG_RESOURCESTATE_ALLOC) {
            // allocating an image handle has succeeded, otherwise
            // the image pool is full
        }

    Such an 'incomplete' handle can be used in most sokol-gfx rendering functions
    without doing any harm, sokol-gfx will simply skip any rendering operation
    that involve resources which are not in VALID state.

    At a later time (for instance once the texture has completed loading
    asynchronously), the resource creation can be completed by calling one of
    the 'init functions', those functions take an existing resource handle and
    'desc struct':

        void sg_init_buffer(sg_buffer buf, const sg_buffer_desc* desc)
        void sg_init_image(sg_image img, const sg_image_desc* desc)
        void sg_init_sampler(sg_sampler smp, const sg_sampler_desc* desc)
        void sg_init_shader(sg_shader shd, const sg_shader_desc* desc)
        void sg_init_pipeline(sg_pipeline pip, const sg_pipeline_desc* desc)
        void sg_init_view(sg_view view, const sg_view_desc* desc)

    The init functions expect a resource in ALLOC state, and after the function
    returns, the resource will be either in VALID or FAILED state. Calling
    an 'alloc function' followed by the matching 'init function' is fully
    equivalent with calling the 'make function' alone.

    Destruction can also happen as a two-step process. The 'uninit functions'
    will put a resource object from the VALID or FAILED state back into the
    ALLOC state:

        void sg_uninit_buffer(sg_buffer buf)
        void sg_uninit_image(sg_image img)
        void sg_uninit_sampler(sg_sampler smp)
        void sg_uninit_shader(sg_shader shd)
        void sg_uninit_pipeline(sg_pipeline pip)
        void sg_uninit_view(sg_view view)

    Calling the 'uninit functions' with a resource that is not in the VALID or
    FAILED state is a no-op.

    To finally free the pool slot for recycling call the 'dealloc functions':

        void sg_dealloc_buffer(sg_buffer buf)
        void sg_dealloc_image(sg_image img)
        void sg_dealloc_sampler(sg_sampler smp)
        void sg_dealloc_shader(sg_shader shd)
        void sg_dealloc_pipeline(sg_pipeline pip)
        void sg_dealloc_view(sg_view view)

    Calling the 'dealloc functions' on a resource that's not in ALLOC state is
    a no-op, but will generate a warning log message.

    Calling an 'uninit function' and 'dealloc function' in sequence is equivalent
    with calling the associated 'destroy function':

        void sg_destroy_buffer(sg_buffer buf)
        void sg_destroy_image(sg_image img)
        void sg_destroy_sampler(sg_sampler smp)
        void sg_destroy_shader(sg_shader shd)
        void sg_destroy_pipeline(sg_pipeline pip)
        void sg_destroy_view(sg_view view)

    The 'destroy functions' can be called on resources in any state and generally
    do the right thing (for instance if the resource is in ALLOC state, the destroy
    function will be equivalent to the 'dealloc function' and skip the 'uninit part').

    And finally to close the circle, the 'fail functions' can be called to manually
    put a resource in ALLOC state into the FAILED state:

        sg_fail_buffer(sg_buffer buf)
        sg_fail_image(sg_image img)
        sg_fail_sampler(sg_sampler smp)
        sg_fail_shader(sg_shader shd)
        sg_fail_pipeline(sg_pipeline pip)
        sg_fail_view(sg_view view)

    This is recommended if anything went wrong outside of sokol-gfx during asynchronous
    resource setup (for instance a file loading operation failed). In this case,
    the 'fail function' should be called instead of the 'init function'.

    Calling a 'fail function' on a resource that's not in ALLOC state is a no-op,
    but will generate a warning log message.

    NOTE: that two-step resource creation usually only makes sense for buffers,
    images and views, but not for samplers, shaders or pipelines. Most notably, trying
    to create a pipeline object with a shader that's not in VALID state will
    trigger a validation layer error, or if the validation layer is disabled,
    result in a pipeline object in FAILED state.


    WEBGPU CAVEATS
    ==============
    For a general overview and design notes of the WebGPU backend see:

        https://floooh.github.io/2023/10/16/sokol-webgpu.html

    In general, don't expect an automatic speedup when switching from the WebGL2
    backend to the WebGPU backend. Some WebGPU functions currently actually
    have a higher CPU overhead than similar WebGL2 functions, leading to the
    paradoxical situation that some WebGPU code may be slower than similar WebGL2
    code.

    - when writing WGSL shader code by hand, a specific bind-slot convention
      must be used:

      All uniform block structs must use `@group(0)` and bindings in the
      range 0..15

        @group(0) @binding(0..15)

      All textures, samplers, storage-buffers and storage-images must use `@group(1)`
      and bindings must be in the range 0..127:

        @group(1) @binding(0..127)

      Note that the number of texture, sampler, storage-buffer storage-image bindings
      is still limited despite the large bind range:

        - up to 16 textures and sampler across all shader stages
        - up to 8 storage buffers across all shader stages
        - up to 4 storage images on the compute shader stage

      If you use sokol-shdc to generate WGSL shader code, you don't need to worry
      about the above binding conventions since sokol-shdc will allocate
      the WGSL bindslots).

    - The sokol-gfx WebGPU backend uses the sg_desc.uniform_buffer_size item
      to allocate a single per-frame uniform buffer which must be big enough
      to hold all data written by sg_apply_uniforms() during a single frame,
      including a worst-case 256-byte alignment (e.g. each sg_apply_uniform
      call will cost at least 256 bytes of uniform buffer size). The default size
      is 4 MB, which is enough for 16384 sg_apply_uniform() calls per
      frame (assuming the uniform data 'payload' is less than 256 bytes
      per call). These rules are the same as for the Metal backend, so if
      you are already using the Metal backend you'll be fine.

    - sg_apply_bindings(): the sokol-gfx WebGPU backend implements a bindgroup
      cache to prevent excessive creation and destruction of BindGroup objects
      when calling sg_apply_bindings(). The number of slots in the bindgroups
      cache is defined in sg_desc.wgpu_bindgroups_cache_size when calling
      sg_setup. The cache size must be a power-of-2 number, with the default being
      1024. The bindgroups cache behaviour can be observed by calling the new
      function sg_query_frame_stats(), where the following struct items are
      of interest:

        .wgpu.num_bindgroup_cache_hits
        .wgpu.num_bindgroup_cache_misses
        .wgpu.num_bindgroup_cache_collisions
        .wgpu_num_bindgroup_cache_invalidates
        .wgpu.num_bindgroup_cache_vs_hash_key_mismatch

      The value to pay attention to is `.wgpu.num_bindgroup_cache_collisions`,
      if this number is consistently higher than a few percent of the
      .wgpu.num_set_bindgroup value, it might be a good idea to bump the
      bindgroups cache size to the next power-of-2.

    - sg_apply_viewport(): WebGPU currently has a unique restriction that viewport
      rectangles must be contained entirely within the framebuffer. As a shitty
      workaround sokol_gfx.h will clip incoming viewport rectangles against
      the framebuffer, but this will distort the clipspace-to-screenspace mapping.
      There's no proper way to handle this inside sokol_gfx.h, this must be fixed
      in a future WebGPU update (see: https://github.com/gpuweb/gpuweb/issues/373
      and https://github.com/gpuweb/gpuweb/pull/5025)

    - The sokol shader compiler generally adds `diagnostic(off, derivative_uniformity);`
      into the WGSL output. Currently only the Chrome WebGPU implementation seems
      to recognize this.

    - Likewise, the following sokol-gfx pixel formats are not supported in WebGPU:
      R16, R16SN, RG16, RG16SN, RGBA16, RGBA16SN.
      Unlike unsupported vertex formats, unsupported pixel formats can be queried
      in cross-backend code via sg_query_pixelformat() though.

    - The Emscripten WebGPU shim currently doesn't support the Closure minification
      post-link-step (e.g. currently the emcc argument '--closure 1' or '--closure 2'
      will generate broken Javascript code.

    - sokol-gfx requires the WebGPU device feature `depth32float-stencil8` to be enabled
      (this should be widely supported)

    - sokol-gfx expects that the WebGPU device feature `float32-filterable` to *not* be
      enabled (since this would exclude all iOS devices)


    LICENSE
    =======
    zlib/libpng license

    Copyright (c) 2018 Andre Weissflog

    This software is provided 'as-is', without any express or implied warranty.
    In no event will the authors be held liable for any damages arising from the
    use of this software.

    Permission is granted to anyone to use this software for any purpose,
    including commercial applications, and to alter it and redistribute it
    freely, subject to the following restrictions:

        1. The origin of this software must not be misrepresented; you must not
        claim that you wrote the original software. If you use this software in a
        product, an acknowledgment in the product documentation would be
        appreciated but is not required.

        2. Altered source versions must be plainly marked as such, and must not
        be misrepresented as being the original software.

        3. This notice may not be removed or altered from any source
        distribution.
*/
#define SOKOL_GFX_INCLUDED (1)
#include <stddef.h>     // size_t
#include <stdint.h>
#include <stdbool.h>

#if defined(SOKOL_API_DECL) && !defined(SOKOL_GFX_API_DECL)
#define SOKOL_GFX_API_DECL SOKOL_API_DECL
#endif
#ifndef SOKOL_GFX_API_DECL
#if defined(_WIN32) && defined(SOKOL_DLL) && defined(SOKOL_GFX_IMPL)
#define SOKOL_GFX_API_DECL __declspec(dllexport)
#elif defined(_WIN32) && defined(SOKOL_DLL)
#define SOKOL_GFX_API_DECL __declspec(dllimport)
#else
#define SOKOL_GFX_API_DECL extern
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*
    Resource id typedefs:

    sg_buffer:      vertex- and index-buffers
    sg_image:       images used as textures and render-pass attachments
    sg_sampler      sampler objects describing how a texture is sampled in a shader
    sg_shader:      vertex- and fragment-shaders and shader interface information
    sg_pipeline:    associated shader and vertex-layouts, and render states
    sg_view:        a resource view object used for bindings and render-pass attachments

    Instead of pointers, resource creation functions return a 32-bit
    handle which uniquely identifies the resource object.

    The 32-bit resource id is split into a 16-bit pool index in the lower bits,
    and a 16-bit 'generation counter' in the upper bits. The index allows fast
    pool lookups, and combined with the generation-counter it allows to detect
    'dangling accesses' (trying to use an object which no longer exists, and
    its pool slot has been reused for a new object)

    The resource ids are wrapped into a strongly-typed struct so that
    trying to pass an incompatible resource id is a compile error.
*/
typedef struct sg_buffer        { uint32_t id; } sg_buffer;
typedef struct sg_image         { uint32_t id; } sg_image;
typedef struct sg_sampler       { uint32_t id; } sg_sampler;
typedef struct sg_shader        { uint32_t id; } sg_shader;
typedef struct sg_pipeline      { uint32_t id; } sg_pipeline;
typedef struct sg_view          { uint32_t id; } sg_view;

/*
    sg_range is a pointer-size-pair struct used to pass memory blobs into
    sokol-gfx. When initialized from a value type (array or struct), you can
    use the SG_RANGE() macro to build an sg_range struct. For functions which
    take either a sg_range pointer, or a (C++) sg_range reference, use the
    SG_RANGE_REF macro as a solution which compiles both in C and C++.
*/
typedef struct sg_range {
    const void* ptr;
    size_t size;
} sg_range;

// disabling this for every includer isn't great, but the warnings are also quite pointless
#if defined(_MSC_VER)
#pragma warning(disable:4221)   // /W4 only: nonstandard extension used: 'x': cannot be initialized using address of automatic variable 'y'
#pragma warning(disable:4204)   // VS2015: nonstandard extension used: non-constant aggregate initializer
#endif
#if defined(__cplusplus)
#define SG_RANGE(x) sg_range{ &x, sizeof(x) }
#define SG_RANGE_REF(x) sg_range{ &x, sizeof(x) }
#else
#define SG_RANGE(x) (sg_range){ &x, sizeof(x) }
#define SG_RANGE_REF(x) &(sg_range){ &x, sizeof(x) }
#endif

// various compile-time constants in the public API
enum {
    SG_INVALID_ID = 0,
    SG_NUM_INFLIGHT_FRAMES = 2,
    SG_MAX_COLOR_ATTACHMENTS = 4,
    SG_MAX_UNIFORMBLOCK_MEMBERS = 16,
    SG_MAX_VERTEX_ATTRIBUTES = 16,
    SG_MAX_MIPMAPS = 16,
    SG_MAX_TEXTUREARRAY_LAYERS = 128,
    SG_MAX_VERTEXBUFFER_BINDSLOTS = 8,
    SG_MAX_UNIFORMBLOCK_BINDSLOTS = 8,
    SG_MAX_VIEW_BINDSLOTS = 28,
    SG_MAX_SAMPLER_BINDSLOTS = 16,
    SG_MAX_TEXTURE_SAMPLER_PAIRS = 16,
};

/*
    sg_color

    An RGBA color value.
*/
typedef struct sg_color { float r, g, b, a; } sg_color;

/*
    sg_backend

    The active 3D-API backend, use the function sg_query_backend()
    to get the currently active backend.
*/
typedef enum sg_backend {
    SG_BACKEND_GLCORE,
    SG_BACKEND_GLES3,
    SG_BACKEND_D3D11,
    SG_BACKEND_METAL_IOS,
    SG_BACKEND_METAL_MACOS,
    SG_BACKEND_METAL_SIMULATOR,
    SG_BACKEND_WGPU,
    SG_BACKEND_DUMMY,
} sg_backend;

/*
    sg_pixel_format

    sokol_gfx.h basically uses the same pixel formats as WebGPU, since these
    are supported on most newer GPUs.

    A pixelformat name consist of three parts:

        - components (R, RG, RGB or RGBA)
        - bit width per component (8, 16 or 32)
        - component data type:
            - unsigned normalized (no postfix)
            - signed normalized (SN postfix)
            - unsigned integer (UI postfix)
            - signed integer (SI postfix)
            - float (F postfix)

    Not all pixel formats can be used for everything, call sg_query_pixelformat()
    to inspect the capabilities of a given pixelformat. The function returns
    an sg_pixelformat_info struct with the following members:

        - sample: the pixelformat can be sampled as texture at least with
                  nearest filtering
        - filter: the pixelformat can be sampled as texture with linear
                  filtering
        - render: the pixelformat can be used as render-pass attachment
        - blend:  blending is supported when used as render-pass attachment
        - msaa:   multisample-antialiasing is supported when used
                  as render-pass attachment
        - depth:  the pixelformat can be used for depth-stencil attachments
        - compressed: this is a block-compressed format
        - bytes_per_pixel: the numbers of bytes in a pixel (0 for compressed formats)

    The default pixel format for texture images is SG_PIXELFORMAT_RGBA8.

    The default pixel format for render target images is platform-dependent
    and taken from the sg_environment struct passed into sg_setup(). Typically
    the default formats are:

        - for the Metal, D3D11 and WebGPU backends: SG_PIXELFORMAT_BGRA8
        - for GL backends: SG_PIXELFORMAT_RGBA8
*/
typedef enum sg_pixel_format {
    _SG_PIXELFORMAT_DEFAULT,    // value 0 reserved for default-init
    SG_PIXELFORMAT_NONE,

    SG_PIXELFORMAT_R8,
    SG_PIXELFORMAT_R8SN,
    SG_PIXELFORMAT_R8UI,
    SG_PIXELFORMAT_R8SI,

    SG_PIXELFORMAT_R16,
    SG_PIXELFORMAT_R16SN,
    SG_PIXELFORMAT_R16UI,
    SG_PIXELFORMAT_R16SI,
    SG_PIXELFORMAT_R16F,
    SG_PIXELFORMAT_RG8,
    SG_PIXELFORMAT_RG8SN,
    SG_PIXELFORMAT_RG8UI,
    SG_PIXELFORMAT_RG8SI,

    SG_PIXELFORMAT_R32UI,
    SG_PIXELFORMAT_R32SI,
    SG_PIXELFORMAT_R32F,
    SG_PIXELFORMAT_RG16,
    SG_PIXELFORMAT_RG16SN,
    SG_PIXELFORMAT_RG16UI,
    SG_PIXELFORMAT_RG16SI,
    SG_PIXELFORMAT_RG16F,
    SG_PIXELFORMAT_RGBA8,
    SG_PIXELFORMAT_SRGB8A8,
    SG_PIXELFORMAT_RGBA8SN,
    SG_PIXELFORMAT_RGBA8UI,
    SG_PIXELFORMAT_RGBA8SI,
    SG_PIXELFORMAT_BGRA8,
    SG_PIXELFORMAT_RGB10A2,
    SG_PIXELFORMAT_RG11B10F,
    SG_PIXELFORMAT_RGB9E5,

    SG_PIXELFORMAT_RG32UI,
    SG_PIXELFORMAT_RG32SI,
    SG_PIXELFORMAT_RG32F,
    SG_PIXELFORMAT_RGBA16,
    SG_PIXELFORMAT_RGBA16SN,
    SG_PIXELFORMAT_RGBA16UI,
    SG_PIXELFORMAT_RGBA16SI,
    SG_PIXELFORMAT_RGBA16F,

    SG_PIXELFORMAT_RGBA32UI,
    SG_PIXELFORMAT_RGBA32SI,
    SG_PIXELFORMAT_RGBA32F,

    // NOTE: when adding/removing pixel formats before DEPTH, also update sokol_app.h/_SAPP_PIXELFORMAT_*
    SG_PIXELFORMAT_DEPTH,
    SG_PIXELFORMAT_DEPTH_STENCIL,

    // NOTE: don't put any new compressed format in front of here
    SG_PIXELFORMAT_BC1_RGBA,
    SG_PIXELFORMAT_BC2_RGBA,
    SG_PIXELFORMAT_BC3_RGBA,
    SG_PIXELFORMAT_BC3_SRGBA,
    SG_PIXELFORMAT_BC4_R,
    SG_PIXELFORMAT_BC4_RSN,
    SG_PIXELFORMAT_BC5_RG,
    SG_PIXELFORMAT_BC5_RGSN,
    SG_PIXELFORMAT_BC6H_RGBF,
    SG_PIXELFORMAT_BC6H_RGBUF,
    SG_PIXELFORMAT_BC7_RGBA,
    SG_PIXELFORMAT_BC7_SRGBA,
    SG_PIXELFORMAT_ETC2_RGB8,
    SG_PIXELFORMAT_ETC2_SRGB8,
    SG_PIXELFORMAT_ETC2_RGB8A1,
    SG_PIXELFORMAT_ETC2_RGBA8,
    SG_PIXELFORMAT_ETC2_SRGB8A8,
    SG_PIXELFORMAT_EAC_R11,
    SG_PIXELFORMAT_EAC_R11SN,
    SG_PIXELFORMAT_EAC_RG11,
    SG_PIXELFORMAT_EAC_RG11SN,

    SG_PIXELFORMAT_ASTC_4x4_RGBA,
    SG_PIXELFORMAT_ASTC_4x4_SRGBA,

    _SG_PIXELFORMAT_NUM,
    _SG_PIXELFORMAT_FORCE_U32 = 0x7FFFFFFF
} sg_pixel_format;

/*
    Runtime information about a pixel format, returned by sg_query_pixelformat().
*/
typedef struct sg_pixelformat_info {
    bool sample;            // pixel format can be sampled in shaders at least with nearest filtering
    bool filter;            // pixel format can be sampled with linear filtering
    bool render;            // pixel format can be used as render-pass attachment
    bool blend;             // pixel format supports alpha-blending when used as render-pass attachment
    bool msaa;              // pixel format supports MSAA when used as render-pass attachment
    bool depth;             // pixel format is a depth format
    bool compressed;        // true if this is a hardware-compressed format
    bool read;              // true if format supports compute shader read access
    bool write;             // true if format supports compute shader write access
    int bytes_per_pixel;    // NOTE: this is 0 for compressed formats, use sg_query_row_pitch() / sg_query_surface_pitch() as alternative
} sg_pixelformat_info;

/*
    Runtime information about available optional features, returned by sg_query_features()
*/
typedef struct sg_features {
    bool origin_top_left;               // framebuffer- and texture-origin is in top left corner
    bool image_clamp_to_border;         // border color and clamp-to-border uv-wrap mode is supported
    bool mrt_independent_blend_state;   // multiple-render-target rendering can use per-render-target blend state
    bool mrt_independent_write_mask;    // multiple-render-target rendering can use per-render-target color write masks
    bool compute;                       // storage buffers and compute shaders are supported
    bool msaa_texture_bindings;         // if true, multisampled images can be bound as textures
    bool separate_buffer_types;         // cannot use the same buffer for vertex and indices (only WebGL2)
    bool gl_texture_views;              // supports 'proper' texture views (GL 4.3+)
} sg_features;

/*
    Runtime information about resource limits, returned by sg_query_limit()
*/
typedef struct sg_limits {
    int max_image_size_2d;          // max width/height of SG_IMAGETYPE_2D images
    int max_image_size_cube;        // max width/height of SG_IMAGETYPE_CUBE images
    int max_image_size_3d;          // max width/height/depth of SG_IMAGETYPE_3D images
    int max_image_size_array;       // max width/height of SG_IMAGETYPE_ARRAY images
    int max_image_array_layers;     // max number of layers in SG_IMAGETYPE_ARRAY images
    int max_vertex_attrs;           // max number of vertex attributes, clamped to SG_MAX_VERTEX_ATTRIBUTES
    int gl_max_vertex_uniform_components;    // <= GL_MAX_VERTEX_UNIFORM_COMPONENTS (only on GL backends)
    int gl_max_combined_texture_image_units; // <= GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS (only on GL backends)
} sg_limits;

/*
    sg_resource_state

    The current state of a resource in its resource pool.
    Resources start in the INITIAL state, which means the
    pool slot is unoccupied and can be allocated. When a resource is
    created, first an id is allocated, and the resource pool slot
    is set to state ALLOC. After allocation, the resource is
    initialized, which may result in the VALID or FAILED state. The
    reason why allocation and initialization are separate is because
    some resource types (e.g. buffers and images) might be asynchronously
    initialized by the user application. If a resource which is not
    in the VALID state is attempted to be used for rendering, rendering
    operations will silently be dropped.

    The special INVALID state is returned in sg_query_xxx_state() if no
    resource object exists for the provided resource id.
*/
typedef enum sg_resource_state {
    SG_RESOURCESTATE_INITIAL,
    SG_RESOURCESTATE_ALLOC,
    SG_RESOURCESTATE_VALID,
    SG_RESOURCESTATE_FAILED,
    SG_RESOURCESTATE_INVALID,
    _SG_RESOURCESTATE_FORCE_U32 = 0x7FFFFFFF
} sg_resource_state;

/*
    sg_index_type

    Indicates whether indexed rendering (fetching vertex-indices from an
    index buffer) is used, and if yes, the index data type (16- or 32-bits).

    This is used in the sg_pipeline_desc.index_type member when creating a
    pipeline object.

    The default index type is SG_INDEXTYPE_NONE.
*/
typedef enum sg_index_type {
    _SG_INDEXTYPE_DEFAULT,   // value 0 reserved for default-init
    SG_INDEXTYPE_NONE,
    SG_INDEXTYPE_UINT16,
    SG_INDEXTYPE_UINT32,
    _SG_INDEXTYPE_NUM,
    _SG_INDEXTYPE_FORCE_U32 = 0x7FFFFFFF
} sg_index_type;

/*
    sg_image_type

    Indicates the basic type of an image object (2D-texture, cubemap,
    3D-texture or 2D-array-texture). Used in the sg_image_desc.type member when
    creating an image, and in sg_shader_image_desc to describe a sampled texture
    in the shader (both must match and will be checked in the validation layer
    when calling sg_apply_bindings).

    The default image type when creating an image is SG_IMAGETYPE_2D.
*/
typedef enum sg_image_type {
    _SG_IMAGETYPE_DEFAULT,  // value 0 reserved for default-init
    SG_IMAGETYPE_2D,
    SG_IMAGETYPE_CUBE,
    SG_IMAGETYPE_3D,
    SG_IMAGETYPE_ARRAY,
    _SG_IMAGETYPE_NUM,
    _SG_IMAGETYPE_FORCE_U32 = 0x7FFFFFFF
} sg_image_type;

/*
    sg_image_sample_type

    The basic data type of a texture sample as expected by a shader.
    Must be provided in sg_shader_image and used by the validation
    layer in sg_apply_bindings() to check if the provided image object
    is compatible with what the shader expects. Apart from the sokol-gfx
    validation layer, WebGPU is the only backend API which actually requires
    matching texture and sampler type to be provided upfront for validation
    (other 3D APIs treat texture/sampler type mismatches as undefined behaviour).

    NOTE that the following texture pixel formats require the use
    of SG_IMAGESAMPLETYPE_UNFILTERABLE_FLOAT, combined with a sampler
    of type SG_SAMPLERTYPE_NONFILTERING:

    - SG_PIXELFORMAT_R32F
    - SG_PIXELFORMAT_RG32F
    - SG_PIXELFORMAT_RGBA32F

    (when using sokol-shdc, also check out the meta tags `@image_sample_type`
    and `@sampler_type`)
*/
typedef enum sg_image_sample_type {
    _SG_IMAGESAMPLETYPE_DEFAULT,  // value 0 reserved for default-init
    SG_IMAGESAMPLETYPE_FLOAT,
    SG_IMAGESAMPLETYPE_DEPTH,
    SG_IMAGESAMPLETYPE_SINT,
    SG_IMAGESAMPLETYPE_UINT,
    SG_IMAGESAMPLETYPE_UNFILTERABLE_FLOAT,
    _SG_IMAGESAMPLETYPE_NUM,
    _SG_IMAGESAMPLETYPE_FORCE_U32 = 0x7FFFFFFF
} sg_image_sample_type;

/*
    sg_sampler_type

    The basic type of a texture sampler (sampling vs comparison) as
    defined in a shader. Must be provided in sg_shader_sampler_desc.

    sg_image_sample_type and sg_sampler_type for a texture/sampler
    pair must be compatible with each other, specifically only
    the following pairs are allowed:

    - SG_IMAGESAMPLETYPE_FLOAT => (SG_SAMPLERTYPE_FILTERING or SG_SAMPLERTYPE_NONFILTERING)
    - SG_IMAGESAMPLETYPE_UNFILTERABLE_FLOAT => SG_SAMPLERTYPE_NONFILTERING
    - SG_IMAGESAMPLETYPE_SINT => SG_SAMPLERTYPE_NONFILTERING
    - SG_IMAGESAMPLETYPE_UINT => SG_SAMPLERTYPE_NONFILTERING
    - SG_IMAGESAMPLETYPE_DEPTH => SG_SAMPLERTYPE_COMPARISON
*/
typedef enum sg_sampler_type {
    _SG_SAMPLERTYPE_DEFAULT,
    SG_SAMPLERTYPE_FILTERING,
    SG_SAMPLERTYPE_NONFILTERING,
    SG_SAMPLERTYPE_COMPARISON,
    _SG_SAMPLERTYPE_NUM,
    _SG_SAMPLERTYPE_FORCE_U32,
} sg_sampler_type;

/*
    sg_cube_face

    The cubemap faces. Use these as indices in the sg_image_desc.content
    array.
*/
typedef enum sg_cube_face {
    SG_CUBEFACE_POS_X,
    SG_CUBEFACE_NEG_X,
    SG_CUBEFACE_POS_Y,
    SG_CUBEFACE_NEG_Y,
    SG_CUBEFACE_POS_Z,
    SG_CUBEFACE_NEG_Z,
    SG_CUBEFACE_NUM,
    _SG_CUBEFACE_FORCE_U32 = 0x7FFFFFFF
} sg_cube_face;

/*
    sg_primitive_type

    This is the common subset of 3D primitive types supported across all 3D
    APIs. This is used in the sg_pipeline_desc.primitive_type member when
    creating a pipeline object.

    The default primitive type is SG_PRIMITIVETYPE_TRIANGLES.
*/
typedef enum sg_primitive_type {
    _SG_PRIMITIVETYPE_DEFAULT,  // value 0 reserved for default-init
    SG_PRIMITIVETYPE_POINTS,
    SG_PRIMITIVETYPE_LINES,
    SG_PRIMITIVETYPE_LINE_STRIP,
    SG_PRIMITIVETYPE_TRIANGLES,
    SG_PRIMITIVETYPE_TRIANGLE_STRIP,
    _SG_PRIMITIVETYPE_NUM,
    _SG_PRIMITIVETYPE_FORCE_U32 = 0x7FFFFFFF
} sg_primitive_type;

/*
    sg_filter

    The filtering mode when sampling a texture image. This is
    used in the sg_sampler_desc.min_filter, sg_sampler_desc.mag_filter
    and sg_sampler_desc.mipmap_filter members when creating a sampler object.

    For the default is SG_FILTER_NEAREST.
*/
typedef enum sg_filter {
    _SG_FILTER_DEFAULT, // value 0 reserved for default-init
    SG_FILTER_NEAREST,
    SG_FILTER_LINEAR,
    _SG_FILTER_NUM,
    _SG_FILTER_FORCE_U32 = 0x7FFFFFFF
} sg_filter;

/*
    sg_wrap

    The texture coordinates wrapping mode when sampling a texture
    image. This is used in the sg_image_desc.wrap_u, .wrap_v
    and .wrap_w members when creating an image.

    The default wrap mode is SG_WRAP_REPEAT.

    NOTE: SG_WRAP_CLAMP_TO_BORDER is not supported on all backends
    and platforms. To check for support, call sg_query_features()
    and check the "clamp_to_border" boolean in the returned
    sg_features struct.

    Platforms which don't support SG_WRAP_CLAMP_TO_BORDER will silently fall back
    to SG_WRAP_CLAMP_TO_EDGE without a validation error.
*/
typedef enum sg_wrap {
    _SG_WRAP_DEFAULT,   // value 0 reserved for default-init
    SG_WRAP_REPEAT,
    SG_WRAP_CLAMP_TO_EDGE,
    SG_WRAP_CLAMP_TO_BORDER,
    SG_WRAP_MIRRORED_REPEAT,
    _SG_WRAP_NUM,
    _SG_WRAP_FORCE_U32 = 0x7FFFFFFF
} sg_wrap;

/*
    sg_border_color

    The border color to use when sampling a texture, and the UV wrap
    mode is SG_WRAP_CLAMP_TO_BORDER.

    The default border color is SG_BORDERCOLOR_OPAQUE_BLACK
*/
typedef enum sg_border_color {
    _SG_BORDERCOLOR_DEFAULT,    // value 0 reserved for default-init
    SG_BORDERCOLOR_TRANSPARENT_BLACK,
    SG_BORDERCOLOR_OPAQUE_BLACK,
    SG_BORDERCOLOR_OPAQUE_WHITE,
    _SG_BORDERCOLOR_NUM,
    _SG_BORDERCOLOR_FORCE_U32 = 0x7FFFFFFF
} sg_border_color;

/*
    sg_vertex_format

    The data type of a vertex component. This is used to describe
    the layout of input vertex data when creating a pipeline object.

    NOTE that specific mapping rules exist from the CPU-side vertex
    formats to the vertex attribute base type in the vertex shader code
    (see doc header section 'ON VERTEX FORMATS').
*/
typedef enum sg_vertex_format {
    SG_VERTEXFORMAT_INVALID,
    SG_VERTEXFORMAT_FLOAT,
    SG_VERTEXFORMAT_FLOAT2,
    SG_VERTEXFORMAT_FLOAT3,
    SG_VERTEXFORMAT_FLOAT4,
    SG_VERTEXFORMAT_INT,
    SG_VERTEXFORMAT_INT2,
    SG_VERTEXFORMAT_INT3,
    SG_VERTEXFORMAT_INT4,
    SG_VERTEXFORMAT_UINT,
    SG_VERTEXFORMAT_UINT2,
    SG_VERTEXFORMAT_UINT3,
    SG_VERTEXFORMAT_UINT4,
    SG_VERTEXFORMAT_BYTE4,
    SG_VERTEXFORMAT_BYTE4N,
    SG_VERTEXFORMAT_UBYTE4,
    SG_VERTEXFORMAT_UBYTE4N,
    SG_VERTEXFORMAT_SHORT2,
    SG_VERTEXFORMAT_SHORT2N,
    SG_VERTEXFORMAT_USHORT2,
    SG_VERTEXFORMAT_USHORT2N,
    SG_VERTEXFORMAT_SHORT4,
    SG_VERTEXFORMAT_SHORT4N,
    SG_VERTEXFORMAT_USHORT4,
    SG_VERTEXFORMAT_USHORT4N,
    SG_VERTEXFORMAT_UINT10_N2,
    SG_VERTEXFORMAT_HALF2,
    SG_VERTEXFORMAT_HALF4,
    _SG_VERTEXFORMAT_NUM,
    _SG_VERTEXFORMAT_FORCE_U32 = 0x7FFFFFFF
} sg_vertex_format;

/*
    sg_vertex_step

    Defines whether the input pointer of a vertex input stream is advanced
    'per vertex' or 'per instance'. The default step-func is
    SG_VERTEXSTEP_PER_VERTEX. SG_VERTEXSTEP_PER_INSTANCE is used with
    instanced-rendering.

    The vertex-step is part of the vertex-layout definition
    when creating pipeline objects.
*/
typedef enum sg_vertex_step {
    _SG_VERTEXSTEP_DEFAULT,     // value 0 reserved for default-init
    SG_VERTEXSTEP_PER_VERTEX,
    SG_VERTEXSTEP_PER_INSTANCE,
    _SG_VERTEXSTEP_NUM,
    _SG_VERTEXSTEP_FORCE_U32 = 0x7FFFFFFF
} sg_vertex_step;

/*
    sg_uniform_type

    The data type of a uniform block member. This is used to
    describe the internal layout of uniform blocks when creating
    a shader object. This is only required for the GL backend, all
    other backends will ignore the interior layout of uniform blocks.
*/
typedef enum sg_uniform_type {
    SG_UNIFORMTYPE_INVALID,
    SG_UNIFORMTYPE_FLOAT,
    SG_UNIFORMTYPE_FLOAT2,
    SG_UNIFORMTYPE_FLOAT3,
    SG_UNIFORMTYPE_FLOAT4,
    SG_UNIFORMTYPE_INT,
    SG_UNIFORMTYPE_INT2,
    SG_UNIFORMTYPE_INT3,
    SG_UNIFORMTYPE_INT4,
    SG_UNIFORMTYPE_MAT4,
    _SG_UNIFORMTYPE_NUM,
    _SG_UNIFORMTYPE_FORCE_U32 = 0x7FFFFFFF
} sg_uniform_type;

/*
    sg_uniform_layout

    A hint for the interior memory layout of uniform blocks. This is
    only relevant for the GL backend where the internal layout
    of uniform blocks must be known to sokol-gfx. For all other backends the
    internal memory layout of uniform blocks doesn't matter, sokol-gfx
    will just pass uniform data as an opaque memory blob to the
    3D backend.

    SG_UNIFORMLAYOUT_NATIVE (default)
        Native layout means that a 'backend-native' memory layout
        is used. For the GL backend this means that uniforms
        are packed tightly in memory (e.g. there are no padding
        bytes).

    SG_UNIFORMLAYOUT_STD140
        The memory layout is a subset of std140. Arrays are only
        allowed for the FLOAT4, INT4 and MAT4. Alignment is as
        is as follows:

            FLOAT, INT:         4 byte alignment
            FLOAT2, INT2:       8 byte alignment
            FLOAT3, INT3:       16 byte alignment(!)
            FLOAT4, INT4:       16 byte alignment
            MAT4:               16 byte alignment
            FLOAT4[], INT4[]:   16 byte alignment

        The overall size of the uniform block must be a multiple
        of 16.

    For more information search for 'UNIFORM DATA LAYOUT' in the documentation block
    at the start of the header.
*/
typedef enum sg_uniform_layout {
    _SG_UNIFORMLAYOUT_DEFAULT,     // value 0 reserved for default-init
    SG_UNIFORMLAYOUT_NATIVE,       // default: layout depends on currently active backend
    SG_UNIFORMLAYOUT_STD140,       // std140: memory layout according to std140
    _SG_UNIFORMLAYOUT_NUM,
    _SG_UNIFORMLAYOUT_FORCE_U32 = 0x7FFFFFFF
} sg_uniform_layout;

/*
    sg_cull_mode

    The face-culling mode, this is used in the
    sg_pipeline_desc.cull_mode member when creating a
    pipeline object.

    The default cull mode is SG_CULLMODE_NONE
*/
typedef enum sg_cull_mode {
    _SG_CULLMODE_DEFAULT,   // value 0 reserved for default-init
    SG_CULLMODE_NONE,
    SG_CULLMODE_FRONT,
    SG_CULLMODE_BACK,
    _SG_CULLMODE_NUM,
    _SG_CULLMODE_FORCE_U32 = 0x7FFFFFFF
} sg_cull_mode;

/*
    sg_face_winding

    The vertex-winding rule that determines a front-facing primitive. This
    is used in the member sg_pipeline_desc.face_winding
    when creating a pipeline object.

    The default winding is SG_FACEWINDING_CW (clockwise)
*/
typedef enum sg_face_winding {
    _SG_FACEWINDING_DEFAULT,    // value 0 reserved for default-init
    SG_FACEWINDING_CCW,
    SG_FACEWINDING_CW,
    _SG_FACEWINDING_NUM,
    _SG_FACEWINDING_FORCE_U32 = 0x7FFFFFFF
} sg_face_winding;

/*
    sg_compare_func

    The compare-function for configuring depth- and stencil-ref tests
    in pipeline objects, and for texture samplers which perform a comparison
    instead of regular sampling operation.

    Used in the following structs:

    sg_pipeline_desc
        .depth
            .compare
        .stencil
            .front.compare
            .back.compare

    sg_sampler_desc
        .compare

    The default compare func for depth- and stencil-tests is
    SG_COMPAREFUNC_ALWAYS.

    The default compare func for samplers is SG_COMPAREFUNC_NEVER.
*/
typedef enum sg_compare_func {
    _SG_COMPAREFUNC_DEFAULT,    // value 0 reserved for default-init
    SG_COMPAREFUNC_NEVER,
    SG_COMPAREFUNC_LESS,
    SG_COMPAREFUNC_EQUAL,
    SG_COMPAREFUNC_LESS_EQUAL,
    SG_COMPAREFUNC_GREATER,
    SG_COMPAREFUNC_NOT_EQUAL,
    SG_COMPAREFUNC_GREATER_EQUAL,
    SG_COMPAREFUNC_ALWAYS,
    _SG_COMPAREFUNC_NUM,
    _SG_COMPAREFUNC_FORCE_U32 = 0x7FFFFFFF
} sg_compare_func;

/*
    sg_stencil_op

    The operation performed on a currently stored stencil-value when a
    comparison test passes or fails. This is used when creating a pipeline
    object in the following sg_pipeline_desc struct items:

    sg_pipeline_desc
        .stencil
            .front
                .fail_op
                .depth_fail_op
                .pass_op
            .back
                .fail_op
                .depth_fail_op
                .pass_op

    The default value is SG_STENCILOP_KEEP.
*/
typedef enum sg_stencil_op {
    _SG_STENCILOP_DEFAULT,      // value 0 reserved for default-init
    SG_STENCILOP_KEEP,
    SG_STENCILOP_ZERO,
    SG_STENCILOP_REPLACE,
    SG_STENCILOP_INCR_CLAMP,
    SG_STENCILOP_DECR_CLAMP,
    SG_STENCILOP_INVERT,
    SG_STENCILOP_INCR_WRAP,
    SG_STENCILOP_DECR_WRAP,
    _SG_STENCILOP_NUM,
    _SG_STENCILOP_FORCE_U32 = 0x7FFFFFFF
} sg_stencil_op;

/*
    sg_blend_factor

    The source and destination factors in blending operations.
    This is used in the following members when creating a pipeline object:

    sg_pipeline_desc
        .colors[i]
            .blend
                .src_factor_rgb
                .dst_factor_rgb
                .src_factor_alpha
                .dst_factor_alpha

    The default value is SG_BLENDFACTOR_ONE for source
    factors, and for the destination SG_BLENDFACTOR_ZERO if the associated
    blend-op is ADD, SUBTRACT or REVERSE_SUBTRACT or SG_BLENDFACTOR_ONE
    if the associated blend-op is MIN or MAX.
*/
typedef enum sg_blend_factor {
    _SG_BLENDFACTOR_DEFAULT,    // value 0 reserved for default-init
    SG_BLENDFACTOR_ZERO,
    SG_BLENDFACTOR_ONE,
    SG_BLENDFACTOR_SRC_COLOR,
    SG_BLENDFACTOR_ONE_MINUS_SRC_COLOR,
    SG_BLENDFACTOR_SRC_ALPHA,
    SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
    SG_BLENDFACTOR_DST_COLOR,
    SG_BLENDFACTOR_ONE_MINUS_DST_COLOR,
    SG_BLENDFACTOR_DST_ALPHA,
    SG_BLENDFACTOR_ONE_MINUS_DST_ALPHA,
    SG_BLENDFACTOR_SRC_ALPHA_SATURATED,
    SG_BLENDFACTOR_BLEND_COLOR,
    SG_BLENDFACTOR_ONE_MINUS_BLEND_COLOR,
    SG_BLENDFACTOR_BLEND_ALPHA,
    SG_BLENDFACTOR_ONE_MINUS_BLEND_ALPHA,
    _SG_BLENDFACTOR_NUM,
    _SG_BLENDFACTOR_FORCE_U32 = 0x7FFFFFFF
} sg_blend_factor;

/*
    sg_blend_op

    Describes how the source and destination values are combined in the
    fragment blending operation. It is used in the following struct items
    when creating a pipeline object:

    sg_pipeline_desc
        .colors[i]
            .blend
                .op_rgb
                .op_alpha

    The default value is SG_BLENDOP_ADD.
*/
typedef enum sg_blend_op {
    _SG_BLENDOP_DEFAULT,    // value 0 reserved for default-init
    SG_BLENDOP_ADD,
    SG_BLENDOP_SUBTRACT,
    SG_BLENDOP_REVERSE_SUBTRACT,
    SG_BLENDOP_MIN,
    SG_BLENDOP_MAX,
    _SG_BLENDOP_NUM,
    _SG_BLENDOP_FORCE_U32 = 0x7FFFFFFF
} sg_blend_op;

/*
    sg_color_mask

    Selects the active color channels when writing a fragment color to the
    framebuffer. This is used in the members
    sg_pipeline_desc.colors[i].write_mask when creating a pipeline object.

    The default colormask is SG_COLORMASK_RGBA (write all colors channels)

    NOTE: since the color mask value 0 is reserved for the default value
    (SG_COLORMASK_RGBA), use SG_COLORMASK_NONE if all color channels
    should be disabled.
*/
typedef enum sg_color_mask {
    _SG_COLORMASK_DEFAULT = 0,    // value 0 reserved for default-init
    SG_COLORMASK_NONE   = 0x10,   // special value for 'all channels disabled
    SG_COLORMASK_R      = 0x1,
    SG_COLORMASK_G      = 0x2,
    SG_COLORMASK_RG     = 0x3,
    SG_COLORMASK_B      = 0x4,
    SG_COLORMASK_RB     = 0x5,
    SG_COLORMASK_GB     = 0x6,
    SG_COLORMASK_RGB    = 0x7,
    SG_COLORMASK_A      = 0x8,
    SG_COLORMASK_RA     = 0x9,
    SG_COLORMASK_GA     = 0xA,
    SG_COLORMASK_RGA    = 0xB,
    SG_COLORMASK_BA     = 0xC,
    SG_COLORMASK_RBA    = 0xD,
    SG_COLORMASK_GBA    = 0xE,
    SG_COLORMASK_RGBA   = 0xF,
    _SG_COLORMASK_FORCE_U32 = 0x7FFFFFFF
} sg_color_mask;

/*
    sg_load_action

    Defines the load action that should be performed at the start of a render pass:

    SG_LOADACTION_CLEAR:        clear the render target
    SG_LOADACTION_LOAD:         load the previous content of the render target
    SG_LOADACTION_DONTCARE:     leave the render target in an undefined state

    This is used in the sg_pass_action structure.

    The default load action for all pass attachments is SG_LOADACTION_CLEAR,
    with the values rgba = { 0.5f, 0.5f, 0.5f, 1.0f }, depth=1.0f and stencil=0.

    If you want to override the default behaviour, it is important to not
    only set the clear color, but the 'action' field as well (as long as this
    is _SG_LOADACTION_DEFAULT, the value fields will be ignored).
*/
typedef enum sg_load_action {
    _SG_LOADACTION_DEFAULT,
    SG_LOADACTION_CLEAR,
    SG_LOADACTION_LOAD,
    SG_LOADACTION_DONTCARE,
    _SG_LOADACTION_FORCE_U32 = 0x7FFFFFFF
} sg_load_action;

/*
    sg_store_action

    Defines the store action that should be performed at the end of a render pass:

    SG_STOREACTION_STORE:       store the rendered content to the color attachment image
    SG_STOREACTION_DONTCARE:    allows the GPU to discard the rendered content
*/
typedef enum sg_store_action {
    _SG_STOREACTION_DEFAULT,
    SG_STOREACTION_STORE,
    SG_STOREACTION_DONTCARE,
    _SG_STOREACTION_FORCE_U32 = 0x7FFFFFFF
} sg_store_action;


/*
    sg_pass_action

    The sg_pass_action struct defines the actions to be performed
    at the start and end of a render pass.

    - at the start of the pass: whether the render attachments should be cleared,
      loaded with their previous content, or start in an undefined state
    - for clear operations: the clear value (color, depth, or stencil values)
    - at the end of the pass: whether the rendering result should be
      stored back into the render attachment or discarded
*/
typedef struct sg_color_attachment_action {
    sg_load_action load_action;         // default: SG_LOADACTION_CLEAR
    sg_store_action store_action;       // default: SG_STOREACTION_STORE
    sg_color clear_value;               // default: { 0.5f, 0.5f, 0.5f, 1.0f }
} sg_color_attachment_action;

typedef struct sg_depth_attachment_action {
    sg_load_action load_action;         // default: SG_LOADACTION_CLEAR
    sg_store_action store_action;       // default: SG_STOREACTION_DONTCARE
    float clear_value;                  // default: 1.0
} sg_depth_attachment_action;

typedef struct sg_stencil_attachment_action {
    sg_load_action load_action;         // default: SG_LOADACTION_CLEAR
    sg_store_action store_action;       // default: SG_STOREACTION_DONTCARE
    uint8_t clear_value;                // default: 0
} sg_stencil_attachment_action;

typedef struct sg_pass_action {
    sg_color_attachment_action colors[SG_MAX_COLOR_ATTACHMENTS];
    sg_depth_attachment_action depth;
    sg_stencil_attachment_action stencil;
} sg_pass_action;

/*
    sg_swapchain

    Used in sg_begin_pass() to provide details about an external swapchain
    (pixel formats, sample count and backend-API specific render surface objects).

    The following information must be provided:

    - the width and height of the swapchain surfaces in number of pixels,
    - the pixel format of the render- and optional msaa-resolve-surface
    - the pixel format of the optional depth- or depth-stencil-surface
    - the MSAA sample count for the render and depth-stencil surface

    If the pixel formats and MSAA sample counts are left zero-initialized,
    their defaults are taken from the sg_environment struct provided in the
    sg_setup() call.

    The width and height *must* be > 0.

    Additionally the following backend API specific objects must be passed in
    as 'type erased' void pointers:

    GL:
        - on all GL backends, a GL framebuffer object must be provided. This
          can be zero for the default framebuffer.

    D3D11:
        - an ID3D11RenderTargetView for the rendering surface, without
          MSAA rendering this surface will also be displayed
        - an optional ID3D11DepthStencilView for the depth- or depth/stencil
          buffer surface
        - when MSAA rendering is used, another ID3D11RenderTargetView
          which serves as MSAA resolve target and will be displayed

    WebGPU (same as D3D11, except different types)
        - a WGPUTextureView for the rendering surface, without
          MSAA rendering this surface will also be displayed
        - an optional WGPUTextureView for the depth- or depth/stencil
          buffer surface
        - when MSAA rendering is used, another WGPUTextureView
          which serves as MSAA resolve target and will be displayed

    Metal (NOTE that the roles of provided surfaces is slightly different
    than on D3D11 or WebGPU in case of MSAA vs non-MSAA rendering):

        - A current CAMetalDrawable (NOT an MTLDrawable!) which will be presented.
          This will either be rendered to directly (if no MSAA is used), or serve
          as MSAA-resolve target.
        - an optional MTLTexture for the depth- or depth-stencil buffer
        - an optional multisampled MTLTexture which serves as intermediate
          rendering surface which will then be resolved into the
          CAMetalDrawable.

    NOTE that for Metal you must use an ObjC __bridge cast to
    properly tunnel the ObjC object id through a C void*, e.g.:

        swapchain.metal.current_drawable = (__bridge const void*) [mtkView currentDrawable];

    On all other backends you shouldn't need to mess with the reference count.

    It's a good practice to write a helper function which returns an initialized
    sg_swapchain structs, which can then be plugged directly into
    sg_pass.swapchain. Look at the function sglue_swapchain() in the sokol_glue.h
    as an example.
*/
typedef struct sg_metal_swapchain {
    const void* current_drawable;       // CAMetalDrawable (NOT MTLDrawable!!!)
    const void* depth_stencil_texture;  // MTLTexture
    const void* msaa_color_texture;     // MTLTexture
} sg_metal_swapchain;

typedef struct sg_d3d11_swapchain {
    const void* render_view;            // ID3D11RenderTargetView
    const void* resolve_view;           // ID3D11RenderTargetView
    const void* depth_stencil_view;     // ID3D11DepthStencilView
} sg_d3d11_swapchain;

typedef struct sg_wgpu_swapchain {
    const void* render_view;            // WGPUTextureView
    const void* resolve_view;           // WGPUTextureView
    const void* depth_stencil_view;     // WGPUTextureView
} sg_wgpu_swapchain;

typedef struct sg_gl_swapchain {
    uint32_t framebuffer;               // GL framebuffer object
} sg_gl_swapchain;

typedef struct sg_swapchain {
    int width;
    int height;
    int sample_count;
    sg_pixel_format color_format;
    sg_pixel_format depth_format;
    sg_metal_swapchain metal;
    sg_d3d11_swapchain d3d11;
    sg_wgpu_swapchain wgpu;
    sg_gl_swapchain gl;
} sg_swapchain;

/*
    sg_attachments

    Used in sg_pass to provide render pass attachment views. Each
    type of pass attachment has it corresponding view type:

    sg_attachments.colors[]:
        populate with color-attachment views, e.g.:

        sg_make_view(&(sg_view_desc){
            .color_attachment = { ... },
        });

    sg_attachments.resolves[]:
        populate with resolve-attachment views, e.g.:

        sg_make_view(&(sg_view_desc){
            .resolve_attachment = { ... },
        });

    sg_attachments.depth_stencil:
        populate with depth-stencil-attachment views, e.g.:

        sg_make_view(&(sg_view_desc){
            .depth_stencil_attachment = { ... },
        });
*/
typedef struct sg_attachments {
    sg_view colors[SG_MAX_COLOR_ATTACHMENTS];
    sg_view resolves[SG_MAX_COLOR_ATTACHMENTS];
    sg_view depth_stencil;
} sg_attachments;

/*
    sg_pass

    The sg_pass structure is passed as argument into the sg_begin_pass()
    function.

    For a swapchain render pass, provide an sg_pass_action and sg_swapchain
    struct (for instance via the sglue_swapchain() helper function from
    sokol_glue.h):

        sg_begin_pass(&(sg_pass){
            .action = { ... },
            .swapchain = sglue_swapchain(),
        });

    For an offscreen render pass, provide an sg_pass_action struct with
    attachment view objects:

        sg_begin_pass(&(sg_pass){
            .action = { ... },
            .attachments = {
                .colors = { ... },
                .resolves = { ... },
                .depth_stencil = ...,
            },
        });

    You can also omit the .action object to get default pass action behaviour
    (clear to color=grey, depth=1 and stencil=0).

    For a compute pass, just set the sg_pass.compute boolean to true:

        sg_begin_pass(&(sg_pass){ .compute = true });
*/
typedef struct sg_pass {
    uint32_t _start_canary;
    bool compute;
    sg_pass_action action;
    sg_attachments attachments;
    sg_swapchain swapchain;
    const char* label;
    uint32_t _end_canary;
} sg_pass;

/*
    sg_bindings


    The sg_bindings structure defines the resource bindings for
    the next draw call.

    To update the resource bindings, call sg_apply_bindings() with
    a pointer to a populated sg_bindings struct. Note that
    sg_apply_bindings() must be called after sg_apply_pipeline()
    and that bindings are not preserved across sg_apply_pipeline()
    calls, even when the new pipeline uses the same 'bindings layout'.

    A resource binding struct contains:

    - 1..N vertex buffers
    - 1..N vertex buffer offsets
    - 0..1 index buffer
    - 0..1 index buffer offset
    - 0..N resource views (texture-, storage-image, storage-buffer-views)
    - 0..N samplers

    Where 'N' is defined in the following constants:

    - SG_MAX_VERTEXBUFFER_BINDSLOTS
    - SG_MAX_VIEW_BINDSLOTS
    - SG_MAX_SAMPLER_BINDSLOTS

    Note that inside compute passes vertex- and index-buffer-bindings are
    disallowed.

    When using sokol-shdc for shader authoring, the `layout(binding=N)`
    for texture-, storage-image- and storage-buffer-bindings directly
    maps to the views-array index, for instance the following vertex-
    and fragment-shader interface for sokol-shdc:

        @vs vs
        layout(binding=0) uniform vs_params { ... };
        layout(binding=0) readonly buffer ssbo { ... };
        layout(binding=1) uniform texture2D vs_tex;
        layout(binding=0) uniform sampler vs_smp;
        ...
        @end

        @fs fs
        layout(binding=1) uniform fs_params { ... };
        layout(binding=2) uniform texture2D fs_tex;
        layout(binding=1) uniform sampler fs_smp;
        ...
        @end

    ...would map to the following sg_bindings struct:

        const sg_bindings bnd = {
            .vertex_buffers[0] = ...,
            .views[0] = ssbo_view,
            .views[1] = vs_tex_view,
            .views[2] = fs_tex_view,
            .samplers[0] = vs_smp,
            .samplers[1] = fs_smp,
        };

    ...alternatively you can use code-generated slot indices:

        const sg_bindings bnd = {
            .vertex_buffers[0] = ...,
            .views[VIEW_ssbo] = ssbo_view,
            .views[VIEW_vs_tex] = vs_tex_view,
            .views[VIEW_fs_tex] = fs_tex_view,
            .samplers[SMP_vs_smp] = vs_smp,
            .samplers[SMP_fs_smp] = fs_smp,
        };

    Resource bindslots for a specific shader/pipeline may have gaps, and an
    sg_bindings struct may have populated bind slots which are not used by a
    specific shader. This allows to use the same sg_bindings struct across
    different shader variants.

    When not using sokol-shdc, the bindslot indices in the sg_bindings
    struct need to match the per-binding reflection info slot indices
    in the sg_shader_desc struct (for details about that see the
    sg_shader_desc struct documentation).

    The optional buffer offsets can be used to put different unrelated
    chunks of vertex- and/or index-data into the same buffer objects.
*/
typedef struct sg_bindings {
    uint32_t _start_canary;
    sg_buffer vertex_buffers[SG_MAX_VERTEXBUFFER_BINDSLOTS];
    int vertex_buffer_offsets[SG_MAX_VERTEXBUFFER_BINDSLOTS];
    sg_buffer index_buffer;
    int index_buffer_offset;
    sg_view views[SG_MAX_VIEW_BINDSLOTS];
    sg_sampler samplers[SG_MAX_SAMPLER_BINDSLOTS];
    uint32_t _end_canary;
} sg_bindings;

/*
    sg_buffer_usage

    Describes how a buffer object is going to be used:

    .vertex_buffer (default: true)
        the buffer will bound as vertex buffer via sg_bindings.vertex_buffers[]
    .index_buffer (default: false)
        the buffer will bound as index buffer via sg_bindings.index_buffer
    .storage_buffer (default: false)
        the buffer will bound as storage buffer via storage-buffer-view
        in sg_bindings.views[]
    .immutable (default: true)
        the buffer content will never be updated from the CPU side (but
        may be written to by a compute shader)
    .dynamic_update (default: false)
        the buffer content will be infrequently updated from the CPU side
    .stream_upate (default: false)
        the buffer content will be updated each frame from the CPU side
*/
typedef struct sg_buffer_usage {
    bool vertex_buffer;
    bool index_buffer;
    bool storage_buffer;
    bool immutable;
    bool dynamic_update;
    bool stream_update;
} sg_buffer_usage;

/*
    sg_buffer_desc

    Creation parameters for sg_buffer objects, used in the sg_make_buffer() call.

    The default configuration is:

    .size:      0       (*must* be >0 for buffers without data)
    .usage              .vertex_buffer = true, .immutable = true
    .data.ptr   0       (*must* be valid for immutable buffers without storage buffer usage)
    .data.size  0       (*must* be > 0 for immutable buffers without storage buffer usage)
    .label      0       (optional string label)

    For immutable buffers which are initialized with initial data,
    keep the .size item zero-initialized, and set the size together with the
    pointer to the initial data in the .data item.

    For immutable or mutable buffers without initial data, keep the .data item
    zero-initialized, and set the buffer size in the .size item instead.

    You can also set both size values, but currently both size values must
    be identical (this may change in the future when the dynamic resource
    management may become more flexible).

    NOTE: Immutable buffers without storage-buffer-usage *must* be created
    with initial content, this restriction doesn't apply to storage buffer usage,
    because storage buffers may also get their initial content by running
    a compute shader on them.

    NOTE: Buffers without initial data will have undefined content, e.g.
    do *not* expect the buffer to be zero-initialized!

    ADVANCED TOPIC: Injecting native 3D-API buffers:

    The following struct members allow to inject your own GL, Metal
    or D3D11 buffers into sokol_gfx:

    .gl_buffers[SG_NUM_INFLIGHT_FRAMES]
    .mtl_buffers[SG_NUM_INFLIGHT_FRAMES]
    .d3d11_buffer

    You must still provide all other struct items except the .data item, and
    these must match the creation parameters of the native buffers you provide.
    For sg_buffer_desc.usage.immutable buffers, only provide a single native
    3D-API buffer, otherwise you need to provide SG_NUM_INFLIGHT_FRAMES buffers
    (only for GL and Metal, not D3D11). Providing multiple buffers for GL and
    Metal is necessary because sokol_gfx will rotate through them when calling
    sg_update_buffer() to prevent lock-stalls.

    Note that it is expected that immutable injected buffer have already been
    initialized with content, and the .content member must be 0!

    Also you need to call sg_reset_state_cache() after calling native 3D-API
    functions, and before calling any sokol_gfx function.
*/
typedef struct sg_buffer_desc {
    uint32_t _start_canary;
    size_t size;
    sg_buffer_usage usage;
    sg_range data;
    const char* label;
    // optionally inject backend-specific resources
    uint32_t gl_buffers[SG_NUM_INFLIGHT_FRAMES];
    const void* mtl_buffers[SG_NUM_INFLIGHT_FRAMES];
    const void* d3d11_buffer;
    const void* wgpu_buffer;
    uint32_t _end_canary;
} sg_buffer_desc;

/*
    sg_image_usage

    Describes the intended usage of an image object:

    .storage_image (default: false)
        the image can be used as parent resource of a storage-image-view,
        which allows compute shaders to write to the image in a compute
        pass (for read-only access in compute shaders bind the image
        via a texture view instead
    .color_attachment (default: false)
        the image can be used as parent resource of a color-attachment-view,
        which is then passed into sg_begin_pass via sg_pass.attachments.colors[]
        so that fragment shaders can render into the image
    .resolve_attachment (default: false)
        the image can be used as parent resource of a resolve-attachment-view,
        which is then passed into sg_begin_pass via sg_pass.attachments.resolves[]
        as target for an MSAA-resolve operation in sg_end_pass()
    .depth_stencil_attachment (default: false)
        the image can be used as parent resource of a depth-stencil-attachmnet-view
        which is then passes into sg_begin_pass via sg_pass.attachments.depth_stencil
        as depth-stencil-buffer
    .immutable (default: true)
        the image content cannot be updated from the CPU side
        (but may be updated by the GPU in a render- or compute-pass)
    .dynamic_update (default: false)
        the image content is updated infrequently by the CPU
    .stream_update (default: false)
        the image content is updated each frame by the CPU via

    Note that creating a texture view from the image to be used for
    texture-sampling in vertex-, fragment- or compute-shaders
    is always implicitly allowed.
*/
typedef struct sg_image_usage {
    bool storage_image;
    bool color_attachment;
    bool resolve_attachment;
    bool depth_stencil_attachment;
    bool immutable;
    bool dynamic_update;
    bool stream_update;
} sg_image_usage;

/*
    sg_view_type

    Allows to query the type of a view object via the function sg_query_view_type()
*/
typedef enum sg_view_type {
    SG_VIEWTYPE_INVALID,
    SG_VIEWTYPE_STORAGEBUFFER,
    SG_VIEWTYPE_STORAGEIMAGE,
    SG_VIEWTYPE_TEXTURE,
    SG_VIEWTYPE_COLORATTACHMENT,
    SG_VIEWTYPE_RESOLVEATTACHMENT,
    SG_VIEWTYPE_DEPTHSTENCILATTACHMENT,
    _SG_VIEWTYPE_FORCE_U32 = 0x7FFFFFFF
} sg_view_type;

/*
    sg_image_data

    Defines the content of an image through a 2D array of sg_range structs.
    The first array dimension is the cubemap face, and the second array
    dimension the mipmap level.
*/
typedef struct sg_image_data {
    sg_range subimage[SG_CUBEFACE_NUM][SG_MAX_MIPMAPS];
} sg_image_data;

/*
    sg_image_desc

    Creation parameters for sg_image objects, used in the sg_make_image() call.

    The default configuration is:

    .type               SG_IMAGETYPE_2D
    .usage              .immutable = true
    .width              0 (must be set to >0)
    .height             0 (must be set to >0)
    .num_slices         1 (3D textures: depth; array textures: number of layers)
    .num_mipmaps        1
    .pixel_format       SG_PIXELFORMAT_RGBA8 for textures, or sg_desc.environment.defaults.color_format for render targets
    .sample_count       1 for textures, or sg_desc.environment.defaults.sample_count for render targets
    .data               an sg_image_data struct to define the initial content
    .label              0 (optional string label for trace hooks)

    Q: Why is the default sample_count for render targets identical with the
    "default sample count" from sg_desc.environment.defaults.sample_count?

    A: So that it matches the default sample count in pipeline objects. Even
    though it is a bit strange/confusing that offscreen render targets by default
    get the same sample count as 'default swapchains', but it's better that
    an offscreen render target created with default parameters matches
    a pipeline object created with default parameters.

    NOTE:

    Regular images used as texture binding with usage.immutable must be fully
    initialized by providing a valid .data member which points to initialization
    data.

    Images with usage.*_attachment or usage.storage_image must
    *not* be created with initial content. Be aware that the initial
    content of pass attachment and storage images is undefined
    (not guaranteed to be zeroed).

    ADVANCED TOPIC: Injecting native 3D-API textures:

    The following struct members allow to inject your own GL, Metal or D3D11
    textures into sokol_gfx:

    .gl_textures[SG_NUM_INFLIGHT_FRAMES]
    .mtl_textures[SG_NUM_INFLIGHT_FRAMES]
    .d3d11_texture
    .wgpu_texture

    For GL, you can also specify the texture target or leave it empty to use
    the default texture target for the image type (GL_TEXTURE_2D for
    SG_IMAGETYPE_2D etc)

    The same rules apply as for injecting native buffers (see sg_buffer_desc
    documentation for more details).
*/
typedef struct sg_image_desc {
    uint32_t _start_canary;
    sg_image_type type;
    sg_image_usage usage;
    int width;
    int height;
    int num_slices;
    int num_mipmaps;
    sg_pixel_format pixel_format;
    int sample_count;
    sg_image_data data;
    const char* label;
    // optionally inject backend-specific resources
    uint32_t gl_textures[SG_NUM_INFLIGHT_FRAMES];
    uint32_t gl_texture_target;
    const void* mtl_textures[SG_NUM_INFLIGHT_FRAMES];
    const void* d3d11_texture;
    const void* wgpu_texture;
    uint32_t _end_canary;
} sg_image_desc;

/*
    sg_sampler_desc

    Creation parameters for sg_sampler objects, used in the sg_make_sampler() call

    .min_filter:        SG_FILTER_NEAREST
    .mag_filter:        SG_FILTER_NEAREST
    .mipmap_filter      SG_FILTER_NEAREST
    .wrap_u:            SG_WRAP_REPEAT
    .wrap_v:            SG_WRAP_REPEAT
    .wrap_w:            SG_WRAP_REPEAT (only SG_IMAGETYPE_3D)
    .min_lod            0.0f
    .max_lod            FLT_MAX
    .border_color       SG_BORDERCOLOR_OPAQUE_BLACK
    .compare            SG_COMPAREFUNC_NEVER
    .max_anisotropy     1 (must be 1..16)

*/
typedef struct sg_sampler_desc {
    uint32_t _start_canary;
    sg_filter min_filter;
    sg_filter mag_filter;
    sg_filter mipmap_filter;
    sg_wrap wrap_u;
    sg_wrap wrap_v;
    sg_wrap wrap_w;
    float min_lod;
    float max_lod;
    sg_border_color border_color;
    sg_compare_func compare;
    uint32_t max_anisotropy;
    const char* label;
    // optionally inject backend-specific resources
    uint32_t gl_sampler;
    const void* mtl_sampler;
    const void* d3d11_sampler;
    const void* wgpu_sampler;
    uint32_t _end_canary;
} sg_sampler_desc;

/*
    sg_shader_desc

    Used as parameter of sg_make_shader() to create a shader object which
    communicates shader source or bytecode and shader interface
    reflection information to sokol-gfx.

    If you use sokol-shdc you can ignore the following information since
    the sg_shader_desc struct will be code-generated.

    Otherwise you need to provide the following information to the
    sg_make_shader() call:

    - a vertex- and fragment-shader function:
        - the shader source or bytecode
        - an optional entry point name
        - for D3D11: an optional compile target when source code is provided
          (the defaults are "vs_4_0" and "ps_4_0")

    - ...or alternatively, a compute function:
        - the shader source or bytecode
        - an optional entry point name
        - for D3D11: an optional compile target when source code is provided
          (the default is "cs_5_0")

    - vertex attributes required by some backends (not for compute shaders):
        - the vertex attribute base type (undefined, float, signed int, unsigned int),
          this information is only used in the validation layer to check that the
          pipeline object vertex formats are compatible with the input vertex attribute
          type used in the vertex shader. NOTE that the default base type
          'undefined' skips the validation layer check.
        - for the GL backend: optional vertex attribute names used for name lookup
        - for the D3D11 backend: semantic names and indices

    - only for compute shaders on the Metal backend:
        - the workgroup size aka 'threads per thread-group'

          In other 3D APIs this is declared in the shader code:
            - GLSL: `layout(local_size_x=x, local_size_y=y, local_size_y=z) in;`
            - HLSL: `[numthreads(x, y, z)]`
            - WGSL: `@workgroup_size(x, y, z)`
          ...but in Metal the workgroup size is declared on the CPU side

    - reflection information for each uniform block binding used by the shader:
        - the shader stage the uniform block appears in (SG_SHADERSTAGE_*)
        - the size in bytes of the uniform block
        - backend-specific bindslots:
            - HLSL: the constant buffer register `register(b0..7)`
            - MSL: the buffer attribute `[[buffer(0..7)]]`
            - WGSL: the binding in `@group(0) @binding(0..15)`
        - GLSL only: a description of the uniform block interior
            - the memory layout standard (SG_UNIFORMLAYOUT_*)
            - for each member in the uniform block:
                - the member type (SG_UNIFORM_*)
                - if the member is an array, the array count
                - the member name

    - reflection information for each texture-, storage-buffer and
      storage-image bindings by the shader, each with an associated
      view type:
        - texture bindings => texture views
        - storage-buffer bindings => storage-buffer views
        - storage-image bindings => storage-image views

    - texture bindings must provide the following information:
        - the shader stage the texture binding appears in (SG_SHADERSTAGE_*)
        - the image type (SG_IMAGETYPE_*)
        - the image-sample type (SG_IMAGESAMPLETYPE_*)
        - whether the texture is multisampled
        - backend specific bindslots:
            - HLSL: the texture register `register(t0..23)`
            - MSL: the texture attribute `[[texture(0..19)]]`
            - WGSL: the binding in `@group(1) @binding(0..127)`

    - storage-buffer bindings must provide the following information:
        - the shader stage the storage buffer appears in (SG_SHADERSTAGE_*)
        - whether the storage buffer is readonly
        - backend specific bindslots:
            - HLSL:
                - for readonly storage buffer bindings: `register(t0..23)`
                - for read/write storage buffer bindings: `register(u0..11)`
            - MSL: the buffer attribute `[[buffer(8..15)]]`
            - WGSL: the binding in `@group(1) @binding(0..127)`
            - GL: the binding in `layout(binding=0..7)`

    - storage-image bindings must provide the following information:
        - the shader stage (*must* be SG_SHADERSTAGE_COMPUTE)
        - whether the storage image is writeonly or readwrite (for readonly
          access use a regular texture binding instead)
        - the image type expected by the shader (SG_IMAGETYPE_*)
        - the access pixel format expected by the shader (SG_PIXELFORMAT_*),
          note that only a subset of pixel formats is allowed for storage image
          bindings
        - backend specific bindslots:
            - HLSL: the UAV register `register(u0..11)`
            - MSL: the texture attribute `[[texture(0..19)]]`
            - WGSL: the binding in `@group(1) @binding(0..127)`
            - GLSL: the binding in `layout(binding=0..3, [access_format])`

    - reflection information for each sampler used by the shader:
        - the shader stage the sampler appears in (SG_SHADERSTAGE_*)
        - the sampler type (SG_SAMPLERTYPE_*)
        - backend specific bindslots:
            - HLSL: the sampler register `register(s0..15)`
            - MSL: the sampler attribute `[[sampler(0..15)]]`
            - WGSL: the binding in `@group(0) @binding(0..127)`

    - reflection information for each texture-sampler pair used by
      the shader:
        - the shader stage (SG_SHADERSTAGE_*)
        - the texture's array index in the sg_shader_desc.views[] array
        - the sampler's array index in the sg_shader_desc.samplers[] array
        - GLSL only: the name of the combined image-sampler object

    The number and order of items in the sg_shader_desc.attrs[]
    array corresponds to the items in sg_pipeline_desc.layout.attrs.

        - sg_shader_desc.attrs[N] => sg_pipeline_desc.layout.attrs[N]

    NOTE that vertex attribute indices currently cannot have gaps.

    The items index in the sg_shader_desc.uniform_blocks[] array corresponds
    to the ub_slot arg in sg_apply_uniforms():

        - sg_shader_desc.uniform_blocks[N] => sg_apply_uniforms(N, ...)

    The items in the sg_shader_desc.views[] array directly map to
    the views in the sg_bindings.views[] array!

    For all GL backends, shader source-code must be provided. For D3D11 and Metal,
    either shader source-code or byte-code can be provided.

    NOTE that the uniform-block, view and sampler arrays may have gaps. This
    allows to use the same sg_bindings struct for different but related
    shader variations.

    For D3D11, if source code is provided, the d3dcompiler_47.dll will be loaded
    on demand. If this fails, shader creation will fail. When compiling HLSL
    source code, you can provide an optional target string via
    sg_shader_stage_desc.d3d11_target, the default target is "vs_4_0" for the
    vertex shader stage and "ps_4_0" for the pixel shader stage.
    You may optionally provide the file path to enable the default #include handler
    behavior when compiling source code.
*/
typedef enum sg_shader_stage {
    SG_SHADERSTAGE_NONE,
    SG_SHADERSTAGE_VERTEX,
    SG_SHADERSTAGE_FRAGMENT,
    SG_SHADERSTAGE_COMPUTE,
    _SG_SHADERSTAGE_FORCE_U32 = 0x7FFFFFFF,
} sg_shader_stage;

typedef struct sg_shader_function {
    const char* source;
    sg_range bytecode;
    const char* entry;
    const char* d3d11_target;   // default: "vs_4_0" or "ps_4_0"
    const char* d3d11_filepath;
} sg_shader_function;

typedef enum sg_shader_attr_base_type {
    SG_SHADERATTRBASETYPE_UNDEFINED,
    SG_SHADERATTRBASETYPE_FLOAT,
    SG_SHADERATTRBASETYPE_SINT,
    SG_SHADERATTRBASETYPE_UINT,
    _SG_SHADERATTRBASETYPE_FORCE_U32 = 0x7FFFFFFF,
} sg_shader_attr_base_type;

typedef struct sg_shader_vertex_attr {
    sg_shader_attr_base_type base_type;  // default: UNDEFINED (disables validation)
    const char* glsl_name;      // [optional] GLSL attribute name
    const char* hlsl_sem_name;  // HLSL semantic name
    uint8_t hlsl_sem_index;     // HLSL semantic index
} sg_shader_vertex_attr;

typedef struct sg_glsl_shader_uniform {
    sg_uniform_type type;
    uint16_t array_count;       // 0 or 1 for scalars, >1 for arrays
    const char* glsl_name;      // glsl name binding is required on GL 4.1 and WebGL2
} sg_glsl_shader_uniform;

typedef struct sg_shader_uniform_block {
    sg_shader_stage stage;
    uint32_t size;
    uint8_t hlsl_register_b_n;  // HLSL register(bn)
    uint8_t msl_buffer_n;       // MSL [[buffer(n)]]
    uint8_t wgsl_group0_binding_n; // WGSL @group(0) @binding(n)
    sg_uniform_layout layout;
    sg_glsl_shader_uniform glsl_uniforms[SG_MAX_UNIFORMBLOCK_MEMBERS];
} sg_shader_uniform_block;

typedef struct sg_shader_texture_view {
    sg_shader_stage stage;
    sg_image_type image_type;
    sg_image_sample_type sample_type;
    bool multisampled;
    uint8_t hlsl_register_t_n;      // HLSL register(tn) bind slot
    uint8_t msl_texture_n;          // MSL [[texture(n)]] bind slot
    uint8_t wgsl_group1_binding_n;  // WGSL @group(1) @binding(n) bind slot
} sg_shader_texture_view;

typedef struct sg_shader_storage_buffer_view {
    sg_shader_stage stage;
    bool readonly;
    uint8_t hlsl_register_t_n;      // HLSL register(tn) bind slot (for readonly access)
    uint8_t hlsl_register_u_n;      // HLSL register(un) bind slot (for read/write access)
    uint8_t msl_buffer_n;           // MSL [[buffer(n)]] bind slot
    uint8_t wgsl_group1_binding_n;  // WGSL @group(1) @binding(n) bind slot
    uint8_t glsl_binding_n;         // GLSL layout(binding=n)
} sg_shader_storage_buffer_view;

typedef struct sg_shader_storage_image_view {
    sg_shader_stage stage;
    sg_image_type image_type;
    sg_pixel_format access_format;  // shader-access pixel format
    bool writeonly;                 // false means read/write access
    uint8_t hlsl_register_u_n;      // HLSL register(un) bind slot
    uint8_t msl_texture_n;          // MSL [[texture(n)]] bind slot
    uint8_t wgsl_group1_binding_n;  // WGSL @group(2) @binding(n) bind slot
    uint8_t glsl_binding_n;         // GLSL layout(binding=n)
} sg_shader_storage_image_view;

typedef struct sg_shader_view {
    sg_shader_texture_view texture;
    sg_shader_storage_buffer_view storage_buffer;
    sg_shader_storage_image_view storage_image;
} sg_shader_view;

typedef struct sg_shader_sampler {
    sg_shader_stage stage;
    sg_sampler_type sampler_type;
    uint8_t hlsl_register_s_n;      // HLSL register(sn) bind slot
    uint8_t msl_sampler_n;          // MSL [[sampler(n)]] bind slot
    uint8_t wgsl_group1_binding_n;  // WGSL @group(1) @binding(n) bind slot
} sg_shader_sampler;

typedef struct sg_shader_texture_sampler_pair {
    sg_shader_stage stage;
    uint8_t view_slot;              // must be SG_VIEWTYPE_TEXTURE
    uint8_t sampler_slot;
    const char* glsl_name;          // glsl name binding required because of GL 4.1 and WebGL2
} sg_shader_texture_sampler_pair;

typedef struct sg_mtl_shader_threads_per_threadgroup {
    int x, y, z;
} sg_mtl_shader_threads_per_threadgroup;

typedef struct sg_shader_desc {
    uint32_t _start_canary;
    sg_shader_function vertex_func;
    sg_shader_function fragment_func;
    sg_shader_function compute_func;
    sg_shader_vertex_attr attrs[SG_MAX_VERTEX_ATTRIBUTES];
    sg_shader_uniform_block uniform_blocks[SG_MAX_UNIFORMBLOCK_BINDSLOTS];
    sg_shader_view views[SG_MAX_VIEW_BINDSLOTS];
    sg_shader_sampler samplers[SG_MAX_SAMPLER_BINDSLOTS];
    sg_shader_texture_sampler_pair texture_sampler_pairs[SG_MAX_TEXTURE_SAMPLER_PAIRS];
    sg_mtl_shader_threads_per_threadgroup mtl_threads_per_threadgroup;
    const char* label;
    uint32_t _end_canary;
} sg_shader_desc;

/*
    sg_pipeline_desc

    The sg_pipeline_desc struct defines all creation parameters for an
    sg_pipeline object, used as argument to the sg_make_pipeline() function:

    Pipeline objects come in two flavours:

    - render pipelines for use in render passes
    - compute pipelines for use in compute passes

    A compute pipeline only requires a compute shader object but no
    'render state', while a render pipeline requires a vertex/fragment shader
    object and additional render state declarations:

    - the vertex layout for all input vertex buffers
    - a shader object
    - the 3D primitive type (points, lines, triangles, ...)
    - the index type (none, 16- or 32-bit)
    - all the fixed-function-pipeline state (depth-, stencil-, blend-state, etc...)

    If the vertex data has no gaps between vertex components, you can omit
    the .layout.buffers[].stride and layout.attrs[].offset items (leave them
    default-initialized to 0), sokol-gfx will then compute the offsets and
    strides from the vertex component formats (.layout.attrs[].format).
    Please note that ALL vertex attribute offsets must be 0 in order for the
    automatic offset computation to kick in.

    Note that if you use vertex-pulling from storage buffers instead of
    fixed-function vertex input you can simply omit the entire nested .layout
    struct.

    The default configuration is as follows:

    .compute:               false (must be set to true for a compute pipeline)
    .shader:                0 (must be initialized with a valid sg_shader id!)
    .layout:
        .buffers[]:         vertex buffer layouts
            .stride:        0 (if no stride is given it will be computed)
            .step_func      SG_VERTEXSTEP_PER_VERTEX
            .step_rate      1
        .attrs[]:           vertex attribute declarations
            .buffer_index   0 the vertex buffer bind slot
            .offset         0 (offsets can be omitted if the vertex layout has no gaps)
            .format         SG_VERTEXFORMAT_INVALID (must be initialized!)
    .depth:
        .pixel_format:      sg_desc.context.depth_format
        .compare:           SG_COMPAREFUNC_ALWAYS
        .write_enabled:     false
        .bias:              0.0f
        .bias_slope_scale:  0.0f
        .bias_clamp:        0.0f
    .stencil:
        .enabled:           false
        .front/back:
            .compare:       SG_COMPAREFUNC_ALWAYS
            .fail_op:       SG_STENCILOP_KEEP
            .depth_fail_op: SG_STENCILOP_KEEP
            .pass_op:       SG_STENCILOP_KEEP
        .read_mask:         0
        .write_mask:        0
        .ref:               0
    .color_count            1
    .colors[0..color_count]
        .pixel_format       sg_desc.context.color_format
        .write_mask:        SG_COLORMASK_RGBA
        .blend:
            .enabled:           false
            .src_factor_rgb:    SG_BLENDFACTOR_ONE
            .dst_factor_rgb:    SG_BLENDFACTOR_ZERO
            .op_rgb:            SG_BLENDOP_ADD
            .src_factor_alpha:  SG_BLENDFACTOR_ONE
            .dst_factor_alpha:  SG_BLENDFACTOR_ZERO
            .op_alpha:          SG_BLENDOP_ADD
    .primitive_type:            SG_PRIMITIVETYPE_TRIANGLES
    .index_type:                SG_INDEXTYPE_NONE
    .cull_mode:                 SG_CULLMODE_NONE
    .face_winding:              SG_FACEWINDING_CW
    .sample_count:              sg_desc.context.sample_count
    .blend_color:               (sg_color) { 0.0f, 0.0f, 0.0f, 0.0f }
    .alpha_to_coverage_enabled: false
    .label  0       (optional string label for trace hooks)
*/
typedef struct sg_vertex_buffer_layout_state {
    int stride;
    sg_vertex_step step_func;
    int step_rate;
} sg_vertex_buffer_layout_state;

typedef struct sg_vertex_attr_state {
    int buffer_index;
    int offset;
    sg_vertex_format format;
} sg_vertex_attr_state;

typedef struct sg_vertex_layout_state {
    sg_vertex_buffer_layout_state buffers[SG_MAX_VERTEXBUFFER_BINDSLOTS];
    sg_vertex_attr_state attrs[SG_MAX_VERTEX_ATTRIBUTES];
} sg_vertex_layout_state;

typedef struct sg_stencil_face_state {
    sg_compare_func compare;
    sg_stencil_op fail_op;
    sg_stencil_op depth_fail_op;
    sg_stencil_op pass_op;
} sg_stencil_face_state;

typedef struct sg_stencil_state {
    bool enabled;
    sg_stencil_face_state front;
    sg_stencil_face_state back;
    uint8_t read_mask;
    uint8_t write_mask;
    uint8_t ref;
} sg_stencil_state;

typedef struct sg_depth_state {
    sg_pixel_format pixel_format;
    sg_compare_func compare;
    bool write_enabled;
    float bias;
    float bias_slope_scale;
    float bias_clamp;
} sg_depth_state;

typedef struct sg_blend_state {
    bool enabled;
    sg_blend_factor src_factor_rgb;
    sg_blend_factor dst_factor_rgb;
    sg_blend_op op_rgb;
    sg_blend_factor src_factor_alpha;
    sg_blend_factor dst_factor_alpha;
    sg_blend_op op_alpha;
} sg_blend_state;

typedef struct sg_color_target_state {
    sg_pixel_format pixel_format;
    sg_color_mask write_mask;
    sg_blend_state blend;
} sg_color_target_state;

typedef struct sg_pipeline_desc {
    uint32_t _start_canary;
    bool compute;
    sg_shader shader;
    sg_vertex_layout_state layout;
    sg_depth_state depth;
    sg_stencil_state stencil;
    int color_count;
    sg_color_target_state colors[SG_MAX_COLOR_ATTACHMENTS];
    sg_primitive_type primitive_type;
    sg_index_type index_type;
    sg_cull_mode cull_mode;
    sg_face_winding face_winding;
    int sample_count;
    sg_color blend_color;
    bool alpha_to_coverage_enabled;
    const char* label;
    uint32_t _end_canary;
} sg_pipeline_desc;

/*
    sg_view_desc

    Creation params for sg_view objects, passed into sg_make_view() calls.

    View objects are passed into sg_apply_bindings() (for texture-, storage-buffer-
    and storage-image views), and sg_begin_pass() (for color-, resolve-
    and depth-stencil-attachment views).

    The view type is determined by initializing one of the sub-structs of
    sg_view_desc:

    .texture            a texture-view object will be created
        .image          the sg_image parent resource
        .mip_levels     optional mip-level range, keep zero-initialized for the
                        entire mipmap chain
            .base       the first mip level
            .count      number of mip levels, keeping this zero-initialized means
                        'all remaining mip levels'
        .slices         optional slice range, keep zero-initialized to include
                        all slices
            .base       the first slice
            .count      number of slices, keeping this zero-initializied means 'all remaining slices'

    .storage_buffer     a storage-buffer-view object will be created
        .buffer         the sg_buffer parent resource, must have been created
                        with `sg_buffer_desc.usage.storage_buffer = true`
        .offset         optional 256-byte aligned byte-offset into the buffer

    .storage_image      a storage-image-view object will be created
        .image          the sg_image parent resource, must have been created
                        with `sg_image_desc.usage.storage_image = true`
        .mip_level      selects the mip-level for the compute shader to write
        .slice          selects the slice for the compute shader to write

    .color_attachment   a color-attachment-view object will be created
        .image          the sg_image parent resource, must have been created
                        with `sg_image_desc.usage.color_attachment = true`
        .mip_level      selects the mip-level to render into
        .slice          selects the slice to render into

    .resolve_attachment a resolve-attachment-view object will be created
        .image          the sg_image parent resource, must have been created
                        with `sg_image_desc.usage.resolve_attachment = true`
        .mip_level      selects the mip-level to msaa-resolve into
        .slice          selects the slice to msaa-resolve into

    .depth_stencil_attachment   a depth-stencil-attachment-view object will be created
        .image          the sg_image parent resource, must have been created
                        with `sg_image_desc.usage.depth_stencil_attachment = true`
        .mip_level      selects the mip-level to render into
        .slice          selects the slice to render into
*/
typedef struct sg_buffer_view_desc {
    sg_buffer buffer;
    int offset;
} sg_buffer_view_desc;

typedef struct sg_image_view_desc {
    sg_image image;
    int mip_level;
    int slice;      // cube texture: face; array texture: layer; 3D texture: depth-slice
} sg_image_view_desc;

typedef struct sg_texture_view_range {
    int base;
    int count;
} sg_texture_view_range;

typedef struct sg_texture_view_desc {
    sg_image image;
    sg_texture_view_range mip_levels;
    sg_texture_view_range slices;   // cube texture: face; array texture: layer; 3D texture: depth-slice
} sg_texture_view_desc;

typedef struct sg_view_desc {
    uint32_t _start_canary;
    sg_texture_view_desc texture;
    sg_buffer_view_desc storage_buffer;
    sg_image_view_desc storage_image;
    sg_image_view_desc color_attachment;
    sg_image_view_desc resolve_attachment;
    sg_image_view_desc depth_stencil_attachment;
    const char* label;
    uint32_t _end_canary;
} sg_view_desc;

/*
    sg_trace_hooks

    Installable callback functions to keep track of the sokol-gfx calls,
    this is useful for debugging, or keeping track of resource creation
    and destruction.

    Trace hooks are installed with sg_install_trace_hooks(), this returns
    another sg_trace_hooks struct with the previous set of
    trace hook function pointers. These should be invoked by the
    new trace hooks to form a proper call chain.
*/
typedef struct sg_trace_hooks {
    void* user_data;
    void (*reset_state_cache)(void* user_data);
    void (*make_buffer)(const sg_buffer_desc* desc, sg_buffer result, void* user_data);
    void (*make_image)(const sg_image_desc* desc, sg_image result, void* user_data);
    void (*make_sampler)(const sg_sampler_desc* desc, sg_sampler result, void* user_data);
    void (*make_shader)(const sg_shader_desc* desc, sg_shader result, void* user_data);
    void (*make_pipeline)(const sg_pipeline_desc* desc, sg_pipeline result, void* user_data);
    void (*make_view)(const sg_view_desc* desc, sg_view result, void* user_data);
    void (*destroy_buffer)(sg_buffer buf, void* user_data);
    void (*destroy_image)(sg_image img, void* user_data);
    void (*destroy_sampler)(sg_sampler smp, void* user_data);
    void (*destroy_shader)(sg_shader shd, void* user_data);
    void (*destroy_pipeline)(sg_pipeline pip, void* user_data);
    void (*destroy_view)(sg_view view, void* user_data);
    void (*update_buffer)(sg_buffer buf, const sg_range* data, void* user_data);
    void (*update_image)(sg_image img, const sg_image_data* data, void* user_data);
    void (*append_buffer)(sg_buffer buf, const sg_range* data, int result, void* user_data);
    void (*begin_pass)(const sg_pass* pass, void* user_data);
    void (*apply_viewport)(int x, int y, int width, int height, bool origin_top_left, void* user_data);
    void (*apply_scissor_rect)(int x, int y, int width, int height, bool origin_top_left, void* user_data);
    void (*apply_pipeline)(sg_pipeline pip, void* user_data);
    void (*apply_bindings)(const sg_bindings* bindings, void* user_data);
    void (*apply_uniforms)(int ub_index, const sg_range* data, void* user_data);
    void (*draw)(int base_element, int num_elements, int num_instances, void* user_data);
    void (*dispatch)(int num_groups_x, int num_groups_y, int num_groups_z, void* user_data);
    void (*end_pass)(void* user_data);
    void (*commit)(void* user_data);
    void (*alloc_buffer)(sg_buffer result, void* user_data);
    void (*alloc_image)(sg_image result, void* user_data);
    void (*alloc_sampler)(sg_sampler result, void* user_data);
    void (*alloc_shader)(sg_shader result, void* user_data);
    void (*alloc_pipeline)(sg_pipeline result, void* user_data);
    void (*alloc_view)(sg_view result, void* user_data);
    void (*dealloc_buffer)(sg_buffer buf_id, void* user_data);
    void (*dealloc_image)(sg_image img_id, void* user_data);
    void (*dealloc_sampler)(sg_sampler smp_id, void* user_data);
    void (*dealloc_shader)(sg_shader shd_id, void* user_data);
    void (*dealloc_pipeline)(sg_pipeline pip_id, void* user_data);
    void (*dealloc_view)(sg_view view_id, void* user_data);
    void (*init_buffer)(sg_buffer buf_id, const sg_buffer_desc* desc, void* user_data);
    void (*init_image)(sg_image img_id, const sg_image_desc* desc, void* user_data);
    void (*init_sampler)(sg_sampler smp_id, const sg_sampler_desc* desc, void* user_data);
    void (*init_shader)(sg_shader shd_id, const sg_shader_desc* desc, void* user_data);
    void (*init_pipeline)(sg_pipeline pip_id, const sg_pipeline_desc* desc, void* user_data);
    void (*init_view)(sg_view view_id, const sg_view_desc* desc, void* user_data);
    void (*uninit_buffer)(sg_buffer buf_id, void* user_data);
    void (*uninit_image)(sg_image img_id, void* user_data);
    void (*uninit_sampler)(sg_sampler smp_id, void* user_data);
    void (*uninit_shader)(sg_shader shd_id, void* user_data);
    void (*uninit_pipeline)(sg_pipeline pip_id, void* user_data);
    void (*uninit_view)(sg_view view_id, void* user_data);
    void (*fail_buffer)(sg_buffer buf_id, void* user_data);
    void (*fail_image)(sg_image img_id, void* user_data);
    void (*fail_sampler)(sg_sampler smp_id, void* user_data);
    void (*fail_shader)(sg_shader shd_id, void* user_data);
    void (*fail_pipeline)(sg_pipeline pip_id, void* user_data);
    void (*fail_view)(sg_view view_id, void* user_data);
    void (*push_debug_group)(const char* name, void* user_data);
    void (*pop_debug_group)(void* user_data);
} sg_trace_hooks;

/*
    sg_buffer_info
    sg_image_info
    sg_sampler_info
    sg_shader_info
    sg_pipeline_info
    sg_view_info

    These structs contain various internal resource attributes which
    might be useful for debug-inspection. Please don't rely on the
    actual content of those structs too much, as they are quite closely
    tied to sokol_gfx.h internals and may change more frequently than
    the other public API elements.

    The *_info structs are used as the return values of the following functions:

    sg_query_buffer_info()
    sg_query_image_info()
    sg_query_sampler_info()
    sg_query_shader_info()
    sg_query_pipeline_info()
    sg_query_view_info()
*/
typedef struct sg_slot_info {
    sg_resource_state state;    // the current state of this resource slot
    uint32_t res_id;            // type-neutral resource if (e.g. sg_buffer.id)
    uint32_t uninit_count;
} sg_slot_info;

typedef struct sg_buffer_info {
    sg_slot_info slot;              // resource pool slot info
    uint32_t update_frame_index;    // frame index of last sg_update_buffer()
    uint32_t append_frame_index;    // frame index of last sg_append_buffer()
    int append_pos;                 // current position in buffer for sg_append_buffer()
    bool append_overflow;           // is buffer in overflow state (due to sg_append_buffer)
    int num_slots;                  // number of renaming-slots for dynamically updated buffers
    int active_slot;                // currently active write-slot for dynamically updated buffers
} sg_buffer_info;

typedef struct sg_image_info {
    sg_slot_info slot;              // resource pool slot info
    uint32_t upd_frame_index;       // frame index of last sg_update_image()
    int num_slots;                  // number of renaming-slots for dynamically updated images
    int active_slot;                // currently active write-slot for dynamically updated images
} sg_image_info;

typedef struct sg_sampler_info {
    sg_slot_info slot;              // resource pool slot info
} sg_sampler_info;

typedef struct sg_shader_info {
    sg_slot_info slot;              // resource pool slot info
} sg_shader_info;

typedef struct sg_pipeline_info {
    sg_slot_info slot;              // resource pool slot info
} sg_pipeline_info;

typedef struct sg_view_info {
    sg_slot_info slot;              // resource pool slot info
} sg_view_info;

/*
    sg_frame_stats

    Allows to track generic and backend-specific stats about a
    render frame. Obtained by calling sg_query_frame_stats(). The returned
    struct contains information about the *previous* frame.
*/
typedef struct sg_frame_stats_gl {
    uint32_t num_bind_buffer;
    uint32_t num_active_texture;
    uint32_t num_bind_texture;
    uint32_t num_bind_sampler;
    uint32_t num_bind_image_texture;
    uint32_t num_use_program;
    uint32_t num_render_state;
    uint32_t num_vertex_attrib_pointer;
    uint32_t num_vertex_attrib_divisor;
    uint32_t num_enable_vertex_attrib_array;
    uint32_t num_disable_vertex_attrib_array;
    uint32_t num_uniform;
    uint32_t num_memory_barriers;
} sg_frame_stats_gl;

typedef struct sg_frame_stats_d3d11_pass {
    uint32_t num_om_set_render_targets;
    uint32_t num_clear_render_target_view;
    uint32_t num_clear_depth_stencil_view;
    uint32_t num_resolve_subresource;
} sg_frame_stats_d3d11_pass;

typedef struct sg_frame_stats_d3d11_pipeline {
    uint32_t num_rs_set_state;
    uint32_t num_om_set_depth_stencil_state;
    uint32_t num_om_set_blend_state;
    uint32_t num_ia_set_primitive_topology;
    uint32_t num_ia_set_input_layout;
    uint32_t num_vs_set_shader;
    uint32_t num_vs_set_constant_buffers;
    uint32_t num_ps_set_shader;
    uint32_t num_ps_set_constant_buffers;
    uint32_t num_cs_set_shader;
    uint32_t num_cs_set_constant_buffers;
} sg_frame_stats_d3d11_pipeline;

typedef struct sg_frame_stats_d3d11_bindings {
    uint32_t num_ia_set_vertex_buffers;
    uint32_t num_ia_set_index_buffer;
    uint32_t num_vs_set_shader_resources;
    uint32_t num_vs_set_samplers;
    uint32_t num_ps_set_shader_resources;
    uint32_t num_ps_set_samplers;
    uint32_t num_cs_set_shader_resources;
    uint32_t num_cs_set_samplers;
    uint32_t num_cs_set_unordered_access_views;
} sg_frame_stats_d3d11_bindings;

typedef struct sg_frame_stats_d3d11_uniforms {
    uint32_t num_update_subresource;
} sg_frame_stats_d3d11_uniforms;

typedef struct sg_frame_stats_d3d11_draw {
    uint32_t num_draw_indexed_instanced;
    uint32_t num_draw_indexed;
    uint32_t num_draw_instanced;
    uint32_t num_draw;
} sg_frame_stats_d3d11_draw;

typedef struct sg_frame_stats_d3d11 {
    sg_frame_stats_d3d11_pass pass;
    sg_frame_stats_d3d11_pipeline pipeline;
    sg_frame_stats_d3d11_bindings bindings;
    sg_frame_stats_d3d11_uniforms uniforms;
    sg_frame_stats_d3d11_draw draw;
    uint32_t num_map;
    uint32_t num_unmap;
} sg_frame_stats_d3d11;

typedef struct sg_frame_stats_metal_idpool {
    uint32_t num_added;
    uint32_t num_released;
    uint32_t num_garbage_collected;
} sg_frame_stats_metal_idpool;

typedef struct sg_frame_stats_metal_pipeline {
    uint32_t num_set_blend_color;
    uint32_t num_set_cull_mode;
    uint32_t num_set_front_facing_winding;
    uint32_t num_set_stencil_reference_value;
    uint32_t num_set_depth_bias;
    uint32_t num_set_render_pipeline_state;
    uint32_t num_set_depth_stencil_state;
} sg_frame_stats_metal_pipeline;

typedef struct sg_frame_stats_metal_bindings {
    uint32_t num_set_vertex_buffer;
    uint32_t num_set_vertex_buffer_offset;
    uint32_t num_skip_redundant_vertex_buffer;
    uint32_t num_set_vertex_texture;
    uint32_t num_skip_redundant_vertex_texture;
    uint32_t num_set_vertex_sampler_state;
    uint32_t num_skip_redundant_vertex_sampler_state;
    uint32_t num_set_fragment_buffer;
    uint32_t num_set_fragment_buffer_offset;
    uint32_t num_skip_redundant_fragment_buffer;
    uint32_t num_set_fragment_texture;
    uint32_t num_skip_redundant_fragment_texture;
    uint32_t num_set_fragment_sampler_state;
    uint32_t num_skip_redundant_fragment_sampler_state;
    uint32_t num_set_compute_buffer;
    uint32_t num_set_compute_buffer_offset;
    uint32_t num_skip_redundant_compute_buffer;
    uint32_t num_set_compute_texture;
    uint32_t num_skip_redundant_compute_texture;
    uint32_t num_set_compute_sampler_state;
    uint32_t num_skip_redundant_compute_sampler_state;
} sg_frame_stats_metal_bindings;

typedef struct sg_frame_stats_metal_uniforms {
    uint32_t num_set_vertex_buffer_offset;
    uint32_t num_set_fragment_buffer_offset;
    uint32_t num_set_compute_buffer_offset;
} sg_frame_stats_metal_uniforms;

typedef struct sg_frame_stats_metal {
    sg_frame_stats_metal_idpool idpool;
    sg_frame_stats_metal_pipeline pipeline;
    sg_frame_stats_metal_bindings bindings;
    sg_frame_stats_metal_uniforms uniforms;
} sg_frame_stats_metal;

typedef struct sg_frame_stats_wgpu_uniforms {
    uint32_t num_set_bindgroup;
    uint32_t size_write_buffer;
} sg_frame_stats_wgpu_uniforms;

typedef struct sg_frame_stats_wgpu_bindings {
    uint32_t num_set_vertex_buffer;
    uint32_t num_skip_redundant_vertex_buffer;
    uint32_t num_set_index_buffer;
    uint32_t num_skip_redundant_index_buffer;
    uint32_t num_create_bindgroup;
    uint32_t num_discard_bindgroup;
    uint32_t num_set_bindgroup;
    uint32_t num_skip_redundant_bindgroup;
    uint32_t num_bindgroup_cache_hits;
    uint32_t num_bindgroup_cache_misses;
    uint32_t num_bindgroup_cache_collisions;
    uint32_t num_bindgroup_cache_invalidates;
    uint32_t num_bindgroup_cache_hash_vs_key_mismatch;
} sg_frame_stats_wgpu_bindings;

typedef struct sg_frame_stats_wgpu {
    sg_frame_stats_wgpu_uniforms uniforms;
    sg_frame_stats_wgpu_bindings bindings;
} sg_frame_stats_wgpu;

typedef struct sg_frame_stats {
    uint32_t frame_index;   // current frame counter, starts at 0

    uint32_t num_passes;
    uint32_t num_apply_viewport;
    uint32_t num_apply_scissor_rect;
    uint32_t num_apply_pipeline;
    uint32_t num_apply_bindings;
    uint32_t num_apply_uniforms;
    uint32_t num_draw;
    uint32_t num_dispatch;
    uint32_t num_update_buffer;
    uint32_t num_append_buffer;
    uint32_t num_update_image;

    uint32_t size_apply_uniforms;
    uint32_t size_update_buffer;
    uint32_t size_append_buffer;
    uint32_t size_update_image;

    sg_frame_stats_gl gl;
    sg_frame_stats_d3d11 d3d11;
    sg_frame_stats_metal metal;
    sg_frame_stats_wgpu wgpu;
} sg_frame_stats;

/*
    sg_log_item

    An enum with a unique item for each log message, warning, error
    and validation layer message. Note that these messages are only
    visible when a logger function is installed in the sg_setup() call.
*/
#define _SG_LOG_ITEMS \
    _SG_LOGITEM_XMACRO(OK, "Ok") \
    _SG_LOGITEM_XMACRO(MALLOC_FAILED, "memory allocation failed") \
    _SG_LOGITEM_XMACRO(GL_TEXTURE_FORMAT_NOT_SUPPORTED, "pixel format not supported for texture (gl)") \
    _SG_LOGITEM_XMACRO(GL_3D_TEXTURES_NOT_SUPPORTED, "3d textures not supported (gl)") \
    _SG_LOGITEM_XMACRO(GL_ARRAY_TEXTURES_NOT_SUPPORTED, "array textures not supported (gl)") \
    _SG_LOGITEM_XMACRO(GL_STORAGEBUFFER_GLSL_BINDING_OUT_OF_RANGE, "GLSL storage buffer bindslot is out of range (must be 0..7) (gl)") \
    _SG_LOGITEM_XMACRO(GL_STORAGEIMAGE_GLSL_BINDING_OUT_OF_RANGE, "GLSL storage image bindslot is out of range (must be 0..3) (gl)") \
    _SG_LOGITEM_XMACRO(GL_SHADER_COMPILATION_FAILED, "shader compilation failed (gl)") \
    _SG_LOGITEM_XMACRO(GL_SHADER_LINKING_FAILED, "shader linking failed (gl)") \
    _SG_LOGITEM_XMACRO(GL_VERTEX_ATTRIBUTE_NOT_FOUND_IN_SHADER, "vertex attribute not found in shader; NOTE: may be caused by GL driver's GLSL compiler removing unused globals") \
    _SG_LOGITEM_XMACRO(GL_UNIFORMBLOCK_NAME_NOT_FOUND_IN_SHADER, "uniform block name not found in shader; NOTE: may be caused by GL driver's GLSL compiler removing unused globals") \
    _SG_LOGITEM_XMACRO(GL_IMAGE_SAMPLER_NAME_NOT_FOUND_IN_SHADER, "image-sampler name not found in shader; NOTE: may be caused by GL driver's GLSL compiler removing unused globals") \
    _SG_LOGITEM_XMACRO(GL_FRAMEBUFFER_STATUS_UNDEFINED, "framebuffer completeness check failed with GL_FRAMEBUFFER_UNDEFINED (gl)") \
    _SG_LOGITEM_XMACRO(GL_FRAMEBUFFER_STATUS_INCOMPLETE_ATTACHMENT, "framebuffer completeness check failed with GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT (gl)") \
    _SG_LOGITEM_XMACRO(GL_FRAMEBUFFER_STATUS_INCOMPLETE_MISSING_ATTACHMENT, "framebuffer completeness check failed with GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT (gl)") \
    _SG_LOGITEM_XMACRO(GL_FRAMEBUFFER_STATUS_UNSUPPORTED, "framebuffer completeness check failed with GL_FRAMEBUFFER_UNSUPPORTED (gl)") \
    _SG_LOGITEM_XMACRO(GL_FRAMEBUFFER_STATUS_INCOMPLETE_MULTISAMPLE, "framebuffer completeness check failed with GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE (gl)") \
    _SG_LOGITEM_XMACRO(GL_FRAMEBUFFER_STATUS_UNKNOWN, "framebuffer completeness check failed (unknown reason) (gl)") \
    _SG_LOGITEM_XMACRO(D3D11_CREATE_BUFFER_FAILED, "CreateBuffer() failed (d3d11)") \
    _SG_LOGITEM_XMACRO(D3D11_CREATE_BUFFER_SRV_FAILED, "CreateShaderResourceView() failed for storage buffer (d3d11)") \
    _SG_LOGITEM_XMACRO(D3D11_CREATE_BUFFER_UAV_FAILED, "CreateUnorderedAccessView() failed for storage buffer (d3d11)") \
    _SG_LOGITEM_XMACRO(D3D11_CREATE_DEPTH_TEXTURE_UNSUPPORTED_PIXEL_FORMAT, "pixel format not supported for depth-stencil texture (d3d11)") \
    _SG_LOGITEM_XMACRO(D3D11_CREATE_DEPTH_TEXTURE_FAILED, "CreateTexture2D() failed for depth-stencil texture (d3d11)") \
    _SG_LOGITEM_XMACRO(D3D11_CREATE_2D_TEXTURE_UNSUPPORTED_PIXEL_FORMAT, "pixel format not supported for 2d-, cube- or array-texture (d3d11)") \
    _SG_LOGITEM_XMACRO(D3D11_CREATE_2D_TEXTURE_FAILED, "CreateTexture2D() failed for 2d-, cube- or array-texture (d3d11)") \
    _SG_LOGITEM_XMACRO(D3D11_CREATE_2D_SRV_FAILED, "CreateShaderResourceView() failed for 2d-, cube- or array-texture (d3d11)") \
    _SG_LOGITEM_XMACRO(D3D11_CREATE_3D_TEXTURE_UNSUPPORTED_PIXEL_FORMAT, "pixel format not supported for 3D texture (d3d11)") \
    _SG_LOGITEM_XMACRO(D3D11_CREATE_3D_TEXTURE_FAILED, "CreateTexture3D() failed (d3d11)") \
    _SG_LOGITEM_XMACRO(D3D11_CREATE_3D_SRV_FAILED, "CreateShaderResourceView() failed for 3d texture (d3d11)") \
    _SG_LOGITEM_XMACRO(D3D11_CREATE_MSAA_TEXTURE_FAILED, "CreateTexture2D() failed for MSAA render target texture (d3d11)") \
    _SG_LOGITEM_XMACRO(D3D11_CREATE_SAMPLER_STATE_FAILED, "CreateSamplerState() failed (d3d11)") \
    _SG_LOGITEM_XMACRO(D3D11_UNIFORMBLOCK_HLSL_REGISTER_B_OUT_OF_RANGE, "sg_shader_desc.uniform_blocks[].hlsl_register_b_n is out of range (must be 0..7)") \
    _SG_LOGITEM_XMACRO(D3D11_STORAGEBUFFER_HLSL_REGISTER_T_OUT_OF_RANGE, "sg_shader_desc.views[].storage_buffer.hlsl_register_t_n is out of range (must be 0..23)") \
    _SG_LOGITEM_XMACRO(D3D11_STORAGEBUFFER_HLSL_REGISTER_U_OUT_OF_RANGE, "sg_shader_desc.views[].storage_buffer.hlsl_register_u_n is out of range (must be 0..11)") \
    _SG_LOGITEM_XMACRO(D3D11_IMAGE_HLSL_REGISTER_T_OUT_OF_RANGE, "sg_shader_desc.views[].texture.hlsl_register_t_n is out of range (must be 0..23)") \
    _SG_LOGITEM_XMACRO(D3D11_STORAGEIMAGE_HLSL_REGISTER_U_OUT_OF_RANGE, "sg_shader_desc.views[].storage_image.hlsl_register_u_n is out of range (must be 0..11)") \
    _SG_LOGITEM_XMACRO(D3D11_SAMPLER_HLSL_REGISTER_S_OUT_OF_RANGE, "sampler 'hlsl_register_s_n' is out of rang (must be 0..15)") \
    _SG_LOGITEM_XMACRO(D3D11_LOAD_D3DCOMPILER_47_DLL_FAILED, "loading d3dcompiler_47.dll failed (d3d11)") \
    _SG_LOGITEM_XMACRO(D3D11_SHADER_COMPILATION_FAILED, "shader compilation failed (d3d11)") \
    _SG_LOGITEM_XMACRO(D3D11_SHADER_COMPILATION_OUTPUT, "") \
    _SG_LOGITEM_XMACRO(D3D11_CREATE_CONSTANT_BUFFER_FAILED, "CreateBuffer() failed for uniform constant buffer (d3d11)") \
    _SG_LOGITEM_XMACRO(D3D11_CREATE_INPUT_LAYOUT_FAILED, "CreateInputLayout() failed (d3d11)") \
    _SG_LOGITEM_XMACRO(D3D11_CREATE_RASTERIZER_STATE_FAILED, "CreateRasterizerState() failed (d3d11)") \
    _SG_LOGITEM_XMACRO(D3D11_CREATE_DEPTH_STENCIL_STATE_FAILED, "CreateDepthStencilState() failed (d3d11)") \
    _SG_LOGITEM_XMACRO(D3D11_CREATE_BLEND_STATE_FAILED, "CreateBlendState() failed (d3d11)") \
    _SG_LOGITEM_XMACRO(D3D11_CREATE_RTV_FAILED, "CreateRenderTargetView() failed (d3d11)") \
    _SG_LOGITEM_XMACRO(D3D11_CREATE_DSV_FAILED, "CreateDepthStencilView() failed (d3d11)") \
    _SG_LOGITEM_XMACRO(D3D11_CREATE_UAV_FAILED, "CreateUnorderedAccessView() failed (d3d11)") \
    _SG_LOGITEM_XMACRO(D3D11_MAP_FOR_UPDATE_BUFFER_FAILED, "Map() failed when updating buffer (d3d11)") \
    _SG_LOGITEM_XMACRO(D3D11_MAP_FOR_APPEND_BUFFER_FAILED, "Map() failed when appending to buffer (d3d11)") \
    _SG_LOGITEM_XMACRO(D3D11_MAP_FOR_UPDATE_IMAGE_FAILED, "Map() failed when updating image (d3d11)") \
    _SG_LOGITEM_XMACRO(METAL_CREATE_BUFFER_FAILED, "failed to create buffer object (metal)") \
    _SG_LOGITEM_XMACRO(METAL_TEXTURE_FORMAT_NOT_SUPPORTED, "pixel format not supported for texture (metal)") \
    _SG_LOGITEM_XMACRO(METAL_CREATE_TEXTURE_FAILED, "failed to create texture object (metal)") \
    _SG_LOGITEM_XMACRO(METAL_CREATE_SAMPLER_FAILED, "failed to create sampler object (metal)") \
    _SG_LOGITEM_XMACRO(METAL_SHADER_COMPILATION_FAILED, "shader compilation failed (metal)") \
    _SG_LOGITEM_XMACRO(METAL_SHADER_CREATION_FAILED, "shader creation failed (metal)") \
    _SG_LOGITEM_XMACRO(METAL_SHADER_COMPILATION_OUTPUT, "") \
    _SG_LOGITEM_XMACRO(METAL_SHADER_ENTRY_NOT_FOUND, "shader entry function not found (metal)") \
    _SG_LOGITEM_XMACRO(METAL_UNIFORMBLOCK_MSL_BUFFER_SLOT_OUT_OF_RANGE, "uniform block 'msl_buffer_n' is out of range (must be 0..7)") \
    _SG_LOGITEM_XMACRO(METAL_STORAGEBUFFER_MSL_BUFFER_SLOT_OUT_OF_RANGE, "storage buffer 'msl_buffer_n' is out of range (must be 8..15)") \
    _SG_LOGITEM_XMACRO(METAL_STORAGEIMAGE_MSL_TEXTURE_SLOT_OUT_OF_RANGE, "storage image 'msl_texture_n' is out of range (must be 0..19)") \
    _SG_LOGITEM_XMACRO(METAL_IMAGE_MSL_TEXTURE_SLOT_OUT_OF_RANGE, "image 'msl_texture_n' is out of range (must be 0..19)") \
    _SG_LOGITEM_XMACRO(METAL_SAMPLER_MSL_SAMPLER_SLOT_OUT_OF_RANGE, "sampler 'msl_sampler_n' is out of range (must be 0..15)") \
    _SG_LOGITEM_XMACRO(METAL_CREATE_CPS_FAILED, "failed to create compute pipeline state (metal)") \
    _SG_LOGITEM_XMACRO(METAL_CREATE_CPS_OUTPUT, "") \
    _SG_LOGITEM_XMACRO(METAL_CREATE_RPS_FAILED, "failed to create render pipeline state (metal)") \
    _SG_LOGITEM_XMACRO(METAL_CREATE_RPS_OUTPUT, "") \
    _SG_LOGITEM_XMACRO(METAL_CREATE_DSS_FAILED, "failed to create depth stencil state (metal)") \
    _SG_LOGITEM_XMACRO(WGPU_BINDGROUPS_POOL_EXHAUSTED, "bindgroups pool exhausted (increase sg_desc.bindgroups_cache_size) (wgpu)") \
    _SG_LOGITEM_XMACRO(WGPU_BINDGROUPSCACHE_SIZE_GREATER_ONE, "sg_desc.wgpu_bindgroups_cache_size must be > 1 (wgpu)") \
    _SG_LOGITEM_XMACRO(WGPU_BINDGROUPSCACHE_SIZE_POW2, "sg_desc.wgpu_bindgroups_cache_size must be a power of 2 (wgpu)") \
    _SG_LOGITEM_XMACRO(WGPU_CREATEBINDGROUP_FAILED, "wgpuDeviceCreateBindGroup failed") \
    _SG_LOGITEM_XMACRO(WGPU_CREATE_BUFFER_FAILED, "wgpuDeviceCreateBuffer() failed") \
    _SG_LOGITEM_XMACRO(WGPU_CREATE_TEXTURE_FAILED, "wgpuDeviceCreateTexture() failed") \
    _SG_LOGITEM_XMACRO(WGPU_CREATE_TEXTURE_VIEW_FAILED, "wgpuTextureCreateView() failed") \
    _SG_LOGITEM_XMACRO(WGPU_CREATE_SAMPLER_FAILED, "wgpuDeviceCreateSampler() failed") \
    _SG_LOGITEM_XMACRO(WGPU_CREATE_SHADER_MODULE_FAILED, "wgpuDeviceCreateShaderModule() failed") \
    _SG_LOGITEM_XMACRO(WGPU_SHADER_CREATE_BINDGROUP_LAYOUT_FAILED, "wgpuDeviceCreateBindGroupLayout() for shader stage failed") \
    _SG_LOGITEM_XMACRO(WGPU_UNIFORMBLOCK_WGSL_GROUP0_BINDING_OUT_OF_RANGE, "uniform block 'wgsl_group0_binding_n' is out of range (must be 0..15)") \
    _SG_LOGITEM_XMACRO(WGPU_TEXTURE_WGSL_GROUP1_BINDING_OUT_OF_RANGE, "texture 'wgsl_group1_binding_n' is out of range (must be 0..127)") \
    _SG_LOGITEM_XMACRO(WGPU_STORAGEBUFFER_WGSL_GROUP1_BINDING_OUT_OF_RANGE, "storage buffer 'wgsl_group1_binding_n' is out of range (must be 0..127)") \
    _SG_LOGITEM_XMACRO(WGPU_STORAGEIMAGE_WGSL_GROUP1_BINDING_OUT_OF_RANGE, "storage image 'wgsl_group1_binding_n' is out of range (must be 0..127)") \
    _SG_LOGITEM_XMACRO(WGPU_SAMPLER_WGSL_GROUP1_BINDING_OUT_OF_RANGE, "sampler 'wgsl_group1_binding_n' is out of range (must be 0..127)") \
    _SG_LOGITEM_XMACRO(WGPU_CREATE_PIPELINE_LAYOUT_FAILED, "wgpuDeviceCreatePipelineLayout() failed") \
    _SG_LOGITEM_XMACRO(WGPU_CREATE_RENDER_PIPELINE_FAILED, "wgpuDeviceCreateRenderPipeline() failed") \
    _SG_LOGITEM_XMACRO(WGPU_CREATE_COMPUTE_PIPELINE_FAILED, "wgpuDeviceCreateComputePipeline() failed") \
    _SG_LOGITEM_XMACRO(IDENTICAL_COMMIT_LISTENER, "attempting to add identical commit listener") \
    _SG_LOGITEM_XMACRO(COMMIT_LISTENER_ARRAY_FULL, "commit listener array full") \
    _SG_LOGITEM_XMACRO(TRACE_HOOKS_NOT_ENABLED, "sg_install_trace_hooks() called, but SOKOL_TRACE_HOOKS is not defined") \
    _SG_LOGITEM_XMACRO(DEALLOC_BUFFER_INVALID_STATE, "sg_dealloc_buffer(): buffer must be in ALLOC state") \
    _SG_LOGITEM_XMACRO(DEALLOC_IMAGE_INVALID_STATE, "sg_dealloc_image(): image must be in alloc state") \
    _SG_LOGITEM_XMACRO(DEALLOC_SAMPLER_INVALID_STATE, "sg_dealloc_sampler(): sampler must be in alloc state") \
    _SG_LOGITEM_XMACRO(DEALLOC_SHADER_INVALID_STATE, "sg_dealloc_shader(): shader must be in ALLOC state") \
    _SG_LOGITEM_XMACRO(DEALLOC_PIPELINE_INVALID_STATE, "sg_dealloc_pipeline(): pipeline must be in ALLOC state") \
    _SG_LOGITEM_XMACRO(DEALLOC_VIEW_INVALID_STATE, "sg_dealloc_view(): view must be in ALLOC state") \
    _SG_LOGITEM_XMACRO(INIT_BUFFER_INVALID_STATE, "sg_init_buffer(): buffer must be in ALLOC state") \
    _SG_LOGITEM_XMACRO(INIT_IMAGE_INVALID_STATE, "sg_init_image(): image must be in ALLOC state") \
    _SG_LOGITEM_XMACRO(INIT_SAMPLER_INVALID_STATE, "sg_init_sampler(): sampler must be in ALLOC state") \
    _SG_LOGITEM_XMACRO(INIT_SHADER_INVALID_STATE, "sg_init_shader(): shader must be in ALLOC state") \
    _SG_LOGITEM_XMACRO(INIT_PIPELINE_INVALID_STATE, "sg_init_pipeline(): pipeline must be in ALLOC state") \
    _SG_LOGITEM_XMACRO(INIT_VIEW_INVALID_STATE, "sg_init_view(): view must be in ALLOC state") \
    _SG_LOGITEM_XMACRO(UNINIT_BUFFER_INVALID_STATE, "sg_uninit_buffer(): buffer must be in VALID, FAILED or ALLOC state") \
    _SG_LOGITEM_XMACRO(UNINIT_IMAGE_INVALID_STATE, "sg_uninit_image(): image must be in VALID, FAILED or ALLOC state") \
    _SG_LOGITEM_XMACRO(UNINIT_SAMPLER_INVALID_STATE, "sg_uninit_sampler(): sampler must be in VALID, FAILED or ALLOC state") \
    _SG_LOGITEM_XMACRO(UNINIT_SHADER_INVALID_STATE, "sg_uninit_shader(): shader must be in VALID, FAILED or ALLOC state") \
    _SG_LOGITEM_XMACRO(UNINIT_PIPELINE_INVALID_STATE, "sg_uninit_pipeline(): pipeline must be in VALID, FAILED or ALLOC state") \
    _SG_LOGITEM_XMACRO(UNINIT_VIEW_INVALID_STATE, "sg_uninit_view(): view must be in VALID, FAILED or ALLOC state") \
    _SG_LOGITEM_XMACRO(FAIL_BUFFER_INVALID_STATE, "sg_fail_buffer(): buffer must be in ALLOC state") \
    _SG_LOGITEM_XMACRO(FAIL_IMAGE_INVALID_STATE, "sg_fail_image(): image must be in ALLOC state") \
    _SG_LOGITEM_XMACRO(FAIL_SAMPLER_INVALID_STATE, "sg_fail_sampler(): sampler must be in ALLOC state") \
    _SG_LOGITEM_XMACRO(FAIL_SHADER_INVALID_STATE, "sg_fail_shader(): shader must be in ALLOC state") \
    _SG_LOGITEM_XMACRO(FAIL_PIPELINE_INVALID_STATE, "sg_fail_pipeline(): pipeline must be in ALLOC state") \
    _SG_LOGITEM_XMACRO(FAIL_VIEW_INVALID_STATE, "sg_fail_view(): view must be in ALLOC state") \
    _SG_LOGITEM_XMACRO(BUFFER_POOL_EXHAUSTED, "buffer pool exhausted") \
    _SG_LOGITEM_XMACRO(IMAGE_POOL_EXHAUSTED, "image pool exhausted") \
    _SG_LOGITEM_XMACRO(SAMPLER_POOL_EXHAUSTED, "sampler pool exhausted") \
    _SG_LOGITEM_XMACRO(SHADER_POOL_EXHAUSTED, "shader pool exhausted") \
    _SG_LOGITEM_XMACRO(PIPELINE_POOL_EXHAUSTED, "pipeline pool exhausted") \
    _SG_LOGITEM_XMACRO(VIEW_POOL_EXHAUSTED, "view pool exhausted") \
    _SG_LOGITEM_XMACRO(BEGINPASS_ATTACHMENTS_ALIVE, "sg_begin_pass: an attachment was provided that no longer exists") \
    _SG_LOGITEM_XMACRO(DRAW_WITHOUT_BINDINGS, "attempting to draw without resource bindings") \
    _SG_LOGITEM_XMACRO(VALIDATE_BUFFERDESC_CANARY, "sg_buffer_desc not initialized") \
    _SG_LOGITEM_XMACRO(VALIDATE_BUFFERDESC_IMMUTABLE_DYNAMIC_STREAM, "sg_buffer_desc.usage: only one of .immutable, .dynamic_update, .stream_update can be true") \
    _SG_LOGITEM_XMACRO(VALIDATE_BUFFERDESC_SEPARATE_BUFFER_TYPES, "sg_buffer_desc.usage: on WebGL2, only one of .vertex_buffer or .index_buffer can be true (check sg_features.separate_buffer_types)") \
    _SG_LOGITEM_XMACRO(VALIDATE_BUFFERDESC_EXPECT_NONZERO_SIZE, "sg_buffer_desc.size must be greater zero") \
    _SG_LOGITEM_XMACRO(VALIDATE_BUFFERDESC_EXPECT_MATCHING_DATA_SIZE, "sg_buffer_desc.size and .data.size must be equal") \
    _SG_LOGITEM_XMACRO(VALIDATE_BUFFERDESC_EXPECT_ZERO_DATA_SIZE, "sg_buffer_desc.data.size expected to be zero") \
    _SG_LOGITEM_XMACRO(VALIDATE_BUFFERDESC_EXPECT_NO_DATA, "sg_buffer_desc.data.ptr must be null for dynamic/stream buffers") \
    _SG_LOGITEM_XMACRO(VALIDATE_BUFFERDESC_EXPECT_DATA, "sg_buffer_desc: initial content data must be provided for immutable buffers without storage buffer usage") \
    _SG_LOGITEM_XMACRO(VALIDATE_BUFFERDESC_STORAGEBUFFER_SUPPORTED, "storage buffers not supported by the backend 3D API (requires OpenGL >= 4.3)") \
    _SG_LOGITEM_XMACRO(VALIDATE_BUFFERDESC_STORAGEBUFFER_SIZE_MULTIPLE_4, "size of storage buffers must be a multiple of 4") \
    _SG_LOGITEM_XMACRO(VALIDATE_IMAGEDATA_NODATA, "sg_image_data: no data (.ptr and/or .size is zero)") \
    _SG_LOGITEM_XMACRO(VALIDATE_IMAGEDATA_DATA_SIZE, "sg_image_data: data size doesn't match expected surface size") \
    _SG_LOGITEM_XMACRO(VALIDATE_IMAGEDESC_CANARY, "sg_image_desc not initialized") \
    _SG_LOGITEM_XMACRO(VALIDATE_IMAGEDESC_IMMUTABLE_DYNAMIC_STREAM, "sg_image_desc.usage: only one of .immutable, .dynamic_update, .stream_update can be true") \
    _SG_LOGITEM_XMACRO(VALIDATE_IMAGEDESC_WIDTH, "sg_image_desc.width must be > 0") \
    _SG_LOGITEM_XMACRO(VALIDATE_IMAGEDESC_HEIGHT, "sg_image_desc.height must be > 0") \
    _SG_LOGITEM_XMACRO(VALIDATE_IMAGEDESC_NONRT_PIXELFORMAT, "invalid pixel format for non-render-target image") \
    _SG_LOGITEM_XMACRO(VALIDATE_IMAGEDESC_MSAA_BUT_NO_ATTACHMENT, "non-attachment images cannot be multisampled") \
    _SG_LOGITEM_XMACRO(VALIDATE_IMAGEDESC_DEPTH_3D_IMAGE, "3D images cannot have a depth/stencil image format") \
    _SG_LOGITEM_XMACRO(VALIDATE_IMAGEDESC_ATTACHMENT_EXPECT_IMMUTABLE, "attachment and storage images must be sg_image_usage.immutable") \
    _SG_LOGITEM_XMACRO(VALIDATE_IMAGEDESC_ATTACHMENT_EXPECT_NO_DATA, "render/storage attachment images cannot be initialized with data") \
    _SG_LOGITEM_XMACRO(VALIDATE_IMAGEDESC_ATTACHMENT_PIXELFORMAT, "invalid pixel format for render attachment image") \
    _SG_LOGITEM_XMACRO(VALIDATE_IMAGEDESC_ATTACHMENT_RESOLVE_EXPECT_NO_MSAA, "resolve attachment images cannot be multisampled") \
    _SG_LOGITEM_XMACRO(VALIDATE_IMAGEDESC_ATTACHMENT_NO_MSAA_SUPPORT, "multisampling not supported for this pixel format") \
    _SG_LOGITEM_XMACRO(VALIDATE_IMAGEDESC_ATTACHMENT_MSAA_NUM_MIPMAPS, "multisample images must have num_mipmaps == 1") \
    _SG_LOGITEM_XMACRO(VALIDATE_IMAGEDESC_ATTACHMENT_MSAA_3D_IMAGE, "3D images cannot have a sample_count > 1") \
    _SG_LOGITEM_XMACRO(VALIDATE_IMAGEDESC_ATTACHMENT_MSAA_CUBE_IMAGE, "cube images cannot have sample_count > 1") \
    _SG_LOGITEM_XMACRO(VALIDATE_IMAGEDESC_ATTACHMENT_MSAA_ARRAY_IMAGE, "array images cannot have sample_count > 1") \
    _SG_LOGITEM_XMACRO(VALIDATE_IMAGEDESC_STORAGEIMAGE_PIXELFORMAT, "invalid pixel format for storage image") \
    _SG_LOGITEM_XMACRO(VALIDATE_IMAGEDESC_STORAGEIMAGE_EXPECT_NO_MSAA, "storage images cannot be multisampled") \
    _SG_LOGITEM_XMACRO(VALIDATE_IMAGEDESC_INJECTED_NO_DATA, "images with injected textures cannot be initialized with data") \
    _SG_LOGITEM_XMACRO(VALIDATE_IMAGEDESC_DYNAMIC_NO_DATA, "dynamic/stream-update images cannot be initialized with data") \
    _SG_LOGITEM_XMACRO(VALIDATE_IMAGEDESC_COMPRESSED_IMMUTABLE, "compressed images must be immutable") \
    _SG_LOGITEM_XMACRO(VALIDATE_SAMPLERDESC_CANARY, "sg_sampler_desc not initialized") \
    _SG_LOGITEM_XMACRO(VALIDATE_SAMPLERDESC_ANISTROPIC_REQUIRES_LINEAR_FILTERING, "sg_sampler_desc.max_anisotropy > 1 requires min/mag/mipmap_filter to be SG_FILTER_LINEAR") \
    _SG_LOGITEM_XMACRO(VALIDATE_SHADERDESC_CANARY, "sg_shader_desc not initialized") \
    _SG_LOGITEM_XMACRO(VALIDATE_SHADERDESC_VERTEX_SOURCE, "vertex shader source code expected") \
    _SG_LOGITEM_XMACRO(VALIDATE_SHADERDESC_FRAGMENT_SOURCE, "fragment shader source code expected") \
    _SG_LOGITEM_XMACRO(VALIDATE_SHADERDESC_COMPUTE_SOURCE, "compute shader source code expected") \
    _SG_LOGITEM_XMACRO(VALIDATE_SHADERDESC_VERTEX_SOURCE_OR_BYTECODE, "vertex shader source or byte code expected") \
    _SG_LOGITEM_XMACRO(VALIDATE_SHADERDESC_FRAGMENT_SOURCE_OR_BYTECODE, "fragment shader source or byte code expected") \
    _SG_LOGITEM_XMACRO(VALIDATE_SHADERDESC_COMPUTE_SOURCE_OR_BYTECODE, "compute shader source or byte code expected") \
    _SG_LOGITEM_XMACRO(VALIDATE_SHADERDESC_INVALID_SHADER_COMBO, "cannot combine compute shaders with vertex or fragment shaders") \
    _SG_LOGITEM_XMACRO(VALIDATE_SHADERDESC_NO_BYTECODE_SIZE, "shader byte code length (in bytes) required") \
    _SG_LOGITEM_XMACRO(VALIDATE_SHADERDESC_METAL_THREADS_PER_THREADGROUP_INITIALIZED, "sg_shader_desc.mtl_threads_per_threadgroup must be initialized for compute shaders (metal)") \
    _SG_LOGITEM_XMACRO(VALIDATE_SHADERDESC_METAL_THREADS_PER_THREADGROUP_MULTIPLE_32, "sg_shader_desc.mtl_threads_per_threadgroup (x * y * z) must be a multiple of 32 (metal)") \
    _SG_LOGITEM_XMACRO(VALIDATE_SHADERDESC_UNIFORMBLOCK_NO_CONT_MEMBERS, "sg_shader_desc.uniform_blocks[].glsl_uniforms[]: items must occupy continuous slots") \
    _SG_LOGITEM_XMACRO(VALIDATE_SHADERDESC_UNIFORMBLOCK_SIZE_IS_ZERO, "sg_shader_desc.uniform_blocks[].size cannot be zero") \
    _SG_LOGITEM_XMACRO(VALIDATE_SHADERDESC_UNIFORMBLOCK_METAL_BUFFER_SLOT_OUT_OF_RANGE, "sg_shader_desc.uniform_blocks[].msl_buffer_n is out of range (must be 0..7)") \
    _SG_LOGITEM_XMACRO(VALIDATE_SHADERDESC_UNIFORMBLOCK_METAL_BUFFER_SLOT_COLLISION, "sg_shader_desc.uniform_blocks[].msl_buffer_n must be unique across uniform blocks and storage buffers in same shader stage") \
    _SG_LOGITEM_XMACRO(VALIDATE_SHADERDESC_UNIFORMBLOCK_HLSL_REGISTER_B_OUT_OF_RANGE, "sg_shader_desc.uniform_blocks[]hlsl_register_b_n is out of range (must be 0..7)") \
    _SG_LOGITEM_XMACRO(VALIDATE_SHADERDESC_UNIFORMBLOCK_HLSL_REGISTER_B_COLLISION, "sg_shader_desc.uniform_blocks[].hlsl_register_b_n must be unique across uniform blocks in same shader stage") \
    _SG_LOGITEM_XMACRO(VALIDATE_SHADERDESC_UNIFORMBLOCK_WGSL_GROUP0_BINDING_OUT_OF_RANGE, "sg_shader_desc.uniform_blocks[].wgsl_group0_binding_n is out of range (must be 0..15)") \
    _SG_LOGITEM_XMACRO(VALIDATE_SHADERDESC_UNIFORMBLOCK_WGSL_GROUP0_BINDING_COLLISION, "sg_shader_desc.uniform_blocks[].wgsl_group0_binding_n must be unique across all uniform blocks") \
    _SG_LOGITEM_XMACRO(VALIDATE_SHADERDESC_UNIFORMBLOCK_NO_MEMBERS, "sg_shader_desc.uniform_blocks[].glsl_uniforms[]: GL backend requires uniform block member declarations") \
    _SG_LOGITEM_XMACRO(VALIDATE_SHADERDESC_UNIFORMBLOCK_UNIFORM_GLSL_NAME, "sg_shader_desc.uniform_blocks[].glsl_uniforms[].glsl_name missing") \
    _SG_LOGITEM_XMACRO(VALIDATE_SHADERDESC_UNIFORMBLOCK_SIZE_MISMATCH, "sg_shader_desc.uniform_blocks[].glsl_uniforms[]: size of uniform block members doesn't match uniform block size") \
    _SG_LOGITEM_XMACRO(VALIDATE_SHADERDESC_UNIFORMBLOCK_ARRAY_COUNT, "sg_shader_desc.uniform_blocks[].glsl_uniforms[].array_count must be >= 1") \
    _SG_LOGITEM_XMACRO(VALIDATE_SHADERDESC_UNIFORMBLOCK_STD140_ARRAY_TYPE, "sg_shader_desc.uniform_blocks[].glsl_uniforms[].type: uniform arrays only allowed for FLOAT4, INT4, MAT4 in std140 layout") \
    _SG_LOGITEM_XMACRO(VALIDATE_SHADERDESC_VIEW_STORAGEBUFFER_METAL_BUFFER_SLOT_OUT_OF_RANGE, "sg_shader_desc.views[].storage_buffer.msl_buffer_n is out of range (must be 8..15)") \
    _SG_LOGITEM_XMACRO(VALIDATE_SHADERDESC_VIEW_STORAGEBUFFER_METAL_BUFFER_SLOT_COLLISION, "sg_shader_desc.views[].storage_buffer.storagemsl_buffer_n must be unique across uniform blocks and storage buffer in same shader stage") \
    _SG_LOGITEM_XMACRO(VALIDATE_SHADERDESC_VIEW_STORAGEBUFFER_HLSL_REGISTER_T_OUT_OF_RANGE, "sg_shader_desc.views[].storage_buffer.hlsl_register_t_n is out of range (must be 0..23)") \
    _SG_LOGITEM_XMACRO(VALIDATE_SHADERDESC_VIEW_STORAGEBUFFER_HLSL_REGISTER_T_COLLISION, "sg_shader_desc.views[].storage_buffer.hlsl_register_t_n must be unique across read-only storage buffers and images in same shader stage") \
    _SG_LOGITEM_XMACRO(VALIDATE_SHADERDESC_VIEW_STORAGEBUFFER_HLSL_REGISTER_U_OUT_OF_RANGE, "sg_shader_desc.views[].storage_buffer.hlsl_register_u_n is out of range (must be 0..11)") \
    _SG_LOGITEM_XMACRO(VALIDATE_SHADERDESC_VIEW_STORAGEBUFFER_HLSL_REGISTER_U_COLLISION, "sg_shader_desc.views[].storage_buffer.hlsl_register_u_n must be unique across read/write storage buffers and storage images in same shader stage") \
    _SG_LOGITEM_XMACRO(VALIDATE_SHADERDESC_VIEW_STORAGEBUFFER_GLSL_BINDING_OUT_OF_RANGE, "sg_shader_desc.views[].storage_buffer.glsl_binding_n is out of range (must be 0..7)") \
    _SG_LOGITEM_XMACRO(VALIDATE_SHADERDESC_VIEW_STORAGEBUFFER_GLSL_BINDING_COLLISION, "sg_shader_desc.views[].storage_buffer.glsl_binding_n must be unique across shader stages") \
    _SG_LOGITEM_XMACRO(VALIDATE_SHADERDESC_VIEW_STORAGEBUFFER_WGSL_GROUP1_BINDING_OUT_OF_RANGE, "sg_shader_desc.views[].storage_buffer.wgsl_group1_binding_n is out of range (must be 0..127)") \
    _SG_LOGITEM_XMACRO(VALIDATE_SHADERDESC_VIEW_STORAGEBUFFER_WGSL_GROUP1_BINDING_COLLISION, "sg_shader_desc.views[].storage_buffer.wgsl_group1_binding_n must be unique across all images, samplers and storage buffers") \
    _SG_LOGITEM_XMACRO(VALIDATE_SHADERDESC_VIEW_STORAGEIMAGE_EXPECT_COMPUTE_STAGE, "sg_shader_desc.views[].storage_image: storage images are allowed on the compute stage") \
    _SG_LOGITEM_XMACRO(VALIDATE_SHADERDESC_VIEW_STORAGEIMAGE_METAL_TEXTURE_SLOT_OUT_OF_RANGE, "sg_shader_desc.views[].storage_image.msl_texture_n is out of range (must be 0..19") \
    _SG_LOGITEM_XMACRO(VALIDATE_SHADERDESC_VIEW_STORAGEIMAGE_METAL_TEXTURE_SLOT_COLLISION, "sg_shader_desc.views[].storage_image.msl_texture_n must be unique across images and storage images in same shader stage") \
    _SG_LOGITEM_XMACRO(VALIDATE_SHADERDESC_VIEW_STORAGEIMAGE_HLSL_REGISTER_U_OUT_OF_RANGE, "sg_shader_desc.views[].storage_image.hlsl_register_u_n is out of range (must be 0..11)") \
    _SG_LOGITEM_XMACRO(VALIDATE_SHADERDESC_VIEW_STORAGEIMAGE_HLSL_REGISTER_U_COLLISION, "sg_shader_desc.views[].storage_image.hlsl_register_u_n must be unique across storage images and read/write storage buffers in same shader stage") \
    _SG_LOGITEM_XMACRO(VALIDATE_SHADERDESC_VIEW_STORAGEIMAGE_GLSL_BINDING_OUT_OF_RANGE, "sg_shader_desc.views[].storage_image.glsl_binding_n is out of range (must be 0..4)") \
    _SG_LOGITEM_XMACRO(VALIDATE_SHADERDESC_VIEW_STORAGEIMAGE_GLSL_BINDING_COLLISION, "sg_shader_desc.views[].storage_image.glsl_binding_n must be unique across shader stages") \
    _SG_LOGITEM_XMACRO(VALIDATE_SHADERDESC_VIEW_STORAGEIMAGE_WGSL_GROUP1_BINDING_OUT_OF_RANGE, "sg_shader_desc.views[].storage_image.wgsl_group1_binding_n is out of range (must be 0..7)") \
    _SG_LOGITEM_XMACRO(VALIDATE_SHADERDESC_VIEW_STORAGEIMAGE_WGSL_GROUP1_BINDING_COLLISION, "sg_shader_desc.views[].storage_image.wgsl_group1_binding_n must be unique in same shader stage") \
    _SG_LOGITEM_XMACRO(VALIDATE_SHADERDESC_VIEW_TEXTURE_METAL_TEXTURE_SLOT_OUT_OF_RANGE, "sg_shader_desc.views[].texture.msl_texture_n is out of range (must be 0..19)") \
    _SG_LOGITEM_XMACRO(VALIDATE_SHADERDESC_VIEW_TEXTURE_METAL_TEXTURE_SLOT_COLLISION, "sg_shader_desc.views[].texture.msl_texture_n must be unique across textures and storage images in same shader stage") \
    _SG_LOGITEM_XMACRO(VALIDATE_SHADERDESC_VIEW_TEXTURE_HLSL_REGISTER_T_OUT_OF_RANGE, "sg_shader_desc.views[].texture.hlsl_register_t_n is out of range (must be 0..23)") \
    _SG_LOGITEM_XMACRO(VALIDATE_SHADERDESC_VIEW_TEXTURE_HLSL_REGISTER_T_COLLISION, "sg_shader_desc.views[].texture.hlsl_register_t_n must be unique across textures and storage buffers in same shader stage") \
    _SG_LOGITEM_XMACRO(VALIDATE_SHADERDESC_VIEW_TEXTURE_WGSL_GROUP1_BINDING_OUT_OF_RANGE, "sg_shader_desc.views[].texture.wgsl_group1_binding_n is out of range (must be 0..127)") \
    _SG_LOGITEM_XMACRO(VALIDATE_SHADERDESC_VIEW_TEXTURE_WGSL_GROUP1_BINDING_COLLISION, "sg_shader_desc.views[].texture.wgsl_group1_binding_n must be unique across all images, samplers and storage buffers") \
    _SG_LOGITEM_XMACRO(VALIDATE_SHADERDESC_SAMPLER_METAL_SAMPLER_SLOT_OUT_OF_RANGE, "sg_shader_desc.samplers[].msl_sampler_n is out of range (must be 0..15)") \
    _SG_LOGITEM_XMACRO(VALIDATE_SHADERDESC_SAMPLER_METAL_SAMPLER_SLOT_COLLISION, "sg_shader_desc.samplers[].msl_sampler_n must be unique in same shader stage") \
    _SG_LOGITEM_XMACRO(VALIDATE_SHADERDESC_SAMPLER_HLSL_REGISTER_S_OUT_OF_RANGE, "sg_shader_desc.samplers[].hlsl_register_s_n is out of rang (must be 0..15)") \
    _SG_LOGITEM_XMACRO(VALIDATE_SHADERDESC_SAMPLER_HLSL_REGISTER_S_COLLISION, "sg_shader_desc.samplers[].hlsl_register_s_n must be unique in same shader stage") \
    _SG_LOGITEM_XMACRO(VALIDATE_SHADERDESC_SAMPLER_WGSL_GROUP1_BINDING_OUT_OF_RANGE, "sg_shader_desc.samplers[].wgsl_group1_binding_n is out of range (must be 0..127)") \
    _SG_LOGITEM_XMACRO(VALIDATE_SHADERDESC_SAMPLER_WGSL_GROUP1_BINDING_COLLISION, "sg_shader_desc.samplers[].wgsl_group1_binding_n must be unique across all images, samplers and storage buffers") \
    _SG_LOGITEM_XMACRO(VALIDATE_SHADERDESC_TEXTURE_SAMPLER_PAIR_VIEW_SLOT_OUT_OF_RANGE, "texture-sampler-pair view slot index is out of range (sg_shader_desc.texture_sampler_pairs[].view_slot)") \
    _SG_LOGITEM_XMACRO(VALIDATE_SHADERDESC_TEXTURE_SAMPLER_PAIR_SAMPLER_SLOT_OUT_OF_RANGE, "texture-sampler-pair sampler slot index is out of range (sg_shader_desc.texture_sampler_pairs[].sampler_slot)") \
    _SG_LOGITEM_XMACRO(VALIDATE_SHADERDESC_TEXTURE_SAMPLER_PAIR_TEXTURE_STAGE_MISMATCH, "texture-sampler-pair stage doesn't match referenced texture stage") \
    _SG_LOGITEM_XMACRO(VALIDATE_SHADERDESC_TEXTURE_SAMPLER_PAIR_EXPECT_TEXTURE_VIEW, "texture-sampler-pair view must be a texture view (sg_shader_desc.texture_sampler_pairs[].view_slot => sg_shaders_desc.views[i].texture)") \
    _SG_LOGITEM_XMACRO(VALIDATE_SHADERDESC_TEXTURE_SAMPLER_PAIR_SAMPLER_STAGE_MISMATCH, "texture-sampler-pair stage doesn't match referenced sampler stage") \
    _SG_LOGITEM_XMACRO(VALIDATE_SHADERDESC_TEXTURE_SAMPLER_PAIR_GLSL_NAME, "texture-sampler-pair 'glsl_name' missing") \
    _SG_LOGITEM_XMACRO(VALIDATE_SHADERDESC_NONFILTERING_SAMPLER_REQUIRED, "image sample type UNFILTERABLE_FLOAT, UINT, SINT can only be used with NONFILTERING sampler") \
    _SG_LOGITEM_XMACRO(VALIDATE_SHADERDESC_COMPARISON_SAMPLER_REQUIRED, "image sample type DEPTH can only be used with COMPARISON sampler") \
    _SG_LOGITEM_XMACRO(VALIDATE_SHADERDESC_TEXVIEW_NOT_REFERENCED_BY_TEXTURE_SAMPLER_PAIRS, "one or more texture views are not referenced by by texture-sampler-pairs (sg_shader_desc.texture_sampler_pairs[].view_slot)") \
    _SG_LOGITEM_XMACRO(VALIDATE_SHADERDESC_SAMPLER_NOT_REFERENCED_BY_TEXTURE_SAMPLER_PAIRS, "one or more samplers are not referenced by texture-sampler-pairs (sg_shader_desc.texture_sampler_pairs[].sampler_slot)") \
    _SG_LOGITEM_XMACRO(VALIDATE_SHADERDESC_ATTR_STRING_TOO_LONG, "vertex attribute name/semantic string too long (max len 16)") \
    _SG_LOGITEM_XMACRO(VALIDATE_PIPELINEDESC_CANARY, "sg_pipeline_desc not initialized") \
    _SG_LOGITEM_XMACRO(VALIDATE_PIPELINEDESC_SHADER, "sg_pipeline_desc.shader missing or invalid") \
    _SG_LOGITEM_XMACRO(VALIDATE_PIPELINEDESC_COMPUTE_SHADER_EXPECTED, "sg_pipeline_desc.shader must be a compute shader") \
    _SG_LOGITEM_XMACRO(VALIDATE_PIPELINEDESC_NO_COMPUTE_SHADER_EXPECTED, "sg_pipeline_desc.compute is false, but shader is a compute shader") \
    _SG_LOGITEM_XMACRO(VALIDATE_PIPELINEDESC_NO_CONT_ATTRS, "sg_pipeline_desc.layout.attrs is not continuous") \
    _SG_LOGITEM_XMACRO(VALIDATE_PIPELINEDESC_ATTR_BASETYPE_MISMATCH, "sg_pipeline_desc.layout.attrs[].format is incompatible with sg_shader_desc.attrs[].base_type") \
    _SG_LOGITEM_XMACRO(VALIDATE_PIPELINEDESC_LAYOUT_STRIDE4, "sg_pipeline_desc.layout.buffers[].stride must be multiple of 4") \
    _SG_LOGITEM_XMACRO(VALIDATE_PIPELINEDESC_ATTR_SEMANTICS, "D3D11 missing vertex attribute semantics in shader") \
    _SG_LOGITEM_XMACRO(VALIDATE_PIPELINEDESC_SHADER_READONLY_STORAGEBUFFERS, "sg_pipeline_desc.shader: only readonly storage buffer bindings allowed in render pipelines") \
    _SG_LOGITEM_XMACRO(VALIDATE_PIPELINEDESC_BLENDOP_MINMAX_REQUIRES_BLENDFACTOR_ONE, "SG_BLENDOP_MIN/MAX requires all blend factors to be SG_BLENDFACTOR_ONE") \
    _SG_LOGITEM_XMACRO(VALIDATE_VIEWDESC_CANARY, "sg_view_desc not initialized") \
    _SG_LOGITEM_XMACRO(VALIDATE_VIEWDESC_UNIQUE_VIEWTYPE, "sg_view_desc: only one view type can be active") \
    _SG_LOGITEM_XMACRO(VALIDATE_VIEWDESC_ANY_VIEWTYPE, "sg_view_desc: exactly one view type must be active") \
    _SG_LOGITEM_XMACRO(VALIDATE_VIEWDESC_RESOURCE_ALIVE, "sg_view_desc: resource object is no longer alive (.buffer or .image)") \
    _SG_LOGITEM_XMACRO(VALIDATE_VIEWDESC_RESOURCE_FAILED, "sg_view_desc: resource object cannot be in FAILED state (.buffer or .image)") \
    _SG_LOGITEM_XMACRO(VALIDATE_VIEWDESC_STORAGEBUFFER_OFFSET_VS_BUFFER_SIZE, "sg_view_desc.storage_buffer.offset is >= buffer size") \
    _SG_LOGITEM_XMACRO(VALIDATE_VIEWDESC_STORAGEBUFFER_OFFSET_MULTIPLE_256, "sg_view_desc.storage_buffer.offset must be a multiple of 256") \
    _SG_LOGITEM_XMACRO(VALIDATE_VIEWDESC_STORAGEBUFFER_USAGE, "sg_view_desc.storage_buffer.buffer must have been created with sg_buffer_desc.usage.storage_buffer = true") \
    _SG_LOGITEM_XMACRO(VALIDATE_VIEWDESC_STORAGEIMAGE_USAGE, "sg_view_desc.storage_image.image must have been created with sg_image_desc.usage.storage_image = true") \
    _SG_LOGITEM_XMACRO(VALIDATE_VIEWDESC_COLORATTACHMENT_USAGE, "sg_view_desc.color_attachment.image must have been created with sg_image_desc.usage.color_attachment = true") \
    _SG_LOGITEM_XMACRO(VALIDATE_VIEWDESC_RESOLVEATTACHMENT_USAGE, "sg_view_desc.resolve_attachment.image must have been created with sg_image_desc.usage.resolve_attachment = true") \
    _SG_LOGITEM_XMACRO(VALIDATE_VIEWDESC_DEPTHSTENCILATTACHMENT_USAGE, "sg_view_desc.depth_stencil_attachment.image must have been created with sg_image_desc.usage.depth_stencil_attachment = true") \
    _SG_LOGITEM_XMACRO(VALIDATE_VIEWDESC_IMAGE_MIPLEVEL, "sg_view_desc: image/attachment view mip level is out of range (must be >=0 and <image.num_miplevels)") \
    _SG_LOGITEM_XMACRO(VALIDATE_VIEWDESC_IMAGE_2D_SLICE, "sg_view_desc: image/attachment view slice is out of range for 2D image (must be 0)") \
    _SG_LOGITEM_XMACRO(VALIDATE_VIEWDESC_IMAGE_CUBEMAP_SLICE, "sg_view_desc: image/attachment view slice is out of range for cubemap image (must be >=0 and <6)") \
    _SG_LOGITEM_XMACRO(VALIDATE_VIEWDESC_IMAGE_ARRAY_SLICE, "sg_view_desc: image/attachment view slice is out of range for 2D array image (must be >=0 and <image.num_slices") \
    _SG_LOGITEM_XMACRO(VALIDATE_VIEWDESC_IMAGE_3D_SLICE, "sg_view_desc: image/attachment view slice is out of range for 3D image (must be 0)") \
    _SG_LOGITEM_XMACRO(VALIDATE_VIEWDESC_TEXTURE_EXPECT_NO_MSAA, "sg_view_desc: MSAA texture bindings not allowed on this backend (sg_features.msaa_texture_bindings)") \
    _SG_LOGITEM_XMACRO(VALIDATE_VIEWDESC_TEXTURE_MIPLEVELS, "sg_view_desc: texture view mip levels are out of range (must be >=0 and <image.num_miplevels)") \
    _SG_LOGITEM_XMACRO(VALIDATE_VIEWDESC_TEXTURE_2D_SLICES, "sg_view_desc: texture view slices are out of range for 2D image (must be 0)") \
    _SG_LOGITEM_XMACRO(VALIDATE_VIEWDESC_TEXTURE_CUBEMAP_SLICES, "sg_view_desc: texture view slices are out of range for cubemap image (must be 0)") \
    _SG_LOGITEM_XMACRO(VALIDATE_VIEWDESC_TEXTURE_ARRAY_SLICES, "sg_view_desc: texture view slices are out of range for 2D array image (must be >=0 and <image.num_slices") \
    _SG_LOGITEM_XMACRO(VALIDATE_VIEWDESC_TEXTURE_3D_SLICES, "sg_view_desc: texture view slices are out of range for 3D image (must be 0)") \
    _SG_LOGITEM_XMACRO(VALIDATE_VIEWDESC_STORAGEIMAGE_PIXELFORMAT, "sg_view_desc.storage_image_binding: image pixel format must be GPU readable or writable (sg_pixelformat_info.read/write)") \
    _SG_LOGITEM_XMACRO(VALIDATE_VIEWDESC_COLORATTACHMENT_PIXELFORMAT, "sg_view_desc.color_attachment: pixel format of image must be renderable (sg_pixelformat_info.render)") \
    _SG_LOGITEM_XMACRO(VALIDATE_VIEWDESC_DEPTHSTENCILATTACHMENT_PIXELFORMAT, "sg_view_desc.depth_stencil_attachment: pixel format of image must be a depth or depth-stencil format (sg_pixelformat_info.depth)") \
    _SG_LOGITEM_XMACRO(VALIDATE_VIEWDESC_RESOLVEATTACHMENT_SAMPLECOUNT, "sg_view_desc.resolve_attachment: image cannot be multisampled") \
    _SG_LOGITEM_XMACRO(VALIDATE_BEGINPASS_CANARY, "sg_begin_pass: pass struct not initialized") \
    _SG_LOGITEM_XMACRO(VALIDATE_BEGINPASS_COMPUTEPASS_EXPECT_NO_ATTACHMENTS, "sg_begin_pass: compute passes cannot have attachments") \
    _SG_LOGITEM_XMACRO(VALIDATE_BEGINPASS_SWAPCHAIN_EXPECT_WIDTH, "sg_begin_pass: expected pass.swapchain.width > 0") \
    _SG_LOGITEM_XMACRO(VALIDATE_BEGINPASS_SWAPCHAIN_EXPECT_WIDTH_NOTSET, "sg_begin_pass: expected pass.swapchain.width == 0") \
    _SG_LOGITEM_XMACRO(VALIDATE_BEGINPASS_SWAPCHAIN_EXPECT_HEIGHT, "sg_begin_pass: expected pass.swapchain.height > 0") \
    _SG_LOGITEM_XMACRO(VALIDATE_BEGINPASS_SWAPCHAIN_EXPECT_HEIGHT_NOTSET, "sg_begin_pass: expected pass.swapchain.height == 0") \
    _SG_LOGITEM_XMACRO(VALIDATE_BEGINPASS_SWAPCHAIN_EXPECT_SAMPLECOUNT, "sg_begin_pass: expected pass.swapchain.sample_count > 0") \
    _SG_LOGITEM_XMACRO(VALIDATE_BEGINPASS_SWAPCHAIN_EXPECT_SAMPLECOUNT_NOTSET, "sg_begin_pass: expected pass.swapchain.sample_count == 0") \
    _SG_LOGITEM_XMACRO(VALIDATE_BEGINPASS_SWAPCHAIN_EXPECT_COLORFORMAT, "sg_begin_pass: expected pass.swapchain.color_format to be valid") \
    _SG_LOGITEM_XMACRO(VALIDATE_BEGINPASS_SWAPCHAIN_EXPECT_COLORFORMAT_NOTSET, "sg_begin_pass: expected pass.swapchain.color_format to be unset") \
    _SG_LOGITEM_XMACRO(VALIDATE_BEGINPASS_SWAPCHAIN_EXPECT_DEPTHFORMAT_NOTSET, "sg_begin_pass: expected pass.swapchain.depth_format to be unset") \
    _SG_LOGITEM_XMACRO(VALIDATE_BEGINPASS_SWAPCHAIN_METAL_EXPECT_CURRENTDRAWABLE, "sg_begin_pass: expected pass.swapchain.metal.current_drawable != 0") \
    _SG_LOGITEM_XMACRO(VALIDATE_BEGINPASS_SWAPCHAIN_METAL_EXPECT_CURRENTDRAWABLE_NOTSET, "sg_begin_pass: expected pass.swapchain.metal.current_drawable == 0") \
    _SG_LOGITEM_XMACRO(VALIDATE_BEGINPASS_SWAPCHAIN_METAL_EXPECT_DEPTHSTENCILTEXTURE, "sg_begin_pass: expected pass.swapchain.metal.depth_stencil_texture != 0") \
    _SG_LOGITEM_XMACRO(VALIDATE_BEGINPASS_SWAPCHAIN_METAL_EXPECT_DEPTHSTENCILTEXTURE_NOTSET, "sg_begin_pass: expected pass.swapchain.metal.depth_stencil_texture == 0") \
    _SG_LOGITEM_XMACRO(VALIDATE_BEGINPASS_SWAPCHAIN_METAL_EXPECT_MSAACOLORTEXTURE, "sg_begin_pass: expected pass.swapchain.metal.msaa_color_texture != 0") \
    _SG_LOGITEM_XMACRO(VALIDATE_BEGINPASS_SWAPCHAIN_METAL_EXPECT_MSAACOLORTEXTURE_NOTSET, "sg_begin_pass: expected pass.swapchain.metal.msaa_color_texture == 0") \
    _SG_LOGITEM_XMACRO(VALIDATE_BEGINPASS_SWAPCHAIN_D3D11_EXPECT_RENDERVIEW, "sg_begin_pass: expected pass.swapchain.d3d11.render_view != 0") \
    _SG_LOGITEM_XMACRO(VALIDATE_BEGINPASS_SWAPCHAIN_D3D11_EXPECT_RENDERVIEW_NOTSET, "sg_begin_pass: expected pass.swapchain.d3d11.render_view == 0") \
    _SG_LOGITEM_XMACRO(VALIDATE_BEGINPASS_SWAPCHAIN_D3D11_EXPECT_RESOLVEVIEW, "sg_begin_pass: expected pass.swapchain.d3d11.resolve_view != 0") \
    _SG_LOGITEM_XMACRO(VALIDATE_BEGINPASS_SWAPCHAIN_D3D11_EXPECT_RESOLVEVIEW_NOTSET, "sg_begin_pass: expected pass.swapchain.d3d11.resolve_view == 0") \
    _SG_LOGITEM_XMACRO(VALIDATE_BEGINPASS_SWAPCHAIN_D3D11_EXPECT_DEPTHSTENCILVIEW, "sg_begin_pass: expected pass.swapchain.d3d11.depth_stencil_view != 0") \
    _SG_LOGITEM_XMACRO(VALIDATE_BEGINPASS_SWAPCHAIN_D3D11_EXPECT_DEPTHSTENCILVIEW_NOTSET, "sg_begin_pass: expected pass.swapchain.d3d11.depth_stencil_view == 0") \
    _SG_LOGITEM_XMACRO(VALIDATE_BEGINPASS_SWAPCHAIN_WGPU_EXPECT_RENDERVIEW, "sg_begin_pass: expected pass.swapchain.wgpu.render_view != 0") \
    _SG_LOGITEM_XMACRO(VALIDATE_BEGINPASS_SWAPCHAIN_WGPU_EXPECT_RENDERVIEW_NOTSET, "sg_begin_pass: expected pass.swapchain.wgpu.render_view == 0") \
    _SG_LOGITEM_XMACRO(VALIDATE_BEGINPASS_SWAPCHAIN_WGPU_EXPECT_RESOLVEVIEW, "sg_begin_pass: expected pass.swapchain.wgpu.resolve_view != 0") \
    _SG_LOGITEM_XMACRO(VALIDATE_BEGINPASS_SWAPCHAIN_WGPU_EXPECT_RESOLVEVIEW_NOTSET, "sg_begin_pass: expected pass.swapchain.wgpu.resolve_view == 0") \
    _SG_LOGITEM_XMACRO(VALIDATE_BEGINPASS_SWAPCHAIN_WGPU_EXPECT_DEPTHSTENCILVIEW, "sg_begin_pass: expected pass.swapchain.wgpu.depth_stencil_view != 0") \
    _SG_LOGITEM_XMACRO(VALIDATE_BEGINPASS_SWAPCHAIN_WGPU_EXPECT_DEPTHSTENCILVIEW_NOTSET, "sg_begin_pass: expected pass.swapchain.wgpu.depth_stencil_view == 0") \
    _SG_LOGITEM_XMACRO(VALIDATE_BEGINPASS_SWAPCHAIN_GL_EXPECT_FRAMEBUFFER_NOTSET, "sg_begin_pass: expected pass.swapchain.gl.framebuffer == 0") \
    _SG_LOGITEM_XMACRO(VALIDATE_BEGINPASS_COLORATTACHMENTVIEWS_CONTINUOUS, "sg_begin_pass: color attachment view array must be continuous") \
    _SG_LOGITEM_XMACRO(VALIDATE_BEGINPASS_COLORATTACHMENTVIEW_ALIVE, "sg_begin_pass: color attachment view no longer alive") \
    _SG_LOGITEM_XMACRO(VALIDATE_BEGINPASS_COLORATTACHMENTVIEW_VALID, "sg_begin_pass: color attachment view not in valid state (SG_RESOURCESTATE_VALID)") \
    _SG_LOGITEM_XMACRO(VALIDATE_BEGINPASS_COLORATTACHMENTVIEW_TYPE, "sg_begin_pass: color attachment view has wrong type (must be sg_view_desc.color_attachment)") \
    _SG_LOGITEM_XMACRO(VALIDATE_BEGINPASS_COLORATTACHMENTVIEW_IMAGE_ALIVE, "sg_begin_pass: color attachment view's image object is uninitialized or no longer alive") \
    _SG_LOGITEM_XMACRO(VALIDATE_BEGINPASS_COLORATTACHMENTVIEW_IMAGE_VALID, "sg_begin_pass: color attachment view's image is not in valid state (SG_RESOURCESTATE_VALID)") \
    _SG_LOGITEM_XMACRO(VALIDATE_BEGINPASS_COLORATTACHMENTVIEW_SIZES, "sg_begin_pass: all color attachments must have the same width and height") \
    _SG_LOGITEM_XMACRO(VALIDATE_BEGINPASS_COLORATTACHMENTVIEW_SAMPLECOUNTS, "sg_begin_pass: all color attachments must have the same sample count") \
    _SG_LOGITEM_XMACRO(VALIDATE_BEGINPASS_RESOLVEATTACHMENTVIEW_NO_COLORATTACHMENTVIEW, "sg_begin_pass: a resolve attachment view must have an associated color attachment view at the same index") \
    _SG_LOGITEM_XMACRO(VALIDATE_BEGINPASS_RESOLVEATTACHMENTVIEW_ALIVE, "sg_begin_pass: resolve attachment view no longer alive") \
    _SG_LOGITEM_XMACRO(VALIDATE_BEGINPASS_RESOLVEATTACHMENTVIEW_VALID, "sg_begin_pass: resolve attachment view not in valid state (SG_RESOURCESTATE_VALID)") \
    _SG_LOGITEM_XMACRO(VALIDATE_BEGINPASS_RESOLVEATTACHMENTVIEW_TYPE, "sg_begin_pass: resolve attachment view has wrong type (must be sg_view_desc.resolve_attachment)") \
    _SG_LOGITEM_XMACRO(VALIDATE_BEGINPASS_RESOLVEATTACHMENTVIEW_IMAGE_ALIVE, "sg_begin_pass: resolve attachment view's image object is uninitialized or no longer alive") \
    _SG_LOGITEM_XMACRO(VALIDATE_BEGINPASS_RESOLVEATTACHMENTVIEW_IMAGE_VALID, "sg_begin_pass: resolve attachment view's image is not in valid state (SG_RESOURCESTATE_VALID)") \
    _SG_LOGITEM_XMACRO(VALIDATE_BEGINPASS_RESOLVEATTACHMENTVIEW_SIZES, "sg_begin_pass: all attachments must have the same width and height") \
    _SG_LOGITEM_XMACRO(VALIDATE_BEGINPASS_DEPTHSTENCILATTACHMENTVIEWS_CONTINUOUS, "sg_begin_pass: color attachment view array must be continuous") \
    _SG_LOGITEM_XMACRO(VALIDATE_BEGINPASS_DEPTHSTENCILATTACHMENTVIEW_ALIVE, "sg_begin_pass: depth-stencil attachment view no longer alive") \
    _SG_LOGITEM_XMACRO(VALIDATE_BEGINPASS_DEPTHSTENCILATTACHMENTVIEW_VALID, "sg_begin_pass: depth-stencil attachment view not in valid state (SG_RESOURCESTATE_VALID)") \
    _SG_LOGITEM_XMACRO(VALIDATE_BEGINPASS_DEPTHSTENCILATTACHMENTVIEW_TYPE, "sg_begin_pass: depth-stencil attachment view has wrong type (must be sg_view_desc.depth_stencil_attachment)") \
    _SG_LOGITEM_XMACRO(VALIDATE_BEGINPASS_DEPTHSTENCILATTACHMENTVIEW_IMAGE_ALIVE, "sg_begin_pass: depth-stencil attachment view's image object is uninitialized or no longer alive") \
    _SG_LOGITEM_XMACRO(VALIDATE_BEGINPASS_DEPTHSTENCILATTACHMENTVIEW_IMAGE_VALID, "sg_begin_pass: depth-stencil attachment view's image is not in valid state (SG_RESOURCESTATE_VALID)") \
    _SG_LOGITEM_XMACRO(VALIDATE_BEGINPASS_DEPTHSTENCILATTACHMENTVIEW_SIZES, "sg_begin_pass: attachments must have the same width and height") \
    _SG_LOGITEM_XMACRO(VALIDATE_BEGINPASS_DEPTHSTENCILATTACHMENTVIEW_SAMPLECOUNT, "sg_begin_pass: all color attachments must have the same sample count") \
    _SG_LOGITEM_XMACRO(VALIDATE_BEGINPASS_ATTACHMENTS_EXPECTED, "sg_begin_pass: offscreen render passes must have at least one color- or depth-stencil attachment") \
    _SG_LOGITEM_XMACRO(VALIDATE_AVP_RENDERPASS_EXPECTED, "sg_apply_viewport: must be called in a render pass") \
    _SG_LOGITEM_XMACRO(VALIDATE_ASR_RENDERPASS_EXPECTED, "sg_apply_scissor_rect: must be called in a render pass") \
    _SG_LOGITEM_XMACRO(VALIDATE_APIP_PIPELINE_VALID_ID, "sg_apply_pipeline: invalid pipeline id provided") \
    _SG_LOGITEM_XMACRO(VALIDATE_APIP_PIPELINE_EXISTS, "sg_apply_pipeline: pipeline object no longer alive") \
    _SG_LOGITEM_XMACRO(VALIDATE_APIP_PIPELINE_VALID, "sg_apply_pipeline: pipeline object not in valid state (SG_RESOURCESTATE_VALID)") \
    _SG_LOGITEM_XMACRO(VALIDATE_APIP_PASS_EXPECTED, "sg_apply_pipeline: must be called in a pass") \
    _SG_LOGITEM_XMACRO(VALIDATE_APIP_PIPELINE_SHADER_ALIVE, "sg_apply_pipeline: shader object associated with pipeline no longer alive") \
    _SG_LOGITEM_XMACRO(VALIDATE_APIP_PIPELINE_SHADER_VALID, "sg_apply_pipeline: shader object associated with pipeline not in valid state") \
    _SG_LOGITEM_XMACRO(VALIDATE_APIP_COMPUTEPASS_EXPECTED, "sg_apply_pipeline: trying to apply compute pipeline in render pass") \
    _SG_LOGITEM_XMACRO(VALIDATE_APIP_RENDERPASS_EXPECTED, "sg_apply_pipeline: trying to apply render pipeline in compute pass") \
    _SG_LOGITEM_XMACRO(VALIDATE_APIP_SWAPCHAIN_COLOR_COUNT, "sg_apply_pipeline: the pipeline .color_count must be 1 in swapchain render passes") \
    _SG_LOGITEM_XMACRO(VALIDATE_APIP_SWAPCHAIN_COLOR_FORMAT, "sg_apply_pipeline: the pipeline .colors[0].pixel_format doesn't match the sg_pass.swapchain.color_format") \
    _SG_LOGITEM_XMACRO(VALIDATE_APIP_SWAPCHAIN_DEPTH_FORMAT, "sg_apply_pipeline: the pipeline .depth.pixel_format doesn't match the sg_pass.swapchain.depth_format") \
    _SG_LOGITEM_XMACRO(VALIDATE_APIP_SWAPCHAIN_SAMPLE_COUNT, "sg_apply_pipeline: the pipeline .sample_count doesn't match the sg_pass.swapchain.sample_count") \
    _SG_LOGITEM_XMACRO(VALIDATE_APIP_ATTACHMENTS_ALIVE, "sg_apply_pipeline: at least one pass attachment view or base image object is no longer alive") \
    _SG_LOGITEM_XMACRO(VALIDATE_APIP_COLORATTACHMENTS_COUNT, "sg_apply_pipeline: the pipeline .color_count doesn't match the number of render pass color attachments") \
    _SG_LOGITEM_XMACRO(VALIDATE_APIP_COLORATTACHMENTS_VIEW_VALID, "sg_apply_pipeline: a pass color attachment view is not in valid state (SG_RESOURCESTATE_VALID)") \
    _SG_LOGITEM_XMACRO(VALIDATE_APIP_COLORATTACHMENTS_IMAGE_VALID, "sg_apply_pipeline: a pass color attachment view's image object is not in valid state (SG_RESOURCESTATE_VALID)") \
    _SG_LOGITEM_XMACRO(VALIDATE_APIP_COLORATTACHMENTS_FORMAT, "sg_apply_pipeline: a pipeline .colors[n].pixel_format doesn't match sg_pass.attachments.colors[n] image pixel format") \
    _SG_LOGITEM_XMACRO(VALIDATE_APIP_DEPTHSTENCILATTACHMENT_VIEW_VALID, "sg_apply_pipeline: the pass depth-stencil attachment view is not in valid state (SG_RESOURCESTATE_VALID)") \
    _SG_LOGITEM_XMACRO(VALIDATE_APIP_DEPTHSTENCILATTACHMENT_IMAGE_VALID, "sg_apply_pipeline: the pass depth-stencil attachment view's image object is not in valid state (SG_RESOURCESTATE_VALID)") \
    _SG_LOGITEM_XMACRO(VALIDATE_APIP_DEPTHSTENCILATTACHMENT_FORMAT, "sg_apply_pipeline: pipeline .depth.pixel_format doesn't match sg_pass.attachments.depth_stencil image pixel format") \
    _SG_LOGITEM_XMACRO(VALIDATE_APIP_ATTACHMENT_SAMPLE_COUNT, "sg_apply_pipeline: pipeline MSAA sample count doesn't match pass attachment sample count") \
    _SG_LOGITEM_XMACRO(VALIDATE_ABND_PASS_EXPECTED, "sg_apply_bindings: must be called in a pass") \
    _SG_LOGITEM_XMACRO(VALIDATE_ABND_EMPTY_BINDINGS, "sg_apply_bindings: the provided sg_bindings struct is empty") \
    _SG_LOGITEM_XMACRO(VALIDATE_ABND_NO_PIPELINE, "sg_apply_bindings: must be called after sg_apply_pipeline") \
    _SG_LOGITEM_XMACRO(VALIDATE_ABND_PIPELINE_ALIVE, "sg_apply_bindings: currently applied pipeline object no longer alive") \
    _SG_LOGITEM_XMACRO(VALIDATE_ABND_PIPELINE_VALID, "sg_apply_bindings: currently applied pipeline object not in valid state") \
    _SG_LOGITEM_XMACRO(VALIDATE_ABND_PIPELINE_SHADER_ALIVE, "sg_apply_bindings: shader associated with currently applied pipeline is no longer alive") \
    _SG_LOGITEM_XMACRO(VALIDATE_ABND_PIPELINE_SHADER_VALID, "sg_apply_bindings: shader associated with currently applied pipeline is not in valid state") \
    _SG_LOGITEM_XMACRO(VALIDATE_ABND_COMPUTE_EXPECTED_NO_VBUFS, "sg_apply_bindings: vertex buffer bindings not allowed in a compute pass") \
    _SG_LOGITEM_XMACRO(VALIDATE_ABND_COMPUTE_EXPECTED_NO_IBUF, "sg_apply_bindings: index buffer binding not allowed in compute pass") \
    _SG_LOGITEM_XMACRO(VALIDATE_ABND_EXPECTED_VBUF, "sg_apply_bindings: vertex buffer binding is missing or buffer handle is invalid") \
    _SG_LOGITEM_XMACRO(VALIDATE_ABND_VBUF_ALIVE, "sg_apply_bindings: vertex buffer no longer alive") \
    _SG_LOGITEM_XMACRO(VALIDATE_ABND_VBUF_USAGE, "sg_apply_bindings: buffer in vertex buffer bind slot must have usage.vertex_buffer") \
    _SG_LOGITEM_XMACRO(VALIDATE_ABND_VBUF_OVERFLOW, "sg_apply_bindings: buffer in vertex buffer bind slot is overflown") \
    _SG_LOGITEM_XMACRO(VALIDATE_ABND_EXPECTED_NO_IBUF, "sg_apply_bindings: pipeline object defines non-indexed rendering, but index buffer binding provided") \
    _SG_LOGITEM_XMACRO(VALIDATE_ABND_EXPECTED_IBUF, "sg_apply_bindings: pipeline object defines indexed rendering, but no index buffer binding provided") \
    _SG_LOGITEM_XMACRO(VALIDATE_ABND_IBUF_ALIVE, "sg_apply_bindings: index buffer no longer alive") \
    _SG_LOGITEM_XMACRO(VALIDATE_ABND_IBUF_USAGE, "sg_apply_bindings: buffer in index buffer bind slot must have usage.index_buffer") \
    _SG_LOGITEM_XMACRO(VALIDATE_ABND_IBUF_OVERFLOW, "sg_apply_bindings: buffer in index buffer slot is overflown") \
    _SG_LOGITEM_XMACRO(VALIDATE_ABND_EXPECTED_VIEW_BINDING, "sg_apply_bindings: view binding is missing or the view handle is invalid") \
    _SG_LOGITEM_XMACRO(VALIDATE_ABND_VIEW_ALIVE, "sg_apply_bindings: view no longer alive") \
    _SG_LOGITEM_XMACRO(VALIDATE_ABND_EXPECT_TEXVIEW, "sg_apply_bindings: view type mismatch in bindslot (shader expects a texture view)") \
    _SG_LOGITEM_XMACRO(VALIDATE_ABND_EXPECT_SBVIEW, "sg_apply_bindings: view type mismatch in bindslot (shader expects a storage buffer view)") \
    _SG_LOGITEM_XMACRO(VALIDATE_ABND_EXPECT_SIMGVIEW, "sg_apply_bindings: view type mismatch in bindslot (shader expects a storage image view)") \
    _SG_LOGITEM_XMACRO(VALIDATE_ABND_TEXVIEW_IMAGETYPE_MISMATCH, "sg_apply_bindings: image type of bound texture doesn't match shader desc") \
    _SG_LOGITEM_XMACRO(VALIDATE_ABND_TEXVIEW_EXPECTED_MULTISAMPLED_IMAGE, "sg_apply_bindings: texture bindings expects image with sample_count > 1") \
    _SG_LOGITEM_XMACRO(VALIDATE_ABND_TEXVIEW_EXPECTED_NON_MULTISAMPLED_IMAGE, "sg_apply_bindings: texture bindings expects image with sample_count == 1") \
    _SG_LOGITEM_XMACRO(VALIDATE_ABND_TEXVIEW_EXPECTED_FILTERABLE_IMAGE, "sg_apply_bindings: filterable image expected") \
    _SG_LOGITEM_XMACRO(VALIDATE_ABND_TEXVIEW_EXPECTED_DEPTH_IMAGE, "sg_apply_bindings: depth image expected") \
    _SG_LOGITEM_XMACRO(VALIDATE_ABND_SBVIEW_READWRITE_IMMUTABLE, "sg_apply_bindings: storage buffers bound as read/write must have usage immutable") \
    _SG_LOGITEM_XMACRO(VALIDATE_ABND_SIMGVIEW_COMPUTE_PASS_EXPECTED, "sg_apply_bindings: storage image bindings can only appear on compute passes") \
    _SG_LOGITEM_XMACRO(VALIDATE_ABND_SIMGVIEW_IMAGETYPE_MISMATCH, "sg_apply_bindings: image type of bound storage image doesn't match shader desc") \
    _SG_LOGITEM_XMACRO(VALIDATE_ABND_SIMGVIEW_ACCESSFORMAT, "sg_apply_bindings: pixel format of storage image view doesn't match access format in shader desc") \
    _SG_LOGITEM_XMACRO(VALIDATE_ABND_EXPECTED_SAMPLER_BINDING, "sg_apply_bindings: sampler binding is missing or the sampler handle is invalid") \
    _SG_LOGITEM_XMACRO(VALIDATE_ABND_UNEXPECTED_SAMPLER_COMPARE_NEVER, "sg_apply_bindings: shader expects SG_SAMPLERTYPE_COMPARISON but sampler has SG_COMPAREFUNC_NEVER") \
    _SG_LOGITEM_XMACRO(VALIDATE_ABND_EXPECTED_SAMPLER_COMPARE_NEVER, "sg_apply_bindings: shader expects SG_SAMPLERTYPE_FILTERING or SG_SAMPLERTYPE_NONFILTERING but sampler doesn't have SG_COMPAREFUNC_NEVER") \
    _SG_LOGITEM_XMACRO(VALIDATE_ABND_EXPECTED_NONFILTERING_SAMPLER, "sg_apply_bindings: shader expected SG_SAMPLERTYPE_NONFILTERING, but sampler has SG_FILTER_LINEAR filters") \
    _SG_LOGITEM_XMACRO(VALIDATE_ABND_SAMPLER_ALIVE, "sg_apply_bindings: bound sampler no longer alive") \
    _SG_LOGITEM_XMACRO(VALIDATE_ABND_SAMPLER_VALID, "sg_apply_bindings: bound sampler not in valid state") \
    _SG_LOGITEM_XMACRO(VALIDATE_ABND_TEXTURE_BINDING_VS_DEPTHSTENCIL_ATTACHMENT, "sg_apply_bindings: cannot bind texture in the same pass it is used as depth-stencil attachment") \
    _SG_LOGITEM_XMACRO(VALIDATE_ABND_TEXTURE_BINDING_VS_COLOR_ATTACHMENT, "sg_apply_bindings: cannot bind texture in the same pass it is used as color attachment") \
    _SG_LOGITEM_XMACRO(VALIDATE_ABND_TEXTURE_BINDING_VS_RESOLVE_ATTACHMENT, "sg_apply_bindings: cannot bind texture in the same pass it is used as resolve attachment") \
    _SG_LOGITEM_XMACRO(VALIDATE_ABND_TEXTURE_VS_STORAGEIMAGE_BINDING, "sg_apply_bindings: an image cannot be bound as a texture and storage image at the same time") \
    _SG_LOGITEM_XMACRO(VALIDATE_AU_PASS_EXPECTED, "sg_apply_uniforms: must be called in a pass") \
    _SG_LOGITEM_XMACRO(VALIDATE_AU_NO_PIPELINE, "sg_apply_uniforms: must be called after sg_apply_pipeline()") \
    _SG_LOGITEM_XMACRO(VALIDATE_AU_PIPELINE_ALIVE, "sg_apply_uniforms: currently applied pipeline object no longer alive") \
    _SG_LOGITEM_XMACRO(VALIDATE_AU_PIPELINE_VALID, "sg_apply_uniforms: currently applied pipeline object not in valid state") \
    _SG_LOGITEM_XMACRO(VALIDATE_AU_PIPELINE_SHADER_ALIVE, "sg_apply_uniforms: shader associated with currently applied pipeline is no longer alive") \
    _SG_LOGITEM_XMACRO(VALIDATE_AU_PIPELINE_SHADER_VALID, "sg_apply_uniforms: shader associated with currently applied pipeline is not in valid state") \
    _SG_LOGITEM_XMACRO(VALIDATE_AU_NO_UNIFORMBLOCK_AT_SLOT, "sg_apply_uniforms: no uniform block declaration at this shader stage UB slot") \
    _SG_LOGITEM_XMACRO(VALIDATE_AU_SIZE, "sg_apply_uniforms: data size doesn't match declared uniform block size") \
    _SG_LOGITEM_XMACRO(VALIDATE_DRAW_RENDERPASS_EXPECTED, "sg_draw: must be called in a render pass") \
    _SG_LOGITEM_XMACRO(VALIDATE_DRAW_BASEELEMENT, "sg_draw: base_element cannot be < 0") \
    _SG_LOGITEM_XMACRO(VALIDATE_DRAW_NUMELEMENTS, "sg_draw: num_elements cannot be < 0") \
    _SG_LOGITEM_XMACRO(VALIDATE_DRAW_NUMINSTANCES, "sg_draw: num_instances cannot be < 0") \
    _SG_LOGITEM_XMACRO(VALIDATE_DRAW_REQUIRED_BINDINGS_OR_UNIFORMS_MISSING, "sg_draw: call to sg_apply_bindings() and/or sg_apply_uniforms() missing after sg_apply_pipeline()") \
    _SG_LOGITEM_XMACRO(VALIDATE_DISPATCH_COMPUTEPASS_EXPECTED, "sg_dispatch: must be called in a compute pass") \
    _SG_LOGITEM_XMACRO(VALIDATE_DISPATCH_NUMGROUPSX, "sg_dispatch: num_groups_x must be >=0 and <65536") \
    _SG_LOGITEM_XMACRO(VALIDATE_DISPATCH_NUMGROUPSY, "sg_dispatch: num_groups_y must be >=0 and <65536") \
    _SG_LOGITEM_XMACRO(VALIDATE_DISPATCH_NUMGROUPSZ, "sg_dispatch: num_groups_z must be >=0 and <65536") \
    _SG_LOGITEM_XMACRO(VALIDATE_DISPATCH_REQUIRED_BINDINGS_OR_UNIFORMS_MISSING, "sg_dispatch: call to sg_apply_bindings() and/or sg_apply_uniforms() missing after sg_apply_pipeline()") \
    _SG_LOGITEM_XMACRO(VALIDATE_UPDATEBUF_USAGE, "sg_update_buffer: cannot update immutable buffer") \
    _SG_LOGITEM_XMACRO(VALIDATE_UPDATEBUF_SIZE, "sg_update_buffer: update size is bigger than buffer size") \
    _SG_LOGITEM_XMACRO(VALIDATE_UPDATEBUF_ONCE, "sg_update_buffer: only one update allowed per buffer and frame") \
    _SG_LOGITEM_XMACRO(VALIDATE_UPDATEBUF_APPEND, "sg_update_buffer: cannot call sg_update_buffer and sg_append_buffer in same frame") \
    _SG_LOGITEM_XMACRO(VALIDATE_APPENDBUF_USAGE, "sg_append_buffer: cannot append to immutable buffer") \
    _SG_LOGITEM_XMACRO(VALIDATE_APPENDBUF_SIZE, "sg_append_buffer: overall appended size is bigger than buffer size") \
    _SG_LOGITEM_XMACRO(VALIDATE_APPENDBUF_UPDATE, "sg_append_buffer: cannot call sg_append_buffer and sg_update_buffer in same frame") \
    _SG_LOGITEM_XMACRO(VALIDATE_UPDIMG_USAGE, "sg_update_image: cannot update immutable image") \
    _SG_LOGITEM_XMACRO(VALIDATE_UPDIMG_ONCE, "sg_update_image: only one update allowed per image and frame") \
    _SG_LOGITEM_XMACRO(VALIDATION_FAILED, "validation layer checks failed") \

#define _SG_LOGITEM_XMACRO(item,msg) SG_LOGITEM_##item,
typedef enum sg_log_item {
    _SG_LOG_ITEMS
} sg_log_item;
#undef _SG_LOGITEM_XMACRO

/*
    sg_desc

    The sg_desc struct contains configuration values for sokol_gfx,
    it is used as parameter to the sg_setup() call.

    The default configuration is:

    .buffer_pool_size               128
    .image_pool_size                128
    .sampler_pool_size              64
    .shader_pool_size               32
    .pipeline_pool_size             64
    .view_pool_size                 256
    .uniform_buffer_size            4 MB (4*1024*1024)
    .max_commit_listeners           1024
    .disable_validation             false
    .mtl_force_managed_storage_mode false
    .wgpu_disable_bindgroups_cache  false
    .wgpu_bindgroups_cache_size     1024

    .allocator.alloc_fn     0 (in this case, malloc() will be called)
    .allocator.free_fn      0 (in this case, free() will be called)
    .allocator.user_data    0

    .environment.defaults.color_format: default value depends on selected backend:
        all GL backends:    SG_PIXELFORMAT_RGBA8
        Metal and D3D11:    SG_PIXELFORMAT_BGRA8
        WebGPU:             *no default* (must be queried from WebGPU swapchain object)
    .environment.defaults.depth_format: SG_PIXELFORMAT_DEPTH_STENCIL
    .environment.defaults.sample_count: 1

    Metal specific:
        (NOTE: All Objective-C object references are transferred through
        a bridged cast (__bridge const void*) to sokol_gfx, which will use an
        unretained bridged cast (__bridge id<xxx>) to retrieve the Objective-C
        references back. Since the bridge cast is unretained, the caller
        must hold a strong reference to the Objective-C object until sg_setup()
        returns.

        .mtl_force_managed_storage_mode
            when enabled, Metal buffers and texture resources are created in managed storage
            mode, otherwise sokol-gfx will decide whether to create buffers and
            textures in managed or shared storage mode (this is mainly a debugging option)
        .mtl_use_command_buffer_with_retained_references
            when true, the sokol-gfx Metal backend will use Metal command buffers which
            bump the reference count of resource objects as long as they are inflight,
            this is slower than the default command-buffer-with-unretained-references
            method, this may be a workaround when confronted with lifetime validation
            errors from the Metal validation layer until a proper fix has been implemented
        .environment.metal.device
            a pointer to the MTLDevice object

    D3D11 specific:
        .environment.d3d11.device
            a pointer to the ID3D11Device object, this must have been created
            before sg_setup() is called
        .environment.d3d11.device_context
            a pointer to the ID3D11DeviceContext object
        .d3d11_shader_debugging
            set this to true to compile shaders which are provided as HLSL source
            code with debug information and without optimization, this allows
            shader debugging in tools like RenderDoc, to output source code
            instead of byte code from sokol-shdc, omit the `--binary` cmdline
            option

    WebGPU specific:
        .wgpu_disable_bindgroups_cache
            When this is true, the WebGPU backend will create and immediately
            release a BindGroup object in the sg_apply_bindings() call, only
            use this for debugging purposes.
        .wgpu_bindgroups_cache_size
            The size of the bindgroups cache for re-using BindGroup objects
            between sg_apply_bindings() calls. The smaller the cache size,
            the more likely are cache slot collisions which will cause
            a BindGroups object to be destroyed and a new one created.
            Use the information returned by sg_query_stats() to check
            if this is a frequent occurrence, and increase the cache size as
            needed (the default is 1024).
            NOTE: wgpu_bindgroups_cache_size must be a power-of-2 number!
        .environment.wgpu.device
            a WGPUDevice handle

    When using sokol_gfx.h and sokol_app.h together, consider using the
    helper function sglue_environment() in the sokol_glue.h header to
    initialize the sg_desc.environment nested struct. sglue_environment() returns
    a completely initialized sg_environment struct with information
    provided by sokol_app.h.
*/
typedef struct sg_environment_defaults {
    sg_pixel_format color_format;
    sg_pixel_format depth_format;
    int sample_count;
} sg_environment_defaults;

typedef struct sg_metal_environment {
    const void* device;
} sg_metal_environment;

typedef struct sg_d3d11_environment {
    const void* device;
    const void* device_context;
} sg_d3d11_environment;

typedef struct sg_wgpu_environment {
    const void* device;
} sg_wgpu_environment;

typedef struct sg_environment {
    sg_environment_defaults defaults;
    sg_metal_environment metal;
    sg_d3d11_environment d3d11;
    sg_wgpu_environment wgpu;
} sg_environment;

/*
    sg_commit_listener

    Used with function sg_add_commit_listener() to add a callback
    which will be called in sg_commit(). This is useful for libraries
    building on top of sokol-gfx to be notified about when a frame
    ends (instead of having to guess, or add a manual 'new-frame'
    function.
*/
typedef struct sg_commit_listener {
    void (*func)(void* user_data);
    void* user_data;
} sg_commit_listener;

/*
    sg_allocator

    Used in sg_desc to provide custom memory-alloc and -free functions
    to sokol_gfx.h. If memory management should be overridden, both the
    alloc_fn and free_fn function must be provided (e.g. it's not valid to
    override one function but not the other).
*/
typedef struct sg_allocator {
    void* (*alloc_fn)(size_t size, void* user_data);
    void (*free_fn)(void* ptr, void* user_data);
    void* user_data;
} sg_allocator;

/*
    sg_logger

    Used in sg_desc to provide a logging function. Please be aware
    that without logging function, sokol-gfx will be completely
    silent, e.g. it will not report errors, warnings and
    validation layer messages. For maximum error verbosity,
    compile in debug mode (e.g. NDEBUG *not* defined) and provide a
    compatible logger function in the sg_setup() call
    (for instance the standard logging function from sokol_log.h).
*/
typedef struct sg_logger {
    void (*func)(
        const char* tag,                // always "sg"
        uint32_t log_level,             // 0=panic, 1=error, 2=warning, 3=info
        uint32_t log_item_id,           // SG_LOGITEM_*
        const char* message_or_null,    // a message string, may be nullptr in release mode
        uint32_t line_nr,               // line number in sokol_gfx.h
        const char* filename_or_null,   // source filename, may be nullptr in release mode
        void* user_data);
    void* user_data;
} sg_logger;

typedef struct sg_desc {
    uint32_t _start_canary;
    int buffer_pool_size;
    int image_pool_size;
    int sampler_pool_size;
    int shader_pool_size;
    int pipeline_pool_size;
    int view_pool_size;
    int uniform_buffer_size;
    int max_commit_listeners;
    bool disable_validation;    // disable validation layer even in debug mode, useful for tests
    bool d3d11_shader_debugging;    // if true, HLSL shaders are compiled with D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION
    bool mtl_force_managed_storage_mode; // for debugging: use Metal managed storage mode for resources even with UMA
    bool mtl_use_command_buffer_with_retained_references;    // Metal: use a managed MTLCommandBuffer which ref-counts used resources
    bool wgpu_disable_bindgroups_cache;  // set to true to disable the WebGPU backend BindGroup cache
    int wgpu_bindgroups_cache_size;      // number of slots in the WebGPU bindgroup cache (must be 2^N)
    sg_allocator allocator;
    sg_logger logger; // optional log function override
    sg_environment environment;
    uint32_t _end_canary;
} sg_desc;

// setup and misc functions
SOKOL_GFX_API_DECL void sg_setup(const sg_desc* desc);
SOKOL_GFX_API_DECL void sg_shutdown(void);
SOKOL_GFX_API_DECL bool sg_isvalid(void);
SOKOL_GFX_API_DECL void sg_reset_state_cache(void);
SOKOL_GFX_API_DECL sg_trace_hooks sg_install_trace_hooks(const sg_trace_hooks* trace_hooks);
SOKOL_GFX_API_DECL void sg_push_debug_group(const char* name);
SOKOL_GFX_API_DECL void sg_pop_debug_group(void);
SOKOL_GFX_API_DECL bool sg_add_commit_listener(sg_commit_listener listener);
SOKOL_GFX_API_DECL bool sg_remove_commit_listener(sg_commit_listener listener);

// resource creation, destruction and updating
SOKOL_GFX_API_DECL sg_buffer sg_make_buffer(const sg_buffer_desc* desc);
SOKOL_GFX_API_DECL sg_image sg_make_image(const sg_image_desc* desc);
SOKOL_GFX_API_DECL sg_sampler sg_make_sampler(const sg_sampler_desc* desc);
SOKOL_GFX_API_DECL sg_shader sg_make_shader(const sg_shader_desc* desc);
SOKOL_GFX_API_DECL sg_pipeline sg_make_pipeline(const sg_pipeline_desc* desc);
SOKOL_GFX_API_DECL sg_view sg_make_view(const sg_view_desc* desc);
SOKOL_GFX_API_DECL void sg_destroy_buffer(sg_buffer buf);
SOKOL_GFX_API_DECL void sg_destroy_image(sg_image img);
SOKOL_GFX_API_DECL void sg_destroy_sampler(sg_sampler smp);
SOKOL_GFX_API_DECL void sg_destroy_shader(sg_shader shd);
SOKOL_GFX_API_DECL void sg_destroy_pipeline(sg_pipeline pip);
SOKOL_GFX_API_DECL void sg_destroy_view(sg_view view);
SOKOL_GFX_API_DECL void sg_update_buffer(sg_buffer buf, const sg_range* data);
SOKOL_GFX_API_DECL void sg_update_image(sg_image img, const sg_image_data* data);
SOKOL_GFX_API_DECL int sg_append_buffer(sg_buffer buf, const sg_range* data);
SOKOL_GFX_API_DECL bool sg_query_buffer_overflow(sg_buffer buf);
SOKOL_GFX_API_DECL bool sg_query_buffer_will_overflow(sg_buffer buf, size_t size);

// render and compute functions
SOKOL_GFX_API_DECL void sg_begin_pass(const sg_pass* pass);
SOKOL_GFX_API_DECL void sg_apply_viewport(int x, int y, int width, int height, bool origin_top_left);
SOKOL_GFX_API_DECL void sg_apply_viewportf(float x, float y, float width, float height, bool origin_top_left);
SOKOL_GFX_API_DECL void sg_apply_scissor_rect(int x, int y, int width, int height, bool origin_top_left);
SOKOL_GFX_API_DECL void sg_apply_scissor_rectf(float x, float y, float width, float height, bool origin_top_left);
SOKOL_GFX_API_DECL void sg_apply_pipeline(sg_pipeline pip);
SOKOL_GFX_API_DECL void sg_apply_bindings(const sg_bindings* bindings);
SOKOL_GFX_API_DECL void sg_apply_uniforms(int ub_slot, const sg_range* data);
SOKOL_GFX_API_DECL void sg_draw(int base_element, int num_elements, int num_instances);
SOKOL_GFX_API_DECL void sg_dispatch(int num_groups_x, int num_groups_y, int num_groups_z);
SOKOL_GFX_API_DECL void sg_end_pass(void);
SOKOL_GFX_API_DECL void sg_commit(void);

// getting information
SOKOL_GFX_API_DECL sg_desc sg_query_desc(void);
SOKOL_GFX_API_DECL sg_backend sg_query_backend(void);
SOKOL_GFX_API_DECL sg_features sg_query_features(void);
SOKOL_GFX_API_DECL sg_limits sg_query_limits(void);
SOKOL_GFX_API_DECL sg_pixelformat_info sg_query_pixelformat(sg_pixel_format fmt);
SOKOL_GFX_API_DECL int sg_query_row_pitch(sg_pixel_format fmt, int width, int row_align_bytes);
SOKOL_GFX_API_DECL int sg_query_surface_pitch(sg_pixel_format fmt, int width, int height, int row_align_bytes);
// get current state of a resource (INITIAL, ALLOC, VALID, FAILED, INVALID)
SOKOL_GFX_API_DECL sg_resource_state sg_query_buffer_state(sg_buffer buf);
SOKOL_GFX_API_DECL sg_resource_state sg_query_image_state(sg_image img);
SOKOL_GFX_API_DECL sg_resource_state sg_query_sampler_state(sg_sampler smp);
SOKOL_GFX_API_DECL sg_resource_state sg_query_shader_state(sg_shader shd);
SOKOL_GFX_API_DECL sg_resource_state sg_query_pipeline_state(sg_pipeline pip);
SOKOL_GFX_API_DECL sg_resource_state sg_query_view_state(sg_view view);
// get runtime information about a resource
SOKOL_GFX_API_DECL sg_buffer_info sg_query_buffer_info(sg_buffer buf);
SOKOL_GFX_API_DECL sg_image_info sg_query_image_info(sg_image img);
SOKOL_GFX_API_DECL sg_sampler_info sg_query_sampler_info(sg_sampler smp);
SOKOL_GFX_API_DECL sg_shader_info sg_query_shader_info(sg_shader shd);
SOKOL_GFX_API_DECL sg_pipeline_info sg_query_pipeline_info(sg_pipeline pip);
SOKOL_GFX_API_DECL sg_view_info sg_query_view_info(sg_view view);
// get desc structs matching a specific resource (NOTE that not all creation attributes may be provided)
SOKOL_GFX_API_DECL sg_buffer_desc sg_query_buffer_desc(sg_buffer buf);
SOKOL_GFX_API_DECL sg_image_desc sg_query_image_desc(sg_image img);
SOKOL_GFX_API_DECL sg_sampler_desc sg_query_sampler_desc(sg_sampler smp);
SOKOL_GFX_API_DECL sg_shader_desc sg_query_shader_desc(sg_shader shd);
SOKOL_GFX_API_DECL sg_pipeline_desc sg_query_pipeline_desc(sg_pipeline pip);
SOKOL_GFX_API_DECL sg_view_desc sg_query_view_desc(sg_view view);
// get resource creation desc struct with their default values replaced
SOKOL_GFX_API_DECL sg_buffer_desc sg_query_buffer_defaults(const sg_buffer_desc* desc);
SOKOL_GFX_API_DECL sg_image_desc sg_query_image_defaults(const sg_image_desc* desc);
SOKOL_GFX_API_DECL sg_sampler_desc sg_query_sampler_defaults(const sg_sampler_desc* desc);
SOKOL_GFX_API_DECL sg_shader_desc sg_query_shader_defaults(const sg_shader_desc* desc);
SOKOL_GFX_API_DECL sg_pipeline_desc sg_query_pipeline_defaults(const sg_pipeline_desc* desc);
SOKOL_GFX_API_DECL sg_view_desc sg_query_view_defaults(const sg_view_desc* desc);
// assorted query functions
SOKOL_GFX_API_DECL size_t sg_query_buffer_size(sg_buffer buf);
SOKOL_GFX_API_DECL sg_buffer_usage sg_query_buffer_usage(sg_buffer buf);
SOKOL_GFX_API_DECL sg_image_type sg_query_image_type(sg_image img);
SOKOL_GFX_API_DECL int sg_query_image_width(sg_image img);
SOKOL_GFX_API_DECL int sg_query_image_height(sg_image img);
SOKOL_GFX_API_DECL int sg_query_image_num_slices(sg_image img);
SOKOL_GFX_API_DECL int sg_query_image_num_mipmaps(sg_image img);
SOKOL_GFX_API_DECL sg_pixel_format sg_query_image_pixelformat(sg_image img);
SOKOL_GFX_API_DECL sg_image_usage sg_query_image_usage(sg_image img);
SOKOL_GFX_API_DECL int sg_query_image_sample_count(sg_image img);
SOKOL_GFX_API_DECL sg_view_type sg_query_view_type(sg_view view);
SOKOL_GFX_API_DECL sg_image sg_query_view_image(sg_view view);
SOKOL_GFX_API_DECL sg_buffer sg_query_view_buffer(sg_view view);

// separate resource allocation and initialization (for async setup)
SOKOL_GFX_API_DECL sg_buffer sg_alloc_buffer(void);
SOKOL_GFX_API_DECL sg_image sg_alloc_image(void);
SOKOL_GFX_API_DECL sg_sampler sg_alloc_sampler(void);
SOKOL_GFX_API_DECL sg_shader sg_alloc_shader(void);
SOKOL_GFX_API_DECL sg_pipeline sg_alloc_pipeline(void);
SOKOL_GFX_API_DECL sg_view sg_alloc_view(void);
SOKOL_GFX_API_DECL void sg_dealloc_buffer(sg_buffer buf);
SOKOL_GFX_API_DECL void sg_dealloc_image(sg_image img);
SOKOL_GFX_API_DECL void sg_dealloc_sampler(sg_sampler smp);
SOKOL_GFX_API_DECL void sg_dealloc_shader(sg_shader shd);
SOKOL_GFX_API_DECL void sg_dealloc_pipeline(sg_pipeline pip);
SOKOL_GFX_API_DECL void sg_dealloc_view(sg_view view);
SOKOL_GFX_API_DECL void sg_init_buffer(sg_buffer buf, const sg_buffer_desc* desc);
SOKOL_GFX_API_DECL void sg_init_image(sg_image img, const sg_image_desc* desc);
SOKOL_GFX_API_DECL void sg_init_sampler(sg_sampler smg, const sg_sampler_desc* desc);
SOKOL_GFX_API_DECL void sg_init_shader(sg_shader shd, const sg_shader_desc* desc);
SOKOL_GFX_API_DECL void sg_init_pipeline(sg_pipeline pip, const sg_pipeline_desc* desc);
SOKOL_GFX_API_DECL void sg_init_view(sg_view view, const sg_view_desc* desc);
SOKOL_GFX_API_DECL void sg_uninit_buffer(sg_buffer buf);
SOKOL_GFX_API_DECL void sg_uninit_image(sg_image img);
SOKOL_GFX_API_DECL void sg_uninit_sampler(sg_sampler smp);
SOKOL_GFX_API_DECL void sg_uninit_shader(sg_shader shd);
SOKOL_GFX_API_DECL void sg_uninit_pipeline(sg_pipeline pip);
SOKOL_GFX_API_DECL void sg_uninit_view(sg_view view);
SOKOL_GFX_API_DECL void sg_fail_buffer(sg_buffer buf);
SOKOL_GFX_API_DECL void sg_fail_image(sg_image img);
SOKOL_GFX_API_DECL void sg_fail_sampler(sg_sampler smp);
SOKOL_GFX_API_DECL void sg_fail_shader(sg_shader shd);
SOKOL_GFX_API_DECL void sg_fail_pipeline(sg_pipeline pip);
SOKOL_GFX_API_DECL void sg_fail_view(sg_view view);

// frame stats
SOKOL_GFX_API_DECL void sg_enable_frame_stats(void);
SOKOL_GFX_API_DECL void sg_disable_frame_stats(void);
SOKOL_GFX_API_DECL bool sg_frame_stats_enabled(void);
SOKOL_GFX_API_DECL sg_frame_stats sg_query_frame_stats(void);

/* Backend-specific structs and functions, these may come in handy for mixing
   sokol-gfx rendering with 'native backend' rendering functions.

   This group of functions will be expanded as needed.
*/

typedef struct sg_d3d11_buffer_info {
    const void* buf;      // ID3D11Buffer*
} sg_d3d11_buffer_info;

typedef struct sg_d3d11_image_info {
    const void* tex2d;    // ID3D11Texture2D*
    const void* tex3d;    // ID3D11Texture3D*
    const void* res;      // ID3D11Resource* (either tex2d or tex3d)
} sg_d3d11_image_info;

typedef struct sg_d3d11_sampler_info {
    const void* smp;      // ID3D11SamplerState*
} sg_d3d11_sampler_info;

typedef struct sg_d3d11_shader_info {
    const void* cbufs[SG_MAX_UNIFORMBLOCK_BINDSLOTS]; // ID3D11Buffer* (constant buffers by bind slot)
    const void* vs;   // ID3D11VertexShader*
    const void* fs;   // ID3D11PixelShader*
} sg_d3d11_shader_info;

typedef struct sg_d3d11_pipeline_info {
    const void* il;   // ID3D11InputLayout*
    const void* rs;   // ID3D11RasterizerState*
    const void* dss;  // ID3D11DepthStencilState*
    const void* bs;   // ID3D11BlendState*
} sg_d3d11_pipeline_info;

typedef struct sg_d3d11_view_info {
    const void* srv;    // ID3D11ShaderResourceView
    const void* uav;    // ID3D11UnorderedAccessView
    const void* rtv;    // ID3D11RenderTargetView
    const void* dsv;    // ID3D11DepthStencilView
} sg_d3d11_view_info;

typedef struct sg_mtl_buffer_info {
    const void* buf[SG_NUM_INFLIGHT_FRAMES];  // id<MTLBuffer>
    int active_slot;
} sg_mtl_buffer_info;

typedef struct sg_mtl_image_info {
    const void* tex[SG_NUM_INFLIGHT_FRAMES]; // id<MTLTexture>
    int active_slot;
} sg_mtl_image_info;

typedef struct sg_mtl_sampler_info {
    const void* smp;  // id<MTLSamplerState>
} sg_mtl_sampler_info;

typedef struct sg_mtl_shader_info {
    const void* vertex_lib;     // id<MTLLibrary>
    const void* fragment_lib;   // id<MTLLibrary>
    const void* vertex_func;    // id<MTLFunction>
    const void* fragment_func;  // id<MTLFunction>
} sg_mtl_shader_info;

typedef struct sg_mtl_pipeline_info {
    const void* rps;      // id<MTLRenderPipelineState>
    const void* dss;      // id<MTLDepthStencilState>
} sg_mtl_pipeline_info;

typedef struct sg_wgpu_buffer_info {
    const void* buf;  // WGPUBuffer
} sg_wgpu_buffer_info;

typedef struct sg_wgpu_image_info {
    const void* tex;  // WGPUTexture
} sg_wgpu_image_info;

typedef struct sg_wgpu_sampler_info {
    const void* smp;  // WGPUSampler
} sg_wgpu_sampler_info;

typedef struct sg_wgpu_shader_info {
    const void* vs_mod;   // WGPUShaderModule
    const void* fs_mod;   // WGPUShaderModule
    const void* bgl;      // WGPUBindGroupLayout;
} sg_wgpu_shader_info;

typedef struct sg_wgpu_pipeline_info {
    const void* render_pipeline;   // WGPURenderPipeline
    const void* compute_pipeline;  // WGPUComputePipeline
} sg_wgpu_pipeline_info;

typedef struct sg_wgpu_view_info {
    const void* view; // WGPUTextureView
} sg_wgpu_view_info;

typedef struct sg_gl_buffer_info {
    uint32_t buf[SG_NUM_INFLIGHT_FRAMES];
    int active_slot;
} sg_gl_buffer_info;

typedef struct sg_gl_image_info {
    uint32_t tex[SG_NUM_INFLIGHT_FRAMES];
    uint32_t tex_target;
    int active_slot;
} sg_gl_image_info;

typedef struct sg_gl_sampler_info {
    uint32_t smp;
} sg_gl_sampler_info;

typedef struct sg_gl_shader_info {
    uint32_t prog;
} sg_gl_shader_info;

typedef struct sg_gl_view_info {
    uint32_t tex_view[SG_NUM_INFLIGHT_FRAMES];
    uint32_t msaa_render_buffer;
    uint32_t msaa_resolve_frame_buffer;
} sg_gl_view_info;

// D3D11: return ID3D11Device
SOKOL_GFX_API_DECL const void* sg_d3d11_device(void);
// D3D11: return ID3D11DeviceContext
SOKOL_GFX_API_DECL const void* sg_d3d11_device_context(void);
// D3D11: get internal buffer resource objects
SOKOL_GFX_API_DECL sg_d3d11_buffer_info sg_d3d11_query_buffer_info(sg_buffer buf);
// D3D11: get internal image resource objects
SOKOL_GFX_API_DECL sg_d3d11_image_info sg_d3d11_query_image_info(sg_image img);
// D3D11: get internal sampler resource objects
SOKOL_GFX_API_DECL sg_d3d11_sampler_info sg_d3d11_query_sampler_info(sg_sampler smp);
// D3D11: get internal shader resource objects
SOKOL_GFX_API_DECL sg_d3d11_shader_info sg_d3d11_query_shader_info(sg_shader shd);
// D3D11: get internal pipeline resource objects
SOKOL_GFX_API_DECL sg_d3d11_pipeline_info sg_d3d11_query_pipeline_info(sg_pipeline pip);
// D3D11: get internal view resource objects
SOKOL_GFX_API_DECL sg_d3d11_view_info sg_d3d11_query_view_info(sg_view view);

// Metal: return __bridge-casted MTLDevice
SOKOL_GFX_API_DECL const void* sg_mtl_device(void);
// Metal: return __bridge-casted MTLRenderCommandEncoder when inside render pass (otherwise zero)
SOKOL_GFX_API_DECL const void* sg_mtl_render_command_encoder(void);
// Metal: return __bridge-casted MTLComputeCommandEncoder when inside compute pass (otherwise zero)
SOKOL_GFX_API_DECL const void* sg_mtl_compute_command_encoder(void);
// Metal: get internal __bridge-casted buffer resource objects
SOKOL_GFX_API_DECL sg_mtl_buffer_info sg_mtl_query_buffer_info(sg_buffer buf);
// Metal: get internal __bridge-casted image resource objects
SOKOL_GFX_API_DECL sg_mtl_image_info sg_mtl_query_image_info(sg_image img);
// Metal: get internal __bridge-casted sampler resource objects
SOKOL_GFX_API_DECL sg_mtl_sampler_info sg_mtl_query_sampler_info(sg_sampler smp);
// Metal: get internal __bridge-casted shader resource objects
SOKOL_GFX_API_DECL sg_mtl_shader_info sg_mtl_query_shader_info(sg_shader shd);
// Metal: get internal __bridge-casted pipeline resource objects
SOKOL_GFX_API_DECL sg_mtl_pipeline_info sg_mtl_query_pipeline_info(sg_pipeline pip);

// WebGPU: return WGPUDevice object
SOKOL_GFX_API_DECL const void* sg_wgpu_device(void);
// WebGPU: return WGPUQueue object
SOKOL_GFX_API_DECL const void* sg_wgpu_queue(void);
// WebGPU: return this frame's WGPUCommandEncoder
SOKOL_GFX_API_DECL const void* sg_wgpu_command_encoder(void);
// WebGPU: return WGPURenderPassEncoder of current pass (returns 0 when outside pass or in a compute pass)
SOKOL_GFX_API_DECL const void* sg_wgpu_render_pass_encoder(void);
// WebGPU: return WGPUComputePassEncoder of current pass (returns 0 when outside pass or in a render pass)
SOKOL_GFX_API_DECL const void* sg_wgpu_compute_pass_encoder(void);
// WebGPU: get internal buffer resource objects
SOKOL_GFX_API_DECL sg_wgpu_buffer_info sg_wgpu_query_buffer_info(sg_buffer buf);
// WebGPU: get internal image resource objects
SOKOL_GFX_API_DECL sg_wgpu_image_info sg_wgpu_query_image_info(sg_image img);
// WebGPU: get internal sampler resource objects
SOKOL_GFX_API_DECL sg_wgpu_sampler_info sg_wgpu_query_sampler_info(sg_sampler smp);
// WebGPU: get internal shader resource objects
SOKOL_GFX_API_DECL sg_wgpu_shader_info sg_wgpu_query_shader_info(sg_shader shd);
// WebGPU: get internal pipeline resource objects
SOKOL_GFX_API_DECL sg_wgpu_pipeline_info sg_wgpu_query_pipeline_info(sg_pipeline pip);
// WebGPU: get internal view resource objects
SOKOL_GFX_API_DECL sg_wgpu_view_info sg_wgpu_query_view_info(sg_view view);

// GL: get internal buffer resource objects
SOKOL_GFX_API_DECL sg_gl_buffer_info sg_gl_query_buffer_info(sg_buffer buf);
// GL: get internal image resource objects
SOKOL_GFX_API_DECL sg_gl_image_info sg_gl_query_image_info(sg_image img);
// GL: get internal sampler resource objects
SOKOL_GFX_API_DECL sg_gl_sampler_info sg_gl_query_sampler_info(sg_sampler smp);
// GL: get internal shader resource objects
SOKOL_GFX_API_DECL sg_gl_shader_info sg_gl_query_shader_info(sg_shader shd);
// GL: get internal view resource objects
SOKOL_GFX_API_DECL sg_gl_view_info sg_gl_query_view_info(sg_view view);

#ifdef __cplusplus
} // extern "C"

// reference-based equivalents for c++
inline void sg_setup(const sg_desc& desc) { return sg_setup(&desc); }

inline sg_buffer sg_make_buffer(const sg_buffer_desc& desc) { return sg_make_buffer(&desc); }
inline sg_image sg_make_image(const sg_image_desc& desc) { return sg_make_image(&desc); }
inline sg_sampler sg_make_sampler(const sg_sampler_desc& desc) { return sg_make_sampler(&desc); }
inline sg_shader sg_make_shader(const sg_shader_desc& desc) { return sg_make_shader(&desc); }
inline sg_pipeline sg_make_pipeline(const sg_pipeline_desc& desc) { return sg_make_pipeline(&desc); }
inline sg_view sg_make_view(const sg_view_desc& desc) { return sg_make_view(&desc); }
inline void sg_update_image(sg_image img, const sg_image_data& data) { return sg_update_image(img, &data); }

inline void sg_begin_pass(const sg_pass& pass) { return sg_begin_pass(&pass); }
inline void sg_apply_bindings(const sg_bindings& bindings) { return sg_apply_bindings(&bindings); }
inline void sg_apply_uniforms(int ub_slot, const sg_range& data) { return sg_apply_uniforms(ub_slot, &data); }

inline sg_buffer_desc sg_query_buffer_defaults(const sg_buffer_desc& desc) { return sg_query_buffer_defaults(&desc); }
inline sg_image_desc sg_query_image_defaults(const sg_image_desc& desc) { return sg_query_image_defaults(&desc); }
inline sg_sampler_desc sg_query_sampler_defaults(const sg_sampler_desc& desc) { return sg_query_sampler_defaults(&desc); }
inline sg_shader_desc sg_query_shader_defaults(const sg_shader_desc& desc) { return sg_query_shader_defaults(&desc); }
inline sg_pipeline_desc sg_query_pipeline_defaults(const sg_pipeline_desc& desc) { return sg_query_pipeline_defaults(&desc); }
inline sg_view_desc sg_query_view_defaults(const sg_view_desc& desc) { return sg_query_view_defaults(&desc); }

inline void sg_init_buffer(sg_buffer buf, const sg_buffer_desc& desc) { return sg_init_buffer(buf, &desc); }
inline void sg_init_image(sg_image img, const sg_image_desc& desc) { return sg_init_image(img, &desc); }
inline void sg_init_sampler(sg_sampler smp, const sg_sampler_desc& desc) { return sg_init_sampler(smp, &desc); }
inline void sg_init_shader(sg_shader shd, const sg_shader_desc& desc) { return sg_init_shader(shd, &desc); }
inline void sg_init_pipeline(sg_pipeline pip, const sg_pipeline_desc& desc) { return sg_init_pipeline(pip, &desc); }
inline void sg_init_view(sg_view view, const sg_view_desc& desc) { return sg_init_view(view, &desc); }

inline void sg_update_buffer(sg_buffer buf_id, const sg_range& data) { return sg_update_buffer(buf_id, &data); }
inline int sg_append_buffer(sg_buffer buf_id, const sg_range& data) { return sg_append_buffer(buf_id, &data); }
#endif
#endif // SOKOL_GFX_INCLUDED

// ██ ███    ███ ██████  ██      ███████ ███    ███ ███████ ███    ██ ████████  █████  ████████ ██  ██████  ███    ██
// ██ ████  ████ ██   ██ ██      ██      ████  ████ ██      ████   ██    ██    ██   ██    ██    ██ ██    ██ ████   ██
// ██ ██ ████ ██ ██████  ██      █████   ██ ████ ██ █████   ██ ██  ██    ██    ███████    ██    ██ ██    ██ ██ ██  ██
// ██ ██  ██  ██ ██      ██      ██      ██  ██  ██ ██      ██  ██ ██    ██    ██   ██    ██    ██ ██    ██ ██  ██ ██
// ██ ██      ██ ██      ███████ ███████ ██      ██ ███████ ██   ████    ██    ██   ██    ██    ██  ██████  ██   ████
//
// >>implementation
#ifdef SOKOL_GFX_IMPL
#define SOKOL_GFX_IMPL_INCLUDED (1)

#if !(defined(SOKOL_METAL)||defined(SOKOL_DUMMY_BACKEND))
#error "Please select a backend with SOKOL_METAL or SOKOL_DUMMY_BACKEND"
#endif
#if defined(SOKOL_MALLOC) || defined(SOKOL_CALLOC) || defined(SOKOL_FREE)
#error "SOKOL_MALLOC/CALLOC/FREE macros are no longer supported, please use sg_desc.allocator to override memory allocation functions"
#endif

#include <stdlib.h> // malloc, free, qsort
#include <string.h> // memset
#include <float.h> // FLT_MAX

#ifndef SOKOL_API_IMPL
    #define SOKOL_API_IMPL
#endif
#ifndef SOKOL_DEBUG
    #ifndef NDEBUG
        #define SOKOL_DEBUG
    #endif
#endif
#ifndef SOKOL_ASSERT
    #include <assert.h>
    #define SOKOL_ASSERT(c) assert(c)
#endif
#ifndef SOKOL_UNREACHABLE
    #define SOKOL_UNREACHABLE SOKOL_ASSERT(false)
#endif

#ifndef _SOKOL_PRIVATE
    #if defined(__GNUC__) || defined(__clang__)
        #define _SOKOL_PRIVATE __attribute__((unused)) static
    #else
        #define _SOKOL_PRIVATE static
    #endif
#endif

#ifndef _SOKOL_UNUSED
    #define _SOKOL_UNUSED(x) (void)(x)
#endif

#if defined(SOKOL_TRACE_HOOKS)
#define _SG_TRACE_ARGS(fn, ...) if (_sg.hooks.fn) { _sg.hooks.fn(__VA_ARGS__, _sg.hooks.user_data); }
#define _SG_TRACE_NOARGS(fn) if (_sg.hooks.fn) { _sg.hooks.fn(_sg.hooks.user_data); }
#else
#define _SG_TRACE_ARGS(fn, ...)
#define _SG_TRACE_NOARGS(fn)
#endif

// default clear values
#ifndef SG_DEFAULT_CLEAR_RED
#define SG_DEFAULT_CLEAR_RED (0.5f)
#endif
#ifndef SG_DEFAULT_CLEAR_GREEN
#define SG_DEFAULT_CLEAR_GREEN (0.5f)
#endif
#ifndef SG_DEFAULT_CLEAR_BLUE
#define SG_DEFAULT_CLEAR_BLUE (0.5f)
#endif
#ifndef SG_DEFAULT_CLEAR_ALPHA
#define SG_DEFAULT_CLEAR_ALPHA (1.0f)
#endif
#ifndef SG_DEFAULT_CLEAR_DEPTH
#define SG_DEFAULT_CLEAR_DEPTH (1.0f)
#endif
#ifndef SG_DEFAULT_CLEAR_STENCIL
#define SG_DEFAULT_CLEAR_STENCIL (0)
#endif

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4115)   // named type definition in parentheses
#pragma warning(disable:4505)   // unreferenced local function has been removed
#pragma warning(disable:4201)   // nonstandard extension used: nameless struct/union (needed by d3d11.h)
#pragma warning(disable:4054)   // 'type cast': from function pointer
#pragma warning(disable:4055)   // 'type cast': from data pointer
#endif

#if defined(SOKOL_METAL)
    // see https://clang.llvm.org/docs/LanguageExtensions.html#automatic-reference-counting
    #if !defined(__cplusplus)
        #if __has_feature(objc_arc) && !__has_feature(objc_arc_fields)
            #error "sokol_gfx.h requires __has_feature(objc_arc_field) if ARC is enabled (use a more recent compiler version)"
        #endif
    #endif
    #include <TargetConditionals.h>
    #include <AvailabilityMacros.h>
    #if defined(TARGET_OS_IPHONE) && !TARGET_OS_IPHONE
        #define _SG_TARGET_MACOS (1)
    #else
        #define _SG_TARGET_IOS (1)
        #if defined(TARGET_IPHONE_SIMULATOR) && TARGET_IPHONE_SIMULATOR
            #define _SG_TARGET_IOS_SIMULATOR (1)
        #endif
    #endif
    #import <Metal/Metal.h>
    #import <QuartzCore/CoreAnimation.h> // needed for CAMetalDrawable
#endif

//

// ███████ ████████ ██████  ██    ██  ██████ ████████ ███████
// ███████ ████████ ██████  ██    ██  ██████ ████████ ███████
// ██         ██    ██   ██ ██    ██ ██         ██    ██
// ███████    ██    ██████  ██    ██ ██         ██    ███████
//      ██    ██    ██   ██ ██    ██ ██         ██         ██
// ███████    ██    ██   ██  ██████   ██████    ██    ███████
//
// >>structs

typedef struct { int x, y, w, h; } _sg_recti_t;
typedef struct { int width, height; } _sg_dimi_t;

// resource pool slots
typedef struct {
    uint32_t id;
    uint32_t uninit_count;
    sg_resource_state state;
} _sg_slot_t;

// resource pool housekeeping struct
typedef struct {
    int size;
    int queue_top;
    uint32_t* gen_ctrs;
    int* free_queue;
} _sg_pool_t;

// resource func forward decls
struct _sg_buffer_s;
struct _sg_image_s;
struct _sg_sampler_s;
struct _sg_shader_s;
struct _sg_pipeline_s;
struct _sg_view_s;

// a general resource slot reference useful for caches
typedef struct _sg_sref_s {
    uint32_t id;
    uint32_t uninit_count;
} _sg_sref_t;

// safe (in debug mode) internal resource references
typedef struct _sg_buffer_ref_s {
    struct _sg_buffer_s* ptr;
    _sg_sref_t sref;
} _sg_buffer_ref_t;

typedef struct _sg_image_ref_s {
    struct _sg_image_s* ptr;
    _sg_sref_t sref;
} _sg_image_ref_t;

typedef struct _sg_sampler_ref_t {
    struct _sg_sampler_s* ptr;
    _sg_sref_t sref;
} _sg_sampler_ref_t;

typedef struct _sg_shader_ref_s {
    struct _sg_shader_s* ptr;
    _sg_sref_t sref;
} _sg_shader_ref_t;

typedef struct _sg_pipeline_ref_s {
    struct _sg_pipeline_s* ptr;
    _sg_sref_t sref;
} _sg_pipeline_ref_t;

typedef struct _sg_view_ref_s {
    struct _sg_view_s* ptr;
    _sg_sref_t sref;
} _sg_view_ref_t;

// constants
enum {
    _SG_STRING_SIZE = 32,
    _SG_SLOT_SHIFT = 16,
    _SG_SLOT_MASK = (1<<_SG_SLOT_SHIFT)-1,
    _SG_MAX_POOL_SIZE = (1<<_SG_SLOT_SHIFT),
    _SG_DEFAULT_BUFFER_POOL_SIZE = 128,
    _SG_DEFAULT_IMAGE_POOL_SIZE = 128,
    _SG_DEFAULT_SAMPLER_POOL_SIZE = 64,
    _SG_DEFAULT_SHADER_POOL_SIZE = 32,
    _SG_DEFAULT_PIPELINE_POOL_SIZE = 64,
    _SG_DEFAULT_VIEW_POOL_SIZE = 256,
    _SG_DEFAULT_UB_SIZE = 4 * 1024 * 1024,
    _SG_DEFAULT_MAX_COMMIT_LISTENERS = 1024,
    _SG_DEFAULT_WGPU_BINDGROUP_CACHE_SIZE = 1024,
    _SG_MAX_STORAGEBUFFER_BINDINGS_PER_STAGE = 8,
    _SG_MAX_STORAGEIMAGE_BINDINGS_PER_STAGE = 4,
    _SG_MAX_TEXTURE_BINDINGS_PER_STAGE = 16,
    _SG_MAX_UNIFORMBLOCK_BINDINGS_PER_STAGE = 8,
};

// fixed-size string
typedef struct {
    char buf[_SG_STRING_SIZE];
} _sg_str_t;

typedef struct {
    int size;
    int append_pos;
    bool append_overflow;
    uint32_t update_frame_index;
    uint32_t append_frame_index;
    int num_slots;
    int active_slot;
    sg_buffer_usage usage;
} _sg_buffer_common_t;

typedef struct {
    uint32_t upd_frame_index;
    int num_slots;
    int active_slot;
    sg_image_type type;
    int width;
    int height;
    int num_slices;
    int num_mipmaps;
    sg_image_usage usage;
    sg_pixel_format pixel_format;
    int sample_count;
} _sg_image_common_t;

typedef struct {
    sg_filter min_filter;
    sg_filter mag_filter;
    sg_filter mipmap_filter;
    sg_wrap wrap_u;
    sg_wrap wrap_v;
    sg_wrap wrap_w;
    float min_lod;
    float max_lod;
    sg_border_color border_color;
    sg_compare_func compare;
    uint32_t max_anisotropy;
} _sg_sampler_common_t;

typedef struct {
    sg_shader_attr_base_type base_type;
} _sg_shader_attr_t;

typedef struct {
    sg_shader_stage stage;
    uint32_t size;
} _sg_shader_uniform_block_t;

typedef struct {
    sg_shader_stage stage;
    sg_view_type view_type;
    sg_image_type image_type;
    sg_pixel_format access_format;
    sg_image_sample_type sample_type;
    bool sbuf_readonly;
    bool simg_writeonly;
    bool multisampled;
} _sg_shader_view_t;

typedef struct {
    sg_shader_stage stage;
    sg_sampler_type sampler_type;
} _sg_shader_sampler_t;

typedef struct {
    sg_shader_stage stage;
    uint8_t view_slot;
    uint8_t sampler_slot;
} _sg_shader_texture_sampler_t;

typedef struct {
    uint32_t required_bindings_and_uniforms;
    bool is_compute;
    _sg_shader_attr_t attrs[SG_MAX_VERTEX_ATTRIBUTES];
    _sg_shader_uniform_block_t uniform_blocks[SG_MAX_UNIFORMBLOCK_BINDSLOTS];
    _sg_shader_view_t views[SG_MAX_VIEW_BINDSLOTS];
    _sg_shader_sampler_t samplers[SG_MAX_SAMPLER_BINDSLOTS];
    _sg_shader_texture_sampler_t texture_samplers[SG_MAX_TEXTURE_SAMPLER_PAIRS];
} _sg_shader_common_t;

typedef struct {
    bool vertex_buffer_layout_active[SG_MAX_VERTEXBUFFER_BINDSLOTS];
    bool use_instanced_draw;
    bool is_compute;
    uint32_t required_bindings_and_uniforms;
    _sg_shader_ref_t shader;
    sg_vertex_layout_state layout;
    sg_depth_state depth;
    sg_stencil_state stencil;
    int color_count;
    sg_color_target_state colors[SG_MAX_COLOR_ATTACHMENTS];
    sg_primitive_type primitive_type;
    sg_index_type index_type;
    sg_cull_mode cull_mode;
    sg_face_winding face_winding;
    int sample_count;
    sg_color blend_color;
    bool alpha_to_coverage_enabled;
} _sg_pipeline_common_t;

typedef struct {
    _sg_buffer_ref_t ref;
    int offset;
} _sg_buffer_view_common_t;

typedef struct {
    _sg_image_ref_t ref;
    int mip_level;
    int slice;
    int mip_level_count;
    int slice_count;
} _sg_image_view_common_t;

typedef struct {
    sg_view_type type;
    _sg_buffer_view_common_t buf;
    _sg_image_view_common_t img;
} _sg_view_common_t;

#if defined(SOKOL_DUMMY_BACKEND)
typedef struct _sg_buffer_s {
    _sg_slot_t slot;
    _sg_buffer_common_t cmn;
} _sg_dummy_buffer_t;
typedef _sg_dummy_buffer_t _sg_buffer_t;

typedef struct _sg_image_s {
    _sg_slot_t slot;
    _sg_image_common_t cmn;
} _sg_dummy_image_t;
typedef _sg_dummy_image_t _sg_image_t;

typedef struct _sg_sampler_s {
    _sg_slot_t slot;
    _sg_sampler_common_t cmn;
} _sg_dummy_sampler_t;
typedef _sg_dummy_sampler_t _sg_sampler_t;

typedef struct _sg_shader_s {
    _sg_slot_t slot;
    _sg_shader_common_t cmn;
} _sg_dummy_shader_t;
typedef _sg_dummy_shader_t _sg_shader_t;

typedef struct _sg_pipeline_s {
    _sg_slot_t slot;
    _sg_pipeline_common_t cmn;
} _sg_dummy_pipeline_t;
typedef _sg_dummy_pipeline_t _sg_pipeline_t;

typedef struct _sg_view_s {
    _sg_slot_t slot;
    _sg_view_common_t cmn;
} _sg_dummy_view_t;
typedef _sg_dummy_view_t _sg_view_t;

#elif defined(SOKOL_METAL)

#if defined(_SG_TARGET_MACOS) || defined(_SG_TARGET_IOS_SIMULATOR)
#define _SG_MTL_UB_ALIGN (256)
#else
#define _SG_MTL_UB_ALIGN (16)
#endif
#define _SG_MTL_INVALID_SLOT_INDEX (0)

typedef struct {
    uint32_t frame_index;   // frame index at which it is safe to release this resource
    int slot_index;
} _sg_mtl_release_item_t;

typedef struct {
    NSMutableArray* pool;
    int num_slots;
    int free_queue_top;
    int* free_queue;
    int release_queue_front;
    int release_queue_back;
    _sg_mtl_release_item_t* release_queue;
} _sg_mtl_idpool_t;

typedef struct _sg_buffer_s {
    _sg_slot_t slot;
    _sg_buffer_common_t cmn;
    struct {
        int buf[SG_NUM_INFLIGHT_FRAMES];  // index into _sg_mtl_pool
    } mtl;
} _sg_mtl_buffer_t;
typedef _sg_mtl_buffer_t _sg_buffer_t;

typedef struct _sg_image_s {
    _sg_slot_t slot;
    _sg_image_common_t cmn;
    struct {
        int tex[SG_NUM_INFLIGHT_FRAMES];
    } mtl;
} _sg_mtl_image_t;
typedef _sg_mtl_image_t _sg_image_t;

typedef struct _sg_sampler_s {
    _sg_slot_t slot;
    _sg_sampler_common_t cmn;
    struct {
        int sampler_state;
    } mtl;
} _sg_mtl_sampler_t;
typedef _sg_mtl_sampler_t _sg_sampler_t;

typedef struct {
    int mtl_lib;
    int mtl_func;
} _sg_mtl_shader_func_t;

typedef struct _sg_shader_s {
    _sg_slot_t slot;
    _sg_shader_common_t cmn;
    struct {
        _sg_mtl_shader_func_t vertex_func;
        _sg_mtl_shader_func_t fragment_func;
        _sg_mtl_shader_func_t compute_func;
        MTLSize threads_per_threadgroup;
        uint8_t ub_buffer_n[SG_MAX_UNIFORMBLOCK_BINDSLOTS];
        uint8_t view_buffer_texture_n[SG_MAX_VIEW_BINDSLOTS];
        uint8_t smp_sampler_n[SG_MAX_SAMPLER_BINDSLOTS];
    } mtl;
} _sg_mtl_shader_t;
typedef _sg_mtl_shader_t _sg_shader_t;

typedef struct _sg_pipeline_s {
    _sg_slot_t slot;
    _sg_pipeline_common_t cmn;
    struct {
        MTLPrimitiveType prim_type;
        int index_size;
        MTLIndexType index_type;
        MTLCullMode cull_mode;
        MTLWinding winding;
        uint32_t stencil_ref;
        MTLSize threads_per_threadgroup;
        int cps;    // MTLComputePipelineState
        int rps;    // MTLRenderPipelineState
        int dss;    // MTLDepthStencilState
    } mtl;
} _sg_mtl_pipeline_t;
typedef _sg_mtl_pipeline_t _sg_pipeline_t;

typedef struct _sg_view_s {
    _sg_slot_t slot;
    _sg_view_common_t cmn;
    struct {
        int tex_view[SG_NUM_INFLIGHT_FRAMES];
    } mtl;
} _sg_mtl_view_t;
typedef _sg_mtl_view_t _sg_view_t;

// resource binding state cache
#define _SG_MTL_MAX_STAGE_UB_BINDINGS (_SG_MAX_UNIFORMBLOCK_BINDINGS_PER_STAGE)
#define _SG_MTL_MAX_STAGE_UB_SBUF_BINDINGS (_SG_MTL_MAX_STAGE_UB_BINDINGS + _SG_MAX_STORAGEBUFFER_BINDINGS_PER_STAGE)
#define _SG_MTL_MAX_STAGE_BUFFER_BINDINGS (_SG_MTL_MAX_STAGE_UB_SBUF_BINDINGS + SG_MAX_VERTEXBUFFER_BINDSLOTS)
#define _SG_MTL_MAX_STAGE_TEXTURE_BINDINGS (_SG_MAX_TEXTURE_BINDINGS_PER_STAGE + _SG_MAX_STORAGEIMAGE_BINDINGS_PER_STAGE)
#define _SG_MTL_MAX_STAGE_SAMPLER_BINDINGS (SG_MAX_SAMPLER_BINDSLOTS)

typedef struct {
    _sg_sref_t sref;
    int active_slot;
    int offset;
} _sg_mtl_cache_buf_t;

typedef struct {
    _sg_sref_t sref;
    int active_slot;
    int offset;
} _sg_mtl_cache_tex_t;

typedef enum {
    _SG_MTL_CACHE_CMP_EQUAL = 0,
    _SG_MTL_CACHE_CMP_SREF = (1<<1),
    _SG_MTL_CACHE_CMP_OFFSET = (1<<2),
    _SG_MTL_CACHE_CMP_ACTIVESLOT = (1<<3),
} _sg_mtl_cache_cmp_result_t;

typedef struct {
    _sg_sref_t cur_pip;
    _sg_buffer_ref_t cur_ibuf;
    int cur_ibuf_offset;
    _sg_mtl_cache_buf_t cur_vsbufs[_SG_MTL_MAX_STAGE_BUFFER_BINDINGS];
    _sg_mtl_cache_buf_t cur_fsbufs[_SG_MTL_MAX_STAGE_BUFFER_BINDINGS];
    _sg_mtl_cache_buf_t cur_csbufs[_SG_MTL_MAX_STAGE_BUFFER_BINDINGS];
    _sg_mtl_cache_tex_t cur_vstexs[_SG_MTL_MAX_STAGE_TEXTURE_BINDINGS];
    _sg_mtl_cache_tex_t cur_fstexs[_SG_MTL_MAX_STAGE_TEXTURE_BINDINGS];
    _sg_mtl_cache_tex_t cur_cstexs[_SG_MTL_MAX_STAGE_TEXTURE_BINDINGS];
    _sg_sref_t cur_vssmps[_SG_MTL_MAX_STAGE_SAMPLER_BINDINGS];
    _sg_sref_t cur_fssmps[_SG_MTL_MAX_STAGE_SAMPLER_BINDINGS];
    _sg_sref_t cur_cssmps[_SG_MTL_MAX_STAGE_SAMPLER_BINDINGS];
} _sg_mtl_cache_t;

typedef struct {
    bool valid;
    bool use_shared_storage_mode;
    uint32_t cur_frame_rotate_index;
    int ub_size;
    int cur_ub_offset;
    uint8_t* cur_ub_base_ptr;
    _sg_mtl_cache_t cache;
    _sg_mtl_idpool_t idpool;
    dispatch_semaphore_t sem;
    id<MTLDevice> device;
    id<MTLCommandQueue> cmd_queue;
    id<MTLCommandBuffer> cmd_buffer;
    id<MTLRenderCommandEncoder> render_cmd_encoder;
    id<MTLComputeCommandEncoder> compute_cmd_encoder;
    id<CAMetalDrawable> cur_drawable;
    id<MTLBuffer> uniform_buffers[SG_NUM_INFLIGHT_FRAMES];
} _sg_mtl_backend_t;


// this *MUST* remain 0
#define _SG_INVALID_SLOT_INDEX (0)

typedef struct _sg_pools_s {
    _sg_pool_t buffer_pool;
    _sg_pool_t image_pool;
    _sg_pool_t sampler_pool;
    _sg_pool_t shader_pool;
    _sg_pool_t pipeline_pool;
    _sg_pool_t view_pool;
    _sg_buffer_t* buffers;
    _sg_image_t* images;
    _sg_sampler_t* samplers;
    _sg_shader_t* shaders;
    _sg_pipeline_t* pipelines;
    _sg_view_t* views;
} _sg_pools_t;

typedef struct {
    int num;        // number of allocated commit listener items
    int upper;      // the current upper index (no valid items past this point)
    sg_commit_listener* items;
} _sg_commit_listeners_t;

// resolved pass attachments struct
typedef struct {
    bool empty;
    int num_color_views;
    _sg_view_t* color_views[SG_MAX_COLOR_ATTACHMENTS];
    _sg_view_t* resolve_views[SG_MAX_COLOR_ATTACHMENTS];
    _sg_view_t* ds_view;
} _sg_attachments_ptrs_t;

// resolved resource bindings struct
typedef struct {
    _sg_pipeline_t* pip;
    int vb_offsets[SG_MAX_VERTEXBUFFER_BINDSLOTS];
    int ib_offset;
    _sg_buffer_t* vbs[SG_MAX_VERTEXBUFFER_BINDSLOTS];
    _sg_buffer_t* ib;
    _sg_view_t* views[SG_MAX_VIEW_BINDSLOTS];
    _sg_sampler_t* smps[SG_MAX_SAMPLER_BINDSLOTS];
} _sg_bindings_ptrs_t;

typedef struct {
    bool sample;
    bool filter;
    bool render;
    bool blend;
    bool msaa;
    bool depth;
    bool read;
    bool write;
} _sg_pixelformat_info_t;

typedef struct {
    bool valid;
    sg_desc desc;       // original desc with default values patched in
    uint32_t frame_index;
    struct {
        bool valid;
        bool in_pass;
        bool is_compute;
        _sg_dimi_t dim;
        sg_attachments atts;
        struct {
            sg_pixel_format color_fmt;
            sg_pixel_format depth_fmt;
            int sample_count;
        } swapchain;
    } cur_pass;
    _sg_pipeline_ref_t cur_pip;
    bool next_draw_valid;
    uint32_t required_bindings_and_uniforms;    // used to check that bindings and uniforms are applied after applying pipeline
    uint32_t applied_bindings_and_uniforms;     // bits 0..7: uniform blocks, bit 8: bindings
    #if defined(SOKOL_DEBUG)
    sg_log_item validate_error;
    #endif
    _sg_pools_t pools;
    sg_backend backend;
    sg_features features;
    sg_limits limits;
    _sg_pixelformat_info_t formats[_SG_PIXELFORMAT_NUM];
    bool stats_enabled;
    sg_frame_stats stats;
    sg_frame_stats prev_stats;
    #if defined(SOKOL_METAL)
    _sg_mtl_backend_t mtl;
    #elif defined(SOKOL_DUMMY_BACKEND)
    // dummy backend doesn't need state
    #endif
    #if defined(SOKOL_TRACE_HOOKS)
    sg_trace_hooks hooks;
    #endif
    _sg_commit_listeners_t commit_listeners;
} _sg_state_t;
static _sg_state_t _sg;

// ██       ██████   ██████   ██████  ██ ███    ██  ██████
// ██      ██    ██ ██       ██       ██ ████   ██ ██
// ██      ██    ██ ██   ███ ██   ███ ██ ██ ██  ██ ██   ███
// ██      ██    ██ ██    ██ ██    ██ ██ ██  ██ ██ ██    ██
// ███████  ██████   ██████   ██████  ██ ██   ████  ██████
//
// >>logging
#if defined(SOKOL_DEBUG)
#define _SG_LOGITEM_XMACRO(item,msg) #item ": " msg,
static const char* _sg_log_messages[] = {
    _SG_LOG_ITEMS
};
#undef _SG_LOGITEM_XMACRO
#endif // SOKOL_DEBUG

#define _SG_PANIC(code) _sg_log(SG_LOGITEM_ ##code, 0, 0, __LINE__)
#define _SG_ERROR(code) _sg_log(SG_LOGITEM_ ##code, 1, 0, __LINE__)
#define _SG_WARN(code) _sg_log(SG_LOGITEM_ ##code, 2, 0, __LINE__)
#define _SG_INFO(code) _sg_log(SG_LOGITEM_ ##code, 3, 0, __LINE__)
#define _SG_LOGMSG(code,msg) _sg_log(SG_LOGITEM_ ##code, 3, msg, __LINE__)
#define _SG_VALIDATE(cond,code) if (!(cond)){ _sg.validate_error = SG_LOGITEM_ ##code; _sg_log(SG_LOGITEM_ ##code, 1, 0, __LINE__); }

static void _sg_log(sg_log_item log_item, uint32_t log_level, const char* msg, uint32_t line_nr) {
    if (_sg.desc.logger.func) {
        const char* filename = 0;
        #if defined(SOKOL_DEBUG)
            filename = __FILE__;
            if (0 == msg) {
                msg = _sg_log_messages[log_item];
            }
        #endif
        _sg.desc.logger.func("sg", log_level, (uint32_t)log_item, msg, line_nr, filename, _sg.desc.logger.user_data);
    } else {
        // for log level PANIC it would be 'undefined behaviour' to continue
        if (log_level == 0) {
            abort();
        }
    }
}

// ███    ███ ███████ ███    ███  ██████  ██████  ██    ██
// ████  ████ ██      ████  ████ ██    ██ ██   ██  ██  ██
// ██ ████ ██ █████   ██ ████ ██ ██    ██ ██████    ████
// ██  ██  ██ ██      ██  ██  ██ ██    ██ ██   ██    ██
// ██      ██ ███████ ██      ██  ██████  ██   ██    ██
//
// >>memory

// a helper macro to clear a struct with potentially ARC'ed ObjC references
#if defined(SOKOL_METAL)
    #if defined(__cplusplus)
        #define _SG_CLEAR_ARC_STRUCT(type, item) { item = type(); }
    #else
        #define _SG_CLEAR_ARC_STRUCT(type, item) { item = (type) { 0 }; }
    #endif
#else
    #define _SG_CLEAR_ARC_STRUCT(type, item) { _sg_clear(&item, sizeof(item)); }
#endif

_SOKOL_PRIVATE void _sg_clear(void* ptr, size_t size) {
    SOKOL_ASSERT(ptr && (size > 0));
    memset(ptr, 0, size);
}

_SOKOL_PRIVATE void* _sg_malloc(size_t size) {
    SOKOL_ASSERT(size > 0);
    void* ptr;
    if (_sg.desc.allocator.alloc_fn) {
        ptr = _sg.desc.allocator.alloc_fn(size, _sg.desc.allocator.user_data);
    } else {
        ptr = malloc(size);
    }
    if (0 == ptr) {
        _SG_PANIC(MALLOC_FAILED);
    }
    return ptr;
}

_SOKOL_PRIVATE void* _sg_malloc_clear(size_t size) {
    void* ptr = _sg_malloc(size);
    _sg_clear(ptr, size);
    return ptr;
}

_SOKOL_PRIVATE void _sg_free(void* ptr) {
    if (_sg.desc.allocator.free_fn) {
        _sg.desc.allocator.free_fn(ptr, _sg.desc.allocator.user_data);
    } else {
        free(ptr);
    }
}

_SOKOL_PRIVATE bool _sg_strempty(const _sg_str_t* str) {
    return 0 == str->buf[0];
}

_SOKOL_PRIVATE const char* _sg_strptr(const _sg_str_t* str) {
    return &str->buf[0];
}

_SOKOL_PRIVATE void _sg_strcpy(_sg_str_t* dst, const char* src) {
    SOKOL_ASSERT(dst);
    if (src) {
        #if defined(_MSC_VER)
        strncpy_s(dst->buf, _SG_STRING_SIZE, src, (_SG_STRING_SIZE-1));
        #else
        strncpy(dst->buf, src, _SG_STRING_SIZE);
        #endif
        dst->buf[_SG_STRING_SIZE-1] = 0;
    } else {
        _sg_clear(dst->buf, _SG_STRING_SIZE);
    }
}

// ██████   ██████   ██████  ██
// ██   ██ ██    ██ ██    ██ ██
// ██████  ██    ██ ██    ██ ██
// ██      ██    ██ ██    ██ ██
// ██       ██████   ██████  ███████
//
// >>pool
_SOKOL_PRIVATE void _sg_pool_init(_sg_pool_t* pool, int num) {
    SOKOL_ASSERT(pool && (num >= 1));
    // slot 0 is reserved for the 'invalid id', so bump the pool size by 1
    pool->size = num + 1;
    pool->queue_top = 0;
    // generation counters indexable by pool slot index, slot 0 is reserved
    size_t gen_ctrs_size = sizeof(uint32_t) * (size_t)pool->size;
    pool->gen_ctrs = (uint32_t*)_sg_malloc_clear(gen_ctrs_size);
    // it's not a bug to only reserve 'num' here
    pool->free_queue = (int*) _sg_malloc_clear(sizeof(int) * (size_t)num);
    // never allocate the zero-th pool item since the invalid id is 0
    for (int i = pool->size-1; i >= 1; i--) {
        pool->free_queue[pool->queue_top++] = i;
    }
}

_SOKOL_PRIVATE void _sg_pool_discard(_sg_pool_t* pool) {
    SOKOL_ASSERT(pool);
    SOKOL_ASSERT(pool->free_queue);
    _sg_free(pool->free_queue);
    pool->free_queue = 0;
    SOKOL_ASSERT(pool->gen_ctrs);
    _sg_free(pool->gen_ctrs);
    pool->gen_ctrs = 0;
    pool->size = 0;
    pool->queue_top = 0;
}

_SOKOL_PRIVATE int _sg_pool_alloc_index(_sg_pool_t* pool) {
    SOKOL_ASSERT(pool);
    SOKOL_ASSERT(pool->free_queue);
    if (pool->queue_top > 0) {
        int slot_index = pool->free_queue[--pool->queue_top];
        SOKOL_ASSERT((slot_index > 0) && (slot_index < pool->size));
        return slot_index;
    } else {
        // pool exhausted
        return _SG_INVALID_SLOT_INDEX;
    }
}

_SOKOL_PRIVATE void _sg_pool_free_index(_sg_pool_t* pool, int slot_index) {
    SOKOL_ASSERT((slot_index > _SG_INVALID_SLOT_INDEX) && (slot_index < pool->size));
    SOKOL_ASSERT(pool);
    SOKOL_ASSERT(pool->free_queue);
    SOKOL_ASSERT(pool->queue_top < pool->size);
    #ifdef SOKOL_DEBUG
    // debug check against double-free
    for (int i = 0; i < pool->queue_top; i++) {
        SOKOL_ASSERT(pool->free_queue[i] != slot_index);
    }
    #endif
    pool->free_queue[pool->queue_top++] = slot_index;
    SOKOL_ASSERT(pool->queue_top <= (pool->size-1));
}

_SOKOL_PRIVATE void _sg_slot_reset(_sg_slot_t* slot) {
    SOKOL_ASSERT(slot);
    _sg_clear(slot, sizeof(_sg_slot_t));
}

_SOKOL_PRIVATE void _sg_reset_buffer_to_alloc_state(_sg_buffer_t* buf) {
    SOKOL_ASSERT(buf);
    _sg_slot_t slot = buf->slot;
    _sg_clear(buf, sizeof(*buf));
    buf->slot = slot;
    buf->slot.uninit_count += 1;
    buf->slot.state = SG_RESOURCESTATE_ALLOC;
}

_SOKOL_PRIVATE void _sg_reset_image_to_alloc_state(_sg_image_t* img) {
    SOKOL_ASSERT(img);
    _sg_slot_t slot = img->slot;
    _sg_clear(img, sizeof(*img));
    img->slot = slot;
    img->slot.uninit_count += 1;
    img->slot.state = SG_RESOURCESTATE_ALLOC;
}

_SOKOL_PRIVATE void _sg_reset_sampler_to_alloc_state(_sg_sampler_t* smp) {
    SOKOL_ASSERT(smp);
    _sg_slot_t slot = smp->slot;
    _sg_clear(smp, sizeof(*smp));
    smp->slot = slot;
    smp->slot.uninit_count += 1;
    smp->slot.state = SG_RESOURCESTATE_ALLOC;
}

_SOKOL_PRIVATE void _sg_reset_shader_to_alloc_state(_sg_shader_t* shd) {
    SOKOL_ASSERT(shd);
    _sg_slot_t slot = shd->slot;
    _sg_clear(shd, sizeof(*shd));
    shd->slot = slot;
    shd->slot.uninit_count += 1;
    shd->slot.state = SG_RESOURCESTATE_ALLOC;
}

_SOKOL_PRIVATE void _sg_reset_pipeline_to_alloc_state(_sg_pipeline_t* pip) {
    SOKOL_ASSERT(pip);
    _sg_slot_t slot = pip->slot;
    _sg_clear(pip, sizeof(*pip));
    pip->slot = slot;
    pip->slot.uninit_count += 1;
    pip->slot.state = SG_RESOURCESTATE_ALLOC;
}

_SOKOL_PRIVATE void _sg_reset_view_to_alloc_state(_sg_view_t* view) {
    SOKOL_ASSERT(view);
    _sg_slot_t slot = view->slot;
    _sg_clear(view, sizeof(*view));
    view->slot = slot;
    view->slot.uninit_count += 1;
    view->slot.state = SG_RESOURCESTATE_ALLOC;
}

_SOKOL_PRIVATE void _sg_setup_pools(_sg_pools_t* p, const sg_desc* desc) {
    SOKOL_ASSERT(p);
    SOKOL_ASSERT(desc);
    // note: the pools here will have an additional item, since slot 0 is reserved
    SOKOL_ASSERT((desc->buffer_pool_size > 0) && (desc->buffer_pool_size < _SG_MAX_POOL_SIZE));
    _sg_pool_init(&p->buffer_pool, desc->buffer_pool_size);
    size_t buffer_pool_byte_size = sizeof(_sg_buffer_t) * (size_t)p->buffer_pool.size;
    p->buffers = (_sg_buffer_t*) _sg_malloc_clear(buffer_pool_byte_size);

    SOKOL_ASSERT((desc->image_pool_size > 0) && (desc->image_pool_size < _SG_MAX_POOL_SIZE));
    _sg_pool_init(&p->image_pool, desc->image_pool_size);
    size_t image_pool_byte_size = sizeof(_sg_image_t) * (size_t)p->image_pool.size;
    p->images = (_sg_image_t*) _sg_malloc_clear(image_pool_byte_size);

    SOKOL_ASSERT((desc->sampler_pool_size > 0) && (desc->sampler_pool_size < _SG_MAX_POOL_SIZE));
    _sg_pool_init(&p->sampler_pool, desc->sampler_pool_size);
    size_t sampler_pool_byte_size = sizeof(_sg_sampler_t) * (size_t)p->sampler_pool.size;
    p->samplers = (_sg_sampler_t*) _sg_malloc_clear(sampler_pool_byte_size);

    SOKOL_ASSERT((desc->shader_pool_size > 0) && (desc->shader_pool_size < _SG_MAX_POOL_SIZE));
    _sg_pool_init(&p->shader_pool, desc->shader_pool_size);
    size_t shader_pool_byte_size = sizeof(_sg_shader_t) * (size_t)p->shader_pool.size;
    p->shaders = (_sg_shader_t*) _sg_malloc_clear(shader_pool_byte_size);

    SOKOL_ASSERT((desc->pipeline_pool_size > 0) && (desc->pipeline_pool_size < _SG_MAX_POOL_SIZE));
    _sg_pool_init(&p->pipeline_pool, desc->pipeline_pool_size);
    size_t pipeline_pool_byte_size = sizeof(_sg_pipeline_t) * (size_t)p->pipeline_pool.size;
    p->pipelines = (_sg_pipeline_t*) _sg_malloc_clear(pipeline_pool_byte_size);

    SOKOL_ASSERT((desc->view_pool_size > 0) && (desc->view_pool_size < _SG_MAX_POOL_SIZE));
    _sg_pool_init(&p->view_pool, desc->view_pool_size);
    size_t view_pool_byte_size = sizeof(_sg_view_t) * (size_t)p->view_pool.size;
    p->views = (_sg_view_t*) _sg_malloc_clear(view_pool_byte_size);
}

_SOKOL_PRIVATE void _sg_discard_pools(_sg_pools_t* p) {
    SOKOL_ASSERT(p);
    _sg_free(p->views);     p->views = 0;
    _sg_free(p->pipelines); p->pipelines = 0;
    _sg_free(p->shaders);   p->shaders = 0;
    _sg_free(p->samplers);  p->samplers = 0;
    _sg_free(p->images);    p->images = 0;
    _sg_free(p->buffers);   p->buffers = 0;
    _sg_pool_discard(&p->view_pool);
    _sg_pool_discard(&p->pipeline_pool);
    _sg_pool_discard(&p->shader_pool);
    _sg_pool_discard(&p->sampler_pool);
    _sg_pool_discard(&p->image_pool);
    _sg_pool_discard(&p->buffer_pool);
}

/* allocate the slot at slot_index:
    - bump the slot's generation counter
    - create a resource id from the generation counter and slot index
    - set the slot's id to this id
    - set the slot's state to ALLOC
    - return the resource id
*/
_SOKOL_PRIVATE uint32_t _sg_slot_alloc(_sg_pool_t* pool, _sg_slot_t* slot, int slot_index) {
    /* FIXME: add handling for an overflowing generation counter,
       for now, just overflow (another option is to disable
       the slot)
    */
    SOKOL_ASSERT(pool && pool->gen_ctrs);
    SOKOL_ASSERT((slot_index > _SG_INVALID_SLOT_INDEX) && (slot_index < pool->size));
    SOKOL_ASSERT(slot->id == SG_INVALID_ID);
    SOKOL_ASSERT(slot->state == SG_RESOURCESTATE_INITIAL);
    uint32_t ctr = ++pool->gen_ctrs[slot_index];
    slot->id = (ctr<<_SG_SLOT_SHIFT)|(slot_index & _SG_SLOT_MASK);
    slot->state = SG_RESOURCESTATE_ALLOC;
    return slot->id;
}

// extract slot index from id
_SOKOL_PRIVATE int _sg_slot_index(uint32_t id) {
    int slot_index = (int) (id & _SG_SLOT_MASK);
    SOKOL_ASSERT(_SG_INVALID_SLOT_INDEX != slot_index);
    return slot_index;
}

// returns pointer to resource by id without matching id check
_SOKOL_PRIVATE _sg_buffer_t* _sg_buffer_at(uint32_t buf_id) {
    SOKOL_ASSERT(SG_INVALID_ID != buf_id);
    int slot_index = _sg_slot_index(buf_id);
    SOKOL_ASSERT((slot_index > _SG_INVALID_SLOT_INDEX) && (slot_index < _sg.pools.buffer_pool.size));
    return &_sg.pools.buffers[slot_index];
}

_SOKOL_PRIVATE _sg_image_t* _sg_image_at(uint32_t img_id) {
    SOKOL_ASSERT(SG_INVALID_ID != img_id);
    int slot_index = _sg_slot_index(img_id);
    SOKOL_ASSERT((slot_index > _SG_INVALID_SLOT_INDEX) && (slot_index < _sg.pools.image_pool.size));
    return &_sg.pools.images[slot_index];
}

_SOKOL_PRIVATE _sg_sampler_t* _sg_sampler_at(uint32_t smp_id) {
    SOKOL_ASSERT(SG_INVALID_ID != smp_id);
    int slot_index = _sg_slot_index(smp_id);
    SOKOL_ASSERT((slot_index > _SG_INVALID_SLOT_INDEX) && (slot_index < _sg.pools.sampler_pool.size));
    return &_sg.pools.samplers[slot_index];
}

_SOKOL_PRIVATE _sg_shader_t* _sg_shader_at(uint32_t shd_id) {
    SOKOL_ASSERT(SG_INVALID_ID != shd_id);
    int slot_index = _sg_slot_index(shd_id);
    SOKOL_ASSERT((slot_index > _SG_INVALID_SLOT_INDEX) && (slot_index < _sg.pools.shader_pool.size));
    return &_sg.pools.shaders[slot_index];
}

_SOKOL_PRIVATE _sg_pipeline_t* _sg_pipeline_at(uint32_t pip_id) {
    SOKOL_ASSERT(SG_INVALID_ID != pip_id);
    int slot_index = _sg_slot_index(pip_id);
    SOKOL_ASSERT((slot_index > _SG_INVALID_SLOT_INDEX) && (slot_index < _sg.pools.pipeline_pool.size));
    return &_sg.pools.pipelines[slot_index];
}

_SOKOL_PRIVATE _sg_view_t* _sg_view_at(uint32_t view_id) {
    SOKOL_ASSERT(SG_INVALID_ID != view_id);
    int slot_index = _sg_slot_index(view_id);
    SOKOL_ASSERT((slot_index > _SG_INVALID_SLOT_INDEX) && (slot_index < _sg.pools.view_pool.size));
    return &_sg.pools.views[slot_index];
}

// returns pointer to resource with matching id check, may return 0
_SOKOL_PRIVATE _sg_buffer_t* _sg_lookup_buffer(uint32_t buf_id) {
    if (SG_INVALID_ID != buf_id) {
        _sg_buffer_t* buf = _sg_buffer_at(buf_id);
        if (buf->slot.id == buf_id) {
            return buf;
        }
    }
    return 0;
}

_SOKOL_PRIVATE _sg_image_t* _sg_lookup_image(uint32_t img_id) {
    if (SG_INVALID_ID != img_id) {
        _sg_image_t* img = _sg_image_at(img_id);
        if (img->slot.id == img_id) {
            return img;
        }
    }
    return 0;
}

_SOKOL_PRIVATE _sg_sampler_t* _sg_lookup_sampler(uint32_t smp_id) {
    if (SG_INVALID_ID != smp_id) {
        _sg_sampler_t* smp = _sg_sampler_at(smp_id);
        if (smp->slot.id == smp_id) {
            return smp;
        }
    }
    return 0;
}

_SOKOL_PRIVATE _sg_shader_t* _sg_lookup_shader(uint32_t shd_id) {
    if (SG_INVALID_ID != shd_id) {
        _sg_shader_t* shd = _sg_shader_at(shd_id);
        if (shd->slot.id == shd_id) {
            return shd;
        }
    }
    return 0;
}

_SOKOL_PRIVATE _sg_pipeline_t* _sg_lookup_pipeline(uint32_t pip_id) {
    if (SG_INVALID_ID != pip_id) {
        _sg_pipeline_t* pip = _sg_pipeline_at(pip_id);
        if (pip->slot.id == pip_id) {
            return pip;
        }
    }
    return 0;
}

_SOKOL_PRIVATE _sg_view_t* _sg_lookup_view(uint32_t view_id) {
    if (SG_INVALID_ID != view_id) {
        _sg_view_t* view = _sg_view_at(view_id);
        if (view->slot.id == view_id) {
            return view;
        }
    }
    return 0;
}

// ██████  ███████ ███████ ███████
// ██   ██ ██      ██      ██
// ██████  █████   █████   ███████
// ██   ██ ██      ██           ██
// ██   ██ ███████ ██      ███████
//
// >>refs
_SOKOL_PRIVATE _sg_sref_t _sg_sref(const _sg_slot_t* slot) {
    _sg_sref_t sref; _sg_clear(&sref, sizeof(sref));
    if (slot) {
        sref.id = slot->id;
        sref.uninit_count = slot->uninit_count;
    }
    return sref;
}

_SOKOL_PRIVATE bool _sg_sref_slot_eql(const _sg_sref_t* sref, const _sg_slot_t* slot) {
    SOKOL_ASSERT(sref && slot);
    return (sref->id == slot->id) && (sref->uninit_count == slot->uninit_count);
}

_SOKOL_PRIVATE bool _sg_sref_sref_eql(const _sg_sref_t* sref0, const _sg_sref_t* sref1) {
    SOKOL_ASSERT(sref0 && sref1);
    return (sref0->id == sref1->id) && (sref0->uninit_count == sref1->uninit_count);
}

_SOKOL_PRIVATE _sg_buffer_ref_t _sg_buffer_ref(_sg_buffer_t* buf_or_null) {
    _sg_buffer_ref_t ref; _sg_clear(&ref, sizeof(ref));
    if (buf_or_null) {
        _sg_buffer_t* buf = buf_or_null;
        SOKOL_ASSERT(buf->slot.id != SG_INVALID_ID);
        ref.ptr = buf;
        ref.sref = _sg_sref(&buf->slot);
    }
    return ref;
}

_SOKOL_PRIVATE _sg_image_ref_t _sg_image_ref(_sg_image_t* img_or_null) {
    _sg_image_ref_t ref; _sg_clear(&ref, sizeof(ref));
    if (img_or_null) {
        _sg_image_t* img = img_or_null;
        SOKOL_ASSERT(img->slot.id != SG_INVALID_ID);
        ref.ptr = img;
        ref.sref = _sg_sref(&img->slot);
    }
    return ref;
}

_SOKOL_PRIVATE _sg_sampler_ref_t _sg_sampler_ref(_sg_sampler_t* smp_or_null) {
    _sg_sampler_ref_t ref; _sg_clear(&ref, sizeof(ref));
    if (smp_or_null) {
        _sg_sampler_t* smp = smp_or_null;
        SOKOL_ASSERT(smp->slot.id != SG_INVALID_ID);
        ref.ptr = smp;
        ref.sref = _sg_sref(&smp->slot);
    }
    return ref;
}

_SOKOL_PRIVATE _sg_shader_ref_t _sg_shader_ref(_sg_shader_t* shd_or_null) {
    _sg_shader_ref_t ref; _sg_clear(&ref, sizeof(ref));
    if (shd_or_null) {
        _sg_shader_t* shd = shd_or_null;
        SOKOL_ASSERT(shd->slot.id != SG_INVALID_ID);
        ref.ptr = shd;
        ref.sref = _sg_sref(&shd->slot);
    }
    return ref;
}

_SOKOL_PRIVATE _sg_pipeline_ref_t _sg_pipeline_ref(_sg_pipeline_t* pip_or_null) {
    _sg_pipeline_ref_t ref; _sg_clear(&ref, sizeof(ref));
    if (pip_or_null) {
        _sg_pipeline_t* pip = pip_or_null;
        SOKOL_ASSERT(pip->slot.id != SG_INVALID_ID);
        ref.ptr = pip;
        ref.sref = _sg_sref(&pip->slot);
    }
    return ref;
}

_SOKOL_PRIVATE _sg_view_ref_t _sg_view_ref(_sg_view_t* view_or_null) {
    _sg_view_ref_t ref; _sg_clear(&ref, sizeof(ref));
    if (view_or_null) {
        _sg_view_t* view = view_or_null;
        SOKOL_ASSERT(view->slot.id != SG_INVALID_ID);
        ref.ptr = view;
        ref.sref = _sg_sref(&view->slot);
    }
    return ref;
}

#define _SG_IMPL_RES_EQL(NAME,REF,RES) _SOKOL_PRIVATE bool NAME(const REF* ref, const RES* res) { SOKOL_ASSERT(ref && res); return _sg_sref_slot_eql(&ref->sref, &res->slot); }
_SG_IMPL_RES_EQL(_sg_buffer_ref_eql, _sg_buffer_ref_t, _sg_buffer_t)
_SG_IMPL_RES_EQL(_sg_image_ref_eql, _sg_image_ref_t, _sg_image_t)
_SG_IMPL_RES_EQL(_sg_sampler_ref_eql, _sg_sampler_ref_t, _sg_sampler_t)
_SG_IMPL_RES_EQL(_sg_shader_ref_eql, _sg_shader_ref_t, _sg_shader_t)
_SG_IMPL_RES_EQL(_sg_pipeline_ref_eql, _sg_pipeline_ref_t, _sg_pipeline_t)
_SG_IMPL_RES_EQL(_sg_view_ref_eql, _sg_view_ref_t, _sg_view_t)

#define _SG_IMPL_RES_NULL(NAME,REF) _SOKOL_PRIVATE bool NAME(const REF* ref) { SOKOL_ASSERT(ref); return SG_INVALID_ID == ref->sref.id; }
_SG_IMPL_RES_NULL(_sg_buffer_ref_null, _sg_buffer_ref_t)
_SG_IMPL_RES_NULL(_sg_image_ref_null, _sg_image_ref_t)
_SG_IMPL_RES_NULL(_sg_sampler_ref_null, _sg_sampler_ref_t)
_SG_IMPL_RES_NULL(_sg_shader_ref_null, _sg_shader_ref_t)
_SG_IMPL_RES_NULL(_sg_pipeline_ref_null, _sg_pipeline_ref_t)
_SG_IMPL_RES_NULL(_sg_view_ref_null, _sg_view_ref_t)

#define _SG_IMPL_RES_ALIVE(NAME,REF) _SOKOL_PRIVATE bool NAME(const REF* ref) { SOKOL_ASSERT(ref); return ref->ptr && _sg_sref_slot_eql(&ref->sref, &ref->ptr->slot); }
_SG_IMPL_RES_ALIVE(_sg_buffer_ref_alive, _sg_buffer_ref_t)
_SG_IMPL_RES_ALIVE(_sg_image_ref_alive, _sg_image_ref_t)
_SG_IMPL_RES_ALIVE(_sg_sampler_ref_alive, _sg_sampler_ref_t)
_SG_IMPL_RES_ALIVE(_sg_shader_ref_alive, _sg_shader_ref_t)
_SG_IMPL_RES_ALIVE(_sg_pipeline_ref_alive, _sg_pipeline_ref_t)
_SG_IMPL_RES_ALIVE(_sg_view_ref_alive, _sg_view_ref_t)

#define _SG_IMPL_RES_VALID(NAME,REF) _SOKOL_PRIVATE bool NAME(const REF* ref) { SOKOL_ASSERT(ref); return ref->ptr && _sg_sref_slot_eql(&ref->sref, &ref->ptr->slot) && (ref->ptr->slot.state == SG_RESOURCESTATE_VALID); }
_SG_IMPL_RES_VALID(_sg_buffer_ref_valid, _sg_buffer_ref_t)
_SG_IMPL_RES_VALID(_sg_image_ref_valid, _sg_image_ref_t)
_SG_IMPL_RES_VALID(_sg_sampler_ref_valid, _sg_sampler_ref_t)
_SG_IMPL_RES_VALID(_sg_shader_ref_valid, _sg_shader_ref_t)
_SG_IMPL_RES_VALID(_sg_pipeline_ref_valid, _sg_pipeline_ref_t)
_SG_IMPL_RES_VALID(_sg_view_ref_valid, _sg_view_ref_t)

#define _SG_IMPL_RES_PTR(NAME,REF,RES) _SOKOL_PRIVATE RES* NAME(const REF* ref) { SOKOL_ASSERT(ref && ref->ptr && _sg_sref_slot_eql(&ref->sref, &ref->ptr->slot)); return ref->ptr; }
_SG_IMPL_RES_PTR(_sg_buffer_ref_ptr, _sg_buffer_ref_t, _sg_buffer_t)
_SG_IMPL_RES_PTR(_sg_image_ref_ptr, _sg_image_ref_t, _sg_image_t)
_SG_IMPL_RES_PTR(_sg_sampler_ref_ptr, _sg_sampler_ref_t, _sg_sampler_t)
_SG_IMPL_RES_PTR(_sg_shader_ref_ptr, _sg_shader_ref_t, _sg_shader_t)
_SG_IMPL_RES_PTR(_sg_pipeline_ref_ptr, _sg_pipeline_ref_t, _sg_pipeline_t)
_SG_IMPL_RES_PTR(_sg_view_ref_ptr, _sg_view_ref_t, _sg_view_t)

#define _SG_IMPL_RES_PTR_OR_NULL(NAME,REF,RES) _SOKOL_PRIVATE RES* NAME(const REF* ref) { SOKOL_ASSERT(ref); if ((SG_INVALID_ID != ref->sref.id) && _sg_sref_slot_eql(&ref->sref, &ref->ptr->slot)) { return ref->ptr; } else { return 0; } }
_SG_IMPL_RES_PTR_OR_NULL(_sg_buffer_ref_ptr_or_null, _sg_buffer_ref_t, _sg_buffer_t)
_SG_IMPL_RES_PTR_OR_NULL(_sg_image_ref_ptr_or_null, _sg_image_ref_t, _sg_image_t)
_SG_IMPL_RES_PTR_OR_NULL(_sg_sampler_ref_ptr_or_null, _sg_sampler_ref_t, _sg_sampler_t)
_SG_IMPL_RES_PTR_OR_NULL(_sg_shader_ref_ptr_or_null, _sg_shader_ref_t, _sg_shader_t)
_SG_IMPL_RES_PTR_OR_NULL(_sg_pipeline_ref_ptr_or_null, _sg_pipeline_ref_t, _sg_pipeline_t)
_SG_IMPL_RES_PTR_OR_NULL(_sg_view_ref_ptr_or_null, _sg_view_ref_t, _sg_view_t)

// ██   ██ ███████ ██      ██████  ███████ ██████  ███████
// ██   ██ ██      ██      ██   ██ ██      ██   ██ ██
// ███████ █████   ██      ██████  █████   ██████  ███████
// ██   ██ ██      ██      ██      ██      ██   ██      ██
// ██   ██ ███████ ███████ ██      ███████ ██   ██ ███████
//
// >>helpers

// helper macros
#define _sg_def(val, def) (((val) == 0) ? (def) : (val))
#define _sg_def_flt(val, def) (((val) == 0.0f) ? (def) : (val))
#define _sg_min(a,b) (((a)<(b))?(a):(b))
#define _sg_max(a,b) (((a)>(b))?(a):(b))
#define _sg_clamp(v,v0,v1) (((v)<(v0))?(v0):(((v)>(v1))?(v1):(v)))
#define _sg_fequal(val,cmp,delta) ((((val)-(cmp))> -(delta))&&(((val)-(cmp))<(delta)))
#define _sg_ispow2(val) ((val&(val-1))==0)
#define _sg_stats_add(key,val) {if(_sg.stats_enabled){ _sg.stats.key+=val;}}

_SOKOL_PRIVATE uint32_t _sg_align_u32(uint32_t val, uint32_t align) {
    SOKOL_ASSERT((align > 0) && ((align & (align - 1)) == 0));
    return (val + (align - 1)) & ~(align - 1);
}

_SOKOL_PRIVATE _sg_recti_t _sg_clipi(int x, int y, int w, int h, int clip_width, int clip_height) {
    x = _sg_min(_sg_max(0, x), clip_width-1);
    y = _sg_min(_sg_max(0, y), clip_height-1);
    if ((x + w) > clip_width) {
        w = clip_width - x;
    }
    if ((y + h) > clip_height) {
        h = clip_height - y;
    }
    w = _sg_max(w, 1);
    h = _sg_max(h, 1);
    const _sg_recti_t res = { x, y, w, h };
    return res;
}

// return size of a mipmap level
_SOKOL_PRIVATE int _sg_miplevel_dim(int base_dim, int mip_level) {
    return _sg_max(base_dim >> mip_level, 1);
}

_SOKOL_PRIVATE bool _sg_image_view_alive(const _sg_view_t* view) {
    return view && _sg_image_ref_alive(&view->cmn.img.ref);
}

_SOKOL_PRIVATE _sg_dimi_t _sg_image_view_dim(const _sg_view_t* view) {
    SOKOL_ASSERT(view);
    const _sg_image_t* img = _sg_image_ref_ptr(&view->cmn.img.ref);
    SOKOL_ASSERT((img->cmn.width > 0) && (img->cmn.height > 0));
    _sg_dimi_t res; _sg_clear(&res, sizeof(res));
    res.width = _sg_miplevel_dim(img->cmn.width, view->cmn.img.mip_level);
    res.height = _sg_miplevel_dim(img->cmn.height, view->cmn.img.mip_level);
    return res;
}

_SOKOL_PRIVATE bool _sg_attachments_empty(const sg_attachments* atts) {
    SOKOL_ASSERT(atts);
    for (size_t i = 0; i < SG_MAX_COLOR_ATTACHMENTS; i++) {
        if (atts->colors[i].id != SG_INVALID_ID) {
            return false;
        }
        if (atts->resolves[i].id != SG_INVALID_ID) {
            return false;
        }
    }
    if (atts->depth_stencil.id != SG_INVALID_ID) {
        return false;
    }
    return true;
}

_SOKOL_PRIVATE _sg_attachments_ptrs_t _sg_attachments_ptrs(const sg_attachments* atts) {
    SOKOL_ASSERT(atts);
    _sg_attachments_ptrs_t res;
    _sg_clear(&res, sizeof(res));
    res.empty = true;
    for (int i = 0; i < SG_MAX_COLOR_ATTACHMENTS; i++) {
        if (atts->colors[i].id != SG_INVALID_ID) {
            res.empty = false;
            res.num_color_views += 1;
            res.color_views[i] = _sg_lookup_view(atts->colors[i].id);
        }
        if (atts->resolves[i].id != SG_INVALID_ID) {
            SOKOL_ASSERT(atts->colors[i].id != SG_INVALID_ID);
            res.empty = false;
            res.resolve_views[i] = _sg_lookup_view(atts->resolves[i].id);
        }
    }
    if (atts->depth_stencil.id != SG_INVALID_ID) {
        res.empty = false;
        res.ds_view = _sg_lookup_view(atts->depth_stencil.id);
    }
    return res;
}

_SOKOL_PRIVATE _sg_dimi_t _sg_attachments_dim(const _sg_attachments_ptrs_t* atts_ptrs) {
    if (atts_ptrs->ds_view) {
        return _sg_image_view_dim(atts_ptrs->ds_view);
    } else {
        SOKOL_ASSERT(atts_ptrs->color_views[0]);
        return _sg_image_view_dim(atts_ptrs->color_views[0]);
    }
}

_SOKOL_PRIVATE bool _sg_attachments_alive(const _sg_attachments_ptrs_t* atts_ptrs) {
    for (int i = 0; i < atts_ptrs->num_color_views; i++) {
        if (!_sg_image_view_alive(atts_ptrs->color_views[i])) {
            return false;
        }
        if (atts_ptrs->resolve_views[i] && !_sg_image_view_alive(atts_ptrs->resolve_views[i])) {
            return false;
        }
    }
    if (atts_ptrs->ds_view && !_sg_image_view_alive(atts_ptrs->ds_view)) {
        return false;
    }
    return true;
}

_SOKOL_PRIVATE void _sg_buffer_common_init(_sg_buffer_common_t* cmn, const sg_buffer_desc* desc) {
    cmn->size = (int)desc->size;
    cmn->append_pos = 0;
    cmn->append_overflow = false;
    cmn->update_frame_index = 0;
    cmn->append_frame_index = 0;
    cmn->num_slots = desc->usage.immutable ? 1 : SG_NUM_INFLIGHT_FRAMES;
    cmn->active_slot = 0;
    cmn->usage = desc->usage;
}

_SOKOL_PRIVATE void _sg_image_common_init(_sg_image_common_t* cmn, const sg_image_desc* desc) {
    cmn->upd_frame_index = 0;
    cmn->num_slots = desc->usage.immutable ? 1 : SG_NUM_INFLIGHT_FRAMES;
    cmn->active_slot = 0;
    cmn->type = desc->type;
    cmn->width = desc->width;
    cmn->height = desc->height;
    cmn->num_slices = desc->num_slices;
    cmn->num_mipmaps = desc->num_mipmaps;
    cmn->usage = desc->usage;
    cmn->pixel_format = desc->pixel_format;
    cmn->sample_count = desc->sample_count;
}

_SOKOL_PRIVATE void _sg_sampler_common_init(_sg_sampler_common_t* cmn, const sg_sampler_desc* desc) {
    cmn->min_filter = desc->min_filter;
    cmn->mag_filter = desc->mag_filter;
    cmn->mipmap_filter = desc->mipmap_filter;
    cmn->wrap_u = desc->wrap_u;
    cmn->wrap_v = desc->wrap_v;
    cmn->wrap_w = desc->wrap_w;
    cmn->min_lod = desc->min_lod;
    cmn->max_lod = desc->max_lod;
    cmn->border_color = desc->border_color;
    cmn->compare = desc->compare;
    cmn->max_anisotropy = desc->max_anisotropy;
}

_SOKOL_PRIVATE void _sg_shader_common_init(_sg_shader_common_t* cmn, const sg_shader_desc* desc) {
    cmn->is_compute = desc->compute_func.source || desc->compute_func.bytecode.ptr;
    for (size_t i = 0; i < SG_MAX_VERTEX_ATTRIBUTES; i++) {
        cmn->attrs[i].base_type = desc->attrs[i].base_type;
    }
    for (size_t i = 0; i < SG_MAX_UNIFORMBLOCK_BINDSLOTS; i++) {
        const sg_shader_uniform_block* src = &desc->uniform_blocks[i];
        _sg_shader_uniform_block_t* dst = &cmn->uniform_blocks[i];
        if (src->stage != SG_SHADERSTAGE_NONE) {
            cmn->required_bindings_and_uniforms |= (1 << i);
            dst->stage = src->stage;
            dst->size = src->size;
        }
    }
    const uint32_t required_bindings_flag = (1 << SG_MAX_UNIFORMBLOCK_BINDSLOTS);
    for (size_t i = 0; i < SG_MAX_VIEW_BINDSLOTS; i++) {
        _sg_shader_view_t* dst = &cmn->views[i];
        if (desc->views[i].texture.stage != SG_SHADERSTAGE_NONE) {
            const sg_shader_texture_view* src = &desc->views[i].texture;
            dst->stage = src->stage;
            dst->view_type = SG_VIEWTYPE_TEXTURE;
            dst->image_type = src->image_type;
            dst->sample_type = src->sample_type;
            dst->multisampled = src->multisampled;
        } else if (desc->views[i].storage_buffer.stage != SG_SHADERSTAGE_NONE) {
            const sg_shader_storage_buffer_view* src = &desc->views[i].storage_buffer;
            cmn->required_bindings_and_uniforms |= required_bindings_flag;
            dst->stage = src->stage;
            dst->view_type = SG_VIEWTYPE_STORAGEBUFFER;
            dst->sbuf_readonly = src->readonly;
        } else if (desc->views[i].storage_image.stage != SG_SHADERSTAGE_NONE) {
            const sg_shader_storage_image_view* src = &desc->views[i].storage_image;
            cmn->required_bindings_and_uniforms |= required_bindings_flag;
            dst->stage = src->stage;
            dst->view_type = SG_VIEWTYPE_STORAGEIMAGE;
            dst->image_type = src->image_type;
            dst->access_format = src->access_format;
            dst->simg_writeonly = src->writeonly;
        }
    }
    for (size_t i = 0; i < SG_MAX_SAMPLER_BINDSLOTS; i++) {
        const sg_shader_sampler* src = &desc->samplers[i];
        _sg_shader_sampler_t* dst = &cmn->samplers[i];
        if (src->stage != SG_SHADERSTAGE_NONE) {
            cmn->required_bindings_and_uniforms |= required_bindings_flag;
            dst->stage = src->stage;
            dst->sampler_type = src->sampler_type;
        }
    }
    for (size_t i = 0; i < SG_MAX_TEXTURE_SAMPLER_PAIRS; i++) {
        const sg_shader_texture_sampler_pair* src = &desc->texture_sampler_pairs[i];
        _sg_shader_texture_sampler_t* dst = &cmn->texture_samplers[i];
        if (src->stage != SG_SHADERSTAGE_NONE) {
            dst->stage = src->stage;
            SOKOL_ASSERT((src->view_slot >= 0) && (src->view_slot < SG_MAX_VIEW_BINDSLOTS));
            SOKOL_ASSERT(cmn->views[src->view_slot].view_type == SG_VIEWTYPE_TEXTURE);
            SOKOL_ASSERT(cmn->views[src->view_slot].stage == src->stage);
            dst->view_slot = src->view_slot;
            SOKOL_ASSERT((src->sampler_slot >= 0) && (src->sampler_slot < SG_MAX_SAMPLER_BINDSLOTS));
            SOKOL_ASSERT(desc->samplers[src->sampler_slot].stage == src->stage);
            dst->sampler_slot = src->sampler_slot;
        }
    }
}

_SOKOL_PRIVATE void _sg_pipeline_common_init(_sg_pipeline_common_t* cmn, const sg_pipeline_desc* desc, _sg_shader_t* shd) {
    SOKOL_ASSERT((desc->color_count >= 0) && (desc->color_count <= SG_MAX_COLOR_ATTACHMENTS));

    // FIXME: most of this isn't needed for compute pipelines

    const uint32_t required_bindings_flag = (1 << SG_MAX_UNIFORMBLOCK_BINDSLOTS);
    for (int i = 0; i < SG_MAX_VERTEXBUFFER_BINDSLOTS; i++) {
        const sg_vertex_attr_state* a_state = &desc->layout.attrs[i];
        if (a_state->format != SG_VERTEXFORMAT_INVALID) {
            SOKOL_ASSERT(a_state->buffer_index < SG_MAX_VERTEXBUFFER_BINDSLOTS);
            cmn->vertex_buffer_layout_active[a_state->buffer_index] = true;
            cmn->required_bindings_and_uniforms |= required_bindings_flag;
        }
    }
    cmn->is_compute = desc->compute;
    cmn->use_instanced_draw = false;
    cmn->shader = _sg_shader_ref(shd);
    cmn->layout = desc->layout;
    cmn->depth = desc->depth;
    cmn->stencil = desc->stencil;
    cmn->color_count = desc->color_count;
    for (int i = 0; i < desc->color_count; i++) {
        cmn->colors[i] = desc->colors[i];
    }
    cmn->primitive_type = desc->primitive_type;
    cmn->index_type = desc->index_type;
    if (cmn->index_type != SG_INDEXTYPE_NONE) {
        cmn->required_bindings_and_uniforms |= required_bindings_flag;
    }
    cmn->cull_mode = desc->cull_mode;
    cmn->face_winding = desc->face_winding;
    cmn->sample_count = desc->sample_count;
    cmn->blend_color = desc->blend_color;
    cmn->alpha_to_coverage_enabled = desc->alpha_to_coverage_enabled;
}

_SOKOL_PRIVATE void _sg_buffer_view_common_init(_sg_buffer_view_common_t* cmn, const sg_buffer_view_desc* desc, _sg_buffer_t* buf) {
    SOKOL_ASSERT(SG_RESOURCESTATE_VALID == buf->slot.state);
    cmn->ref = _sg_buffer_ref(buf);
    cmn->offset = desc->offset;
}

_SOKOL_PRIVATE void _sg_texture_view_common_init(_sg_image_view_common_t* cmn, const sg_texture_view_desc* desc, _sg_image_t* img) {
    SOKOL_ASSERT(SG_RESOURCESTATE_VALID == img->slot.state);
    cmn->ref = _sg_image_ref(img);
    cmn->mip_level = desc->mip_levels.base;
    cmn->mip_level_count = _sg_def(desc->mip_levels.count, img->cmn.num_mipmaps - cmn->mip_level);
    cmn->slice = desc->slices.base;
    switch (img->cmn.type) {
        case SG_IMAGETYPE_2D:
            cmn->slice_count = 1;
            break;
        case SG_IMAGETYPE_CUBE:
            cmn->slice_count = 6;
            break;
        case SG_IMAGETYPE_3D:
            cmn->slice_count = 1;
            break;
        case SG_IMAGETYPE_ARRAY:
            cmn->slice_count = _sg_def(desc->slices.count, img->cmn.num_slices - cmn->slice);
            break;
        default:
            SOKOL_UNREACHABLE;
    }
}

_SOKOL_PRIVATE void _sg_image_view_common_init(_sg_image_view_common_t* cmn, const sg_image_view_desc* desc, _sg_image_t* img) {
    SOKOL_ASSERT(SG_RESOURCESTATE_VALID == img->slot.state);
    cmn->ref = _sg_image_ref(img);
    cmn->mip_level = desc->mip_level;
    cmn->mip_level_count = 1;
    cmn->slice = desc->slice;
    cmn->slice_count = 1;
}

_SOKOL_PRIVATE void _sg_view_common_init(_sg_view_common_t* cmn, const sg_view_desc* desc, _sg_buffer_t* buf, _sg_image_t* img) {
    if (desc->texture.image.id != SG_INVALID_ID) {
        SOKOL_ASSERT(img);
        cmn->type = SG_VIEWTYPE_TEXTURE;
        _sg_texture_view_common_init(&cmn->img, &desc->texture, img);
    } else if (desc->storage_buffer.buffer.id != SG_INVALID_ID) {
        SOKOL_ASSERT(buf);
        cmn->type = SG_VIEWTYPE_STORAGEBUFFER;
        _sg_buffer_view_common_init(&cmn->buf, &desc->storage_buffer, buf);
    } else if (desc->storage_image.image.id != SG_INVALID_ID) {
        SOKOL_ASSERT(img);
        cmn->type = SG_VIEWTYPE_STORAGEIMAGE;
        _sg_image_view_common_init(&cmn->img, &desc->storage_image, img);
    } else if (desc->color_attachment.image.id != SG_INVALID_ID) {
        SOKOL_ASSERT(img);
        cmn->type = SG_VIEWTYPE_COLORATTACHMENT;
        _sg_image_view_common_init(&cmn->img, &desc->color_attachment, img);
    } else if (desc->resolve_attachment.image.id != SG_INVALID_ID) {
        SOKOL_ASSERT(img);
        cmn->type = SG_VIEWTYPE_RESOLVEATTACHMENT;
        _sg_image_view_common_init(&cmn->img, &desc->resolve_attachment, img);
    } else if (desc->depth_stencil_attachment.image.id != SG_INVALID_ID) {
        SOKOL_ASSERT(img);
        cmn->type = SG_VIEWTYPE_DEPTHSTENCILATTACHMENT;
        _sg_image_view_common_init(&cmn->img, &desc->depth_stencil_attachment, img);
    } else {
        SOKOL_UNREACHABLE;
    }
}

_SOKOL_PRIVATE int _sg_vertexformat_bytesize(sg_vertex_format fmt) {
    switch (fmt) {
        case SG_VERTEXFORMAT_FLOAT:     return 4;
        case SG_VERTEXFORMAT_FLOAT2:    return 8;
        case SG_VERTEXFORMAT_FLOAT3:    return 12;
        case SG_VERTEXFORMAT_FLOAT4:    return 16;
        case SG_VERTEXFORMAT_INT:       return 4;
        case SG_VERTEXFORMAT_INT2:      return 8;
        case SG_VERTEXFORMAT_INT3:      return 12;
        case SG_VERTEXFORMAT_INT4:      return 16;
        case SG_VERTEXFORMAT_UINT:      return 4;
        case SG_VERTEXFORMAT_UINT2:     return 8;
        case SG_VERTEXFORMAT_UINT3:     return 12;
        case SG_VERTEXFORMAT_UINT4:     return 16;
        case SG_VERTEXFORMAT_BYTE4:     return 4;
        case SG_VERTEXFORMAT_BYTE4N:    return 4;
        case SG_VERTEXFORMAT_UBYTE4:    return 4;
        case SG_VERTEXFORMAT_UBYTE4N:   return 4;
        case SG_VERTEXFORMAT_SHORT2:    return 4;
        case SG_VERTEXFORMAT_SHORT2N:   return 4;
        case SG_VERTEXFORMAT_USHORT2:   return 4;
        case SG_VERTEXFORMAT_USHORT2N:  return 4;
        case SG_VERTEXFORMAT_SHORT4:    return 8;
        case SG_VERTEXFORMAT_SHORT4N:   return 8;
        case SG_VERTEXFORMAT_USHORT4:   return 8;
        case SG_VERTEXFORMAT_USHORT4N:  return 8;
        case SG_VERTEXFORMAT_UINT10_N2: return 4;
        case SG_VERTEXFORMAT_HALF2:     return 4;
        case SG_VERTEXFORMAT_HALF4:     return 8;
        case SG_VERTEXFORMAT_INVALID:   return 0;
        default:
            SOKOL_UNREACHABLE;
            return -1;
    }
}

_SOKOL_PRIVATE const char* _sg_vertexformat_to_string(sg_vertex_format fmt) {
    switch (fmt) {
        case SG_VERTEXFORMAT_FLOAT:     return "FLOAT";
        case SG_VERTEXFORMAT_FLOAT2:    return "FLOAT2";
        case SG_VERTEXFORMAT_FLOAT3:    return "FLOAT3";
        case SG_VERTEXFORMAT_FLOAT4:    return "FLOAT4";
        case SG_VERTEXFORMAT_INT:       return "INT";
        case SG_VERTEXFORMAT_INT2:      return "INT2";
        case SG_VERTEXFORMAT_INT3:      return "INT3";
        case SG_VERTEXFORMAT_INT4:      return "INT4";
        case SG_VERTEXFORMAT_UINT:      return "UINT";
        case SG_VERTEXFORMAT_UINT2:     return "UINT2";
        case SG_VERTEXFORMAT_UINT3:     return "UINT3";
        case SG_VERTEXFORMAT_UINT4:     return "UINT4";
        case SG_VERTEXFORMAT_BYTE4:     return "BYTE4";
        case SG_VERTEXFORMAT_BYTE4N:    return "BYTE4N";
        case SG_VERTEXFORMAT_UBYTE4:    return "UBYTE4";
        case SG_VERTEXFORMAT_UBYTE4N:   return "UBYTE4N";
        case SG_VERTEXFORMAT_SHORT2:    return "SHORT2";
        case SG_VERTEXFORMAT_SHORT2N:   return "SHORT2N";
        case SG_VERTEXFORMAT_USHORT2:   return "USHORT2";
        case SG_VERTEXFORMAT_USHORT2N:  return "USHORT2N";
        case SG_VERTEXFORMAT_SHORT4:    return "SHORT4";
        case SG_VERTEXFORMAT_SHORT4N:   return "SHORT4N";
        case SG_VERTEXFORMAT_USHORT4:   return "USHORT4";
        case SG_VERTEXFORMAT_USHORT4N:  return "USHORT4N";
        case SG_VERTEXFORMAT_UINT10_N2: return "UINT10_N2";
        case SG_VERTEXFORMAT_HALF2:     return "HALF2";
        case SG_VERTEXFORMAT_HALF4:     return "HALF4";
        default:
            SOKOL_UNREACHABLE;
            return "INVALID";
    }
}

_SOKOL_PRIVATE const char* _sg_shaderattrbasetype_to_string(sg_shader_attr_base_type b) {
    switch (b) {
        case SG_SHADERATTRBASETYPE_UNDEFINED:   return "UNDEFINED";
        case SG_SHADERATTRBASETYPE_FLOAT:       return "FLOAT";
        case SG_SHADERATTRBASETYPE_SINT:        return "SINT";
        case SG_SHADERATTRBASETYPE_UINT:        return "UINT";
        default:
            SOKOL_UNREACHABLE;
            return "INVALID";
    }
}

_SOKOL_PRIVATE sg_shader_attr_base_type _sg_vertexformat_basetype(sg_vertex_format fmt) {
    switch (fmt) {
        case SG_VERTEXFORMAT_FLOAT:
        case SG_VERTEXFORMAT_FLOAT2:
        case SG_VERTEXFORMAT_FLOAT3:
        case SG_VERTEXFORMAT_FLOAT4:
        case SG_VERTEXFORMAT_HALF2:
        case SG_VERTEXFORMAT_HALF4:
        case SG_VERTEXFORMAT_BYTE4N:
        case SG_VERTEXFORMAT_UBYTE4N:
        case SG_VERTEXFORMAT_SHORT2N:
        case SG_VERTEXFORMAT_USHORT2N:
        case SG_VERTEXFORMAT_SHORT4N:
        case SG_VERTEXFORMAT_USHORT4N:
        case SG_VERTEXFORMAT_UINT10_N2:
            return SG_SHADERATTRBASETYPE_FLOAT;
        case SG_VERTEXFORMAT_INT:
        case SG_VERTEXFORMAT_INT2:
        case SG_VERTEXFORMAT_INT3:
        case SG_VERTEXFORMAT_INT4:
        case SG_VERTEXFORMAT_BYTE4:
        case SG_VERTEXFORMAT_SHORT2:
        case SG_VERTEXFORMAT_SHORT4:
            return SG_SHADERATTRBASETYPE_SINT;
        case SG_VERTEXFORMAT_UINT:
        case SG_VERTEXFORMAT_UINT2:
        case SG_VERTEXFORMAT_UINT3:
        case SG_VERTEXFORMAT_UINT4:
        case SG_VERTEXFORMAT_UBYTE4:
        case SG_VERTEXFORMAT_USHORT2:
        case SG_VERTEXFORMAT_USHORT4:
            return SG_SHADERATTRBASETYPE_UINT;
        default:
            SOKOL_UNREACHABLE;
            return SG_SHADERATTRBASETYPE_UNDEFINED;
    }
}

_SOKOL_PRIVATE uint32_t _sg_uniform_alignment(sg_uniform_type type, int array_count, sg_uniform_layout ub_layout) {
    if (ub_layout == SG_UNIFORMLAYOUT_NATIVE) {
        return 1;
    } else {
        SOKOL_ASSERT(array_count > 0);
        if (array_count == 1) {
            switch (type) {
                case SG_UNIFORMTYPE_FLOAT:
                case SG_UNIFORMTYPE_INT:
                    return 4;
                case SG_UNIFORMTYPE_FLOAT2:
                case SG_UNIFORMTYPE_INT2:
                    return 8;
                case SG_UNIFORMTYPE_FLOAT3:
                case SG_UNIFORMTYPE_FLOAT4:
                case SG_UNIFORMTYPE_INT3:
                case SG_UNIFORMTYPE_INT4:
                    return 16;
                case SG_UNIFORMTYPE_MAT4:
                    return 16;
                default:
                    SOKOL_UNREACHABLE;
                    return 1;
            }
        } else {
            return 16;
        }
    }
}

_SOKOL_PRIVATE uint32_t _sg_uniform_size(sg_uniform_type type, int array_count, sg_uniform_layout ub_layout) {
    SOKOL_ASSERT(array_count > 0);
    if (array_count == 1) {
        switch (type) {
            case SG_UNIFORMTYPE_FLOAT:
            case SG_UNIFORMTYPE_INT:
                return 4;
            case SG_UNIFORMTYPE_FLOAT2:
            case SG_UNIFORMTYPE_INT2:
                return 8;
            case SG_UNIFORMTYPE_FLOAT3:
            case SG_UNIFORMTYPE_INT3:
                return 12;
            case SG_UNIFORMTYPE_FLOAT4:
            case SG_UNIFORMTYPE_INT4:
                return 16;
            case SG_UNIFORMTYPE_MAT4:
                return 64;
            default:
                SOKOL_UNREACHABLE;
                return 0;
        }
    } else {
        if (ub_layout == SG_UNIFORMLAYOUT_NATIVE) {
            switch (type) {
                case SG_UNIFORMTYPE_FLOAT:
                case SG_UNIFORMTYPE_INT:
                    return 4 * (uint32_t)array_count;
                case SG_UNIFORMTYPE_FLOAT2:
                case SG_UNIFORMTYPE_INT2:
                    return 8 * (uint32_t)array_count;
                case SG_UNIFORMTYPE_FLOAT3:
                case SG_UNIFORMTYPE_INT3:
                    return 12 * (uint32_t)array_count;
                case SG_UNIFORMTYPE_FLOAT4:
                case SG_UNIFORMTYPE_INT4:
                    return 16 * (uint32_t)array_count;
                case SG_UNIFORMTYPE_MAT4:
                    return 64 * (uint32_t)array_count;
                default:
                    SOKOL_UNREACHABLE;
                    return 0;
            }
        } else {
            switch (type) {
                case SG_UNIFORMTYPE_FLOAT:
                case SG_UNIFORMTYPE_FLOAT2:
                case SG_UNIFORMTYPE_FLOAT3:
                case SG_UNIFORMTYPE_FLOAT4:
                case SG_UNIFORMTYPE_INT:
                case SG_UNIFORMTYPE_INT2:
                case SG_UNIFORMTYPE_INT3:
                case SG_UNIFORMTYPE_INT4:
                    return 16 * (uint32_t)array_count;
                case SG_UNIFORMTYPE_MAT4:
                    return 64 * (uint32_t)array_count;
                default:
                    SOKOL_UNREACHABLE;
                    return 0;
            }
        }
    }
}

_SOKOL_PRIVATE bool _sg_is_compressed_pixel_format(sg_pixel_format fmt) {
    switch (fmt) {
        case SG_PIXELFORMAT_BC1_RGBA:
        case SG_PIXELFORMAT_BC2_RGBA:
        case SG_PIXELFORMAT_BC3_RGBA:
        case SG_PIXELFORMAT_BC3_SRGBA:
        case SG_PIXELFORMAT_BC4_R:
        case SG_PIXELFORMAT_BC4_RSN:
        case SG_PIXELFORMAT_BC5_RG:
        case SG_PIXELFORMAT_BC5_RGSN:
        case SG_PIXELFORMAT_BC6H_RGBF:
        case SG_PIXELFORMAT_BC6H_RGBUF:
        case SG_PIXELFORMAT_BC7_RGBA:
        case SG_PIXELFORMAT_BC7_SRGBA:
        case SG_PIXELFORMAT_ETC2_RGB8:
        case SG_PIXELFORMAT_ETC2_SRGB8:
        case SG_PIXELFORMAT_ETC2_RGB8A1:
        case SG_PIXELFORMAT_ETC2_RGBA8:
        case SG_PIXELFORMAT_ETC2_SRGB8A8:
        case SG_PIXELFORMAT_EAC_R11:
        case SG_PIXELFORMAT_EAC_R11SN:
        case SG_PIXELFORMAT_EAC_RG11:
        case SG_PIXELFORMAT_EAC_RG11SN:
        case SG_PIXELFORMAT_ASTC_4x4_RGBA:
        case SG_PIXELFORMAT_ASTC_4x4_SRGBA:
            return true;
        default:
            return false;
    }
}

_SOKOL_PRIVATE bool _sg_is_valid_attachment_color_format(sg_pixel_format fmt) {
    const int fmt_index = (int) fmt;
    SOKOL_ASSERT((fmt_index >= 0) && (fmt_index < _SG_PIXELFORMAT_NUM));
    return _sg.formats[fmt_index].render && !_sg.formats[fmt_index].depth;
}

_SOKOL_PRIVATE bool _sg_is_valid_attachment_depth_format(sg_pixel_format fmt) {
    const int fmt_index = (int) fmt;
    SOKOL_ASSERT((fmt_index >= 0) && (fmt_index < _SG_PIXELFORMAT_NUM));
    return _sg.formats[fmt_index].render && _sg.formats[fmt_index].depth;
}

_SOKOL_PRIVATE bool _sg_is_valid_storage_image_format(sg_pixel_format fmt) {
    const int fmt_index = (int) fmt;
    SOKOL_ASSERT((fmt_index >= 0) && (fmt_index < _SG_PIXELFORMAT_NUM));
    return _sg.formats[fmt_index].read || _sg.formats[fmt_index].write;
}

_SOKOL_PRIVATE bool _sg_is_depth_or_depth_stencil_format(sg_pixel_format fmt) {
    return (SG_PIXELFORMAT_DEPTH == fmt) || (SG_PIXELFORMAT_DEPTH_STENCIL == fmt);
}

_SOKOL_PRIVATE bool _sg_is_depth_stencil_format(sg_pixel_format fmt) {
    return (SG_PIXELFORMAT_DEPTH_STENCIL == fmt);
}

_SOKOL_PRIVATE int _sg_pixelformat_bytesize(sg_pixel_format fmt) {
    switch (fmt) {
        case SG_PIXELFORMAT_R8:
        case SG_PIXELFORMAT_R8SN:
        case SG_PIXELFORMAT_R8UI:
        case SG_PIXELFORMAT_R8SI:
            return 1;
        case SG_PIXELFORMAT_R16:
        case SG_PIXELFORMAT_R16SN:
        case SG_PIXELFORMAT_R16UI:
        case SG_PIXELFORMAT_R16SI:
        case SG_PIXELFORMAT_R16F:
        case SG_PIXELFORMAT_RG8:
        case SG_PIXELFORMAT_RG8SN:
        case SG_PIXELFORMAT_RG8UI:
        case SG_PIXELFORMAT_RG8SI:
            return 2;
        case SG_PIXELFORMAT_R32UI:
        case SG_PIXELFORMAT_R32SI:
        case SG_PIXELFORMAT_R32F:
        case SG_PIXELFORMAT_RG16:
        case SG_PIXELFORMAT_RG16SN:
        case SG_PIXELFORMAT_RG16UI:
        case SG_PIXELFORMAT_RG16SI:
        case SG_PIXELFORMAT_RG16F:
        case SG_PIXELFORMAT_RGBA8:
        case SG_PIXELFORMAT_SRGB8A8:
        case SG_PIXELFORMAT_RGBA8SN:
        case SG_PIXELFORMAT_RGBA8UI:
        case SG_PIXELFORMAT_RGBA8SI:
        case SG_PIXELFORMAT_BGRA8:
        case SG_PIXELFORMAT_RGB10A2:
        case SG_PIXELFORMAT_RG11B10F:
        case SG_PIXELFORMAT_RGB9E5:
            return 4;
        case SG_PIXELFORMAT_RG32UI:
        case SG_PIXELFORMAT_RG32SI:
        case SG_PIXELFORMAT_RG32F:
        case SG_PIXELFORMAT_RGBA16:
        case SG_PIXELFORMAT_RGBA16SN:
        case SG_PIXELFORMAT_RGBA16UI:
        case SG_PIXELFORMAT_RGBA16SI:
        case SG_PIXELFORMAT_RGBA16F:
            return 8;
        case SG_PIXELFORMAT_RGBA32UI:
        case SG_PIXELFORMAT_RGBA32SI:
        case SG_PIXELFORMAT_RGBA32F:
            return 16;
        case SG_PIXELFORMAT_DEPTH:
        case SG_PIXELFORMAT_DEPTH_STENCIL:
            return 4;
        default:
            SOKOL_UNREACHABLE;
            return 0;
    }
}

_SOKOL_PRIVATE int _sg_roundup(int val, int round_to) {
    return (val+(round_to-1)) & ~(round_to-1);
}

_SOKOL_PRIVATE uint32_t _sg_roundup_u32(uint32_t val, uint32_t round_to) {
    return (val+(round_to-1)) & ~(round_to-1);
}

_SOKOL_PRIVATE uint64_t _sg_roundup_u64(uint64_t val, uint64_t round_to) {
    return (val+(round_to-1)) & ~(round_to-1);
}

_SOKOL_PRIVATE bool _sg_multiple_u64(uint64_t val, uint64_t of) {
    return (val & (of-1)) == 0;
}

/* return row pitch for an image

    see ComputePitch in https://github.com/microsoft/DirectXTex/blob/master/DirectXTex/DirectXTexUtil.cpp
*/
_SOKOL_PRIVATE int _sg_row_pitch(sg_pixel_format fmt, int width, int row_align) {
    int pitch;
    switch (fmt) {
        case SG_PIXELFORMAT_BC1_RGBA:
        case SG_PIXELFORMAT_BC4_R:
        case SG_PIXELFORMAT_BC4_RSN:
        case SG_PIXELFORMAT_ETC2_RGB8:
        case SG_PIXELFORMAT_ETC2_SRGB8:
        case SG_PIXELFORMAT_ETC2_RGB8A1:
        case SG_PIXELFORMAT_EAC_R11:
        case SG_PIXELFORMAT_EAC_R11SN:
            pitch = ((width + 3) / 4) * 8;
            pitch = pitch < 8 ? 8 : pitch;
            break;
        case SG_PIXELFORMAT_BC2_RGBA:
        case SG_PIXELFORMAT_BC3_RGBA:
        case SG_PIXELFORMAT_BC3_SRGBA:
        case SG_PIXELFORMAT_BC5_RG:
        case SG_PIXELFORMAT_BC5_RGSN:
        case SG_PIXELFORMAT_BC6H_RGBF:
        case SG_PIXELFORMAT_BC6H_RGBUF:
        case SG_PIXELFORMAT_BC7_RGBA:
        case SG_PIXELFORMAT_BC7_SRGBA:
        case SG_PIXELFORMAT_ETC2_RGBA8:
        case SG_PIXELFORMAT_ETC2_SRGB8A8:
        case SG_PIXELFORMAT_EAC_RG11:
        case SG_PIXELFORMAT_EAC_RG11SN:
        case SG_PIXELFORMAT_ASTC_4x4_RGBA:
        case SG_PIXELFORMAT_ASTC_4x4_SRGBA:
            pitch = ((width + 3) / 4) * 16;
            pitch = pitch < 16 ? 16 : pitch;
            break;
        default:
            pitch = width * _sg_pixelformat_bytesize(fmt);
            break;
    }
    pitch = _sg_roundup(pitch, row_align);
    return pitch;
}

// compute the number of rows in a surface depending on pixel format
_SOKOL_PRIVATE int _sg_num_rows(sg_pixel_format fmt, int height) {
    int num_rows;
    switch (fmt) {
        case SG_PIXELFORMAT_BC1_RGBA:
        case SG_PIXELFORMAT_BC4_R:
        case SG_PIXELFORMAT_BC4_RSN:
        case SG_PIXELFORMAT_ETC2_RGB8:
        case SG_PIXELFORMAT_ETC2_SRGB8:
        case SG_PIXELFORMAT_ETC2_RGB8A1:
        case SG_PIXELFORMAT_ETC2_RGBA8:
        case SG_PIXELFORMAT_ETC2_SRGB8A8:
        case SG_PIXELFORMAT_EAC_R11:
        case SG_PIXELFORMAT_EAC_R11SN:
        case SG_PIXELFORMAT_EAC_RG11:
        case SG_PIXELFORMAT_EAC_RG11SN:
        case SG_PIXELFORMAT_BC2_RGBA:
        case SG_PIXELFORMAT_BC3_RGBA:
        case SG_PIXELFORMAT_BC3_SRGBA:
        case SG_PIXELFORMAT_BC5_RG:
        case SG_PIXELFORMAT_BC5_RGSN:
        case SG_PIXELFORMAT_BC6H_RGBF:
        case SG_PIXELFORMAT_BC6H_RGBUF:
        case SG_PIXELFORMAT_BC7_RGBA:
        case SG_PIXELFORMAT_BC7_SRGBA:
        case SG_PIXELFORMAT_ASTC_4x4_RGBA:
        case SG_PIXELFORMAT_ASTC_4x4_SRGBA:
            num_rows = ((height + 3) / 4);
            break;
        default:
            num_rows = height;
            break;
    }
    if (num_rows < 1) {
        num_rows = 1;
    }
    return num_rows;
}

/* return pitch of a 2D subimage / texture slice
    see ComputePitch in https://github.com/microsoft/DirectXTex/blob/master/DirectXTex/DirectXTexUtil.cpp
*/
_SOKOL_PRIVATE int _sg_surface_pitch(sg_pixel_format fmt, int width, int height, int row_align) {
    int num_rows = _sg_num_rows(fmt, height);
    return num_rows * _sg_row_pitch(fmt, width, row_align);
}

// capability table pixel format helper functions
_SOKOL_PRIVATE void _sg_pixelformat_all(_sg_pixelformat_info_t* pfi) {
    pfi->sample = true;
    pfi->filter = true;
    pfi->blend = true;
    pfi->render = true;
    pfi->msaa = true;
}

_SOKOL_PRIVATE void _sg_pixelformat_s(_sg_pixelformat_info_t* pfi) {
    pfi->sample = true;
}

_SOKOL_PRIVATE void _sg_pixelformat_sf(_sg_pixelformat_info_t* pfi) {
    pfi->sample = true;
    pfi->filter = true;
}

_SOKOL_PRIVATE void _sg_pixelformat_sr(_sg_pixelformat_info_t* pfi) {
    pfi->sample = true;
    pfi->render = true;
}

_SOKOL_PRIVATE void _sg_pixelformat_sfr(_sg_pixelformat_info_t* pfi) {
    pfi->sample = true;
    pfi->filter = true;
    pfi->render = true;
}

_SOKOL_PRIVATE void _sg_pixelformat_srmd(_sg_pixelformat_info_t* pfi) {
    pfi->sample = true;
    pfi->render = true;
    pfi->msaa = true;
    pfi->depth = true;
}

_SOKOL_PRIVATE void _sg_pixelformat_srm(_sg_pixelformat_info_t* pfi) {
    pfi->sample = true;
    pfi->render = true;
    pfi->msaa = true;
}

_SOKOL_PRIVATE void _sg_pixelformat_sfrm(_sg_pixelformat_info_t* pfi) {
    pfi->sample = true;
    pfi->filter = true;
    pfi->render = true;
    pfi->msaa = true;
}
_SOKOL_PRIVATE void _sg_pixelformat_sbrm(_sg_pixelformat_info_t* pfi) {
    pfi->sample = true;
    pfi->blend = true;
    pfi->render = true;
    pfi->msaa = true;
}

_SOKOL_PRIVATE void _sg_pixelformat_sbr(_sg_pixelformat_info_t* pfi) {
    pfi->sample = true;
    pfi->blend = true;
    pfi->render = true;
}

_SOKOL_PRIVATE void _sg_pixelformat_sfbr(_sg_pixelformat_info_t* pfi) {
    pfi->sample = true;
    pfi->filter = true;
    pfi->blend = true;
    pfi->render = true;
}

_SOKOL_PRIVATE void _sg_pixelformat_compute_all(_sg_pixelformat_info_t* pfi) {
    pfi->read = true;
    pfi->write = true;
}

_SOKOL_PRIVATE void _sg_pixelformat_compute_writeonly(_sg_pixelformat_info_t* pfi) {
    pfi->read = false;
    pfi->write = true;
}

_SOKOL_PRIVATE sg_pass_action _sg_pass_action_defaults(const sg_pass_action* action) {
    SOKOL_ASSERT(action);
    sg_pass_action res = *action;
    for (int i = 0; i < SG_MAX_COLOR_ATTACHMENTS; i++) {
        if (res.colors[i].load_action == _SG_LOADACTION_DEFAULT) {
            res.colors[i].load_action = SG_LOADACTION_CLEAR;
            res.colors[i].clear_value.r = SG_DEFAULT_CLEAR_RED;
            res.colors[i].clear_value.g = SG_DEFAULT_CLEAR_GREEN;
            res.colors[i].clear_value.b = SG_DEFAULT_CLEAR_BLUE;
            res.colors[i].clear_value.a = SG_DEFAULT_CLEAR_ALPHA;
        }
        if (res.colors[i].store_action == _SG_STOREACTION_DEFAULT) {
            res.colors[i].store_action = SG_STOREACTION_STORE;
        }
    }
    if (res.depth.load_action == _SG_LOADACTION_DEFAULT) {
        res.depth.load_action = SG_LOADACTION_CLEAR;
        res.depth.clear_value = SG_DEFAULT_CLEAR_DEPTH;
    }
    if (res.depth.store_action == _SG_STOREACTION_DEFAULT) {
        res.depth.store_action = SG_STOREACTION_DONTCARE;
    }
    if (res.stencil.load_action == _SG_LOADACTION_DEFAULT) {
        res.stencil.load_action = SG_LOADACTION_CLEAR;
        res.stencil.clear_value = SG_DEFAULT_CLEAR_STENCIL;
    }
    if (res.stencil.store_action == _SG_STOREACTION_DEFAULT) {
        res.stencil.store_action = SG_STOREACTION_DONTCARE;
    }
    return res;
}

// ██████  ██    ██ ███    ███ ███    ███ ██    ██     ██████   █████   ██████ ██   ██ ███████ ███    ██ ██████
// ██   ██ ██    ██ ████  ████ ████  ████  ██  ██      ██   ██ ██   ██ ██      ██  ██  ██      ████   ██ ██   ██
// ██   ██ ██    ██ ██ ████ ██ ██ ████ ██   ████       ██████  ███████ ██      █████   █████   ██ ██  ██ ██   ██
// ██   ██ ██    ██ ██  ██  ██ ██  ██  ██    ██        ██   ██ ██   ██ ██      ██  ██  ██      ██  ██ ██ ██   ██
// ██████   ██████  ██      ██ ██      ██    ██        ██████  ██   ██  ██████ ██   ██ ███████ ██   ████ ██████
//
// >>dummy backend
#if defined(SOKOL_DUMMY_BACKEND)

_SOKOL_PRIVATE void _sg_dummy_setup_backend(const sg_desc* desc) {
    SOKOL_ASSERT(desc);
    _SOKOL_UNUSED(desc);
    _sg.backend = SG_BACKEND_DUMMY;
    for (int i = SG_PIXELFORMAT_R8; i < SG_PIXELFORMAT_BC1_RGBA; i++) {
        _sg.formats[i].sample = true;
        _sg.formats[i].filter = true;
        _sg.formats[i].render = true;
        _sg.formats[i].blend = true;
        _sg.formats[i].msaa = true;
    }
    _sg.formats[SG_PIXELFORMAT_DEPTH].depth = true;
    _sg.formats[SG_PIXELFORMAT_DEPTH_STENCIL].depth = true;
}

_SOKOL_PRIVATE void _sg_dummy_discard_backend(void) {
    // empty
}

_SOKOL_PRIVATE void _sg_dummy_reset_state_cache(void) {
    // empty
}

_SOKOL_PRIVATE sg_resource_state _sg_dummy_create_buffer(_sg_buffer_t* buf, const sg_buffer_desc* desc) {
    SOKOL_ASSERT(buf && desc);
    _SOKOL_UNUSED(buf);
    _SOKOL_UNUSED(desc);
    return SG_RESOURCESTATE_VALID;
}

_SOKOL_PRIVATE void _sg_dummy_discard_buffer(_sg_buffer_t* buf) {
    SOKOL_ASSERT(buf);
    _SOKOL_UNUSED(buf);
}

_SOKOL_PRIVATE sg_resource_state _sg_dummy_create_image(_sg_image_t* img, const sg_image_desc* desc) {
    SOKOL_ASSERT(img && desc);
    _SOKOL_UNUSED(img);
    _SOKOL_UNUSED(desc);
    return SG_RESOURCESTATE_VALID;
}

_SOKOL_PRIVATE void _sg_dummy_discard_image(_sg_image_t* img) {
    SOKOL_ASSERT(img);
    _SOKOL_UNUSED(img);
}

_SOKOL_PRIVATE sg_resource_state _sg_dummy_create_sampler(_sg_sampler_t* smp, const sg_sampler_desc* desc) {
    SOKOL_ASSERT(smp && desc);
    _SOKOL_UNUSED(smp);
    _SOKOL_UNUSED(desc);
    return SG_RESOURCESTATE_VALID;
}

_SOKOL_PRIVATE void _sg_dummy_discard_sampler(_sg_sampler_t* smp) {
    SOKOL_ASSERT(smp);
    _SOKOL_UNUSED(smp);
}

_SOKOL_PRIVATE sg_resource_state _sg_dummy_create_shader(_sg_shader_t* shd, const sg_shader_desc* desc) {
    SOKOL_ASSERT(shd && desc);
    _SOKOL_UNUSED(shd);
    _SOKOL_UNUSED(desc);
    return SG_RESOURCESTATE_VALID;
}

_SOKOL_PRIVATE void _sg_dummy_discard_shader(_sg_shader_t* shd) {
    SOKOL_ASSERT(shd);
    _SOKOL_UNUSED(shd);
}

_SOKOL_PRIVATE sg_resource_state _sg_dummy_create_pipeline(_sg_pipeline_t* pip, const sg_pipeline_desc* desc) {
    SOKOL_ASSERT(pip && desc);
    _SOKOL_UNUSED(pip);
    _SOKOL_UNUSED(desc);
    return SG_RESOURCESTATE_VALID;
}

_SOKOL_PRIVATE void _sg_dummy_discard_pipeline(_sg_pipeline_t* pip) {
    SOKOL_ASSERT(pip);
    _SOKOL_UNUSED(pip);
}

_SOKOL_PRIVATE sg_resource_state _sg_dummy_create_view(_sg_view_t* view, const sg_view_desc* desc) {
    SOKOL_ASSERT(view && desc);
    _SOKOL_UNUSED(view);
    _SOKOL_UNUSED(desc);
    return SG_RESOURCESTATE_VALID;
}

_SOKOL_PRIVATE void _sg_dummy_discard_view(_sg_view_t* view) {
    SOKOL_ASSERT(view);
    _SOKOL_UNUSED(view);
}

_SOKOL_PRIVATE void _sg_dummy_begin_pass(const sg_pass* pass, const _sg_attachments_ptrs_t* atts) {
    SOKOL_ASSERT(pass && atts);
    _SOKOL_UNUSED(pass);
    _SOKOL_UNUSED(atts);
}

_SOKOL_PRIVATE void _sg_dummy_end_pass(const _sg_attachments_ptrs_t* atts) {
    SOKOL_ASSERT(atts);
    _SOKOL_UNUSED(atts);
}

_SOKOL_PRIVATE void _sg_dummy_commit(void) {
    // empty
}

_SOKOL_PRIVATE void _sg_dummy_apply_viewport(int x, int y, int w, int h, bool origin_top_left) {
    _SOKOL_UNUSED(x);
    _SOKOL_UNUSED(y);
    _SOKOL_UNUSED(w);
    _SOKOL_UNUSED(h);
    _SOKOL_UNUSED(origin_top_left);
}

_SOKOL_PRIVATE void _sg_dummy_apply_scissor_rect(int x, int y, int w, int h, bool origin_top_left) {
    _SOKOL_UNUSED(x);
    _SOKOL_UNUSED(y);
    _SOKOL_UNUSED(w);
    _SOKOL_UNUSED(h);
    _SOKOL_UNUSED(origin_top_left);
}

_SOKOL_PRIVATE void _sg_dummy_apply_pipeline(_sg_pipeline_t* pip) {
    SOKOL_ASSERT(pip);
    _SOKOL_UNUSED(pip);
}

_SOKOL_PRIVATE bool _sg_dummy_apply_bindings(_sg_bindings_ptrs_t* bnd) {
    SOKOL_ASSERT(bnd);
    SOKOL_ASSERT(bnd->pip);
    _SOKOL_UNUSED(bnd);
    return true;
}

_SOKOL_PRIVATE void _sg_dummy_apply_uniforms(int ub_slot, const sg_range* data) {
    _SOKOL_UNUSED(ub_slot);
    _SOKOL_UNUSED(data);
}

_SOKOL_PRIVATE void _sg_dummy_draw(int base_element, int num_elements, int num_instances) {
    _SOKOL_UNUSED(base_element);
    _SOKOL_UNUSED(num_elements);
    _SOKOL_UNUSED(num_instances);
}

_SOKOL_PRIVATE void _sg_dummy_dispatch(int num_groups_x, int num_groups_y, int num_groups_z) {
    _SOKOL_UNUSED(num_groups_x);
    _SOKOL_UNUSED(num_groups_y);
    _SOKOL_UNUSED(num_groups_z);
}

_SOKOL_PRIVATE void _sg_dummy_update_buffer(_sg_buffer_t* buf, const sg_range* data) {
    SOKOL_ASSERT(buf && data && data->ptr && (data->size > 0));
    _SOKOL_UNUSED(data);
    if (++buf->cmn.active_slot >= buf->cmn.num_slots) {
        buf->cmn.active_slot = 0;
    }
}

_SOKOL_PRIVATE bool _sg_dummy_append_buffer(_sg_buffer_t* buf, const sg_range* data, bool new_frame) {
    SOKOL_ASSERT(buf && data && data->ptr && (data->size > 0));
    _SOKOL_UNUSED(data);
    if (new_frame) {
        if (++buf->cmn.active_slot >= buf->cmn.num_slots) {
            buf->cmn.active_slot = 0;
        }
    }
    return true;
}

_SOKOL_PRIVATE void _sg_dummy_update_image(_sg_image_t* img, const sg_image_data* data) {
    SOKOL_ASSERT(img && data);
    _SOKOL_UNUSED(data);
    if (++img->cmn.active_slot >= img->cmn.num_slots) {
        img->cmn.active_slot = 0;
    }
}

//  ██████  ██████  ███████ ███    ██  ██████  ██          ██████   █████   ██████ ██   ██ ███████ ███    ██ ██████
// ██    ██ ██   ██ ██      ████   ██ ██       ██          ██   ██ ██   ██ ██      ██  ██  ██      ████   ██ ██   ██
// ██    ██ ██████  █████   ██ ██  ██ ██   ███ ██          ██████  ███████ ██      █████   █████   ██ ██  ██ ██   ██
// ██    ██ ██      ██      ██  ██ ██ ██    ██ ██          ██   ██ ██   ██ ██      ██  ██  ██      ██  ██ ██ ██   ██
//  ██████  ██      ███████ ██   ████  ██████  ███████     ██████  ██   ██  ██████ ██   ██ ███████ ██   ████ ██████
//
// >>metal backend
#elif defined(SOKOL_METAL)

#if __has_feature(objc_arc)
#define _SG_OBJC_RETAIN(obj) { }
#define _SG_OBJC_RELEASE(obj) { obj = nil; }
#else
#define _SG_OBJC_RETAIN(obj) { [obj retain]; }
#define _SG_OBJC_RELEASE(obj) { [obj release]; obj = nil; }
#endif

//-- enum translation functions ------------------------------------------------
_SOKOL_PRIVATE MTLLoadAction _sg_mtl_load_action(sg_load_action a) {
    switch (a) {
        case SG_LOADACTION_CLEAR:       return MTLLoadActionClear;
        case SG_LOADACTION_LOAD:        return MTLLoadActionLoad;
        case SG_LOADACTION_DONTCARE:    return MTLLoadActionDontCare;
        default: SOKOL_UNREACHABLE;     return (MTLLoadAction)0;
    }
}

_SOKOL_PRIVATE MTLStoreAction _sg_mtl_store_action(sg_store_action a, bool resolve) {
    switch (a) {
        case SG_STOREACTION_STORE:
            if (resolve) {
                return MTLStoreActionStoreAndMultisampleResolve;
            } else {
                return MTLStoreActionStore;
            }
            break;
        case SG_STOREACTION_DONTCARE:
            if (resolve) {
                return MTLStoreActionMultisampleResolve;
            } else {
                return MTLStoreActionDontCare;
            }
            break;
        default: SOKOL_UNREACHABLE; return (MTLStoreAction)0;
    }
}

_SOKOL_PRIVATE MTLResourceOptions _sg_mtl_resource_options_storage_mode_managed_or_shared(void) {
    #if defined(_SG_TARGET_MACOS)
    if (_sg.mtl.use_shared_storage_mode) {
        return MTLResourceStorageModeShared;
    } else {
        return MTLResourceStorageModeManaged;
    }
    #else
        // MTLResourceStorageModeManaged is not even defined on iOS SDK
        return MTLResourceStorageModeShared;
    #endif
}

_SOKOL_PRIVATE MTLResourceOptions _sg_mtl_buffer_resource_options(const sg_buffer_usage* usage) {
    if (usage->immutable) {
        return _sg_mtl_resource_options_storage_mode_managed_or_shared();
    } else {
        return MTLResourceCPUCacheModeWriteCombined | _sg_mtl_resource_options_storage_mode_managed_or_shared();
    }
}

_SOKOL_PRIVATE MTLVertexStepFunction _sg_mtl_step_function(sg_vertex_step step) {
    switch (step) {
        case SG_VERTEXSTEP_PER_VERTEX:      return MTLVertexStepFunctionPerVertex;
        case SG_VERTEXSTEP_PER_INSTANCE:    return MTLVertexStepFunctionPerInstance;
        default: SOKOL_UNREACHABLE; return (MTLVertexStepFunction)0;
    }
}

_SOKOL_PRIVATE MTLVertexFormat _sg_mtl_vertex_format(sg_vertex_format fmt) {
    switch (fmt) {
        case SG_VERTEXFORMAT_FLOAT:     return MTLVertexFormatFloat;
        case SG_VERTEXFORMAT_FLOAT2:    return MTLVertexFormatFloat2;
        case SG_VERTEXFORMAT_FLOAT3:    return MTLVertexFormatFloat3;
        case SG_VERTEXFORMAT_FLOAT4:    return MTLVertexFormatFloat4;
        case SG_VERTEXFORMAT_INT:       return MTLVertexFormatInt;
        case SG_VERTEXFORMAT_INT2:      return MTLVertexFormatInt2;
        case SG_VERTEXFORMAT_INT3:      return MTLVertexFormatInt3;
        case SG_VERTEXFORMAT_INT4:      return MTLVertexFormatInt4;
        case SG_VERTEXFORMAT_UINT:      return MTLVertexFormatUInt;
        case SG_VERTEXFORMAT_UINT2:     return MTLVertexFormatUInt2;
        case SG_VERTEXFORMAT_UINT3:     return MTLVertexFormatUInt3;
        case SG_VERTEXFORMAT_UINT4:     return MTLVertexFormatUInt4;
        case SG_VERTEXFORMAT_BYTE4:     return MTLVertexFormatChar4;
        case SG_VERTEXFORMAT_BYTE4N:    return MTLVertexFormatChar4Normalized;
        case SG_VERTEXFORMAT_UBYTE4:    return MTLVertexFormatUChar4;
        case SG_VERTEXFORMAT_UBYTE4N:   return MTLVertexFormatUChar4Normalized;
        case SG_VERTEXFORMAT_SHORT2:    return MTLVertexFormatShort2;
        case SG_VERTEXFORMAT_SHORT2N:   return MTLVertexFormatShort2Normalized;
        case SG_VERTEXFORMAT_USHORT2:   return MTLVertexFormatUShort2;
        case SG_VERTEXFORMAT_USHORT2N:  return MTLVertexFormatUShort2Normalized;
        case SG_VERTEXFORMAT_SHORT4:    return MTLVertexFormatShort4;
        case SG_VERTEXFORMAT_SHORT4N:   return MTLVertexFormatShort4Normalized;
        case SG_VERTEXFORMAT_USHORT4:   return MTLVertexFormatUShort4;
        case SG_VERTEXFORMAT_USHORT4N:  return MTLVertexFormatUShort4Normalized;
        case SG_VERTEXFORMAT_UINT10_N2: return MTLVertexFormatUInt1010102Normalized;
        case SG_VERTEXFORMAT_HALF2:     return MTLVertexFormatHalf2;
        case SG_VERTEXFORMAT_HALF4:     return MTLVertexFormatHalf4;
        default: SOKOL_UNREACHABLE; return (MTLVertexFormat)0;
    }
}

_SOKOL_PRIVATE MTLPrimitiveType _sg_mtl_primitive_type(sg_primitive_type t) {
    switch (t) {
        case SG_PRIMITIVETYPE_POINTS:           return MTLPrimitiveTypePoint;
        case SG_PRIMITIVETYPE_LINES:            return MTLPrimitiveTypeLine;
        case SG_PRIMITIVETYPE_LINE_STRIP:       return MTLPrimitiveTypeLineStrip;
        case SG_PRIMITIVETYPE_TRIANGLES:        return MTLPrimitiveTypeTriangle;
        case SG_PRIMITIVETYPE_TRIANGLE_STRIP:   return MTLPrimitiveTypeTriangleStrip;
        default: SOKOL_UNREACHABLE; return (MTLPrimitiveType)0;
    }
}

_SOKOL_PRIVATE MTLPixelFormat _sg_mtl_pixel_format(sg_pixel_format fmt) {
    switch (fmt) {
        case SG_PIXELFORMAT_R8:                     return MTLPixelFormatR8Unorm;
        case SG_PIXELFORMAT_R8SN:                   return MTLPixelFormatR8Snorm;
        case SG_PIXELFORMAT_R8UI:                   return MTLPixelFormatR8Uint;
        case SG_PIXELFORMAT_R8SI:                   return MTLPixelFormatR8Sint;
        case SG_PIXELFORMAT_R16:                    return MTLPixelFormatR16Unorm;
        case SG_PIXELFORMAT_R16SN:                  return MTLPixelFormatR16Snorm;
        case SG_PIXELFORMAT_R16UI:                  return MTLPixelFormatR16Uint;
        case SG_PIXELFORMAT_R16SI:                  return MTLPixelFormatR16Sint;
        case SG_PIXELFORMAT_R16F:                   return MTLPixelFormatR16Float;
        case SG_PIXELFORMAT_RG8:                    return MTLPixelFormatRG8Unorm;
        case SG_PIXELFORMAT_RG8SN:                  return MTLPixelFormatRG8Snorm;
        case SG_PIXELFORMAT_RG8UI:                  return MTLPixelFormatRG8Uint;
        case SG_PIXELFORMAT_RG8SI:                  return MTLPixelFormatRG8Sint;
        case SG_PIXELFORMAT_R32UI:                  return MTLPixelFormatR32Uint;
        case SG_PIXELFORMAT_R32SI:                  return MTLPixelFormatR32Sint;
        case SG_PIXELFORMAT_R32F:                   return MTLPixelFormatR32Float;
        case SG_PIXELFORMAT_RG16:                   return MTLPixelFormatRG16Unorm;
        case SG_PIXELFORMAT_RG16SN:                 return MTLPixelFormatRG16Snorm;
        case SG_PIXELFORMAT_RG16UI:                 return MTLPixelFormatRG16Uint;
        case SG_PIXELFORMAT_RG16SI:                 return MTLPixelFormatRG16Sint;
        case SG_PIXELFORMAT_RG16F:                  return MTLPixelFormatRG16Float;
        case SG_PIXELFORMAT_RGBA8:                  return MTLPixelFormatRGBA8Unorm;
        case SG_PIXELFORMAT_SRGB8A8:                return MTLPixelFormatRGBA8Unorm_sRGB;
        case SG_PIXELFORMAT_RGBA8SN:                return MTLPixelFormatRGBA8Snorm;
        case SG_PIXELFORMAT_RGBA8UI:                return MTLPixelFormatRGBA8Uint;
        case SG_PIXELFORMAT_RGBA8SI:                return MTLPixelFormatRGBA8Sint;
        case SG_PIXELFORMAT_BGRA8:                  return MTLPixelFormatBGRA8Unorm;
        case SG_PIXELFORMAT_RGB10A2:                return MTLPixelFormatRGB10A2Unorm;
        case SG_PIXELFORMAT_RG11B10F:               return MTLPixelFormatRG11B10Float;
        case SG_PIXELFORMAT_RGB9E5:                 return MTLPixelFormatRGB9E5Float;
        case SG_PIXELFORMAT_RG32UI:                 return MTLPixelFormatRG32Uint;
        case SG_PIXELFORMAT_RG32SI:                 return MTLPixelFormatRG32Sint;
        case SG_PIXELFORMAT_RG32F:                  return MTLPixelFormatRG32Float;
        case SG_PIXELFORMAT_RGBA16:                 return MTLPixelFormatRGBA16Unorm;
        case SG_PIXELFORMAT_RGBA16SN:               return MTLPixelFormatRGBA16Snorm;
        case SG_PIXELFORMAT_RGBA16UI:               return MTLPixelFormatRGBA16Uint;
        case SG_PIXELFORMAT_RGBA16SI:               return MTLPixelFormatRGBA16Sint;
        case SG_PIXELFORMAT_RGBA16F:                return MTLPixelFormatRGBA16Float;
        case SG_PIXELFORMAT_RGBA32UI:               return MTLPixelFormatRGBA32Uint;
        case SG_PIXELFORMAT_RGBA32SI:               return MTLPixelFormatRGBA32Sint;
        case SG_PIXELFORMAT_RGBA32F:                return MTLPixelFormatRGBA32Float;
        case SG_PIXELFORMAT_DEPTH:                  return MTLPixelFormatDepth32Float;
        case SG_PIXELFORMAT_DEPTH_STENCIL:          return MTLPixelFormatDepth32Float_Stencil8;
        #if defined(_SG_TARGET_MACOS)
        case SG_PIXELFORMAT_BC1_RGBA:               return MTLPixelFormatBC1_RGBA;
        case SG_PIXELFORMAT_BC2_RGBA:               return MTLPixelFormatBC2_RGBA;
        case SG_PIXELFORMAT_BC3_RGBA:               return MTLPixelFormatBC3_RGBA;
        case SG_PIXELFORMAT_BC3_SRGBA:              return MTLPixelFormatBC3_RGBA_sRGB;
        case SG_PIXELFORMAT_BC4_R:                  return MTLPixelFormatBC4_RUnorm;
        case SG_PIXELFORMAT_BC4_RSN:                return MTLPixelFormatBC4_RSnorm;
        case SG_PIXELFORMAT_BC5_RG:                 return MTLPixelFormatBC5_RGUnorm;
        case SG_PIXELFORMAT_BC5_RGSN:               return MTLPixelFormatBC5_RGSnorm;
        case SG_PIXELFORMAT_BC6H_RGBF:              return MTLPixelFormatBC6H_RGBFloat;
        case SG_PIXELFORMAT_BC6H_RGBUF:             return MTLPixelFormatBC6H_RGBUfloat;
        case SG_PIXELFORMAT_BC7_RGBA:               return MTLPixelFormatBC7_RGBAUnorm;
        case SG_PIXELFORMAT_BC7_SRGBA:              return MTLPixelFormatBC7_RGBAUnorm_sRGB;
        #else
        case SG_PIXELFORMAT_ETC2_RGB8:              return MTLPixelFormatETC2_RGB8;
        case SG_PIXELFORMAT_ETC2_SRGB8:             return MTLPixelFormatETC2_RGB8_sRGB;
        case SG_PIXELFORMAT_ETC2_RGB8A1:            return MTLPixelFormatETC2_RGB8A1;
        case SG_PIXELFORMAT_ETC2_RGBA8:             return MTLPixelFormatEAC_RGBA8;
        case SG_PIXELFORMAT_ETC2_SRGB8A8:           return MTLPixelFormatEAC_RGBA8_sRGB;
        case SG_PIXELFORMAT_EAC_R11:                return MTLPixelFormatEAC_R11Unorm;
        case SG_PIXELFORMAT_EAC_R11SN:              return MTLPixelFormatEAC_R11Snorm;
        case SG_PIXELFORMAT_EAC_RG11:               return MTLPixelFormatEAC_RG11Unorm;
        case SG_PIXELFORMAT_EAC_RG11SN:             return MTLPixelFormatEAC_RG11Snorm;
        case SG_PIXELFORMAT_ASTC_4x4_RGBA:          return MTLPixelFormatASTC_4x4_LDR;
        case SG_PIXELFORMAT_ASTC_4x4_SRGBA:         return MTLPixelFormatASTC_4x4_sRGB;
        #endif
        default: return MTLPixelFormatInvalid;
    }
}

_SOKOL_PRIVATE MTLColorWriteMask _sg_mtl_color_write_mask(sg_color_mask m) {
    MTLColorWriteMask mtl_mask = MTLColorWriteMaskNone;
    if (m & SG_COLORMASK_R) {
        mtl_mask |= MTLColorWriteMaskRed;
    }
    if (m & SG_COLORMASK_G) {
        mtl_mask |= MTLColorWriteMaskGreen;
    }
    if (m & SG_COLORMASK_B) {
        mtl_mask |= MTLColorWriteMaskBlue;
    }
    if (m & SG_COLORMASK_A) {
        mtl_mask |= MTLColorWriteMaskAlpha;
    }
    return mtl_mask;
}

_SOKOL_PRIVATE MTLBlendOperation _sg_mtl_blend_op(sg_blend_op op) {
    switch (op) {
        case SG_BLENDOP_ADD:                return MTLBlendOperationAdd;
        case SG_BLENDOP_SUBTRACT:           return MTLBlendOperationSubtract;
        case SG_BLENDOP_REVERSE_SUBTRACT:   return MTLBlendOperationReverseSubtract;
        case SG_BLENDOP_MIN:                return MTLBlendOperationMin;
        case SG_BLENDOP_MAX:                return MTLBlendOperationMax;
        default: SOKOL_UNREACHABLE; return (MTLBlendOperation)0;
    }
}

_SOKOL_PRIVATE MTLBlendFactor _sg_mtl_blend_factor(sg_blend_factor f) {
    switch (f) {
        case SG_BLENDFACTOR_ZERO:                   return MTLBlendFactorZero;
        case SG_BLENDFACTOR_ONE:                    return MTLBlendFactorOne;
        case SG_BLENDFACTOR_SRC_COLOR:              return MTLBlendFactorSourceColor;
        case SG_BLENDFACTOR_ONE_MINUS_SRC_COLOR:    return MTLBlendFactorOneMinusSourceColor;
        case SG_BLENDFACTOR_SRC_ALPHA:              return MTLBlendFactorSourceAlpha;
        case SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA:    return MTLBlendFactorOneMinusSourceAlpha;
        case SG_BLENDFACTOR_DST_COLOR:              return MTLBlendFactorDestinationColor;
        case SG_BLENDFACTOR_ONE_MINUS_DST_COLOR:    return MTLBlendFactorOneMinusDestinationColor;
        case SG_BLENDFACTOR_DST_ALPHA:              return MTLBlendFactorDestinationAlpha;
        case SG_BLENDFACTOR_ONE_MINUS_DST_ALPHA:    return MTLBlendFactorOneMinusDestinationAlpha;
        case SG_BLENDFACTOR_SRC_ALPHA_SATURATED:    return MTLBlendFactorSourceAlphaSaturated;
        case SG_BLENDFACTOR_BLEND_COLOR:            return MTLBlendFactorBlendColor;
        case SG_BLENDFACTOR_ONE_MINUS_BLEND_COLOR:  return MTLBlendFactorOneMinusBlendColor;
        case SG_BLENDFACTOR_BLEND_ALPHA:            return MTLBlendFactorBlendAlpha;
        case SG_BLENDFACTOR_ONE_MINUS_BLEND_ALPHA:  return MTLBlendFactorOneMinusBlendAlpha;
        default: SOKOL_UNREACHABLE; return (MTLBlendFactor)0;
    }
}

_SOKOL_PRIVATE MTLCompareFunction _sg_mtl_compare_func(sg_compare_func f) {
    switch (f) {
        case SG_COMPAREFUNC_NEVER:          return MTLCompareFunctionNever;
        case SG_COMPAREFUNC_LESS:           return MTLCompareFunctionLess;
        case SG_COMPAREFUNC_EQUAL:          return MTLCompareFunctionEqual;
        case SG_COMPAREFUNC_LESS_EQUAL:     return MTLCompareFunctionLessEqual;
        case SG_COMPAREFUNC_GREATER:        return MTLCompareFunctionGreater;
        case SG_COMPAREFUNC_NOT_EQUAL:      return MTLCompareFunctionNotEqual;
        case SG_COMPAREFUNC_GREATER_EQUAL:  return MTLCompareFunctionGreaterEqual;
        case SG_COMPAREFUNC_ALWAYS:         return MTLCompareFunctionAlways;
        default: SOKOL_UNREACHABLE; return (MTLCompareFunction)0;
    }
}

_SOKOL_PRIVATE MTLStencilOperation _sg_mtl_stencil_op(sg_stencil_op op) {
    switch (op) {
        case SG_STENCILOP_KEEP:         return MTLStencilOperationKeep;
        case SG_STENCILOP_ZERO:         return MTLStencilOperationZero;
        case SG_STENCILOP_REPLACE:      return MTLStencilOperationReplace;
        case SG_STENCILOP_INCR_CLAMP:   return MTLStencilOperationIncrementClamp;
        case SG_STENCILOP_DECR_CLAMP:   return MTLStencilOperationDecrementClamp;
        case SG_STENCILOP_INVERT:       return MTLStencilOperationInvert;
        case SG_STENCILOP_INCR_WRAP:    return MTLStencilOperationIncrementWrap;
        case SG_STENCILOP_DECR_WRAP:    return MTLStencilOperationDecrementWrap;
        default: SOKOL_UNREACHABLE; return (MTLStencilOperation)0;
    }
}

_SOKOL_PRIVATE MTLCullMode _sg_mtl_cull_mode(sg_cull_mode m) {
    switch (m) {
        case SG_CULLMODE_NONE:  return MTLCullModeNone;
        case SG_CULLMODE_FRONT: return MTLCullModeFront;
        case SG_CULLMODE_BACK:  return MTLCullModeBack;
        default: SOKOL_UNREACHABLE; return (MTLCullMode)0;
    }
}

_SOKOL_PRIVATE MTLWinding _sg_mtl_winding(sg_face_winding w) {
    switch (w) {
        case SG_FACEWINDING_CW:     return MTLWindingClockwise;
        case SG_FACEWINDING_CCW:    return MTLWindingCounterClockwise;
        default: SOKOL_UNREACHABLE; return (MTLWinding)0;
    }
}

_SOKOL_PRIVATE MTLIndexType _sg_mtl_index_type(sg_index_type t) {
    switch (t) {
        case SG_INDEXTYPE_UINT16:   return MTLIndexTypeUInt16;
        case SG_INDEXTYPE_UINT32:   return MTLIndexTypeUInt32;
        default: SOKOL_UNREACHABLE; return (MTLIndexType)0;
    }
}

_SOKOL_PRIVATE int _sg_mtl_index_size(sg_index_type t) {
    switch (t) {
        case SG_INDEXTYPE_NONE:     return 0;
        case SG_INDEXTYPE_UINT16:   return 2;
        case SG_INDEXTYPE_UINT32:   return 4;
        default: SOKOL_UNREACHABLE; return 0;
    }
}

_SOKOL_PRIVATE MTLTextureType _sg_mtl_texture_type(sg_image_type t, bool msaa) {
    switch (t) {
        case SG_IMAGETYPE_2D:       return msaa ? MTLTextureType2DMultisample : MTLTextureType2D;
        case SG_IMAGETYPE_CUBE:     return MTLTextureTypeCube;
        case SG_IMAGETYPE_3D:       return MTLTextureType3D;
        // NOTE: MTLTextureType2DMultisampleArray requires macOS 10.14+, iOS 14.0+
        case SG_IMAGETYPE_ARRAY:    return MTLTextureType2DArray;
        default: SOKOL_UNREACHABLE; return (MTLTextureType)0;
    }
}

_SOKOL_PRIVATE MTLSamplerAddressMode _sg_mtl_address_mode(sg_wrap w) {
    if (_sg.features.image_clamp_to_border) {
        if (@available(macOS 12.0, iOS 14.0, *)) {
            // border color feature available
            switch (w) {
                case SG_WRAP_REPEAT:            return MTLSamplerAddressModeRepeat;
                case SG_WRAP_CLAMP_TO_EDGE:     return MTLSamplerAddressModeClampToEdge;
                case SG_WRAP_CLAMP_TO_BORDER:   return MTLSamplerAddressModeClampToBorderColor;
                case SG_WRAP_MIRRORED_REPEAT:   return MTLSamplerAddressModeMirrorRepeat;
                default: SOKOL_UNREACHABLE; return (MTLSamplerAddressMode)0;
            }
        }
    }
    // fallthrough: clamp to border no supported
    switch (w) {
        case SG_WRAP_REPEAT:            return MTLSamplerAddressModeRepeat;
        case SG_WRAP_CLAMP_TO_EDGE:     return MTLSamplerAddressModeClampToEdge;
        case SG_WRAP_CLAMP_TO_BORDER:   return MTLSamplerAddressModeClampToEdge;
        case SG_WRAP_MIRRORED_REPEAT:   return MTLSamplerAddressModeMirrorRepeat;
        default: SOKOL_UNREACHABLE; return (MTLSamplerAddressMode)0;
    }
}

_SOKOL_PRIVATE API_AVAILABLE(ios(14.0), macos(12.0)) MTLSamplerBorderColor _sg_mtl_border_color(sg_border_color c) {
    switch (c) {
        case SG_BORDERCOLOR_TRANSPARENT_BLACK: return MTLSamplerBorderColorTransparentBlack;
        case SG_BORDERCOLOR_OPAQUE_BLACK: return MTLSamplerBorderColorOpaqueBlack;
        case SG_BORDERCOLOR_OPAQUE_WHITE: return MTLSamplerBorderColorOpaqueWhite;
        default: SOKOL_UNREACHABLE; return (MTLSamplerBorderColor)0;
    }
}

_SOKOL_PRIVATE MTLSamplerMinMagFilter _sg_mtl_minmag_filter(sg_filter f) {
    switch (f) {
        case SG_FILTER_NEAREST:
            return MTLSamplerMinMagFilterNearest;
        case SG_FILTER_LINEAR:
            return MTLSamplerMinMagFilterLinear;
        default:
            SOKOL_UNREACHABLE; return (MTLSamplerMinMagFilter)0;
    }
}

_SOKOL_PRIVATE MTLSamplerMipFilter _sg_mtl_mipmap_filter(sg_filter f) {
    switch (f) {
        case SG_FILTER_NEAREST:
            return MTLSamplerMipFilterNearest;
        case SG_FILTER_LINEAR:
            return MTLSamplerMipFilterLinear;
        default:
            SOKOL_UNREACHABLE; return (MTLSamplerMipFilter)0;
    }
}

_SOKOL_PRIVATE size_t _sg_mtl_vertexbuffer_bindslot(size_t sokol_bindslot) {
    return sokol_bindslot + _SG_MTL_MAX_STAGE_UB_SBUF_BINDINGS;
}

//-- a pool for all Metal resource objects, with deferred release queue ---------
_SOKOL_PRIVATE void _sg_mtl_init_pool(const sg_desc* desc) {
    _sg.mtl.idpool.num_slots = 2 *
        (
            2 * desc->buffer_pool_size +
            4 * desc->image_pool_size +
            1 * desc->sampler_pool_size +
            4 * desc->shader_pool_size +
            2 * desc->pipeline_pool_size +
            desc->view_pool_size +
            128
        );
    _sg.mtl.idpool.pool = [NSMutableArray arrayWithCapacity:(NSUInteger)_sg.mtl.idpool.num_slots];
    _SG_OBJC_RETAIN(_sg.mtl.idpool.pool);
    NSNull* null = [NSNull null];
    for (int i = 0; i < _sg.mtl.idpool.num_slots; i++) {
        [_sg.mtl.idpool.pool addObject:null];
    }
    SOKOL_ASSERT([_sg.mtl.idpool.pool count] == (NSUInteger)_sg.mtl.idpool.num_slots);
    // a queue of currently free slot indices
    _sg.mtl.idpool.free_queue_top = 0;
    _sg.mtl.idpool.free_queue = (int*)_sg_malloc_clear((size_t)_sg.mtl.idpool.num_slots * sizeof(int));
    // pool slot 0 is reserved!
    for (int i = _sg.mtl.idpool.num_slots-1; i >= 1; i--) {
        _sg.mtl.idpool.free_queue[_sg.mtl.idpool.free_queue_top++] = i;
    }
    // a circular queue which holds release items (frame index when a resource is to be released, and the resource's pool index
    _sg.mtl.idpool.release_queue_front = 0;
    _sg.mtl.idpool.release_queue_back = 0;
    _sg.mtl.idpool.release_queue = (_sg_mtl_release_item_t*)_sg_malloc_clear((size_t)_sg.mtl.idpool.num_slots * sizeof(_sg_mtl_release_item_t));
    for (int i = 0; i < _sg.mtl.idpool.num_slots; i++) {
        _sg.mtl.idpool.release_queue[i].frame_index = 0;
        _sg.mtl.idpool.release_queue[i].slot_index = _SG_MTL_INVALID_SLOT_INDEX;
    }
}

_SOKOL_PRIVATE void _sg_mtl_destroy_pool(void) {
    _sg_free(_sg.mtl.idpool.release_queue);  _sg.mtl.idpool.release_queue = 0;
    _sg_free(_sg.mtl.idpool.free_queue);     _sg.mtl.idpool.free_queue = 0;
    _SG_OBJC_RELEASE(_sg.mtl.idpool.pool);
}

// get a new free resource pool slot
_SOKOL_PRIVATE int _sg_mtl_alloc_pool_slot(void) {
    SOKOL_ASSERT(_sg.mtl.idpool.free_queue_top > 0);
    const int slot_index = _sg.mtl.idpool.free_queue[--_sg.mtl.idpool.free_queue_top];
    SOKOL_ASSERT((slot_index > 0) && (slot_index < _sg.mtl.idpool.num_slots));
    return slot_index;
}

// put a free resource pool slot back into the free-queue
_SOKOL_PRIVATE void _sg_mtl_free_pool_slot(int slot_index) {
    SOKOL_ASSERT(_sg.mtl.idpool.free_queue_top < _sg.mtl.idpool.num_slots);
    SOKOL_ASSERT((slot_index > 0) && (slot_index < _sg.mtl.idpool.num_slots));
    _sg.mtl.idpool.free_queue[_sg.mtl.idpool.free_queue_top++] = slot_index;
}

// add an MTLResource to the pool, return pool index or 0 if input was 'nil'
_SOKOL_PRIVATE int _sg_mtl_add_resource(id res) {
    if (nil == res) {
        return _SG_MTL_INVALID_SLOT_INDEX;
    }
    _sg_stats_add(metal.idpool.num_added, 1);
    const int slot_index = _sg_mtl_alloc_pool_slot();
    // NOTE: the NSMutableArray will take ownership of its items
    SOKOL_ASSERT([NSNull null] == _sg.mtl.idpool.pool[(NSUInteger)slot_index]);
    _sg.mtl.idpool.pool[(NSUInteger)slot_index] = res;
    return slot_index;
}

/*  mark an MTLResource for release, this will put the resource into the
    deferred-release queue, and the resource will then be released N frames later,
    the special pool index 0 will be ignored (this means that a nil
    value was provided to _sg_mtl_add_resource()
*/
_SOKOL_PRIVATE void _sg_mtl_release_resource(uint32_t frame_index, int slot_index) {
    if (slot_index == _SG_MTL_INVALID_SLOT_INDEX) {
        return;
    }
    _sg_stats_add(metal.idpool.num_released, 1);
    SOKOL_ASSERT((slot_index > 0) && (slot_index < _sg.mtl.idpool.num_slots));
    SOKOL_ASSERT([NSNull null] != _sg.mtl.idpool.pool[(NSUInteger)slot_index]);
    int release_index = _sg.mtl.idpool.release_queue_front++;
    if (_sg.mtl.idpool.release_queue_front >= _sg.mtl.idpool.num_slots) {
        // wrap-around
        _sg.mtl.idpool.release_queue_front = 0;
    }
    // release queue full?
    SOKOL_ASSERT(_sg.mtl.idpool.release_queue_front != _sg.mtl.idpool.release_queue_back);
    SOKOL_ASSERT(0 == _sg.mtl.idpool.release_queue[release_index].frame_index);
    const uint32_t safe_to_release_frame_index = frame_index + SG_NUM_INFLIGHT_FRAMES + 1;
    _sg.mtl.idpool.release_queue[release_index].frame_index = safe_to_release_frame_index;
    _sg.mtl.idpool.release_queue[release_index].slot_index = slot_index;
}

// run garbage-collection pass on all resources in the release-queue
_SOKOL_PRIVATE void _sg_mtl_garbage_collect(uint32_t frame_index) {
    while (_sg.mtl.idpool.release_queue_back != _sg.mtl.idpool.release_queue_front) {
        if (frame_index < _sg.mtl.idpool.release_queue[_sg.mtl.idpool.release_queue_back].frame_index) {
            // don't need to check further, release-items past this are too young
            break;
        }
        _sg_stats_add(metal.idpool.num_garbage_collected, 1);
        // safe to release this resource
        const int slot_index = _sg.mtl.idpool.release_queue[_sg.mtl.idpool.release_queue_back].slot_index;
        SOKOL_ASSERT((slot_index > 0) && (slot_index < _sg.mtl.idpool.num_slots));
        // note: the NSMutableArray takes ownership of its items, assigning an NSNull object will
        // release the object, no matter if using ARC or not
        SOKOL_ASSERT(_sg.mtl.idpool.pool[(NSUInteger)slot_index] != [NSNull null]);
        _sg.mtl.idpool.pool[(NSUInteger)slot_index] = [NSNull null];
        // put the now free pool index back on the free queue
        _sg_mtl_free_pool_slot(slot_index);
        // reset the release queue slot and advance the back index
        _sg.mtl.idpool.release_queue[_sg.mtl.idpool.release_queue_back].frame_index = 0;
        _sg.mtl.idpool.release_queue[_sg.mtl.idpool.release_queue_back].slot_index = _SG_MTL_INVALID_SLOT_INDEX;
        _sg.mtl.idpool.release_queue_back++;
        if (_sg.mtl.idpool.release_queue_back >= _sg.mtl.idpool.num_slots) {
            // wrap-around
            _sg.mtl.idpool.release_queue_back = 0;
        }
    }
}

_SOKOL_PRIVATE id _sg_mtl_id(int slot_index) {
    return _sg.mtl.idpool.pool[(NSUInteger)slot_index];
}

_SOKOL_PRIVATE void _sg_mtl_clear_state_cache(void) {
    _sg_clear(&_sg.mtl.cache, sizeof(_sg.mtl.cache));
}

// https://developer.apple.com/metal/Metal-Feature-Set-Tables.pdf
_SOKOL_PRIVATE void _sg_mtl_init_caps(void) {
    #if defined(_SG_TARGET_MACOS)
        _sg.backend = SG_BACKEND_METAL_MACOS;
    #elif defined(_SG_TARGET_IOS)
        #if defined(_SG_TARGET_IOS_SIMULATOR)
            _sg.backend = SG_BACKEND_METAL_SIMULATOR;
        #else
            _sg.backend = SG_BACKEND_METAL_IOS;
        #endif
    #endif
    _sg.features.origin_top_left = true;
    _sg.features.mrt_independent_blend_state = true;
    _sg.features.mrt_independent_write_mask = true;
    _sg.features.compute = true;
    _sg.features.msaa_texture_bindings = true;

    _sg.features.image_clamp_to_border = false;
    #if (MAC_OS_X_VERSION_MAX_ALLOWED >= 120000) || (__IPHONE_OS_VERSION_MAX_ALLOWED >= 140000)
    if (@available(macOS 12.0, iOS 14.0, *)) {
        _sg.features.image_clamp_to_border = [_sg.mtl.device supportsFamily:MTLGPUFamilyApple7]
                                             || [_sg.mtl.device supportsFamily:MTLGPUFamilyMac2];
        #if (MAC_OS_X_VERSION_MAX_ALLOWED >= 130000) || (__IPHONE_OS_VERSION_MAX_ALLOWED >= 160000)
        if (!_sg.features.image_clamp_to_border) {
            if (@available(macOS 13.0, iOS 16.0, *)) {
                _sg.features.image_clamp_to_border = [_sg.mtl.device supportsFamily:MTLGPUFamilyMetal3];
            }
        }
        #endif
    }
    #endif

    #if defined(_SG_TARGET_MACOS)
        _sg.limits.max_image_size_2d = 16 * 1024;
        _sg.limits.max_image_size_cube = 16 * 1024;
        _sg.limits.max_image_size_3d = 2 * 1024;
        _sg.limits.max_image_size_array = 16 * 1024;
        _sg.limits.max_image_array_layers = 2 * 1024;
    #else
        // FIXME: newer iOS devices support 16k textures
        _sg.limits.max_image_size_2d = 8 * 1024;
        _sg.limits.max_image_size_cube = 8 * 1024;
        _sg.limits.max_image_size_3d = 2 * 1024;
        _sg.limits.max_image_size_array = 8 * 1024;
        _sg.limits.max_image_array_layers = 2 * 1024;
    #endif
    _sg.limits.max_vertex_attrs = SG_MAX_VERTEX_ATTRIBUTES;

    _sg_pixelformat_all(&_sg.formats[SG_PIXELFORMAT_R8]);
    _sg_pixelformat_all(&_sg.formats[SG_PIXELFORMAT_R8SN]);
    _sg_pixelformat_srm(&_sg.formats[SG_PIXELFORMAT_R8UI]);
    _sg_pixelformat_srm(&_sg.formats[SG_PIXELFORMAT_R8SI]);
    #if defined(_SG_TARGET_MACOS)
        _sg_pixelformat_all(&_sg.formats[SG_PIXELFORMAT_R16]);
        _sg_pixelformat_all(&_sg.formats[SG_PIXELFORMAT_R16SN]);
    #else
        _sg_pixelformat_sfbr(&_sg.formats[SG_PIXELFORMAT_R16]);
        _sg_pixelformat_sfbr(&_sg.formats[SG_PIXELFORMAT_R16SN]);
    #endif
    _sg_pixelformat_srm(&_sg.formats[SG_PIXELFORMAT_R16UI]);
    _sg_pixelformat_srm(&_sg.formats[SG_PIXELFORMAT_R16SI]);
    _sg_pixelformat_all(&_sg.formats[SG_PIXELFORMAT_R16F]);
    _sg_pixelformat_all(&_sg.formats[SG_PIXELFORMAT_RG8]);
    _sg_pixelformat_all(&_sg.formats[SG_PIXELFORMAT_RG8SN]);
    _sg_pixelformat_srm(&_sg.formats[SG_PIXELFORMAT_RG8UI]);
    _sg_pixelformat_srm(&_sg.formats[SG_PIXELFORMAT_RG8SI]);
    _sg_pixelformat_sr(&_sg.formats[SG_PIXELFORMAT_R32UI]);
    _sg_pixelformat_sr(&_sg.formats[SG_PIXELFORMAT_R32SI]);
    #if defined(_SG_TARGET_MACOS)
        _sg_pixelformat_all(&_sg.formats[SG_PIXELFORMAT_R32F]);
    #else
        _sg_pixelformat_sbr(&_sg.formats[SG_PIXELFORMAT_R32F]);
    #endif
    #if defined(_SG_TARGET_MACOS)
        _sg_pixelformat_all(&_sg.formats[SG_PIXELFORMAT_RG16]);
        _sg_pixelformat_all(&_sg.formats[SG_PIXELFORMAT_RG16SN]);
    #else
        _sg_pixelformat_sfbr(&_sg.formats[SG_PIXELFORMAT_RG16]);
        _sg_pixelformat_sfbr(&_sg.formats[SG_PIXELFORMAT_RG16SN]);
    #endif
    _sg_pixelformat_srm(&_sg.formats[SG_PIXELFORMAT_RG16UI]);
    _sg_pixelformat_srm(&_sg.formats[SG_PIXELFORMAT_RG16SI]);
    _sg_pixelformat_all(&_sg.formats[SG_PIXELFORMAT_RG16F]);
    _sg_pixelformat_all(&_sg.formats[SG_PIXELFORMAT_RGBA8]);
    _sg_pixelformat_all(&_sg.formats[SG_PIXELFORMAT_SRGB8A8]);
    _sg_pixelformat_all(&_sg.formats[SG_PIXELFORMAT_RGBA8SN]);
    _sg_pixelformat_srm(&_sg.formats[SG_PIXELFORMAT_RGBA8UI]);
    _sg_pixelformat_srm(&_sg.formats[SG_PIXELFORMAT_RGBA8SI]);
    _sg_pixelformat_all(&_sg.formats[SG_PIXELFORMAT_BGRA8]);
    _sg_pixelformat_all(&_sg.formats[SG_PIXELFORMAT_RGB10A2]);
    _sg_pixelformat_all(&_sg.formats[SG_PIXELFORMAT_RG11B10F]);
    #if defined(_SG_TARGET_MACOS)
        _sg_pixelformat_sf(&_sg.formats[SG_PIXELFORMAT_RGB9E5]);
        _sg_pixelformat_srm(&_sg.formats[SG_PIXELFORMAT_RG32UI]);
        _sg_pixelformat_srm(&_sg.formats[SG_PIXELFORMAT_RG32SI]);
    #else
        _sg_pixelformat_all(&_sg.formats[SG_PIXELFORMAT_RGB9E5]);
        _sg_pixelformat_sr(&_sg.formats[SG_PIXELFORMAT_RG32UI]);
        _sg_pixelformat_sr(&_sg.formats[SG_PIXELFORMAT_RG32SI]);
    #endif
    #if defined(_SG_TARGET_MACOS)
        _sg_pixelformat_all(&_sg.formats[SG_PIXELFORMAT_RG32F]);
    #else
        _sg_pixelformat_sbr(&_sg.formats[SG_PIXELFORMAT_RG32F]);
    #endif
    #if defined(_SG_TARGET_MACOS)
        _sg_pixelformat_all(&_sg.formats[SG_PIXELFORMAT_RGBA16]);
        _sg_pixelformat_all(&_sg.formats[SG_PIXELFORMAT_RGBA16SN]);
    #else
        _sg_pixelformat_sfbr(&_sg.formats[SG_PIXELFORMAT_RGBA16]);
        _sg_pixelformat_sfbr(&_sg.formats[SG_PIXELFORMAT_RGBA16SN]);
    #endif
    _sg_pixelformat_srm(&_sg.formats[SG_PIXELFORMAT_RGBA16UI]);
    _sg_pixelformat_srm(&_sg.formats[SG_PIXELFORMAT_RGBA16SI]);
    _sg_pixelformat_all(&_sg.formats[SG_PIXELFORMAT_RGBA16F]);
    #if defined(_SG_TARGET_MACOS)
        _sg_pixelformat_srm(&_sg.formats[SG_PIXELFORMAT_RGBA32UI]);
        _sg_pixelformat_srm(&_sg.formats[SG_PIXELFORMAT_RGBA32SI]);
        _sg_pixelformat_all(&_sg.formats[SG_PIXELFORMAT_RGBA32F]);
    #else
        _sg_pixelformat_sr(&_sg.formats[SG_PIXELFORMAT_RGBA32UI]);
        _sg_pixelformat_sr(&_sg.formats[SG_PIXELFORMAT_RGBA32SI]);
        _sg_pixelformat_sr(&_sg.formats[SG_PIXELFORMAT_RGBA32F]);
    #endif
    _sg_pixelformat_srmd(&_sg.formats[SG_PIXELFORMAT_DEPTH]);
    _sg_pixelformat_srmd(&_sg.formats[SG_PIXELFORMAT_DEPTH_STENCIL]);
    #if defined(_SG_TARGET_MACOS)
        _sg_pixelformat_sf(&_sg.formats[SG_PIXELFORMAT_BC1_RGBA]);
        _sg_pixelformat_sf(&_sg.formats[SG_PIXELFORMAT_BC2_RGBA]);
        _sg_pixelformat_sf(&_sg.formats[SG_PIXELFORMAT_BC3_RGBA]);
        _sg_pixelformat_sf(&_sg.formats[SG_PIXELFORMAT_BC3_SRGBA]);
        _sg_pixelformat_sf(&_sg.formats[SG_PIXELFORMAT_BC4_R]);
        _sg_pixelformat_sf(&_sg.formats[SG_PIXELFORMAT_BC4_RSN]);
        _sg_pixelformat_sf(&_sg.formats[SG_PIXELFORMAT_BC5_RG]);
        _sg_pixelformat_sf(&_sg.formats[SG_PIXELFORMAT_BC5_RGSN]);
        _sg_pixelformat_sf(&_sg.formats[SG_PIXELFORMAT_BC6H_RGBF]);
        _sg_pixelformat_sf(&_sg.formats[SG_PIXELFORMAT_BC6H_RGBUF]);
        _sg_pixelformat_sf(&_sg.formats[SG_PIXELFORMAT_BC7_RGBA]);
        _sg_pixelformat_sf(&_sg.formats[SG_PIXELFORMAT_BC7_SRGBA]);
    #else
        _sg_pixelformat_sf(&_sg.formats[SG_PIXELFORMAT_ETC2_RGB8]);
        _sg_pixelformat_sf(&_sg.formats[SG_PIXELFORMAT_ETC2_SRGB8]);
        _sg_pixelformat_sf(&_sg.formats[SG_PIXELFORMAT_ETC2_RGB8A1]);
        _sg_pixelformat_sf(&_sg.formats[SG_PIXELFORMAT_ETC2_RGBA8]);
        _sg_pixelformat_sf(&_sg.formats[SG_PIXELFORMAT_ETC2_SRGB8A8]);
        _sg_pixelformat_sf(&_sg.formats[SG_PIXELFORMAT_EAC_R11]);
        _sg_pixelformat_sf(&_sg.formats[SG_PIXELFORMAT_EAC_R11SN]);
        _sg_pixelformat_sf(&_sg.formats[SG_PIXELFORMAT_EAC_RG11]);
        _sg_pixelformat_sf(&_sg.formats[SG_PIXELFORMAT_EAC_RG11SN]);
        _sg_pixelformat_sf(&_sg.formats[SG_PIXELFORMAT_ASTC_4x4_RGBA]);
        _sg_pixelformat_sf(&_sg.formats[SG_PIXELFORMAT_ASTC_4x4_SRGBA]);
    #endif

    // compute shader access (see: https://github.com/gpuweb/gpuweb/issues/513)
    // for now let's use the same conservative set on all backends even though
    // some backends are less restrictive
    _sg_pixelformat_compute_all(&_sg.formats[SG_PIXELFORMAT_RGBA8]);
    _sg_pixelformat_compute_all(&_sg.formats[SG_PIXELFORMAT_RGBA8SN]);
    _sg_pixelformat_compute_all(&_sg.formats[SG_PIXELFORMAT_RGBA8UI]);
    _sg_pixelformat_compute_all(&_sg.formats[SG_PIXELFORMAT_RGBA8SI]);
    _sg_pixelformat_compute_all(&_sg.formats[SG_PIXELFORMAT_RGBA16UI]);
    _sg_pixelformat_compute_all(&_sg.formats[SG_PIXELFORMAT_RGBA16SI]);
    _sg_pixelformat_compute_all(&_sg.formats[SG_PIXELFORMAT_RGBA16F]);
    _sg_pixelformat_compute_all(&_sg.formats[SG_PIXELFORMAT_R32UI]);
    _sg_pixelformat_compute_all(&_sg.formats[SG_PIXELFORMAT_R32SI]);
    _sg_pixelformat_compute_all(&_sg.formats[SG_PIXELFORMAT_R32F]);
    _sg_pixelformat_compute_all(&_sg.formats[SG_PIXELFORMAT_RG32UI]);
    _sg_pixelformat_compute_all(&_sg.formats[SG_PIXELFORMAT_RG32SI]);
    _sg_pixelformat_compute_all(&_sg.formats[SG_PIXELFORMAT_RG32F]);
    _sg_pixelformat_compute_all(&_sg.formats[SG_PIXELFORMAT_RGBA32UI]);
    _sg_pixelformat_compute_all(&_sg.formats[SG_PIXELFORMAT_RGBA32SI]);
    _sg_pixelformat_compute_all(&_sg.formats[SG_PIXELFORMAT_RGBA32F]);
}

//-- main Metal backend state and functions ------------------------------------
_SOKOL_PRIVATE void _sg_mtl_setup_backend(const sg_desc* desc) {
    // assume already zero-initialized
    SOKOL_ASSERT(desc);
    SOKOL_ASSERT(desc->environment.metal.device);
    SOKOL_ASSERT(desc->uniform_buffer_size > 0);
    _sg_mtl_init_pool(desc);
    _sg_mtl_clear_state_cache();
    _sg.mtl.valid = true;
    _sg.mtl.ub_size = desc->uniform_buffer_size;
    _sg.mtl.sem = dispatch_semaphore_create(SG_NUM_INFLIGHT_FRAMES);
    _sg.mtl.device = (__bridge id<MTLDevice>) desc->environment.metal.device;
    _sg.mtl.cmd_queue = [_sg.mtl.device newCommandQueue];

    for (int i = 0; i < SG_NUM_INFLIGHT_FRAMES; i++) {
        _sg.mtl.uniform_buffers[i] = [_sg.mtl.device
            newBufferWithLength:(NSUInteger)_sg.mtl.ub_size
            options:MTLResourceCPUCacheModeWriteCombined|MTLResourceStorageModeShared
        ];
        #if defined(SOKOL_DEBUG)
            _sg.mtl.uniform_buffers[i].label = [NSString stringWithFormat:@"sg-uniform-buffer.%d", i];
        #endif
    }

    if (desc->mtl_force_managed_storage_mode) {
        _sg.mtl.use_shared_storage_mode = false;
    } else if (@available(macOS 10.15, iOS 13.0, *)) {
        // on Intel Macs, always use managed resources even though the
        // device says it supports unified memory (because of texture restrictions)
        const bool is_apple_gpu = [_sg.mtl.device supportsFamily:MTLGPUFamilyApple1];
        if (!is_apple_gpu) {
            _sg.mtl.use_shared_storage_mode = false;
        } else {
            _sg.mtl.use_shared_storage_mode = true;
        }
    } else {
        #if defined(_SG_TARGET_MACOS)
            _sg.mtl.use_shared_storage_mode = false;
        #else
            _sg.mtl.use_shared_storage_mode = true;
        #endif
    }
    _sg_mtl_init_caps();
}

_SOKOL_PRIVATE void _sg_mtl_discard_backend(void) {
    SOKOL_ASSERT(_sg.mtl.valid);
    // wait for the last frame to finish
    for (int i = 0; i < SG_NUM_INFLIGHT_FRAMES; i++) {
        dispatch_semaphore_wait(_sg.mtl.sem, DISPATCH_TIME_FOREVER);
    }
    // semaphore must be "relinquished" before destruction
    for (int i = 0; i < SG_NUM_INFLIGHT_FRAMES; i++) {
        dispatch_semaphore_signal(_sg.mtl.sem);
    }
    _sg_mtl_garbage_collect(_sg.frame_index + SG_NUM_INFLIGHT_FRAMES + 2);
    _sg_mtl_destroy_pool();
    _sg.mtl.valid = false;

    _SG_OBJC_RELEASE(_sg.mtl.sem);
    _SG_OBJC_RELEASE(_sg.mtl.device);
    _SG_OBJC_RELEASE(_sg.mtl.cmd_queue);
    for (int i = 0; i < SG_NUM_INFLIGHT_FRAMES; i++) {
        _SG_OBJC_RELEASE(_sg.mtl.uniform_buffers[i]);
    }
    // NOTE: MTLCommandBuffer, MTLRenderCommandEncoder and MTLComputeCommandEncoder are auto-released
    _sg.mtl.cmd_buffer = nil;
    _sg.mtl.render_cmd_encoder = nil;
    _sg.mtl.compute_cmd_encoder = nil;
}

_SOKOL_PRIVATE void _sg_mtl_reset_state_cache(void) {
    _sg_mtl_clear_state_cache();
}

_SOKOL_PRIVATE sg_resource_state _sg_mtl_create_buffer(_sg_buffer_t* buf, const sg_buffer_desc* desc) {
    SOKOL_ASSERT(buf && desc);
    SOKOL_ASSERT(buf->cmn.size > 0);
    const bool injected = (0 != desc->mtl_buffers[0]);
    MTLResourceOptions mtl_options = _sg_mtl_buffer_resource_options(&buf->cmn.usage);
    for (int slot = 0; slot < buf->cmn.num_slots; slot++) {
        id<MTLBuffer> mtl_buf;
        if (injected) {
            SOKOL_ASSERT(desc->mtl_buffers[slot]);
            mtl_buf = (__bridge id<MTLBuffer>) desc->mtl_buffers[slot];
        } else {
            if (desc->data.ptr) {
                SOKOL_ASSERT(desc->data.size > 0);
                mtl_buf = [_sg.mtl.device newBufferWithBytes:desc->data.ptr length:(NSUInteger)buf->cmn.size options:mtl_options];
            } else {
                mtl_buf = [_sg.mtl.device newBufferWithLength:(NSUInteger)buf->cmn.size options:mtl_options];
            }
            if (nil == mtl_buf) {
                _SG_ERROR(METAL_CREATE_BUFFER_FAILED);
                return SG_RESOURCESTATE_FAILED;
            }
        }
        #if defined(SOKOL_DEBUG)
            if (desc->label) {
                mtl_buf.label = [NSString stringWithFormat:@"%s.%d", desc->label, slot];
            }
        #endif
        buf->mtl.buf[slot] = _sg_mtl_add_resource(mtl_buf);
        _SG_OBJC_RELEASE(mtl_buf);
    }
    return SG_RESOURCESTATE_VALID;
}

_SOKOL_PRIVATE void _sg_mtl_discard_buffer(_sg_buffer_t* buf) {
    SOKOL_ASSERT(buf);
    for (int slot = 0; slot < buf->cmn.num_slots; slot++) {
        // it's valid to call release resource with '0'
        _sg_mtl_release_resource(_sg.frame_index, buf->mtl.buf[slot]);
    }
}

_SOKOL_PRIVATE void _sg_mtl_copy_image_data(const _sg_image_t* img, __unsafe_unretained id<MTLTexture> mtl_tex, const sg_image_data* data) {
    const int num_faces = (img->cmn.type == SG_IMAGETYPE_CUBE) ? 6:1;
    const int num_slices = (img->cmn.type == SG_IMAGETYPE_ARRAY) ? img->cmn.num_slices : 1;
    for (int face_index = 0; face_index < num_faces; face_index++) {
        for (int mip_index = 0; mip_index < img->cmn.num_mipmaps; mip_index++) {
            SOKOL_ASSERT(data->subimage[face_index][mip_index].ptr);
            SOKOL_ASSERT(data->subimage[face_index][mip_index].size > 0);
            const uint8_t* data_ptr = (const uint8_t*)data->subimage[face_index][mip_index].ptr;
            const int mip_width = _sg_miplevel_dim(img->cmn.width, mip_index);
            const int mip_height = _sg_miplevel_dim(img->cmn.height, mip_index);
            int bytes_per_row = _sg_row_pitch(img->cmn.pixel_format, mip_width, 1);
            int bytes_per_slice = _sg_surface_pitch(img->cmn.pixel_format, mip_width, mip_height, 1);
            /* bytesPerImage special case: https://developer.apple.com/documentation/metal/mtltexture/1515679-replaceregion

                "Supply a nonzero value only when you copy data to a MTLTextureType3D type texture"
            */
            MTLRegion region;
            int bytes_per_image;
            if (img->cmn.type == SG_IMAGETYPE_3D) {
                const int mip_depth = _sg_miplevel_dim(img->cmn.num_slices, mip_index);
                region = MTLRegionMake3D(0, 0, 0, (NSUInteger)mip_width, (NSUInteger)mip_height, (NSUInteger)mip_depth);
                bytes_per_image = bytes_per_slice;
                // FIXME: apparently the minimal bytes_per_image size for 3D texture is 4 KByte... somehow need to handle this
            } else {
                region = MTLRegionMake2D(0, 0, (NSUInteger)mip_width, (NSUInteger)mip_height);
                bytes_per_image = 0;
            }

            for (int slice_index = 0; slice_index < num_slices; slice_index++) {
                const int mtl_slice_index = (img->cmn.type == SG_IMAGETYPE_CUBE) ? face_index : slice_index;
                const int slice_offset = slice_index * bytes_per_slice;
                SOKOL_ASSERT((slice_offset + bytes_per_slice) <= (int)data->subimage[face_index][mip_index].size);
                [mtl_tex replaceRegion:region
                    mipmapLevel:(NSUInteger)mip_index
                    slice:(NSUInteger)mtl_slice_index
                    withBytes:data_ptr + slice_offset
                    bytesPerRow:(NSUInteger)bytes_per_row
                    bytesPerImage:(NSUInteger)bytes_per_image];
            }
        }
    }
}

_SOKOL_PRIVATE bool _sg_mtl_init_texdesc(MTLTextureDescriptor* mtl_desc, _sg_image_t* img) {
    mtl_desc.textureType = _sg_mtl_texture_type(img->cmn.type, img->cmn.sample_count > 1);
    mtl_desc.pixelFormat = _sg_mtl_pixel_format(img->cmn.pixel_format);
    if (MTLPixelFormatInvalid == mtl_desc.pixelFormat) {
        _SG_ERROR(METAL_TEXTURE_FORMAT_NOT_SUPPORTED);
        return false;
    }
    mtl_desc.width = (NSUInteger)img->cmn.width;
    mtl_desc.height = (NSUInteger)img->cmn.height;
    if (SG_IMAGETYPE_3D == img->cmn.type) {
        mtl_desc.depth = (NSUInteger)img->cmn.num_slices;
    } else {
        mtl_desc.depth = 1;
    }
    mtl_desc.mipmapLevelCount = (NSUInteger)img->cmn.num_mipmaps;
    if (SG_IMAGETYPE_ARRAY == img->cmn.type) {
        mtl_desc.arrayLength = (NSUInteger)img->cmn.num_slices;
    } else {
        mtl_desc.arrayLength = 1;
    }
    mtl_desc.sampleCount = (NSUInteger)img->cmn.sample_count;

    const sg_image_usage* usg = &img->cmn.usage;
    const bool any_attachment = usg->color_attachment || usg->resolve_attachment || usg->depth_stencil_attachment;
    MTLTextureUsage mtl_tex_usage = MTLTextureUsageShaderRead;
    if (any_attachment) {
        mtl_tex_usage |= MTLTextureUsageRenderTarget;
    }
    if (img->cmn.usage.storage_image) {
        mtl_tex_usage |= MTLTextureUsageShaderWrite;
    }
    mtl_desc.usage = mtl_tex_usage;

    MTLResourceOptions mtl_res_options = 0;
    if (any_attachment || img->cmn.usage.storage_image) {
        mtl_res_options |= MTLResourceStorageModePrivate;
    } else {
        mtl_res_options |= _sg_mtl_resource_options_storage_mode_managed_or_shared();
        if (!img->cmn.usage.immutable) {
            mtl_res_options |= MTLResourceCPUCacheModeWriteCombined;
        }
    }
    mtl_desc.resourceOptions = mtl_res_options;
    return true;
}

_SOKOL_PRIVATE sg_resource_state _sg_mtl_create_image(_sg_image_t* img, const sg_image_desc* desc) {
    SOKOL_ASSERT(img && desc);
    const bool injected = (0 != desc->mtl_textures[0]);

    // first initialize all Metal resource pool slots to 'empty'
    for (int i = 0; i < SG_NUM_INFLIGHT_FRAMES; i++) {
        img->mtl.tex[i] = _sg_mtl_add_resource(nil);
    }

    // initialize a Metal texture descriptor
    MTLTextureDescriptor* mtl_desc = [[MTLTextureDescriptor alloc] init];
    if (!_sg_mtl_init_texdesc(mtl_desc, img)) {
        _SG_OBJC_RELEASE(mtl_desc);
        return SG_RESOURCESTATE_FAILED;
    }
    for (int slot = 0; slot < img->cmn.num_slots; slot++) {
        id<MTLTexture> mtl_tex;
        if (injected) {
            SOKOL_ASSERT(desc->mtl_textures[slot]);
            mtl_tex = (__bridge id<MTLTexture>) desc->mtl_textures[slot];
        } else {
            mtl_tex = [_sg.mtl.device newTextureWithDescriptor:mtl_desc];
            if (nil == mtl_tex) {
                _SG_OBJC_RELEASE(mtl_desc);
                _SG_ERROR(METAL_CREATE_TEXTURE_FAILED);
                return SG_RESOURCESTATE_FAILED;
            }
            if (desc->data.subimage[0][0].ptr) {
                _sg_mtl_copy_image_data(img, mtl_tex, &desc->data);
            }
        }
        #if defined(SOKOL_DEBUG)
            if (desc->label) {
                mtl_tex.label = [NSString stringWithFormat:@"%s.%d", desc->label, slot];
            }
        #endif
        img->mtl.tex[slot] = _sg_mtl_add_resource(mtl_tex);
        _SG_OBJC_RELEASE(mtl_tex);
    }
    _SG_OBJC_RELEASE(mtl_desc);
    return SG_RESOURCESTATE_VALID;
}

_SOKOL_PRIVATE void _sg_mtl_discard_image(_sg_image_t* img) {
    SOKOL_ASSERT(img);
    // it's valid to call release resource with a 'null resource'
    for (int slot = 0; slot < img->cmn.num_slots; slot++) {
        _sg_mtl_release_resource(_sg.frame_index, img->mtl.tex[slot]);
    }
}

_SOKOL_PRIVATE sg_resource_state _sg_mtl_create_sampler(_sg_sampler_t* smp, const sg_sampler_desc* desc) {
    SOKOL_ASSERT(smp && desc);
    id<MTLSamplerState> mtl_smp;
    const bool injected = (0 != desc->mtl_sampler);
    if (injected) {
        SOKOL_ASSERT(desc->mtl_sampler);
        mtl_smp = (__bridge id<MTLSamplerState>) desc->mtl_sampler;
    } else {
        MTLSamplerDescriptor* mtl_desc = [[MTLSamplerDescriptor alloc] init];
        mtl_desc.sAddressMode = _sg_mtl_address_mode(desc->wrap_u);
        mtl_desc.tAddressMode = _sg_mtl_address_mode(desc->wrap_v);
        mtl_desc.rAddressMode = _sg_mtl_address_mode(desc->wrap_w);
        if (_sg.features.image_clamp_to_border) {
            if (@available(macOS 12.0, iOS 14.0, *)) {
                mtl_desc.borderColor  = _sg_mtl_border_color(desc->border_color);
            }
        }
        mtl_desc.minFilter = _sg_mtl_minmag_filter(desc->min_filter);
        mtl_desc.magFilter = _sg_mtl_minmag_filter(desc->mag_filter);
        mtl_desc.mipFilter = _sg_mtl_mipmap_filter(desc->mipmap_filter);
        mtl_desc.lodMinClamp = desc->min_lod;
        mtl_desc.lodMaxClamp = desc->max_lod;
        // FIXME: lodAverage?
        mtl_desc.maxAnisotropy = desc->max_anisotropy;
        mtl_desc.normalizedCoordinates = YES;
        mtl_desc.compareFunction = _sg_mtl_compare_func(desc->compare);
        #if defined(SOKOL_DEBUG)
            if (desc->label) {
                mtl_desc.label = [NSString stringWithUTF8String:desc->label];
            }
        #endif
        mtl_smp = [_sg.mtl.device newSamplerStateWithDescriptor:mtl_desc];
        _SG_OBJC_RELEASE(mtl_desc);
        if (nil == mtl_smp) {
            _SG_ERROR(METAL_CREATE_SAMPLER_FAILED);
            return SG_RESOURCESTATE_FAILED;
        }
    }
    smp->mtl.sampler_state = _sg_mtl_add_resource(mtl_smp);
    _SG_OBJC_RELEASE(mtl_smp);
    return SG_RESOURCESTATE_VALID;
}

_SOKOL_PRIVATE void _sg_mtl_discard_sampler(_sg_sampler_t* smp) {
    SOKOL_ASSERT(smp);
    // it's valid to call release resource with a 'null resource'
    _sg_mtl_release_resource(_sg.frame_index, smp->mtl.sampler_state);
}

_SOKOL_PRIVATE id<MTLLibrary> _sg_mtl_compile_library(const char* src) {
    NSError* err = NULL;
    id<MTLLibrary> lib = [_sg.mtl.device
        newLibraryWithSource:[NSString stringWithUTF8String:src]
        options:nil
        error:&err
    ];
    if (err) {
        _SG_ERROR(METAL_SHADER_COMPILATION_FAILED);
        _SG_LOGMSG(METAL_SHADER_COMPILATION_OUTPUT, [err.localizedDescription UTF8String]);
    }
    return lib;
}

_SOKOL_PRIVATE id<MTLLibrary> _sg_mtl_library_from_bytecode(const void* ptr, size_t num_bytes) {
    NSError* err = NULL;
    dispatch_data_t lib_data = dispatch_data_create(ptr, num_bytes, NULL, DISPATCH_DATA_DESTRUCTOR_DEFAULT);
    id<MTLLibrary> lib = [_sg.mtl.device newLibraryWithData:lib_data error:&err];
    if (err) {
        _SG_ERROR(METAL_SHADER_CREATION_FAILED);
        _SG_LOGMSG(METAL_SHADER_COMPILATION_OUTPUT, [err.localizedDescription UTF8String]);
    }
    _SG_OBJC_RELEASE(lib_data);
    return lib;
}

_SOKOL_PRIVATE bool _sg_mtl_create_shader_func(const sg_shader_function* func, const char* label, const char* label_ext, _sg_mtl_shader_func_t* res) {
    SOKOL_ASSERT(res->mtl_lib == _SG_MTL_INVALID_SLOT_INDEX);
    SOKOL_ASSERT(res->mtl_func == _SG_MTL_INVALID_SLOT_INDEX);
    id<MTLLibrary> mtl_lib = nil;
    if (func->bytecode.ptr) {
        SOKOL_ASSERT(func->bytecode.size > 0);
        mtl_lib = _sg_mtl_library_from_bytecode(func->bytecode.ptr, func->bytecode.size);
    } else if (func->source) {
        mtl_lib = _sg_mtl_compile_library(func->source);
    }
    if (mtl_lib == nil) {
        return false;
    }
    #if defined(SOKOL_DEBUG)
    if (label) {
        SOKOL_ASSERT(label_ext);
        mtl_lib.label = [NSString stringWithFormat:@"%s.%s", label, label_ext];
    }
    #else
    _SOKOL_UNUSED(label);
    _SOKOL_UNUSED(label_ext);
    #endif
    SOKOL_ASSERT(func->entry);
    id<MTLFunction> mtl_func = [mtl_lib newFunctionWithName:[NSString stringWithUTF8String:func->entry]];
    if (mtl_func == nil) {
        _SG_ERROR(METAL_SHADER_ENTRY_NOT_FOUND);
        _SG_OBJC_RELEASE(mtl_lib);
        return false;
    }
    res->mtl_lib = _sg_mtl_add_resource(mtl_lib);
    res->mtl_func = _sg_mtl_add_resource(mtl_func);
    _SG_OBJC_RELEASE(mtl_lib);
    _SG_OBJC_RELEASE(mtl_func);
    return true;
}

_SOKOL_PRIVATE void _sg_mtl_discard_shader_func(const _sg_mtl_shader_func_t* func) {
    // it is valid to call _sg_mtl_release_resource with a 'null resource'
    _sg_mtl_release_resource(_sg.frame_index, func->mtl_func);
    _sg_mtl_release_resource(_sg.frame_index, func->mtl_lib);
}

// NOTE: this is an out-of-range check for MSL bindslots that's also active in release mode
_SOKOL_PRIVATE bool _sg_mtl_ensure_msl_bindslot_ranges(const sg_shader_desc* desc) {
    SOKOL_ASSERT(desc);
    for (size_t i = 0; i < SG_MAX_UNIFORMBLOCK_BINDSLOTS; i++) {
        if (desc->uniform_blocks[i].msl_buffer_n >= _SG_MTL_MAX_STAGE_UB_BINDINGS) {
            _SG_ERROR(METAL_UNIFORMBLOCK_MSL_BUFFER_SLOT_OUT_OF_RANGE);
            return false;
        }
    }
    for (size_t i = 0; i < SG_MAX_VIEW_BINDSLOTS; i++) {
        if (desc->views[i].texture.msl_texture_n >= _SG_MTL_MAX_STAGE_TEXTURE_BINDINGS) {
            _SG_ERROR(METAL_IMAGE_MSL_TEXTURE_SLOT_OUT_OF_RANGE);
            return false;
        }
        if (desc->views[i].storage_buffer.msl_buffer_n >= _SG_MTL_MAX_STAGE_UB_SBUF_BINDINGS) {
            _SG_ERROR(METAL_STORAGEBUFFER_MSL_BUFFER_SLOT_OUT_OF_RANGE);
            return false;
        }
        if (desc->views[i].storage_image.msl_texture_n >= _SG_MTL_MAX_STAGE_TEXTURE_BINDINGS) {
            _SG_ERROR(METAL_STORAGEIMAGE_MSL_TEXTURE_SLOT_OUT_OF_RANGE);
            return false;
        }
    }
    for (size_t i = 0; i < SG_MAX_SAMPLER_BINDSLOTS; i++) {
        if (desc->samplers[i].msl_sampler_n >= _SG_MTL_MAX_STAGE_SAMPLER_BINDINGS) {
            _SG_ERROR(METAL_SAMPLER_MSL_SAMPLER_SLOT_OUT_OF_RANGE);
            return false;
        }
    }
    return true;
}

_SOKOL_PRIVATE sg_resource_state _sg_mtl_create_shader(_sg_shader_t* shd, const sg_shader_desc* desc) {
    SOKOL_ASSERT(shd && desc);

    // do a MSL bindslot range check also in release mode, and if that fails,
    // also fail shader creation
    if (!_sg_mtl_ensure_msl_bindslot_ranges(desc)) {
        return SG_RESOURCESTATE_FAILED;
    }

    shd->mtl.threads_per_threadgroup = MTLSizeMake(
        (NSUInteger)desc->mtl_threads_per_threadgroup.x,
        (NSUInteger)desc->mtl_threads_per_threadgroup.y,
        (NSUInteger)desc->mtl_threads_per_threadgroup.z);

    // copy resource bindslot mappings
    for (size_t i = 0; i < SG_MAX_UNIFORMBLOCK_BINDSLOTS; i++) {
        shd->mtl.ub_buffer_n[i] = desc->uniform_blocks[i].msl_buffer_n;
    }
    for (size_t i = 0; i < SG_MAX_VIEW_BINDSLOTS; i++) {
        const sg_shader_view* view = &desc->views[i];
        SOKOL_ASSERT(0 == shd->mtl.view_buffer_texture_n[i]);
        if (view->storage_buffer.stage != SG_SHADERSTAGE_NONE) {
            shd->mtl.view_buffer_texture_n[i] = view->storage_buffer.msl_buffer_n;
        } else if (view->texture.stage != SG_SHADERSTAGE_NONE) {
            shd->mtl.view_buffer_texture_n[i] = view->texture.msl_texture_n;
        } else if (view->storage_image.stage != SG_SHADERSTAGE_NONE) {
            shd->mtl.view_buffer_texture_n[i] = view->storage_image.msl_texture_n;
        }
    }
    for (size_t i = 0; i < SG_MAX_SAMPLER_BINDSLOTS; i++) {
        shd->mtl.smp_sampler_n[i] = desc->samplers[i].msl_sampler_n;
    }


    // create metal library and function objects
    bool shd_valid = true;
    if (desc->vertex_func.source || desc->vertex_func.bytecode.ptr) {
        shd_valid &= _sg_mtl_create_shader_func(&desc->vertex_func, desc->label, "vs", &shd->mtl.vertex_func);
    }
    if (desc->fragment_func.source || desc->fragment_func.bytecode.ptr) {
        shd_valid &= _sg_mtl_create_shader_func(&desc->fragment_func, desc->label, "fs", &shd->mtl.fragment_func);
    }
    if (desc->compute_func.source || desc->compute_func.bytecode.ptr) {
        shd_valid &= _sg_mtl_create_shader_func(&desc->compute_func, desc->label, "cs", &shd->mtl.compute_func);
    }
    if (!shd_valid) {
        _sg_mtl_discard_shader_func(&shd->mtl.vertex_func);
        _sg_mtl_discard_shader_func(&shd->mtl.fragment_func);
        _sg_mtl_discard_shader_func(&shd->mtl.compute_func);
    }
    return shd_valid ? SG_RESOURCESTATE_VALID : SG_RESOURCESTATE_FAILED;
}

_SOKOL_PRIVATE void _sg_mtl_discard_shader(_sg_shader_t* shd) {
    SOKOL_ASSERT(shd);
    _sg_mtl_discard_shader_func(&shd->mtl.vertex_func);
    _sg_mtl_discard_shader_func(&shd->mtl.fragment_func);
    _sg_mtl_discard_shader_func(&shd->mtl.compute_func);
}

_SOKOL_PRIVATE sg_resource_state _sg_mtl_create_pipeline(_sg_pipeline_t* pip, const sg_pipeline_desc* desc) {
    SOKOL_ASSERT(pip && desc);
    _sg_shader_t* shd = _sg_shader_ref_ptr(&pip->cmn.shader);
    if (pip->cmn.is_compute) {
        NSError* err = NULL;
        MTLComputePipelineDescriptor* cp_desc = [[MTLComputePipelineDescriptor alloc] init];
        cp_desc.computeFunction = _sg_mtl_id(shd->mtl.compute_func.mtl_func);
        cp_desc.threadGroupSizeIsMultipleOfThreadExecutionWidth = true;
        for (size_t i = 0; i < SG_MAX_VIEW_BINDSLOTS; i++) {
            const _sg_shader_view_t* view = &shd->cmn.views[i];
            if (view->view_type != SG_VIEWTYPE_STORAGEBUFFER) {
                continue;
            }
            if (!view->sbuf_readonly) {
                continue;
            }
            SOKOL_ASSERT(view->stage == SG_SHADERSTAGE_COMPUTE);
            const NSUInteger mtl_slot = shd->mtl.view_buffer_texture_n[i];
            SOKOL_ASSERT(mtl_slot < _SG_MTL_MAX_STAGE_BUFFER_BINDINGS);
            cp_desc.buffers[mtl_slot].mutability = MTLMutabilityImmutable;
        }
        #if defined(SOKOL_DEBUG)
            if (desc->label) {
                cp_desc.label = [NSString stringWithFormat:@"%s", desc->label];
            }
        #endif
        id<MTLComputePipelineState> mtl_cps = [_sg.mtl.device
            newComputePipelineStateWithDescriptor:cp_desc
            options:MTLPipelineOptionNone
            reflection:nil
            error:&err];
        _SG_OBJC_RELEASE(cp_desc);
        if (nil == mtl_cps) {
            SOKOL_ASSERT(err);
            _SG_ERROR(METAL_CREATE_CPS_FAILED);
            _SG_LOGMSG(METAL_CREATE_CPS_OUTPUT, [err.localizedDescription UTF8String]);
            return SG_RESOURCESTATE_FAILED;
        }
        pip->mtl.cps = _sg_mtl_add_resource(mtl_cps);
        _SG_OBJC_RELEASE(mtl_cps);
        pip->mtl.threads_per_threadgroup = shd->mtl.threads_per_threadgroup;
    } else {
        sg_primitive_type prim_type = desc->primitive_type;
        pip->mtl.prim_type = _sg_mtl_primitive_type(prim_type);
        pip->mtl.index_size = _sg_mtl_index_size(pip->cmn.index_type);
        if (SG_INDEXTYPE_NONE != pip->cmn.index_type) {
            pip->mtl.index_type = _sg_mtl_index_type(pip->cmn.index_type);
        }
        pip->mtl.cull_mode = _sg_mtl_cull_mode(desc->cull_mode);
        pip->mtl.winding = _sg_mtl_winding(desc->face_winding);
        pip->mtl.stencil_ref = desc->stencil.ref;

        // create vertex-descriptor
        MTLVertexDescriptor* vtx_desc = [MTLVertexDescriptor vertexDescriptor];
        for (NSUInteger attr_index = 0; attr_index < SG_MAX_VERTEX_ATTRIBUTES; attr_index++) {
            const sg_vertex_attr_state* a_state = &desc->layout.attrs[attr_index];
            if (a_state->format == SG_VERTEXFORMAT_INVALID) {
                break;
            }
            SOKOL_ASSERT(a_state->buffer_index < SG_MAX_VERTEXBUFFER_BINDSLOTS);
            SOKOL_ASSERT(pip->cmn.vertex_buffer_layout_active[a_state->buffer_index]);
            vtx_desc.attributes[attr_index].format = _sg_mtl_vertex_format(a_state->format);
            vtx_desc.attributes[attr_index].offset = (NSUInteger)a_state->offset;
            vtx_desc.attributes[attr_index].bufferIndex = _sg_mtl_vertexbuffer_bindslot((size_t)a_state->buffer_index);
        }
        for (NSUInteger layout_index = 0; layout_index < SG_MAX_VERTEXBUFFER_BINDSLOTS; layout_index++) {
            if (pip->cmn.vertex_buffer_layout_active[layout_index]) {
                const sg_vertex_buffer_layout_state* l_state = &desc->layout.buffers[layout_index];
                const NSUInteger mtl_vb_slot = _sg_mtl_vertexbuffer_bindslot(layout_index);
                SOKOL_ASSERT(l_state->stride > 0);
                vtx_desc.layouts[mtl_vb_slot].stride = (NSUInteger)l_state->stride;
                vtx_desc.layouts[mtl_vb_slot].stepFunction = _sg_mtl_step_function(l_state->step_func);
                vtx_desc.layouts[mtl_vb_slot].stepRate = (NSUInteger)l_state->step_rate;
                if (SG_VERTEXSTEP_PER_INSTANCE == l_state->step_func) {
                    // NOTE: not actually used in _sg_mtl_draw()
                    pip->cmn.use_instanced_draw = true;
                }
            }
        }

        // render-pipeline descriptor
        MTLRenderPipelineDescriptor* rp_desc = [[MTLRenderPipelineDescriptor alloc] init];
        rp_desc.vertexDescriptor = vtx_desc;
        SOKOL_ASSERT(shd->mtl.vertex_func.mtl_func != _SG_MTL_INVALID_SLOT_INDEX);
        rp_desc.vertexFunction = _sg_mtl_id(shd->mtl.vertex_func.mtl_func);
        SOKOL_ASSERT(shd->mtl.fragment_func.mtl_func != _SG_MTL_INVALID_SLOT_INDEX);
        rp_desc.fragmentFunction = _sg_mtl_id(shd->mtl.fragment_func.mtl_func);
        rp_desc.rasterSampleCount = (NSUInteger)desc->sample_count;
        rp_desc.alphaToCoverageEnabled = desc->alpha_to_coverage_enabled;
        rp_desc.alphaToOneEnabled = NO;
        rp_desc.rasterizationEnabled = YES;
        rp_desc.depthAttachmentPixelFormat = _sg_mtl_pixel_format(desc->depth.pixel_format);
        if (desc->depth.pixel_format == SG_PIXELFORMAT_DEPTH_STENCIL) {
            rp_desc.stencilAttachmentPixelFormat = _sg_mtl_pixel_format(desc->depth.pixel_format);
        }
        for (NSUInteger i = 0; i < (NSUInteger)desc->color_count; i++) {
            SOKOL_ASSERT(i < SG_MAX_COLOR_ATTACHMENTS);
            const sg_color_target_state* cs = &desc->colors[i];
            rp_desc.colorAttachments[i].pixelFormat = _sg_mtl_pixel_format(cs->pixel_format);
            rp_desc.colorAttachments[i].writeMask = _sg_mtl_color_write_mask(cs->write_mask);
            rp_desc.colorAttachments[i].blendingEnabled = cs->blend.enabled;
            rp_desc.colorAttachments[i].alphaBlendOperation = _sg_mtl_blend_op(cs->blend.op_alpha);
            rp_desc.colorAttachments[i].rgbBlendOperation = _sg_mtl_blend_op(cs->blend.op_rgb);
            rp_desc.colorAttachments[i].destinationAlphaBlendFactor = _sg_mtl_blend_factor(cs->blend.dst_factor_alpha);
            rp_desc.colorAttachments[i].destinationRGBBlendFactor = _sg_mtl_blend_factor(cs->blend.dst_factor_rgb);
            rp_desc.colorAttachments[i].sourceAlphaBlendFactor = _sg_mtl_blend_factor(cs->blend.src_factor_alpha);
            rp_desc.colorAttachments[i].sourceRGBBlendFactor = _sg_mtl_blend_factor(cs->blend.src_factor_rgb);
        }
        // Set buffer mutability for all buffers (vertex buffers and storage buffers).
        // For vertex buffer it is guaranteed that neither the GPU nor CPU will update their content
        // as long as it is in flight (since dynamic buffers are double-buffered, and vertex-buffers
        // are not updated by the GPU).
        // For storage buffer the same double-buffering applies, and if they are applied
        // to the vertex- or fragment-stage must be declared as readonly in the shader.
        for (size_t i = 0; i < SG_MAX_VERTEXBUFFER_BINDSLOTS; i++) {
            if (pip->cmn.vertex_buffer_layout_active[i]) {
                const NSUInteger mtl_slot = _sg_mtl_vertexbuffer_bindslot(i);
                rp_desc.vertexBuffers[mtl_slot].mutability = MTLMutabilityImmutable;
            }
        }
        for (size_t i = 0; i < SG_MAX_VIEW_BINDSLOTS; i++) {
            const _sg_shader_view_t* view = &shd->cmn.views[i];
            if (view->view_type != SG_VIEWTYPE_STORAGEBUFFER) {
                continue;
            }
            const sg_shader_stage stage = view->stage;
            SOKOL_ASSERT(view->stage != SG_SHADERSTAGE_COMPUTE);
            SOKOL_ASSERT(view->sbuf_readonly);
            const NSUInteger mtl_slot = shd->mtl.view_buffer_texture_n[i];
            SOKOL_ASSERT(mtl_slot < _SG_MTL_MAX_STAGE_BUFFER_BINDINGS);
            if (stage == SG_SHADERSTAGE_VERTEX) {
                rp_desc.vertexBuffers[mtl_slot].mutability = MTLMutabilityImmutable;
            } else if (stage == SG_SHADERSTAGE_FRAGMENT) {
                rp_desc.fragmentBuffers[mtl_slot].mutability = MTLMutabilityImmutable;
            }
        }
        #if defined(SOKOL_DEBUG)
            if (desc->label) {
                rp_desc.label = [NSString stringWithFormat:@"%s", desc->label];
            }
        #endif
        NSError* err = NULL;
        id<MTLRenderPipelineState> mtl_rps = [_sg.mtl.device newRenderPipelineStateWithDescriptor:rp_desc error:&err];
        _SG_OBJC_RELEASE(rp_desc);
        if (nil == mtl_rps) {
            SOKOL_ASSERT(err);
            _SG_ERROR(METAL_CREATE_RPS_FAILED);
            _SG_LOGMSG(METAL_CREATE_RPS_OUTPUT, [err.localizedDescription UTF8String]);
            return SG_RESOURCESTATE_FAILED;
        }
        pip->mtl.rps = _sg_mtl_add_resource(mtl_rps);
        _SG_OBJC_RELEASE(mtl_rps);

        // depth-stencil-state
        MTLDepthStencilDescriptor* ds_desc = [[MTLDepthStencilDescriptor alloc] init];
        ds_desc.depthCompareFunction = _sg_mtl_compare_func(desc->depth.compare);
        ds_desc.depthWriteEnabled = desc->depth.write_enabled;
        if (desc->stencil.enabled) {
            const sg_stencil_face_state* sb = &desc->stencil.back;
            ds_desc.backFaceStencil = [[MTLStencilDescriptor alloc] init];
            ds_desc.backFaceStencil.stencilFailureOperation = _sg_mtl_stencil_op(sb->fail_op);
            ds_desc.backFaceStencil.depthFailureOperation = _sg_mtl_stencil_op(sb->depth_fail_op);
            ds_desc.backFaceStencil.depthStencilPassOperation = _sg_mtl_stencil_op(sb->pass_op);
            ds_desc.backFaceStencil.stencilCompareFunction = _sg_mtl_compare_func(sb->compare);
            ds_desc.backFaceStencil.readMask = desc->stencil.read_mask;
            ds_desc.backFaceStencil.writeMask = desc->stencil.write_mask;
            const sg_stencil_face_state* sf = &desc->stencil.front;
            ds_desc.frontFaceStencil = [[MTLStencilDescriptor alloc] init];
            ds_desc.frontFaceStencil.stencilFailureOperation = _sg_mtl_stencil_op(sf->fail_op);
            ds_desc.frontFaceStencil.depthFailureOperation = _sg_mtl_stencil_op(sf->depth_fail_op);
            ds_desc.frontFaceStencil.depthStencilPassOperation = _sg_mtl_stencil_op(sf->pass_op);
            ds_desc.frontFaceStencil.stencilCompareFunction = _sg_mtl_compare_func(sf->compare);
            ds_desc.frontFaceStencil.readMask = desc->stencil.read_mask;
            ds_desc.frontFaceStencil.writeMask = desc->stencil.write_mask;
        }
        #if defined(SOKOL_DEBUG)
            if (desc->label) {
                ds_desc.label = [NSString stringWithFormat:@"%s.dss", desc->label];
            }
        #endif
        id<MTLDepthStencilState> mtl_dss = [_sg.mtl.device newDepthStencilStateWithDescriptor:ds_desc];
        _SG_OBJC_RELEASE(ds_desc);
        if (nil == mtl_dss) {
            _SG_ERROR(METAL_CREATE_DSS_FAILED);
            return SG_RESOURCESTATE_FAILED;
        }
        pip->mtl.dss = _sg_mtl_add_resource(mtl_dss);
        _SG_OBJC_RELEASE(mtl_dss);
    }
    return SG_RESOURCESTATE_VALID;
}

_SOKOL_PRIVATE void _sg_mtl_discard_pipeline(_sg_pipeline_t* pip) {
    SOKOL_ASSERT(pip);
    // it's valid to call release resource with a 'null resource'
    _sg_mtl_release_resource(_sg.frame_index, pip->mtl.cps);
    _sg_mtl_release_resource(_sg.frame_index, pip->mtl.rps);
    _sg_mtl_release_resource(_sg.frame_index, pip->mtl.dss);
}

_SOKOL_PRIVATE sg_resource_state _sg_mtl_create_view(_sg_view_t* view, const sg_view_desc* desc) {
    SOKOL_ASSERT(view && desc);
    _SOKOL_UNUSED(desc);
    if ((SG_VIEWTYPE_TEXTURE == view->cmn.type) || (SG_VIEWTYPE_STORAGEIMAGE == view->cmn.type)) {
        const _sg_image_view_common_t* cmn = &view->cmn.img;
        const _sg_image_t* img = _sg_image_ref_ptr(&cmn->ref);
        SOKOL_ASSERT(cmn->mip_level_count >= 1);
        SOKOL_ASSERT(cmn->slice_count >= 1);
        for (int slot = 0; slot < img->cmn.num_slots; slot++) {
            SOKOL_ASSERT(img->mtl.tex[slot] != _SG_MTL_INVALID_SLOT_INDEX);
            id<MTLTexture> mtl_tex_view = [_sg_mtl_id(img->mtl.tex[slot])
                newTextureViewWithPixelFormat: _sg_mtl_pixel_format(img->cmn.pixel_format)
                textureType: _sg_mtl_texture_type(img->cmn.type, img->cmn.sample_count > 1)
                levels: NSMakeRange((NSUInteger)cmn->mip_level, (NSUInteger)cmn->mip_level_count)
                slices: NSMakeRange((NSUInteger)cmn->slice, (NSUInteger)cmn->slice_count)];
            #if defined(SOKOL_DEBUG)
                if (desc->label) {
                    mtl_tex_view.label = [NSString stringWithFormat:@"%s.%d", desc->label, slot];
                }
            #endif
            view->mtl.tex_view[slot] = _sg_mtl_add_resource(mtl_tex_view);
            _SG_OBJC_RELEASE(mtl_tex_view);
        }
    }
    return SG_RESOURCESTATE_VALID;
}

_SOKOL_PRIVATE void _sg_mtl_discard_view(_sg_view_t* view) {
    SOKOL_ASSERT(view);
    for (size_t i = 0; i < SG_NUM_INFLIGHT_FRAMES; i++) {
        // it's valid to call _sg_mtl_release_resource with a null handle
        _sg_mtl_release_resource(_sg.frame_index, view->mtl.tex_view[i]);
    }
}

_SOKOL_PRIVATE void _sg_mtl_bind_uniform_buffers(void) {
    // In the Metal backend, uniform buffer bindings happen once in sg_begin_pass() and
    // remain valid for the entire pass. Only binding offsets will be updated
    // in sg_apply_uniforms()
    if (_sg.cur_pass.is_compute) {
        SOKOL_ASSERT(nil != _sg.mtl.compute_cmd_encoder);
        for (size_t slot = 0; slot < SG_MAX_UNIFORMBLOCK_BINDSLOTS; slot++) {
            [_sg.mtl.compute_cmd_encoder
                setBuffer:_sg.mtl.uniform_buffers[_sg.mtl.cur_frame_rotate_index]
                offset:0
                atIndex:slot];
        }
    } else {
        SOKOL_ASSERT(nil != _sg.mtl.render_cmd_encoder);
        for (size_t slot = 0; slot < SG_MAX_UNIFORMBLOCK_BINDSLOTS; slot++) {
            [_sg.mtl.render_cmd_encoder
                setVertexBuffer:_sg.mtl.uniform_buffers[_sg.mtl.cur_frame_rotate_index]
                offset:0
                atIndex:slot];
            [_sg.mtl.render_cmd_encoder
                setFragmentBuffer:_sg.mtl.uniform_buffers[_sg.mtl.cur_frame_rotate_index]
                offset:0
                atIndex:slot];
        }
    }
}

_SOKOL_PRIVATE void _sg_mtl_begin_compute_pass(const sg_pass* pass) {
    SOKOL_ASSERT(pass); (void)pass;
    SOKOL_ASSERT(nil != _sg.mtl.cmd_buffer);
    SOKOL_ASSERT(nil == _sg.mtl.compute_cmd_encoder);
    SOKOL_ASSERT(nil == _sg.mtl.render_cmd_encoder);

    _sg.mtl.compute_cmd_encoder = [_sg.mtl.cmd_buffer computeCommandEncoder];
    if (nil == _sg.mtl.compute_cmd_encoder) {
        _sg.cur_pass.valid = false;
        return;
    }

    #if defined(SOKOL_DEBUG)
    if (pass->label) {
        _sg.mtl.compute_cmd_encoder.label = [NSString stringWithUTF8String:pass->label];
    }
    #endif
}

_SOKOL_PRIVATE void _sg_mtl_begin_render_pass(const sg_pass* pass, const _sg_attachments_ptrs_t* atts) {
    SOKOL_ASSERT(pass && atts);
    SOKOL_ASSERT(nil != _sg.mtl.cmd_buffer);
    SOKOL_ASSERT(nil == _sg.mtl.render_cmd_encoder);
    SOKOL_ASSERT(nil == _sg.mtl.compute_cmd_encoder);

    const sg_swapchain* swapchain = &pass->swapchain;
    const sg_pass_action* action = &pass->action;

    MTLRenderPassDescriptor* pass_desc = [MTLRenderPassDescriptor renderPassDescriptor];
    SOKOL_ASSERT(pass_desc);
    if (!atts->empty) {
        // setup pass descriptor for offscreen rendering
        for (NSUInteger i = 0; i < (NSUInteger)atts->num_color_views; i++) {
            const _sg_view_t* color_view = atts->color_views[i];
            SOKOL_ASSERT(color_view);
            const _sg_view_t* resolve_view = atts->resolve_views[i];
            const _sg_image_t* color_img = _sg_image_ref_ptr(&color_view->cmn.img.ref);
            SOKOL_ASSERT(color_img->slot.state == SG_RESOURCESTATE_VALID);
            SOKOL_ASSERT(color_img->cmn.active_slot == 0);
            SOKOL_ASSERT(color_img->mtl.tex[0] != _SG_MTL_INVALID_SLOT_INDEX);
            pass_desc.colorAttachments[i].loadAction = _sg_mtl_load_action(action->colors[i].load_action);
            pass_desc.colorAttachments[i].storeAction = _sg_mtl_store_action(action->colors[i].store_action, resolve_view != 0);
            sg_color c = action->colors[i].clear_value;
            pass_desc.colorAttachments[i].clearColor = MTLClearColorMake(c.r, c.g, c.b, c.a);
            pass_desc.colorAttachments[i].texture = _sg_mtl_id(color_img->mtl.tex[0]);
            pass_desc.colorAttachments[i].level = (NSUInteger)color_view->cmn.img.mip_level;
            switch (color_img->cmn.type) {
                case SG_IMAGETYPE_CUBE:
                case SG_IMAGETYPE_ARRAY:
                    pass_desc.colorAttachments[i].slice = (NSUInteger)color_view->cmn.img.slice;
                    break;
                case SG_IMAGETYPE_3D:
                    pass_desc.colorAttachments[i].depthPlane = (NSUInteger)color_view->cmn.img.slice;
                    break;
                default: break;
            }
            if (resolve_view) {
                const _sg_image_t* resolve_img = _sg_image_ref_ptr(&resolve_view->cmn.img.ref);
                SOKOL_ASSERT(resolve_img->slot.state == SG_RESOURCESTATE_VALID);
                SOKOL_ASSERT(resolve_img->cmn.active_slot == 0);
                SOKOL_ASSERT(resolve_img->mtl.tex[0] != _SG_MTL_INVALID_SLOT_INDEX);
                pass_desc.colorAttachments[i].resolveTexture = _sg_mtl_id(resolve_img->mtl.tex[0]);
                pass_desc.colorAttachments[i].resolveLevel = (NSUInteger)resolve_view->cmn.img.mip_level;
                switch (resolve_img->cmn.type) {
                    case SG_IMAGETYPE_CUBE:
                    case SG_IMAGETYPE_ARRAY:
                        pass_desc.colorAttachments[i].resolveSlice = (NSUInteger)resolve_view->cmn.img.slice;
                        break;
                    case SG_IMAGETYPE_3D:
                        pass_desc.colorAttachments[i].resolveDepthPlane = (NSUInteger)resolve_view->cmn.img.slice;
                        break;
                    default: break;
                }
            }
        }
        if (atts->ds_view) {
            const _sg_view_t* ds_view = atts->ds_view;
            const _sg_image_t* ds_img = _sg_image_ref_ptr(&ds_view->cmn.img.ref);
            SOKOL_ASSERT(ds_img->slot.state == SG_RESOURCESTATE_VALID);
            SOKOL_ASSERT(ds_img->cmn.active_slot == 0);
            SOKOL_ASSERT(ds_img->mtl.tex[0] != _SG_MTL_INVALID_SLOT_INDEX);
            pass_desc.depthAttachment.texture = _sg_mtl_id(ds_img->mtl.tex[0]);
            pass_desc.depthAttachment.loadAction = _sg_mtl_load_action(action->depth.load_action);
            pass_desc.depthAttachment.storeAction = _sg_mtl_store_action(action->depth.store_action, false);
            pass_desc.depthAttachment.clearDepth = action->depth.clear_value;
            switch (ds_img->cmn.type) {
                case SG_IMAGETYPE_CUBE:
                case SG_IMAGETYPE_ARRAY:
                    pass_desc.depthAttachment.slice = (NSUInteger)ds_view->cmn.img.slice;
                    break;
                case SG_IMAGETYPE_3D:
                    pass_desc.depthAttachment.resolveDepthPlane = (NSUInteger)ds_view->cmn.img.slice;
                    break;
                default: break;
            }
            if (_sg_is_depth_stencil_format(ds_img->cmn.pixel_format)) {
                pass_desc.stencilAttachment.texture = _sg_mtl_id(ds_img->mtl.tex[0]);
                pass_desc.stencilAttachment.loadAction = _sg_mtl_load_action(action->stencil.load_action);
                pass_desc.stencilAttachment.storeAction = _sg_mtl_store_action(action->depth.store_action, false);
                pass_desc.stencilAttachment.clearStencil = action->stencil.clear_value;
                switch (ds_img->cmn.type) {
                    case SG_IMAGETYPE_CUBE:
                    case SG_IMAGETYPE_ARRAY:
                        pass_desc.stencilAttachment.slice = (NSUInteger)ds_view->cmn.img.slice;
                        break;
                    case SG_IMAGETYPE_3D:
                        pass_desc.stencilAttachment.resolveDepthPlane = (NSUInteger)ds_view->cmn.img.slice;
                        break;
                    default: break;
                }
            }
        }
    } else {
        // setup pass descriptor for swapchain rendering
        //
        // NOTE: at least in macOS Sonoma this no longer seems to be the case, the
        // current drawable is also valid in a minimized window
        // ===
        // an MTKView current_drawable will not be valid if window is minimized, don't do any rendering in this case
        if (0 == swapchain->metal.current_drawable) {
            _sg.cur_pass.valid = false;
            return;
        }
        // pin the swapchain resources into memory so that they outlive their command buffer
        // (this is necessary because the command buffer doesn't retain references)
        int pass_desc_ref = _sg_mtl_add_resource(pass_desc);
        _sg_mtl_release_resource(_sg.frame_index, pass_desc_ref);

        _sg.mtl.cur_drawable = (__bridge id<CAMetalDrawable>) swapchain->metal.current_drawable;
        if (swapchain->sample_count > 1) {
            // multi-sampling: render into msaa texture, resolve into drawable texture
            id<MTLTexture> msaa_tex = (__bridge id<MTLTexture>) swapchain->metal.msaa_color_texture;
            SOKOL_ASSERT(msaa_tex != nil);
            pass_desc.colorAttachments[0].texture = msaa_tex;
            pass_desc.colorAttachments[0].resolveTexture = _sg.mtl.cur_drawable.texture;
            pass_desc.colorAttachments[0].storeAction = MTLStoreActionMultisampleResolve;
        } else {
            // non-msaa: render into current_drawable
            pass_desc.colorAttachments[0].texture = _sg.mtl.cur_drawable.texture;
            pass_desc.colorAttachments[0].storeAction = MTLStoreActionStore;
        }
        pass_desc.colorAttachments[0].loadAction = _sg_mtl_load_action(action->colors[0].load_action);
        const sg_color c = action->colors[0].clear_value;
        pass_desc.colorAttachments[0].clearColor = MTLClearColorMake(c.r, c.g, c.b, c.a);

        // optional depth-stencil texture
        if (swapchain->metal.depth_stencil_texture) {
            id<MTLTexture> ds_tex = (__bridge id<MTLTexture>) swapchain->metal.depth_stencil_texture;
            SOKOL_ASSERT(ds_tex != nil);
            pass_desc.depthAttachment.texture = ds_tex;
            pass_desc.depthAttachment.storeAction = MTLStoreActionDontCare;
            pass_desc.depthAttachment.loadAction = _sg_mtl_load_action(action->depth.load_action);
            pass_desc.depthAttachment.clearDepth = action->depth.clear_value;
            if (_sg_is_depth_stencil_format(swapchain->depth_format)) {
                pass_desc.stencilAttachment.texture = ds_tex;
                pass_desc.stencilAttachment.storeAction = MTLStoreActionDontCare;
                pass_desc.stencilAttachment.loadAction = _sg_mtl_load_action(action->stencil.load_action);
                pass_desc.stencilAttachment.clearStencil = action->stencil.clear_value;
            }
        }
    }

    // NOTE: at least in macOS Sonoma, the following is no longer the case, a valid
    // render command encoder is also returned in a minimized window
    // ===
    // create a render command encoder, this might return nil if window is minimized
    _sg.mtl.render_cmd_encoder = [_sg.mtl.cmd_buffer renderCommandEncoderWithDescriptor:pass_desc];
    if (nil == _sg.mtl.render_cmd_encoder) {
        _sg.cur_pass.valid = false;
        return;
    }

    #if defined(SOKOL_DEBUG)
    if (pass->label) {
        _sg.mtl.render_cmd_encoder.label = [NSString stringWithUTF8String:pass->label];
    }
    #endif
}

_SOKOL_PRIVATE void _sg_mtl_begin_pass(const sg_pass* pass, const _sg_attachments_ptrs_t* atts) {
    SOKOL_ASSERT(pass && atts);
    SOKOL_ASSERT(_sg.mtl.cmd_queue);
    SOKOL_ASSERT(nil == _sg.mtl.compute_cmd_encoder);
    SOKOL_ASSERT(nil == _sg.mtl.render_cmd_encoder);
    SOKOL_ASSERT(nil == _sg.mtl.cur_drawable);
    _sg_mtl_clear_state_cache();

    // if this is the first pass in the frame, create one command buffer and blit-cmd-encoder for the entire frame
    if (nil == _sg.mtl.cmd_buffer) {
        // block until the oldest frame in flight has finished
        dispatch_semaphore_wait(_sg.mtl.sem, DISPATCH_TIME_FOREVER);
        if (_sg.desc.mtl_use_command_buffer_with_retained_references) {
            _sg.mtl.cmd_buffer = [_sg.mtl.cmd_queue commandBuffer];
        } else {
            _sg.mtl.cmd_buffer = [_sg.mtl.cmd_queue commandBufferWithUnretainedReferences];
        }
        [_sg.mtl.cmd_buffer enqueue];
        [_sg.mtl.cmd_buffer addCompletedHandler:^(id<MTLCommandBuffer> cmd_buf) {
            // NOTE: this code is called on a different thread!
            _SOKOL_UNUSED(cmd_buf);
            dispatch_semaphore_signal(_sg.mtl.sem);
        }];
    }

    // if this is first pass in frame, get uniform buffer base pointer
    if (0 == _sg.mtl.cur_ub_base_ptr) {
        _sg.mtl.cur_ub_base_ptr = (uint8_t*)[_sg.mtl.uniform_buffers[_sg.mtl.cur_frame_rotate_index] contents];
    }

    if (pass->compute) {
        _sg_mtl_begin_compute_pass(pass);
    } else {
        _sg_mtl_begin_render_pass(pass, atts);
    }

    // bind uniform buffers, those bindings remain valid for the entire pass
    if (_sg.cur_pass.valid) {
        _sg_mtl_bind_uniform_buffers();
    }
}

_SOKOL_PRIVATE void _sg_mtl_end_pass(const _sg_attachments_ptrs_t* atts) {
    _SOKOL_UNUSED(atts);
    if (nil != _sg.mtl.render_cmd_encoder) {
        [_sg.mtl.render_cmd_encoder endEncoding];
        // NOTE: MTLRenderCommandEncoder is autoreleased
        _sg.mtl.render_cmd_encoder = nil;
    }
    if (nil != _sg.mtl.compute_cmd_encoder) {
        [_sg.mtl.compute_cmd_encoder endEncoding];
        // NOTE: MTLComputeCommandEncoder is autoreleased
        _sg.mtl.compute_cmd_encoder = nil;
    }
    // if this is a swapchain pass, present the drawable
    if (nil != _sg.mtl.cur_drawable) {
        [_sg.mtl.cmd_buffer presentDrawable:_sg.mtl.cur_drawable];
        _sg.mtl.cur_drawable = nil;
    }
}

_SOKOL_PRIVATE void _sg_mtl_commit(void) {
    SOKOL_ASSERT(nil == _sg.mtl.render_cmd_encoder);
    SOKOL_ASSERT(nil == _sg.mtl.compute_cmd_encoder);
    SOKOL_ASSERT(nil != _sg.mtl.cmd_buffer);

    // commit the frame's command buffer
    [_sg.mtl.cmd_buffer commit];

    // garbage-collect resources pending for release
    _sg_mtl_garbage_collect(_sg.frame_index);

    // rotate uniform buffer slot
    if (++_sg.mtl.cur_frame_rotate_index >= SG_NUM_INFLIGHT_FRAMES) {
        _sg.mtl.cur_frame_rotate_index = 0;
    }
    _sg.mtl.cur_ub_offset = 0;
    _sg.mtl.cur_ub_base_ptr = 0;
    // NOTE: MTLCommandBuffer is autoreleased
    _sg.mtl.cmd_buffer = nil;
}

_SOKOL_PRIVATE void _sg_mtl_apply_viewport(int x, int y, int w, int h, bool origin_top_left) {
    SOKOL_ASSERT(nil != _sg.mtl.render_cmd_encoder);
    SOKOL_ASSERT(_sg.cur_pass.dim.height > 0);
    MTLViewport vp;
    vp.originX = (double) x;
    vp.originY = (double) (origin_top_left ? y : (_sg.cur_pass.dim.height - (y + h)));
    vp.width   = (double) w;
    vp.height  = (double) h;
    vp.znear   = 0.0;
    vp.zfar    = 1.0;
    [_sg.mtl.render_cmd_encoder setViewport:vp];
}

_SOKOL_PRIVATE void _sg_mtl_apply_scissor_rect(int x, int y, int w, int h, bool origin_top_left) {
    SOKOL_ASSERT(nil != _sg.mtl.render_cmd_encoder);
    SOKOL_ASSERT(_sg.cur_pass.dim.width > 0);
    SOKOL_ASSERT(_sg.cur_pass.dim.height > 0);
    // clip against framebuffer rect
    const _sg_recti_t clip = _sg_clipi(x, y, w, h, _sg.cur_pass.dim.width, _sg.cur_pass.dim.height);
    MTLScissorRect r;
    r.x = (NSUInteger)clip.x;
    r.y = (NSUInteger) (origin_top_left ? clip.y : (_sg.cur_pass.dim.height - (clip.y + clip.h)));
    r.width = (NSUInteger)clip.w;
    r.height = (NSUInteger)clip.h;
    [_sg.mtl.render_cmd_encoder setScissorRect:r];
}

_SOKOL_PRIVATE void _sg_mtl_apply_pipeline(_sg_pipeline_t* pip) {
    SOKOL_ASSERT(pip);
    if (!_sg_sref_slot_eql(&_sg.mtl.cache.cur_pip, &pip->slot)) {
        _sg.mtl.cache.cur_pip = _sg_sref(&pip->slot);
        if (pip->cmn.is_compute) {
            SOKOL_ASSERT(_sg.cur_pass.is_compute);
            SOKOL_ASSERT(nil != _sg.mtl.compute_cmd_encoder);
            SOKOL_ASSERT(pip->mtl.cps != _SG_MTL_INVALID_SLOT_INDEX);
            [_sg.mtl.compute_cmd_encoder setComputePipelineState:_sg_mtl_id(pip->mtl.cps)];
        } else {
            SOKOL_ASSERT(!_sg.cur_pass.is_compute);
            SOKOL_ASSERT(nil != _sg.mtl.render_cmd_encoder);
            sg_color c = pip->cmn.blend_color;
            [_sg.mtl.render_cmd_encoder setBlendColorRed:c.r green:c.g blue:c.b alpha:c.a];
            _sg_stats_add(metal.pipeline.num_set_blend_color, 1);
            [_sg.mtl.render_cmd_encoder setCullMode:pip->mtl.cull_mode];
            _sg_stats_add(metal.pipeline.num_set_cull_mode, 1);
            [_sg.mtl.render_cmd_encoder setFrontFacingWinding:pip->mtl.winding];
            _sg_stats_add(metal.pipeline.num_set_front_facing_winding, 1);
            [_sg.mtl.render_cmd_encoder setStencilReferenceValue:pip->mtl.stencil_ref];
            _sg_stats_add(metal.pipeline.num_set_stencil_reference_value, 1);
            [_sg.mtl.render_cmd_encoder setDepthBias:pip->cmn.depth.bias slopeScale:pip->cmn.depth.bias_slope_scale clamp:pip->cmn.depth.bias_clamp];
            _sg_stats_add(metal.pipeline.num_set_depth_bias, 1);
            SOKOL_ASSERT(pip->mtl.rps != _SG_MTL_INVALID_SLOT_INDEX);
            [_sg.mtl.render_cmd_encoder setRenderPipelineState:_sg_mtl_id(pip->mtl.rps)];
            _sg_stats_add(metal.pipeline.num_set_render_pipeline_state, 1);
            SOKOL_ASSERT(pip->mtl.dss != _SG_MTL_INVALID_SLOT_INDEX);
            [_sg.mtl.render_cmd_encoder setDepthStencilState:_sg_mtl_id(pip->mtl.dss)];
            _sg_stats_add(metal.pipeline.num_set_depth_stencil_state, 1);
        }
    }
}

_SOKOL_PRIVATE int _sg_mtl_cache_buf_cmp(const _sg_mtl_cache_buf_t* item, const _sg_slot_t* slot, int active_slot, int offset) {
    int res = _SG_MTL_CACHE_CMP_EQUAL;
    if (!_sg_sref_slot_eql(&item->sref, slot)) {
        res |= _SG_MTL_CACHE_CMP_SREF;
    }
    if (item->active_slot != active_slot) {
        res |= _SG_MTL_CACHE_CMP_ACTIVESLOT;
    }
    if (item->offset != offset) {
        res |= _SG_MTL_CACHE_CMP_OFFSET;
    }
    return res;
}

_SOKOL_PRIVATE void _sg_mtl_cache_buf_upd(_sg_mtl_cache_buf_t* item, const _sg_slot_t* slot, int active_slot, int offset) {
    item->sref = _sg_sref(slot);
    item->offset = offset;
    item->active_slot = active_slot;
}

_SOKOL_PRIVATE int _sg_mtl_cache_tex_cmp(const _sg_mtl_cache_tex_t* item, const _sg_slot_t* slot, int active_slot) {
    int res = _SG_MTL_CACHE_CMP_EQUAL;
    if (!_sg_sref_slot_eql(&item->sref, slot)) {
        res |= _SG_MTL_CACHE_CMP_SREF;
    }
    if (item->active_slot != active_slot) {
        res |= _SG_MTL_CACHE_CMP_ACTIVESLOT;
    }
    return res;
}

_SOKOL_PRIVATE void _sg_mtl_cache_tex_upd(_sg_mtl_cache_tex_t* item, const _sg_slot_t* slot, int active_slot) {
    item->sref = _sg_sref(slot);
    item->active_slot = active_slot;
}


_SOKOL_PRIVATE bool _sg_mtl_apply_bindings(_sg_bindings_ptrs_t* bnd) {
    SOKOL_ASSERT(bnd);
    SOKOL_ASSERT(bnd->pip);
    const _sg_shader_t* shd = _sg_shader_ref_ptr(&bnd->pip->cmn.shader);

    // don't set vertex- and index-buffers in compute passes
    if (!_sg.cur_pass.is_compute) {
        SOKOL_ASSERT(nil != _sg.mtl.render_cmd_encoder);
        // store index buffer binding, this will be needed later in sg_draw()
        _sg.mtl.cache.cur_ibuf = _sg_buffer_ref(bnd->ib);
        _sg.mtl.cache.cur_ibuf_offset = bnd->ib_offset;
        if (bnd->ib) {
            SOKOL_ASSERT(bnd->pip->cmn.index_type != SG_INDEXTYPE_NONE);
        } else {
            SOKOL_ASSERT(bnd->pip->cmn.index_type == SG_INDEXTYPE_NONE);
        }
        // apply vertex buffers
        for (size_t i = 0; i < SG_MAX_VERTEXBUFFER_BINDSLOTS; i++) {
            const _sg_buffer_t* vb = bnd->vbs[i];
            if (vb == 0) {
                continue;
            }
            const NSUInteger mtl_slot = _sg_mtl_vertexbuffer_bindslot(i);
            SOKOL_ASSERT(mtl_slot < _SG_MTL_MAX_STAGE_BUFFER_BINDINGS);
            const int active_slot = vb->cmn.active_slot;
            SOKOL_ASSERT(vb->mtl.buf[active_slot] != _SG_MTL_INVALID_SLOT_INDEX);
            const int offset = bnd->vb_offsets[i];
            _sg_mtl_cache_buf_t* cache_item = &_sg.mtl.cache.cur_vsbufs[i];
            const int cmp = _sg_mtl_cache_buf_cmp(cache_item, &vb->slot, active_slot, offset);
            if (cmp != _SG_MTL_CACHE_CMP_EQUAL) {
                _sg_mtl_cache_buf_upd(cache_item, &vb->slot, active_slot, offset);
                if (0 == (cmp & ~_SG_MTL_CACHE_CMP_OFFSET)) {
                    // only vertex buffer offset has changed
                    [_sg.mtl.render_cmd_encoder setVertexBufferOffset:(NSUInteger)offset atIndex:mtl_slot];
                    _sg_stats_add(metal.bindings.num_set_vertex_buffer_offset, 1);
                } else {
                    [_sg.mtl.render_cmd_encoder setVertexBuffer:_sg_mtl_id(vb->mtl.buf[active_slot]) offset:(NSUInteger)offset atIndex:mtl_slot];
                    _sg_stats_add(metal.bindings.num_set_vertex_buffer, 1);
                }
            } else {
                _sg_stats_add(metal.bindings.num_skip_redundant_vertex_buffer, 1);
            }
        }
    }

    // apply view bindings (textures, storage images, storage buffers)
    for (size_t i = 0; i < SG_MAX_VIEW_BINDSLOTS; i++) {
        const _sg_view_t* view = bnd->views[i];
        if (0 == view) {
            continue;
        }
        const _sg_shader_view_t* shd_view = &shd->cmn.views[i];
        const sg_shader_stage stage = shd_view->stage;
        SOKOL_ASSERT((stage == SG_SHADERSTAGE_VERTEX)
            || (stage == SG_SHADERSTAGE_FRAGMENT)
            || (stage == SG_SHADERSTAGE_COMPUTE));
        SOKOL_ASSERT((shd_view->view_type == SG_VIEWTYPE_TEXTURE)
            || (shd_view->view_type == SG_VIEWTYPE_STORAGEBUFFER)
            || (shd_view->view_type == SG_VIEWTYPE_STORAGEIMAGE));
        const NSUInteger mtl_slot = shd->mtl.view_buffer_texture_n[i];

        // same handling for textures and storage images
        if ((shd_view->view_type == SG_VIEWTYPE_TEXTURE) || (shd_view->view_type == SG_VIEWTYPE_STORAGEIMAGE)) {
            SOKOL_ASSERT(mtl_slot < _SG_MTL_MAX_STAGE_TEXTURE_BINDINGS);
            const int active_slot = _sg_image_ref_ptr(&view->cmn.img.ref)->cmn.active_slot;
            SOKOL_ASSERT(view->mtl.tex_view[active_slot] != _SG_MTL_INVALID_SLOT_INDEX);
            if (stage == SG_SHADERSTAGE_VERTEX) {
                SOKOL_ASSERT(nil != _sg.mtl.render_cmd_encoder);
                _sg_mtl_cache_tex_t* cache_item = &_sg.mtl.cache.cur_vstexs[mtl_slot];
                const int cmp = _sg_mtl_cache_tex_cmp(cache_item, &view->slot, active_slot);
                if (cmp != _SG_MTL_CACHE_CMP_EQUAL) {
                    _sg_mtl_cache_tex_upd(cache_item, &view->slot, active_slot);
                    [_sg.mtl.render_cmd_encoder setVertexTexture:_sg_mtl_id(view->mtl.tex_view[active_slot]) atIndex:mtl_slot];
                    _sg_stats_add(metal.bindings.num_set_vertex_texture, 1);
                } else {
                    _sg_stats_add(metal.bindings.num_skip_redundant_vertex_texture, 1);
                }
            } else if (stage == SG_SHADERSTAGE_FRAGMENT) {
                SOKOL_ASSERT(nil != _sg.mtl.render_cmd_encoder);
                _sg_mtl_cache_tex_t* cache_item = &_sg.mtl.cache.cur_fstexs[mtl_slot];
                const int cmp = _sg_mtl_cache_tex_cmp(cache_item, &view->slot, active_slot);
                if (cmp != _SG_MTL_CACHE_CMP_EQUAL) {
                    _sg_mtl_cache_tex_upd(cache_item, &view->slot, active_slot);
                    [_sg.mtl.render_cmd_encoder setFragmentTexture:_sg_mtl_id(view->mtl.tex_view[active_slot]) atIndex:mtl_slot];
                    _sg_stats_add(metal.bindings.num_set_fragment_texture, 1);
                } else {
                    _sg_stats_add(metal.bindings.num_skip_redundant_fragment_texture, 1);
                }
            } else if (stage == SG_SHADERSTAGE_COMPUTE) {
                SOKOL_ASSERT(nil != _sg.mtl.compute_cmd_encoder);
                _sg_mtl_cache_tex_t* cache_item = &_sg.mtl.cache.cur_cstexs[mtl_slot];
                const int cmp = _sg_mtl_cache_tex_cmp(cache_item, &view->slot, active_slot);
                if (cmp != _SG_MTL_CACHE_CMP_EQUAL) {
                    _sg_mtl_cache_tex_upd(cache_item, &view->slot, active_slot);
                    [_sg.mtl.compute_cmd_encoder setTexture:_sg_mtl_id(view->mtl.tex_view[active_slot]) atIndex:mtl_slot];
                    _sg_stats_add(metal.bindings.num_set_compute_texture, 1);
                } else {
                    _sg_stats_add(metal.bindings.num_skip_redundant_compute_texture, 1);
                }
            } else SOKOL_UNREACHABLE;
        } else if (shd_view->view_type == SG_VIEWTYPE_STORAGEBUFFER) {
            SOKOL_ASSERT(mtl_slot < _SG_MTL_MAX_STAGE_UB_SBUF_BINDINGS);
            const _sg_buffer_t* sbuf = _sg_buffer_ref_ptr(&view->cmn.buf.ref);
            const int active_slot = sbuf->cmn.active_slot;
            SOKOL_ASSERT(sbuf->mtl.buf[sbuf->cmn.active_slot] != _SG_MTL_INVALID_SLOT_INDEX);
            const int offset = view->cmn.buf.offset;
            if (stage == SG_SHADERSTAGE_VERTEX) {
                SOKOL_ASSERT(nil != _sg.mtl.render_cmd_encoder);
                _sg_mtl_cache_buf_t* cache_item = &_sg.mtl.cache.cur_vsbufs[mtl_slot];
                const int cmp = _sg_mtl_cache_buf_cmp(cache_item, &sbuf->slot, active_slot, offset);
                if (cmp != _SG_MTL_CACHE_CMP_EQUAL) {
                    _sg_mtl_cache_buf_upd(cache_item, &sbuf->slot, active_slot, offset);
                    if (0 == (cmp & ~_SG_MTL_CACHE_CMP_OFFSET)) {
                        // only offset has changed
                        [_sg.mtl.render_cmd_encoder setVertexBufferOffset:(NSUInteger)offset atIndex:mtl_slot];
                        _sg_stats_add(metal.bindings.num_set_vertex_buffer_offset, 1);
                    } else {
                        [_sg.mtl.render_cmd_encoder setVertexBuffer:_sg_mtl_id(sbuf->mtl.buf[sbuf->cmn.active_slot]) offset:(NSUInteger)offset atIndex:mtl_slot];
                        _sg_stats_add(metal.bindings.num_set_vertex_buffer, 1);
                    }
                } else {
                    _sg_stats_add(metal.bindings.num_skip_redundant_vertex_buffer, 1);
                }
            } else if (stage == SG_SHADERSTAGE_FRAGMENT) {
                SOKOL_ASSERT(nil != _sg.mtl.render_cmd_encoder);
                _sg_mtl_cache_buf_t* cache_item = &_sg.mtl.cache.cur_fsbufs[mtl_slot];
                const int cmp = _sg_mtl_cache_buf_cmp(cache_item, &sbuf->slot, active_slot, offset);
                if (cmp != _SG_MTL_CACHE_CMP_EQUAL) {
                    _sg_mtl_cache_buf_upd(cache_item, &sbuf->slot, active_slot, offset);
                    if (0 == (cmp & ~_SG_MTL_CACHE_CMP_OFFSET)) {
                        // only offset has changed
                        [_sg.mtl.render_cmd_encoder setFragmentBufferOffset:(NSUInteger)offset atIndex:mtl_slot];
                        _sg_stats_add(metal.bindings.num_set_fragment_buffer_offset, 1);
                    } else {
                        [_sg.mtl.render_cmd_encoder setFragmentBuffer:_sg_mtl_id(sbuf->mtl.buf[active_slot]) offset:(NSUInteger)offset atIndex:mtl_slot];
                        _sg_stats_add(metal.bindings.num_set_fragment_buffer, 1);
                    }
                } else {
                    _sg_stats_add(metal.bindings.num_skip_redundant_fragment_buffer, 1);
                }
            } else if (stage == SG_SHADERSTAGE_COMPUTE) {
                SOKOL_ASSERT(nil != _sg.mtl.compute_cmd_encoder);
                _sg_mtl_cache_buf_t* cache_item = &_sg.mtl.cache.cur_csbufs[mtl_slot];
                const int cmp = _sg_mtl_cache_buf_cmp(cache_item, &sbuf->slot, active_slot, offset);
                if (cmp != _SG_MTL_CACHE_CMP_EQUAL) {
                    _sg_mtl_cache_buf_upd(cache_item, &sbuf->slot, active_slot, offset);
                    if (0 == (cmp & ~_SG_MTL_CACHE_CMP_OFFSET)) {
                        // only offset has changed
                        [_sg.mtl.compute_cmd_encoder setBufferOffset:(NSUInteger)offset atIndex:mtl_slot];
                        _sg_stats_add(metal.bindings.num_set_compute_buffer_offset, 1);
                    } else {
                        [_sg.mtl.compute_cmd_encoder setBuffer:_sg_mtl_id(sbuf->mtl.buf[active_slot]) offset:(NSUInteger)offset atIndex:mtl_slot];
                        _sg_stats_add(metal.bindings.num_set_compute_buffer, 1);
                    }
                } else {
                    _sg_stats_add(metal.bindings.num_skip_redundant_compute_buffer, 1);
                }
            }
        } else SOKOL_UNREACHABLE;
    }

    // apply sampler bindings
    for (size_t i = 0; i < SG_MAX_SAMPLER_BINDSLOTS; i++) {
        const _sg_sampler_t* smp = bnd->smps[i];
        if (smp == 0) {
            continue;
        }
        SOKOL_ASSERT(smp->mtl.sampler_state != _SG_MTL_INVALID_SLOT_INDEX);
        const sg_shader_stage stage = shd->cmn.samplers[i].stage;
        SOKOL_ASSERT((stage == SG_SHADERSTAGE_VERTEX) || (stage == SG_SHADERSTAGE_FRAGMENT) || (stage == SG_SHADERSTAGE_COMPUTE));
        const NSUInteger mtl_slot = shd->mtl.smp_sampler_n[i];
        SOKOL_ASSERT(mtl_slot < _SG_MTL_MAX_STAGE_SAMPLER_BINDINGS);
        if (stage == SG_SHADERSTAGE_VERTEX) {
            SOKOL_ASSERT(nil != _sg.mtl.render_cmd_encoder);
            if (!_sg_sref_slot_eql(&_sg.mtl.cache.cur_vssmps[mtl_slot], &smp->slot)) {
                _sg.mtl.cache.cur_vssmps[mtl_slot] = _sg_sref(&smp->slot);
                [_sg.mtl.render_cmd_encoder setVertexSamplerState:_sg_mtl_id(smp->mtl.sampler_state) atIndex:mtl_slot];
                _sg_stats_add(metal.bindings.num_set_vertex_sampler_state, 1);
            } else {
                _sg_stats_add(metal.bindings.num_skip_redundant_vertex_sampler_state, 1);
            }
        } else if (stage == SG_SHADERSTAGE_FRAGMENT) {
            SOKOL_ASSERT(nil != _sg.mtl.render_cmd_encoder);
            if (!_sg_sref_slot_eql(&_sg.mtl.cache.cur_fssmps[mtl_slot], &smp->slot)) {
                _sg.mtl.cache.cur_fssmps[mtl_slot] = _sg_sref(&smp->slot);
                [_sg.mtl.render_cmd_encoder setFragmentSamplerState:_sg_mtl_id(smp->mtl.sampler_state) atIndex:mtl_slot];
                _sg_stats_add(metal.bindings.num_set_fragment_sampler_state, 1);
            } else {
                _sg_stats_add(metal.bindings.num_skip_redundant_fragment_sampler_state, 1);
            }
        } else if (stage == SG_SHADERSTAGE_COMPUTE) {
            SOKOL_ASSERT(nil != _sg.mtl.compute_cmd_encoder);
            if (!_sg_sref_slot_eql(&_sg.mtl.cache.cur_cssmps[mtl_slot], &smp->slot)) {
                _sg.mtl.cache.cur_cssmps[mtl_slot] = _sg_sref(&smp->slot);
                [_sg.mtl.compute_cmd_encoder setSamplerState:_sg_mtl_id(smp->mtl.sampler_state) atIndex:mtl_slot];
                _sg_stats_add(metal.bindings.num_set_compute_sampler_state, 1);
            } else {
                _sg_stats_add(metal.bindings.num_skip_redundant_compute_sampler_state, 1);
            }
        } else SOKOL_UNREACHABLE;
    }
    return true;
}

_SOKOL_PRIVATE void _sg_mtl_apply_uniforms(int ub_slot, const sg_range* data) {
    SOKOL_ASSERT((ub_slot >= 0) && (ub_slot < SG_MAX_UNIFORMBLOCK_BINDSLOTS));
    SOKOL_ASSERT(((size_t)_sg.mtl.cur_ub_offset + data->size) <= (size_t)_sg.mtl.ub_size);
    SOKOL_ASSERT((_sg.mtl.cur_ub_offset & (_SG_MTL_UB_ALIGN-1)) == 0);
    const _sg_pipeline_t* pip = _sg_pipeline_ref_ptr(&_sg.cur_pip);
    SOKOL_ASSERT(pip);
    const _sg_shader_t* shd = _sg_shader_ref_ptr(&pip->cmn.shader);
    SOKOL_ASSERT(data->size == shd->cmn.uniform_blocks[ub_slot].size);

    const sg_shader_stage stage = shd->cmn.uniform_blocks[ub_slot].stage;
    const NSUInteger mtl_slot = shd->mtl.ub_buffer_n[ub_slot];

    // copy to global uniform buffer, record offset into cmd encoder, and advance offset
    uint8_t* dst = &_sg.mtl.cur_ub_base_ptr[_sg.mtl.cur_ub_offset];
    memcpy(dst, data->ptr, data->size);
    if (stage == SG_SHADERSTAGE_VERTEX) {
        SOKOL_ASSERT(nil != _sg.mtl.render_cmd_encoder);
        [_sg.mtl.render_cmd_encoder setVertexBufferOffset:(NSUInteger)_sg.mtl.cur_ub_offset atIndex:mtl_slot];
        _sg_stats_add(metal.uniforms.num_set_vertex_buffer_offset, 1);
    } else if (stage == SG_SHADERSTAGE_FRAGMENT) {
        SOKOL_ASSERT(nil != _sg.mtl.render_cmd_encoder);
        [_sg.mtl.render_cmd_encoder setFragmentBufferOffset:(NSUInteger)_sg.mtl.cur_ub_offset atIndex:mtl_slot];
        _sg_stats_add(metal.uniforms.num_set_fragment_buffer_offset, 1);
    } else if (stage == SG_SHADERSTAGE_COMPUTE) {
        SOKOL_ASSERT(nil != _sg.mtl.compute_cmd_encoder);
        [_sg.mtl.compute_cmd_encoder setBufferOffset:(NSUInteger)_sg.mtl.cur_ub_offset atIndex:mtl_slot];
        _sg_stats_add(metal.uniforms.num_set_compute_buffer_offset, 1);
    } else {
        SOKOL_UNREACHABLE;
    }
    _sg.mtl.cur_ub_offset = _sg_roundup(_sg.mtl.cur_ub_offset + (int)data->size, _SG_MTL_UB_ALIGN);
}

_SOKOL_PRIVATE void _sg_mtl_draw(int base_element, int num_elements, int num_instances) {
    SOKOL_ASSERT(nil != _sg.mtl.render_cmd_encoder);
    const _sg_pipeline_t* pip = _sg_pipeline_ref_ptr(&_sg.cur_pip);
    SOKOL_ASSERT(pip);
    if (SG_INDEXTYPE_NONE != pip->cmn.index_type) {
        // indexed rendering
        const _sg_buffer_t* ib = _sg_buffer_ref_ptr(&_sg.mtl.cache.cur_ibuf);
        SOKOL_ASSERT(ib && (ib->mtl.buf[ib->cmn.active_slot] != _SG_MTL_INVALID_SLOT_INDEX));
        const NSUInteger index_buffer_offset = (NSUInteger) (_sg.mtl.cache.cur_ibuf_offset + base_element * pip->mtl.index_size);
        [_sg.mtl.render_cmd_encoder drawIndexedPrimitives:pip->mtl.prim_type
            indexCount:(NSUInteger)num_elements
            indexType:pip->mtl.index_type
            indexBuffer:_sg_mtl_id(ib->mtl.buf[ib->cmn.active_slot])
            indexBufferOffset:index_buffer_offset
            instanceCount:(NSUInteger)num_instances];
    } else {
        // non-indexed rendering
        [_sg.mtl.render_cmd_encoder drawPrimitives:pip->mtl.prim_type
            vertexStart:(NSUInteger)base_element
            vertexCount:(NSUInteger)num_elements
            instanceCount:(NSUInteger)num_instances];
    }
}

_SOKOL_PRIVATE void _sg_mtl_dispatch(int num_groups_x, int num_groups_y, int num_groups_z) {
    SOKOL_ASSERT(nil != _sg.mtl.compute_cmd_encoder);
    const _sg_pipeline_t* pip = _sg_pipeline_ref_ptr(&_sg.cur_pip);
    SOKOL_ASSERT(pip);
    const MTLSize thread_groups = MTLSizeMake(
        (NSUInteger)num_groups_x,
        (NSUInteger)num_groups_y,
        (NSUInteger)num_groups_z);
    const MTLSize threads_per_threadgroup = pip->mtl.threads_per_threadgroup;
    [_sg.mtl.compute_cmd_encoder dispatchThreadgroups:thread_groups threadsPerThreadgroup:threads_per_threadgroup];
}

_SOKOL_PRIVATE void _sg_mtl_update_buffer(_sg_buffer_t* buf, const sg_range* data) {
    SOKOL_ASSERT(buf && data && data->ptr && (data->size > 0));
    if (++buf->cmn.active_slot >= buf->cmn.num_slots) {
        buf->cmn.active_slot = 0;
    }
    __unsafe_unretained id<MTLBuffer> mtl_buf = _sg_mtl_id(buf->mtl.buf[buf->cmn.active_slot]);
    void* dst_ptr = [mtl_buf contents];
    memcpy(dst_ptr, data->ptr, data->size);
    #if defined(_SG_TARGET_MACOS)
    if (_sg_mtl_resource_options_storage_mode_managed_or_shared() == MTLResourceStorageModeManaged) {
        [mtl_buf didModifyRange:NSMakeRange(0, data->size)];
    }
    #endif
}

_SOKOL_PRIVATE void _sg_mtl_append_buffer(_sg_buffer_t* buf, const sg_range* data, bool new_frame) {
    SOKOL_ASSERT(buf && data && data->ptr && (data->size > 0));
    if (new_frame) {
        if (++buf->cmn.active_slot >= buf->cmn.num_slots) {
            buf->cmn.active_slot = 0;
        }
    }
    __unsafe_unretained id<MTLBuffer> mtl_buf = _sg_mtl_id(buf->mtl.buf[buf->cmn.active_slot]);
    uint8_t* dst_ptr = (uint8_t*) [mtl_buf contents];
    dst_ptr += buf->cmn.append_pos;
    memcpy(dst_ptr, data->ptr, data->size);
    #if defined(_SG_TARGET_MACOS)
    if (_sg_mtl_resource_options_storage_mode_managed_or_shared() == MTLResourceStorageModeManaged) {
        [mtl_buf didModifyRange:NSMakeRange((NSUInteger)buf->cmn.append_pos, (NSUInteger)data->size)];
    }
    #endif
}

_SOKOL_PRIVATE void _sg_mtl_update_image(_sg_image_t* img, const sg_image_data* data) {
    SOKOL_ASSERT(img && data);
    if (++img->cmn.active_slot >= img->cmn.num_slots) {
        img->cmn.active_slot = 0;
    }
    __unsafe_unretained id<MTLTexture> mtl_tex = _sg_mtl_id(img->mtl.tex[img->cmn.active_slot]);
    _sg_mtl_copy_image_data(img, mtl_tex, data);
}

_SOKOL_PRIVATE void _sg_mtl_push_debug_group(const char* name) {
    SOKOL_ASSERT(name);
    if (_sg.mtl.render_cmd_encoder) {
        [_sg.mtl.render_cmd_encoder pushDebugGroup:[NSString stringWithUTF8String:name]];
    } else if (_sg.mtl.compute_cmd_encoder) {
        [_sg.mtl.compute_cmd_encoder pushDebugGroup:[NSString stringWithUTF8String:name]];
    }
}

_SOKOL_PRIVATE void _sg_mtl_pop_debug_group(void) {
    if (_sg.mtl.render_cmd_encoder) {
        [_sg.mtl.render_cmd_encoder popDebugGroup];
    } else if (_sg.mtl.compute_cmd_encoder) {
        [_sg.mtl.compute_cmd_encoder popDebugGroup];
    }
}

// ██     ██ ███████ ██████   ██████  ██████  ██    ██     ██████   █████   ██████ ██   ██ ███████ ███    ██ ██████
// ██     ██ ██      ██   ██ ██       ██   ██ ██    ██     ██   ██ ██   ██ ██      ██  ██  ██      ████   ██ ██   ██
// ██  █  ██ █████   ██████  ██   ███ ██████  ██    ██     ██████  ███████ ██      █████   █████   ██ ██  ██ ██   ██
// ██ ███ ██ ██      ██   ██ ██    ██ ██      ██    ██     ██   ██ ██   ██ ██      ██  ██  ██      ██  ██ ██ ██   ██
//  ███ ███  ███████ ██████   ██████  ██       ██████      ██████  ██   ██  ██████ ██   ██ ███████ ██   ████ ██████
//
// >>webgpu
// >>generic backend
static inline void _sg_setup_backend(const sg_desc* desc) {
    #elif defined(SOKOL_METAL)
    _sg_mtl_setup_backend(desc);    #elif defined(SOKOL_DUMMY_BACKEND)
    _sg_dummy_setup_backend(desc);
    #else
    #error("INVALID BACKEND");
    #endif
}

static inline void _sg_discard_backend(void) {
    #elif defined(SOKOL_METAL)
    _sg_mtl_discard_backend();    #elif defined(SOKOL_DUMMY_BACKEND)
    _sg_dummy_discard_backend();
    #else
    #error("INVALID BACKEND");
    #endif
}

static inline void _sg_reset_state_cache(void) {
    #elif defined(SOKOL_METAL)
    _sg_mtl_reset_state_cache();    #elif defined(SOKOL_DUMMY_BACKEND)
    _sg_dummy_reset_state_cache();
    #else
    #error("INVALID BACKEND");
    #endif
}

static inline sg_resource_state _sg_create_buffer(_sg_buffer_t* buf, const sg_buffer_desc* desc) {
    #elif defined(SOKOL_METAL)
    return _sg_mtl_create_buffer(buf, desc);    #elif defined(SOKOL_DUMMY_BACKEND)
    return _sg_dummy_create_buffer(buf, desc);
    #else
    #error("INVALID BACKEND");
    #endif
}

static inline void _sg_discard_buffer(_sg_buffer_t* buf) {
    #elif defined(SOKOL_METAL)
    _sg_mtl_discard_buffer(buf);    #elif defined(SOKOL_DUMMY_BACKEND)
    _sg_dummy_discard_buffer(buf);
    #else
    #error("INVALID BACKEND");
    #endif
}

static inline sg_resource_state _sg_create_image(_sg_image_t* img, const sg_image_desc* desc) {
    #elif defined(SOKOL_METAL)
    return _sg_mtl_create_image(img, desc);    #elif defined(SOKOL_DUMMY_BACKEND)
    return _sg_dummy_create_image(img, desc);
    #else
    #error("INVALID BACKEND");
    #endif
}

static inline void _sg_discard_image(_sg_image_t* img) {
    #elif defined(SOKOL_METAL)
    _sg_mtl_discard_image(img);    #elif defined(SOKOL_DUMMY_BACKEND)
    _sg_dummy_discard_image(img);
    #else
    #error("INVALID BACKEND");
    #endif
}

static inline sg_resource_state _sg_create_sampler(_sg_sampler_t* smp, const sg_sampler_desc* desc) {
    #elif defined(SOKOL_METAL)
    return _sg_mtl_create_sampler(smp, desc);    #elif defined(SOKOL_DUMMY_BACKEND)
    return _sg_dummy_create_sampler(smp, desc);
    #else
    #error("INVALID BACKEND");
    #endif
}

static inline void _sg_discard_sampler(_sg_sampler_t* smp) {
    #elif defined(SOKOL_METAL)
    _sg_mtl_discard_sampler(smp);    #elif defined(SOKOL_DUMMY_BACKEND)
    _sg_dummy_discard_sampler(smp);
    #else
    #error("INVALID BACKEND");
    #endif
}

static inline sg_resource_state _sg_create_shader(_sg_shader_t* shd, const sg_shader_desc* desc) {
    #elif defined(SOKOL_METAL)
    return _sg_mtl_create_shader(shd, desc);    #elif defined(SOKOL_DUMMY_BACKEND)
    return _sg_dummy_create_shader(shd, desc);
    #else
    #error("INVALID BACKEND");
    #endif
}

static inline void _sg_discard_shader(_sg_shader_t* shd) {
    #elif defined(SOKOL_METAL)
    _sg_mtl_discard_shader(shd);    #elif defined(SOKOL_DUMMY_BACKEND)
    _sg_dummy_discard_shader(shd);
    #else
    #error("INVALID BACKEND");
    #endif
}

static inline sg_resource_state _sg_create_pipeline(_sg_pipeline_t* pip, const sg_pipeline_desc* desc) {
    #elif defined(SOKOL_METAL)
    return _sg_mtl_create_pipeline(pip, desc);    #elif defined(SOKOL_DUMMY_BACKEND)
    return _sg_dummy_create_pipeline(pip, desc);
    #else
    #error("INVALID BACKEND");
    #endif
}

static inline void _sg_discard_pipeline(_sg_pipeline_t* pip) {
    #elif defined(SOKOL_METAL)
    _sg_mtl_discard_pipeline(pip);    #elif defined(SOKOL_DUMMY_BACKEND)
    _sg_dummy_discard_pipeline(pip);
    #else
    #error("INVALID BACKEND");
    #endif
}

static inline sg_resource_state _sg_create_view(_sg_view_t* view, const sg_view_desc* desc) {
    #elif defined(SOKOL_METAL)
    return _sg_mtl_create_view(view, desc);    #elif defined(SOKOL_DUMMY_BACKEND)
    return _sg_dummy_create_view(view, desc);
    #else
    #error("INVALID BACKEND");
    #endif
}

static inline void _sg_discard_view(_sg_view_t* view) {
    #elif defined(SOKOL_METAL)
    _sg_mtl_discard_view(view);    #elif defined(SOKOL_DUMMY_BACKEND)
    _sg_dummy_discard_view(view);
    #else
    #error("INVALID BACKEND");
    #endif
}

static inline void _sg_begin_pass(const sg_pass* pass, const _sg_attachments_ptrs_t* atts) {
    #elif defined(SOKOL_METAL)
    _sg_mtl_begin_pass(pass, atts);    #elif defined(SOKOL_DUMMY_BACKEND)
    _sg_dummy_begin_pass(pass, atts);
    #else
    #error("INVALID BACKEND");
    #endif
}

static inline void _sg_end_pass(const _sg_attachments_ptrs_t* atts) {
    #elif defined(SOKOL_METAL)
    _sg_mtl_end_pass(atts);    #elif defined(SOKOL_DUMMY_BACKEND)
    _sg_dummy_end_pass(atts);
    #else
    #error("INVALID BACKEND");
    #endif
}

static inline void _sg_apply_viewport(int x, int y, int w, int h, bool origin_top_left) {
    #elif defined(SOKOL_METAL)
    _sg_mtl_apply_viewport(x, y, w, h, origin_top_left);    #elif defined(SOKOL_DUMMY_BACKEND)
    _sg_dummy_apply_viewport(x, y, w, h, origin_top_left);
    #else
    #error("INVALID BACKEND");
    #endif
}

static inline void _sg_apply_scissor_rect(int x, int y, int w, int h, bool origin_top_left) {
    #elif defined(SOKOL_METAL)
    _sg_mtl_apply_scissor_rect(x, y, w, h, origin_top_left);    #elif defined(SOKOL_DUMMY_BACKEND)
    _sg_dummy_apply_scissor_rect(x, y, w, h, origin_top_left);
    #else
    #error("INVALID BACKEND");
    #endif
}

static inline void _sg_apply_pipeline(_sg_pipeline_t* pip) {
    #elif defined(SOKOL_METAL)
    _sg_mtl_apply_pipeline(pip);    #elif defined(SOKOL_DUMMY_BACKEND)
    _sg_dummy_apply_pipeline(pip);
    #else
    #error("INVALID BACKEND");
    #endif
}

static inline bool _sg_apply_bindings(_sg_bindings_ptrs_t* bnd) {
    #elif defined(SOKOL_METAL)
    return _sg_mtl_apply_bindings(bnd);    #elif defined(SOKOL_DUMMY_BACKEND)
    return _sg_dummy_apply_bindings(bnd);
    #else
    #error("INVALID BACKEND");
    #endif
}

static inline void _sg_apply_uniforms(int ub_slot, const sg_range* data) {
    #elif defined(SOKOL_METAL)
    _sg_mtl_apply_uniforms(ub_slot, data);    #elif defined(SOKOL_DUMMY_BACKEND)
    _sg_dummy_apply_uniforms(ub_slot, data);
    #else
    #error("INVALID BACKEND");
    #endif
}

static inline void _sg_draw(int base_element, int num_elements, int num_instances) {
    #elif defined(SOKOL_METAL)
    _sg_mtl_draw(base_element, num_elements, num_instances);    #elif defined(SOKOL_DUMMY_BACKEND)
    _sg_dummy_draw(base_element, num_elements, num_instances);
    #else
    #error("INVALID BACKEND");
    #endif
}

static inline void _sg_dispatch(int num_groups_x, int num_groups_y, int num_groups_z) {
    #elif defined(SOKOL_METAL)
    _sg_mtl_dispatch(num_groups_x, num_groups_y, num_groups_z);    #elif defined(SOKOL_DUMMY_BACKEND)
    _sg_dummy_dispatch(num_groups_x, num_groups_y, num_groups_z);
    #else
    #error("INVALID BACKEND");
    #endif
}

static inline void _sg_commit(void) {
    #elif defined(SOKOL_METAL)
    _sg_mtl_commit();    #elif defined(SOKOL_DUMMY_BACKEND)
    _sg_dummy_commit();
    #else
    #error("INVALID BACKEND");
    #endif
}

static inline void _sg_update_buffer(_sg_buffer_t* buf, const sg_range* data) {
    #elif defined(SOKOL_METAL)
    _sg_mtl_update_buffer(buf, data);    #elif defined(SOKOL_DUMMY_BACKEND)
    _sg_dummy_update_buffer(buf, data);
    #else
    #error("INVALID BACKEND");
    #endif
}

static inline void _sg_append_buffer(_sg_buffer_t* buf, const sg_range* data, bool new_frame) {
    #elif defined(SOKOL_METAL)
    _sg_mtl_append_buffer(buf, data, new_frame);    #elif defined(SOKOL_DUMMY_BACKEND)
    _sg_dummy_append_buffer(buf, data, new_frame);
    #else
    #error("INVALID BACKEND");
    #endif
}

static inline void _sg_update_image(_sg_image_t* img, const sg_image_data* data) {
    #elif defined(SOKOL_METAL)
    _sg_mtl_update_image(img, data);    #elif defined(SOKOL_DUMMY_BACKEND)
    _sg_dummy_update_image(img, data);
    #else
    #error("INVALID BACKEND");
    #endif
}

static inline void _sg_push_debug_group(const char* name) {
    #if defined(SOKOL_METAL)
    _sg_mtl_push_debug_group(name);
    #else
    _SOKOL_UNUSED(name);
    #endif
}

static inline void _sg_pop_debug_group(void) {
    #if defined(SOKOL_METAL)
    _sg_mtl_pop_debug_group();
    #endif
}

// ██    ██  █████  ██      ██ ██████   █████  ████████ ██  ██████  ███    ██
// ██    ██ ██   ██ ██      ██ ██   ██ ██   ██    ██    ██ ██    ██ ████   ██
// ██    ██ ███████ ██      ██ ██   ██ ███████    ██    ██ ██    ██ ██ ██  ██
//  ██  ██  ██   ██ ██      ██ ██   ██ ██   ██    ██    ██ ██    ██ ██  ██ ██
//   ████   ██   ██ ███████ ██ ██████  ██   ██    ██    ██  ██████  ██   ████
//
// >>validation
#if defined(SOKOL_DEBUG)
_SOKOL_PRIVATE void _sg_validate_begin(void) {
    _sg.validate_error = SG_LOGITEM_OK;
}

_SOKOL_PRIVATE bool _sg_validate_end(void) {
    if (_sg.validate_error != SG_LOGITEM_OK) {
        #if !defined(SOKOL_VALIDATE_NON_FATAL)
            _SG_PANIC(VALIDATION_FAILED);
            return false;
        #else
            return false;
        #endif
    } else {
        return true;
    }
}
#endif

_SOKOL_PRIVATE bool _sg_one(bool b0, bool b1, bool b2) {
    return (b0 && !b1 && !b2) || (!b0 && b1 && !b2) || (!b0 && !b1 && b2);
}

_SOKOL_PRIVATE bool _sg_validate_buffer_desc(const sg_buffer_desc* desc) {
    #if !defined(SOKOL_DEBUG)
        _SOKOL_UNUSED(desc);
        return true;
    #else
        if (_sg.desc.disable_validation) {
            return true;
        }
        SOKOL_ASSERT(desc);
        _sg_validate_begin();
        _SG_VALIDATE(desc->_start_canary == 0, VALIDATE_BUFFERDESC_CANARY);
        _SG_VALIDATE(desc->_end_canary == 0, VALIDATE_BUFFERDESC_CANARY);
        _SG_VALIDATE(desc->size > 0, VALIDATE_BUFFERDESC_EXPECT_NONZERO_SIZE);
        _SG_VALIDATE(_sg_one(desc->usage.immutable, desc->usage.dynamic_update, desc->usage.stream_update), VALIDATE_BUFFERDESC_IMMUTABLE_DYNAMIC_STREAM);
        if (_sg.features.separate_buffer_types) {
            _SG_VALIDATE(_sg_one(desc->usage.vertex_buffer, desc->usage.index_buffer, desc->usage.storage_buffer), VALIDATE_BUFFERDESC_SEPARATE_BUFFER_TYPES);
        }
        bool injected = (0 != desc->gl_buffers[0]) ||
                        (0 != desc->mtl_buffers[0]) ||
                        (0 != desc->d3d11_buffer) ||
                        (0 != desc->wgpu_buffer);
        if (!injected && desc->usage.immutable) {
            if (desc->data.ptr) {
                _SG_VALIDATE(desc->size == desc->data.size, VALIDATE_BUFFERDESC_EXPECT_MATCHING_DATA_SIZE);
            } else {
                _SG_VALIDATE(desc->usage.storage_buffer, VALIDATE_BUFFERDESC_EXPECT_DATA);
                _SG_VALIDATE(desc->data.size == 0, VALIDATE_BUFFERDESC_EXPECT_ZERO_DATA_SIZE);
            }
        } else {
            _SG_VALIDATE(0 == desc->data.ptr, VALIDATE_BUFFERDESC_EXPECT_NO_DATA);
            _SG_VALIDATE(desc->data.size == 0, VALIDATE_BUFFERDESC_EXPECT_ZERO_DATA_SIZE);
        }
        if (desc->usage.storage_buffer) {
            _SG_VALIDATE(_sg.features.compute, VALIDATE_BUFFERDESC_STORAGEBUFFER_SUPPORTED);
            _SG_VALIDATE(_sg_multiple_u64(desc->size, 4), VALIDATE_BUFFERDESC_STORAGEBUFFER_SIZE_MULTIPLE_4);
        }
        return _sg_validate_end();
    #endif
}

_SOKOL_PRIVATE void _sg_validate_image_data(const sg_image_data* data, sg_pixel_format fmt, int width, int height, int num_faces, int num_mips, int num_slices) {
    #if !defined(SOKOL_DEBUG)
        _SOKOL_UNUSED(data);
        _SOKOL_UNUSED(fmt);
        _SOKOL_UNUSED(width);
        _SOKOL_UNUSED(height);
        _SOKOL_UNUSED(num_faces);
        _SOKOL_UNUSED(num_mips);
        _SOKOL_UNUSED(num_slices);
    #else
        for (int face_index = 0; face_index < num_faces; face_index++) {
            for (int mip_index = 0; mip_index < num_mips; mip_index++) {
                const bool has_data = data->subimage[face_index][mip_index].ptr != 0;
                const bool has_size = data->subimage[face_index][mip_index].size > 0;
                _SG_VALIDATE(has_data && has_size, VALIDATE_IMAGEDATA_NODATA);
                const int mip_width = _sg_miplevel_dim(width, mip_index);
                const int mip_height = _sg_miplevel_dim(height, mip_index);
                const int bytes_per_slice = _sg_surface_pitch(fmt, mip_width, mip_height, 1);
                const int expected_size = bytes_per_slice * num_slices;
                _SG_VALIDATE(expected_size == (int)data->subimage[face_index][mip_index].size, VALIDATE_IMAGEDATA_DATA_SIZE);
            }
        }
    #endif
}

_SOKOL_PRIVATE bool _sg_validate_image_desc(const sg_image_desc* desc) {
    #if !defined(SOKOL_DEBUG)
        _SOKOL_UNUSED(desc);
        return true;
    #else
        if (_sg.desc.disable_validation) {
            return true;
        }
        const sg_image_usage* usg = &desc->usage;
        const bool any_attachment = usg->color_attachment || usg->resolve_attachment || usg->depth_stencil_attachment;
        SOKOL_ASSERT(desc);
        _sg_validate_begin();
        _SG_VALIDATE(desc->_start_canary == 0, VALIDATE_IMAGEDESC_CANARY);
        _SG_VALIDATE(desc->_end_canary == 0, VALIDATE_IMAGEDESC_CANARY);
        _SG_VALIDATE(_sg_one(usg->immutable, usg->dynamic_update, usg->stream_update), VALIDATE_IMAGEDESC_IMMUTABLE_DYNAMIC_STREAM);
        _SG_VALIDATE(desc->width > 0, VALIDATE_IMAGEDESC_WIDTH);
        _SG_VALIDATE(desc->height > 0, VALIDATE_IMAGEDESC_HEIGHT);
        const sg_pixel_format fmt = desc->pixel_format;
        const bool injected = (0 != desc->gl_textures[0]) ||
                              (0 != desc->mtl_textures[0]) ||
                              (0 != desc->d3d11_texture) ||
                              (0 != desc->wgpu_texture);
        if (_sg_is_depth_or_depth_stencil_format(fmt)) {
            _SG_VALIDATE(desc->type != SG_IMAGETYPE_3D, VALIDATE_IMAGEDESC_DEPTH_3D_IMAGE);
        }
        if (any_attachment || usg->storage_image) {
            SOKOL_ASSERT(((int)fmt >= 0) && ((int)fmt < _SG_PIXELFORMAT_NUM));
            _SG_VALIDATE(usg->immutable, VALIDATE_IMAGEDESC_ATTACHMENT_EXPECT_IMMUTABLE);
            _SG_VALIDATE(desc->data.subimage[0][0].ptr==0, VALIDATE_IMAGEDESC_ATTACHMENT_EXPECT_NO_DATA);
            if (any_attachment) {
                _SG_VALIDATE(_sg.formats[fmt].render, VALIDATE_IMAGEDESC_ATTACHMENT_PIXELFORMAT);
                if (usg->resolve_attachment) {
                    _SG_VALIDATE(desc->sample_count == 1, VALIDATE_IMAGEDESC_ATTACHMENT_RESOLVE_EXPECT_NO_MSAA);
                }
                if (desc->sample_count > 1) {
                    _SG_VALIDATE(_sg.formats[fmt].msaa, VALIDATE_IMAGEDESC_ATTACHMENT_NO_MSAA_SUPPORT);
                    _SG_VALIDATE(desc->num_mipmaps == 1, VALIDATE_IMAGEDESC_ATTACHMENT_MSAA_NUM_MIPMAPS);
                    _SG_VALIDATE(desc->type != SG_IMAGETYPE_ARRAY, VALIDATE_IMAGEDESC_ATTACHMENT_MSAA_ARRAY_IMAGE);
                    _SG_VALIDATE(desc->type != SG_IMAGETYPE_3D, VALIDATE_IMAGEDESC_ATTACHMENT_MSAA_3D_IMAGE);
                    _SG_VALIDATE(desc->type != SG_IMAGETYPE_CUBE, VALIDATE_IMAGEDESC_ATTACHMENT_MSAA_CUBE_IMAGE);
                }
            } else if (usg->storage_image) {
                _SG_VALIDATE(_sg_is_valid_storage_image_format(fmt), VALIDATE_IMAGEDESC_STORAGEIMAGE_PIXELFORMAT);
                // D3D11 doesn't allow multisampled UAVs (see: https://github.com/gpuweb/gpuweb/issues/513)
                _SG_VALIDATE(desc->sample_count == 1, VALIDATE_IMAGEDESC_STORAGEIMAGE_EXPECT_NO_MSAA);
            }
        } else {
            _SG_VALIDATE(desc->sample_count == 1, VALIDATE_IMAGEDESC_MSAA_BUT_NO_ATTACHMENT);
            const bool valid_nonrt_fmt = !_sg_is_valid_attachment_depth_format(fmt);
            _SG_VALIDATE(valid_nonrt_fmt, VALIDATE_IMAGEDESC_NONRT_PIXELFORMAT);
            const bool is_compressed = _sg_is_compressed_pixel_format(desc->pixel_format);
            if (is_compressed) {
                _SG_VALIDATE(usg->immutable, VALIDATE_IMAGEDESC_COMPRESSED_IMMUTABLE);
            }
            if (!injected && usg->immutable) {
                // image desc must have valid data
                _sg_validate_image_data(&desc->data,
                    desc->pixel_format,
                    desc->width,
                    desc->height,
                    (desc->type == SG_IMAGETYPE_CUBE) ? 6 : 1,
                    desc->num_mipmaps,
                    desc->num_slices);
            } else {
                // image desc must not have data
                for (int face_index = 0; face_index < SG_CUBEFACE_NUM; face_index++) {
                    for (int mip_index = 0; mip_index < SG_MAX_MIPMAPS; mip_index++) {
                        const bool no_data = 0 == desc->data.subimage[face_index][mip_index].ptr;
                        const bool no_size = 0 == desc->data.subimage[face_index][mip_index].size;
                        if (injected) {
                            _SG_VALIDATE(no_data && no_size, VALIDATE_IMAGEDESC_INJECTED_NO_DATA);
                        }
                        if (!usg->immutable) {
                            _SG_VALIDATE(no_data && no_size, VALIDATE_IMAGEDESC_DYNAMIC_NO_DATA);
                        }
                    }
                }
            }
        }
        return _sg_validate_end();
    #endif
}

_SOKOL_PRIVATE bool _sg_validate_sampler_desc(const sg_sampler_desc* desc) {
    #if !defined(SOKOL_DEBUG)
        _SOKOL_UNUSED(desc);
        return true;
    #else
        if (_sg.desc.disable_validation) {
            return true;
        }
        SOKOL_ASSERT(desc);
        _sg_validate_begin();
        _SG_VALIDATE(desc->_start_canary == 0, VALIDATE_SAMPLERDESC_CANARY);
        _SG_VALIDATE(desc->_end_canary == 0, VALIDATE_SAMPLERDESC_CANARY);
        // restriction from WebGPU: when anisotropy > 1, all filters must be linear
        if (desc->max_anisotropy > 1) {
            _SG_VALIDATE((desc->min_filter == SG_FILTER_LINEAR)
                      && (desc->mag_filter == SG_FILTER_LINEAR)
                      && (desc->mipmap_filter == SG_FILTER_LINEAR),
                      VALIDATE_SAMPLERDESC_ANISTROPIC_REQUIRES_LINEAR_FILTERING);
        }
        return _sg_validate_end();
    #endif
}

typedef struct {
    uint64_t lo, hi;
} _sg_u128_t;

_SOKOL_PRIVATE _sg_u128_t _sg_u128(void) {
    _sg_u128_t res;
    _sg_clear(&res, sizeof(res));
    return res;
}

_SOKOL_PRIVATE _sg_u128_t _sg_validate_set_slot_bit(_sg_u128_t bits, sg_shader_stage stage, uint8_t slot) {
    switch (stage) {
        case SG_SHADERSTAGE_NONE:
            SOKOL_ASSERT(slot < 128);
            if (slot < 64) {
                bits.lo |= 1ULL << slot;
            } else {
                bits.hi |= 1ULL << (slot - 64);
            }
            break;
        case SG_SHADERSTAGE_VERTEX:
            SOKOL_ASSERT(slot < 64);
            bits.lo |= 1ULL << slot;
            break;
        case SG_SHADERSTAGE_FRAGMENT:
            SOKOL_ASSERT(slot < 64);
            bits.hi |= 1ULL << slot;
            break;
        case SG_SHADERSTAGE_COMPUTE:
            SOKOL_ASSERT(slot < 64);
            bits.lo |= 1ULL << slot;
            break;
        default:
            SOKOL_UNREACHABLE;
            break;
    }
    return bits;
}

_SOKOL_PRIVATE bool _sg_validate_slot_bits(_sg_u128_t bits, sg_shader_stage stage, uint8_t slot) {
    _sg_u128_t mask = _sg_u128();
    switch (stage) {
        case SG_SHADERSTAGE_NONE:
            SOKOL_ASSERT(slot < 128);
            if (slot < 64) {
                mask.lo = 1ULL << slot;
            } else {
                mask.hi = 1ULL << (slot - 64);
            }
            break;
        case SG_SHADERSTAGE_VERTEX:
            SOKOL_ASSERT(slot < 64);
            mask.lo = 1ULL << slot;
            break;
        case SG_SHADERSTAGE_FRAGMENT:
            SOKOL_ASSERT(slot < 64);
            mask.hi = 1ULL << slot;
            break;
        case SG_SHADERSTAGE_COMPUTE:
            SOKOL_ASSERT(slot < 64);
            mask.lo = 1ULL << slot;
            break;
        default:
            SOKOL_UNREACHABLE;
            break;
    }
    return ((bits.lo & mask.lo) == 0) && ((bits.hi & mask.hi) == 0);
}

_SOKOL_PRIVATE bool _sg_validate_shader_desc(const sg_shader_desc* desc) {
    #if !defined(SOKOL_DEBUG)
        _SOKOL_UNUSED(desc);
        return true;
    #else
        if (_sg.desc.disable_validation) {
            return true;
        }
        SOKOL_ASSERT(desc);
        bool is_compute_shader = (desc->compute_func.source != 0) || (desc->compute_func.bytecode.ptr != 0);
        _sg_validate_begin();
        _SG_VALIDATE(desc->_start_canary == 0, VALIDATE_SHADERDESC_CANARY);
        _SG_VALIDATE(desc->_end_canary == 0, VALIDATE_SHADERDESC_CANARY);
        #if defined(SOKOL_GLCORE) || defined(SOKOL_GLES3) || defined(SOKOL_WGPU)
            // on GL or WebGPU, must provide shader source code
            if (is_compute_shader) {
                _SG_VALIDATE(0 != desc->compute_func.source, VALIDATE_SHADERDESC_COMPUTE_SOURCE);
            } else {
                _SG_VALIDATE(0 != desc->vertex_func.source, VALIDATE_SHADERDESC_VERTEX_SOURCE);
                _SG_VALIDATE(0 != desc->fragment_func.source, VALIDATE_SHADERDESC_FRAGMENT_SOURCE);
            }
        #elif defined(SOKOL_METAL) || defined(SOKOL_D3D11)
            // on Metal or D3D11, must provide shader source code or byte code
            if (is_compute_shader) {
                _SG_VALIDATE((0 != desc->compute_func.source) || (0 != desc->compute_func.bytecode.ptr), VALIDATE_SHADERDESC_COMPUTE_SOURCE_OR_BYTECODE);
            } else {
                _SG_VALIDATE((0 != desc->vertex_func.source)|| (0 != desc->vertex_func.bytecode.ptr), VALIDATE_SHADERDESC_VERTEX_SOURCE_OR_BYTECODE);
                _SG_VALIDATE((0 != desc->fragment_func.source) || (0 != desc->fragment_func.bytecode.ptr), VALIDATE_SHADERDESC_FRAGMENT_SOURCE_OR_BYTECODE);
            }
        #else
            // Dummy Backend, don't require source or bytecode
        #endif
        if (is_compute_shader) {
            _SG_VALIDATE((0 == desc->vertex_func.source) && (0 == desc->vertex_func.bytecode.ptr), VALIDATE_SHADERDESC_INVALID_SHADER_COMBO);
            _SG_VALIDATE((0 == desc->fragment_func.source) && (0 == desc->fragment_func.bytecode.ptr), VALIDATE_SHADERDESC_INVALID_SHADER_COMBO);
        } else {
            _SG_VALIDATE((0 == desc->compute_func.source) && (0 == desc->compute_func.bytecode.ptr), VALIDATE_SHADERDESC_INVALID_SHADER_COMBO);
        }
        #if defined(SOKOL_METAL)
        if (is_compute_shader) {
            int x = desc->mtl_threads_per_threadgroup.x;
            int y = desc->mtl_threads_per_threadgroup.y;
            int z = desc->mtl_threads_per_threadgroup.z;
            _SG_VALIDATE((x > 0) && (y > 0) && (z > 0), VALIDATE_SHADERDESC_METAL_THREADS_PER_THREADGROUP_INITIALIZED);
            _SG_VALIDATE(((x * y * z) & 31) == 0, VALIDATE_SHADERDESC_METAL_THREADS_PER_THREADGROUP_MULTIPLE_32);
        }
        #endif
        for (size_t i = 0; i < SG_MAX_VERTEX_ATTRIBUTES; i++) {
            if (desc->attrs[i].glsl_name) {
                _SG_VALIDATE(strlen(desc->attrs[i].glsl_name) < _SG_STRING_SIZE, VALIDATE_SHADERDESC_ATTR_STRING_TOO_LONG);
            }
            if (desc->attrs[i].hlsl_sem_name) {
                _SG_VALIDATE(strlen(desc->attrs[i].hlsl_sem_name) < _SG_STRING_SIZE, VALIDATE_SHADERDESC_ATTR_STRING_TOO_LONG);
            }
        }
        // if shader byte code, the size must also be provided
        if (0 != desc->vertex_func.bytecode.ptr) {
            _SG_VALIDATE(desc->vertex_func.bytecode.size > 0, VALIDATE_SHADERDESC_NO_BYTECODE_SIZE);
        }
        if (0 != desc->fragment_func.bytecode.ptr) {
            _SG_VALIDATE(desc->fragment_func.bytecode.size > 0, VALIDATE_SHADERDESC_NO_BYTECODE_SIZE);
        }
        if (0 != desc->compute_func.bytecode.ptr) {
            _SG_VALIDATE(desc->compute_func.bytecode.size > 0, VALIDATE_SHADERDESC_NO_BYTECODE_SIZE);
        }

        #if defined(SOKOL_METAL)
        _sg_u128_t msl_buf_bits = _sg_u128();
        _sg_u128_t msl_tex_bits = _sg_u128();
            #endif
            uint32_t uniform_offset = 0;
            int num_uniforms = 0;
            for (size_t u_index = 0; u_index < SG_MAX_UNIFORMBLOCK_MEMBERS; u_index++) {
                const sg_glsl_shader_uniform* u_desc = &ub_desc->glsl_uniforms[u_index];
                if (u_desc->type != SG_UNIFORMTYPE_INVALID) {
                    _SG_VALIDATE(uniforms_continuous, VALIDATE_SHADERDESC_UNIFORMBLOCK_NO_CONT_MEMBERS);
                    _SG_VALIDATE(u_desc->glsl_name, VALIDATE_SHADERDESC_UNIFORMBLOCK_UNIFORM_GLSL_NAME);
                    const int array_count = u_desc->array_count;
                    _SG_VALIDATE(array_count > 0, VALIDATE_SHADERDESC_UNIFORMBLOCK_ARRAY_COUNT);
                    const uint32_t u_align = _sg_uniform_alignment(u_desc->type, array_count, ub_desc->layout);
                    const uint32_t u_size  = _sg_uniform_size(u_desc->type, array_count, ub_desc->layout);
                    uniform_offset = _sg_align_u32(uniform_offset, u_align);
                    uniform_offset += u_size;
                    num_uniforms++;
                    // with std140, arrays are only allowed for FLOAT4, INT4, MAT4
                    if (ub_desc->layout == SG_UNIFORMLAYOUT_STD140) {
                        if (array_count > 1) {
                            _SG_VALIDATE((u_desc->type == SG_UNIFORMTYPE_FLOAT4) || (u_desc->type == SG_UNIFORMTYPE_INT4) || (u_desc->type == SG_UNIFORMTYPE_MAT4), VALIDATE_SHADERDESC_UNIFORMBLOCK_STD140_ARRAY_TYPE);
                        }
                    }
                } else {
                    uniforms_continuous = false;
                }
            }
            if (ub_desc->layout == SG_UNIFORMLAYOUT_STD140) {
                uniform_offset = _sg_align_u32(uniform_offset, 16);
            }
            _SG_VALIDATE((size_t)uniform_offset == ub_desc->size, VALIDATE_SHADERDESC_UNIFORMBLOCK_SIZE_MISMATCH);
            _SG_VALIDATE(num_uniforms > 0, VALIDATE_SHADERDESC_UNIFORMBLOCK_NO_MEMBERS);
            #endif
        }

        uint32_t texview_slot_mask = 0;
        for (size_t view_idx = 0; view_idx < SG_MAX_VIEW_BINDSLOTS; view_idx++) {
            const sg_shader_view* view_desc = &desc->views[view_idx];
            if (view_desc->texture.stage != SG_SHADERSTAGE_NONE) {
                const sg_shader_texture_view* tex_desc = &view_desc->texture;
                texview_slot_mask |= (1 << view_idx);
                #if defined(SOKOL_METAL)
                _SG_VALIDATE(tex_desc->msl_texture_n < _SG_MTL_MAX_STAGE_TEXTURE_BINDINGS, VALIDATE_SHADERDESC_VIEW_TEXTURE_METAL_TEXTURE_SLOT_OUT_OF_RANGE);
                _SG_VALIDATE(_sg_validate_slot_bits(msl_tex_bits, tex_desc->stage, tex_desc->msl_texture_n), VALIDATE_SHADERDESC_VIEW_TEXTURE_METAL_TEXTURE_SLOT_COLLISION);
                #elif defined(SOKOL_DUMMY_BACKEND) || defined(_SOKOL_ANY_GL)
                _SOKOL_UNUSED(tex_desc);
                #endif
            } else if (view_desc->storage_buffer.stage != SG_SHADERSTAGE_NONE) {
                const sg_shader_storage_buffer_view* sbuf_desc = &view_desc->storage_buffer;
                #if defined(SOKOL_METAL)
                _SG_VALIDATE((sbuf_desc->msl_buffer_n >= _SG_MTL_MAX_STAGE_UB_BINDINGS) && (sbuf_desc->msl_buffer_n < _SG_MTL_MAX_STAGE_UB_SBUF_BINDINGS), VALIDATE_SHADERDESC_VIEW_STORAGEBUFFER_METAL_BUFFER_SLOT_OUT_OF_RANGE);
                _SG_VALIDATE(_sg_validate_slot_bits(msl_buf_bits, sbuf_desc->stage, sbuf_desc->msl_buffer_n), VALIDATE_SHADERDESC_VIEW_STORAGEBUFFER_METAL_BUFFER_SLOT_COLLISION);
                msl_buf_bits = _sg_validate_set_slot_bit(msl_buf_bits, sbuf_desc->stage, sbuf_desc->msl_buffer_n);                    _SG_VALIDATE(sbuf_desc->hlsl_register_t_n < _SG_D3D11_MAX_STAGE_SRV_BINDINGS, VALIDATE_SHADERDESC_VIEW_STORAGEBUFFER_HLSL_REGISTER_T_OUT_OF_RANGE);
                } else {
                    _SG_VALIDATE(sbuf_desc->hlsl_register_u_n < _SG_D3D11_MAX_STAGE_UAV_BINDINGS, VALIDATE_SHADERDESC_VIEW_STORAGEBUFFER_HLSL_REGISTER_U_OUT_OF_RANGE);
            #endif
        }

        uint32_t ref_texview_slot_mask = 0;
        uint32_t ref_smp_slot_mask = 0;
        for (size_t tex_smp_idx = 0; tex_smp_idx < SG_MAX_TEXTURE_SAMPLER_PAIRS; tex_smp_idx++) {
            const sg_shader_texture_sampler_pair* tex_smp_desc = &desc->texture_sampler_pairs[tex_smp_idx];
            if (tex_smp_desc->stage == SG_SHADERSTAGE_NONE) {
                continue;
            }
            #endif
            const bool view_slot_in_range = tex_smp_desc->view_slot < SG_MAX_VIEW_BINDSLOTS;
            const bool smp_slot_in_range = tex_smp_desc->sampler_slot < SG_MAX_SAMPLER_BINDSLOTS;
            _SG_VALIDATE(view_slot_in_range, VALIDATE_SHADERDESC_TEXTURE_SAMPLER_PAIR_VIEW_SLOT_OUT_OF_RANGE);
            _SG_VALIDATE(smp_slot_in_range, VALIDATE_SHADERDESC_TEXTURE_SAMPLER_PAIR_SAMPLER_SLOT_OUT_OF_RANGE);
            if (view_slot_in_range && smp_slot_in_range) {
                ref_texview_slot_mask |= 1 << tex_smp_desc->view_slot;
                ref_smp_slot_mask |= 1 << tex_smp_desc->sampler_slot;
                const sg_shader_view* view_desc = &desc->views[tex_smp_desc->view_slot];
                const sg_shader_sampler* smp_desc = &desc->samplers[tex_smp_desc->sampler_slot];
                _SG_VALIDATE(view_desc->texture.stage != SG_SHADERSTAGE_NONE, VALIDATE_SHADERDESC_TEXTURE_SAMPLER_PAIR_EXPECT_TEXTURE_VIEW);
                _SG_VALIDATE(view_desc->texture.stage == tex_smp_desc->stage, VALIDATE_SHADERDESC_TEXTURE_SAMPLER_PAIR_TEXTURE_STAGE_MISMATCH);
                _SG_VALIDATE(smp_desc->stage == tex_smp_desc->stage, VALIDATE_SHADERDESC_TEXTURE_SAMPLER_PAIR_SAMPLER_STAGE_MISMATCH);
                const bool needs_nonfiltering = (view_desc->texture.sample_type == SG_IMAGESAMPLETYPE_UINT)
                                             || (view_desc->texture.sample_type == SG_IMAGESAMPLETYPE_SINT)
                                             || (view_desc->texture.sample_type == SG_IMAGESAMPLETYPE_UNFILTERABLE_FLOAT);
                const bool needs_comparison = view_desc->texture.sample_type == SG_IMAGESAMPLETYPE_DEPTH;
                if (needs_nonfiltering) {
                    _SG_VALIDATE(needs_nonfiltering && (smp_desc->sampler_type == SG_SAMPLERTYPE_NONFILTERING), VALIDATE_SHADERDESC_NONFILTERING_SAMPLER_REQUIRED);
                }
                if (needs_comparison) {
                    _SG_VALIDATE(needs_comparison && (smp_desc->sampler_type == SG_SAMPLERTYPE_COMPARISON), VALIDATE_SHADERDESC_COMPARISON_SAMPLER_REQUIRED);
                }
            }
        }
        // each image and sampler must be referenced by an image sampler
        _SG_VALIDATE(texview_slot_mask == ref_texview_slot_mask, VALIDATE_SHADERDESC_TEXVIEW_NOT_REFERENCED_BY_TEXTURE_SAMPLER_PAIRS);
        _SG_VALIDATE(smp_slot_mask == ref_smp_slot_mask, VALIDATE_SHADERDESC_SAMPLER_NOT_REFERENCED_BY_TEXTURE_SAMPLER_PAIRS);

        return _sg_validate_end();
    #endif
}

_SOKOL_PRIVATE bool _sg_validate_pipeline_desc(const sg_pipeline_desc* desc) {
    #if !defined(SOKOL_DEBUG)
        _SOKOL_UNUSED(desc);
        return true;
    #else
        if (_sg.desc.disable_validation) {
            return true;
        }
        SOKOL_ASSERT(desc);
        _sg_validate_begin();
        _SG_VALIDATE(desc->_start_canary == 0, VALIDATE_PIPELINEDESC_CANARY);
        _SG_VALIDATE(desc->_end_canary == 0, VALIDATE_PIPELINEDESC_CANARY);
        _SG_VALIDATE(desc->shader.id != SG_INVALID_ID, VALIDATE_PIPELINEDESC_SHADER);
        const _sg_shader_t* shd = _sg_lookup_shader(desc->shader.id);
        _SG_VALIDATE(0 != shd, VALIDATE_PIPELINEDESC_SHADER);
        if (shd) {
            _SG_VALIDATE(shd->slot.state == SG_RESOURCESTATE_VALID, VALIDATE_PIPELINEDESC_SHADER);
            if (desc->compute) {
                _SG_VALIDATE(shd->cmn.is_compute, VALIDATE_PIPELINEDESC_COMPUTE_SHADER_EXPECTED);
            } else {
                _SG_VALIDATE(!shd->cmn.is_compute, VALIDATE_PIPELINEDESC_NO_COMPUTE_SHADER_EXPECTED);
                bool attrs_cont = true;
                for (size_t attr_index = 0; attr_index < SG_MAX_VERTEX_ATTRIBUTES; attr_index++) {
                    const sg_vertex_attr_state* a_state = &desc->layout.attrs[attr_index];
                    if (a_state->format == SG_VERTEXFORMAT_INVALID) {
                        attrs_cont = false;
                        continue;
                    }
                    _SG_VALIDATE(attrs_cont, VALIDATE_PIPELINEDESC_NO_CONT_ATTRS);
                    SOKOL_ASSERT(a_state->buffer_index < SG_MAX_VERTEXBUFFER_BINDSLOTS);
                    // vertex format must match expected shader attribute base type (if provided)
                    if (shd->cmn.attrs[attr_index].base_type != SG_SHADERATTRBASETYPE_UNDEFINED) {
                        if (_sg_vertexformat_basetype(a_state->format) != shd->cmn.attrs[attr_index].base_type) {
                            _SG_VALIDATE(false, VALIDATE_PIPELINEDESC_ATTR_BASETYPE_MISMATCH);
                            _SG_LOGMSG(VALIDATE_PIPELINEDESC_ATTR_BASETYPE_MISMATCH, "attr format:");
                            _SG_LOGMSG(VALIDATE_PIPELINEDESC_ATTR_BASETYPE_MISMATCH, _sg_vertexformat_to_string(a_state->format));
                            _SG_LOGMSG(VALIDATE_PIPELINEDESC_ATTR_BASETYPE_MISMATCH, "shader attr base type:");
                            _SG_LOGMSG(VALIDATE_PIPELINEDESC_ATTR_BASETYPE_MISMATCH, _sg_shaderattrbasetype_to_string(shd->cmn.attrs[attr_index].base_type));
                        }
                    }
                    #if defined(SOKOL_D3D11)
                    // on D3D11, semantic names (and semantic indices) must be provided
                    _SG_VALIDATE(!_sg_strempty(&shd->d3d11.attrs[attr_index].sem_name), VALIDATE_PIPELINEDESC_ATTR_SEMANTICS);
                    #endif
                }
                // must only use readonly storage buffer bindings in render pipelines
                for (size_t i = 0; i < SG_MAX_VIEW_BINDSLOTS; i++) {
                    if (shd->cmn.views[i].view_type == SG_VIEWTYPE_STORAGEBUFFER) {
                        _SG_VALIDATE(shd->cmn.views[i].sbuf_readonly, VALIDATE_PIPELINEDESC_SHADER_READONLY_STORAGEBUFFERS);
                    }
                }
                for (int buf_index = 0; buf_index < SG_MAX_VERTEXBUFFER_BINDSLOTS; buf_index++) {
                    const sg_vertex_buffer_layout_state* l_state = &desc->layout.buffers[buf_index];
                    if (l_state->stride == 0) {
                        continue;
                    }
                    _SG_VALIDATE(_sg_multiple_u64((uint64_t)l_state->stride, 4), VALIDATE_PIPELINEDESC_LAYOUT_STRIDE4);
                }
            }
        }
        for (size_t color_index = 0; color_index < (size_t)desc->color_count; color_index++) {
            const sg_blend_state* bs = &desc->colors[color_index].blend;
            if ((bs->op_rgb == SG_BLENDOP_MIN) || (bs->op_rgb == SG_BLENDOP_MAX)) {
                _SG_VALIDATE((bs->src_factor_rgb == SG_BLENDFACTOR_ONE) && (bs->dst_factor_rgb == SG_BLENDFACTOR_ONE), VALIDATE_PIPELINEDESC_BLENDOP_MINMAX_REQUIRES_BLENDFACTOR_ONE);
            }
            if ((bs->op_alpha == SG_BLENDOP_MIN) || (bs->op_alpha == SG_BLENDOP_MAX)) {
                _SG_VALIDATE((bs->src_factor_alpha == SG_BLENDFACTOR_ONE) && (bs->dst_factor_alpha == SG_BLENDFACTOR_ONE), VALIDATE_PIPELINEDESC_BLENDOP_MINMAX_REQUIRES_BLENDFACTOR_ONE);
            }
        }
        return _sg_validate_end();
    #endif
}

_SOKOL_PRIVATE bool _sg_validate_view_desc(const sg_view_desc* desc) {
    #if !defined(SOKOL_DEBUG)
        _SOKOL_UNUSED(desc);
        return true;
    #else
        if (_sg.desc.disable_validation) {
            return true;
        }
        SOKOL_ASSERT(desc);
        _sg_validate_begin();
        _SG_VALIDATE(desc->_start_canary == 0, VALIDATE_VIEWDESC_CANARY);
        _SG_VALIDATE(desc->_end_canary == 0, VALIDATE_VIEWDESC_CANARY);

        // only one view type can be define
        sg_view_type view_type = SG_VIEWTYPE_INVALID;
        const sg_image_view_desc* img_desc = 0;
        const sg_texture_view_desc* tex_desc = 0;
        const sg_buffer_view_desc* buf_desc = 0;
        if (desc->texture.image.id != SG_INVALID_ID) {
            view_type = SG_VIEWTYPE_TEXTURE;
            tex_desc = &desc->texture;
        }
        if (desc->storage_buffer.buffer.id != SG_INVALID_ID) {
            _SG_VALIDATE(SG_VIEWTYPE_INVALID == view_type, VALIDATE_VIEWDESC_UNIQUE_VIEWTYPE);
            view_type = SG_VIEWTYPE_STORAGEBUFFER;
            buf_desc = &desc->storage_buffer;
        }
        if (desc->storage_image.image.id != SG_INVALID_ID) {
            _SG_VALIDATE(SG_VIEWTYPE_INVALID == view_type, VALIDATE_VIEWDESC_UNIQUE_VIEWTYPE);
            view_type = SG_VIEWTYPE_STORAGEIMAGE;
            img_desc = &desc->storage_image;
        }
        if (desc->color_attachment.image.id != SG_INVALID_ID) {
            _SG_VALIDATE(SG_VIEWTYPE_INVALID == view_type, VALIDATE_VIEWDESC_UNIQUE_VIEWTYPE);
            view_type = SG_VIEWTYPE_COLORATTACHMENT;
            img_desc = &desc->color_attachment;
        }
        if (desc->resolve_attachment.image.id != SG_INVALID_ID) {
            _SG_VALIDATE(SG_VIEWTYPE_INVALID == view_type, VALIDATE_VIEWDESC_UNIQUE_VIEWTYPE);
            view_type = SG_VIEWTYPE_RESOLVEATTACHMENT;
            img_desc = &desc->resolve_attachment;
        }
        if (desc->depth_stencil_attachment.image.id != SG_INVALID_ID) {
            _SG_VALIDATE(SG_VIEWTYPE_INVALID == view_type, VALIDATE_VIEWDESC_UNIQUE_VIEWTYPE);
            view_type = SG_VIEWTYPE_DEPTHSTENCILATTACHMENT;
            img_desc = &desc->depth_stencil_attachment;
        }
        _SG_VALIDATE(SG_VIEWTYPE_INVALID != view_type, VALIDATE_VIEWDESC_ANY_VIEWTYPE);

        const _sg_buffer_t* buf = 0;
        const _sg_image_t* img = 0;
        bool res_valid = false;
        if (buf_desc) {
            SOKOL_ASSERT((img_desc == 0) && (tex_desc == 0));
            buf = _sg_lookup_buffer(buf_desc->buffer.id);
            _SG_VALIDATE(buf, VALIDATE_VIEWDESC_RESOURCE_ALIVE);
            if (buf) {
                _SG_VALIDATE(buf->slot.state == SG_RESOURCESTATE_VALID, VALIDATE_VIEWDESC_RESOURCE_FAILED);
                res_valid = buf->slot.state == SG_RESOURCESTATE_VALID;
            }
        } else if (img_desc) {
            SOKOL_ASSERT((tex_desc == 0) && (buf_desc == 0));
            img = _sg_lookup_image(img_desc->image.id);
            _SG_VALIDATE(img, VALIDATE_VIEWDESC_RESOURCE_ALIVE);
            if (img) {
                _SG_VALIDATE(img->slot.state == SG_RESOURCESTATE_VALID, VALIDATE_VIEWDESC_RESOURCE_FAILED);
                res_valid = img->slot.state == SG_RESOURCESTATE_VALID;
            }
        } else {
            SOKOL_ASSERT(tex_desc && (img_desc == 0) && (buf_desc == 0));
            img = _sg_lookup_image(tex_desc->image.id);
            _SG_VALIDATE(img, VALIDATE_VIEWDESC_RESOURCE_ALIVE);
            if (img) {
                _SG_VALIDATE(img->slot.state == SG_RESOURCESTATE_VALID, VALIDATE_VIEWDESC_RESOURCE_FAILED);
                res_valid = img->slot.state == SG_RESOURCESTATE_VALID;
            }
        }
        if (res_valid) {
            // check usage flags
            switch (view_type) {
                case SG_VIEWTYPE_STORAGEBUFFER:
                    SOKOL_ASSERT(buf);
                    _SG_VALIDATE(buf->cmn.usage.storage_buffer, VALIDATE_VIEWDESC_STORAGEBUFFER_USAGE);
                    break;
                case SG_VIEWTYPE_STORAGEIMAGE:
                    SOKOL_ASSERT(img);
                    _SG_VALIDATE(img->cmn.usage.storage_image, VALIDATE_VIEWDESC_STORAGEIMAGE_USAGE);
                    _SG_VALIDATE(_sg_is_valid_storage_image_format(img->cmn.pixel_format), VALIDATE_VIEWDESC_STORAGEIMAGE_PIXELFORMAT);
                    break;
                case SG_VIEWTYPE_TEXTURE:
                    if (!_sg.features.msaa_texture_bindings) {
                        _SG_VALIDATE(img->cmn.sample_count == 1, VALIDATE_VIEWDESC_TEXTURE_EXPECT_NO_MSAA);
                    }
                    break;
                case SG_VIEWTYPE_COLORATTACHMENT:
                    SOKOL_ASSERT(img);
                    _SG_VALIDATE(img->cmn.usage.color_attachment, VALIDATE_VIEWDESC_COLORATTACHMENT_USAGE);
                    _SG_VALIDATE(_sg_is_valid_attachment_color_format(img->cmn.pixel_format), VALIDATE_VIEWDESC_COLORATTACHMENT_PIXELFORMAT);
                    break;
                case SG_VIEWTYPE_RESOLVEATTACHMENT:
                    SOKOL_ASSERT(img);
                    _SG_VALIDATE(img->cmn.usage.resolve_attachment, VALIDATE_VIEWDESC_RESOLVEATTACHMENT_USAGE);
                    _SG_VALIDATE(img->cmn.sample_count == 1, VALIDATE_VIEWDESC_RESOLVEATTACHMENT_SAMPLECOUNT);
                    break;
                case SG_VIEWTYPE_DEPTHSTENCILATTACHMENT:
                    SOKOL_ASSERT(img);
                    _SG_VALIDATE(img->cmn.usage.depth_stencil_attachment, VALIDATE_VIEWDESC_DEPTHSTENCILATTACHMENT_USAGE);
                    _SG_VALIDATE(_sg_is_valid_attachment_depth_format(img->cmn.pixel_format), VALIDATE_VIEWDESC_DEPTHSTENCILATTACHMENT_PIXELFORMAT);
                    break;
                default:
                    SOKOL_UNREACHABLE;
                    break;
            }
            if (buf_desc) {
                SOKOL_ASSERT(buf);
                _SG_VALIDATE(buf_desc->offset < buf->cmn.size, VALIDATE_VIEWDESC_STORAGEBUFFER_OFFSET_VS_BUFFER_SIZE);
                _SG_VALIDATE(_sg_multiple_u64((uint64_t)buf_desc->offset, 256), VALIDATE_VIEWDESC_STORAGEBUFFER_OFFSET_MULTIPLE_256);
            } else if (img_desc) {
                SOKOL_ASSERT(img);
                _SG_VALIDATE((img_desc->mip_level >= 0) && (img_desc->mip_level < img->cmn.num_mipmaps), VALIDATE_VIEWDESC_IMAGE_MIPLEVEL);
                if (img->cmn.type == SG_IMAGETYPE_2D) {
                    _SG_VALIDATE(img_desc->slice == 0, VALIDATE_VIEWDESC_IMAGE_2D_SLICE);
                } else if (img->cmn.type == SG_IMAGETYPE_CUBE) {
                    _SG_VALIDATE((img_desc->slice >= 0) && (img_desc->slice < 6), VALIDATE_VIEWDESC_IMAGE_CUBEMAP_SLICE);
                } else if (img->cmn.type == SG_IMAGETYPE_ARRAY) {
                    _SG_VALIDATE((img_desc->slice >= 0) && (img_desc->slice < img->cmn.num_slices), VALIDATE_VIEWDESC_IMAGE_ARRAY_SLICE);
                } else if (img->cmn.type == SG_IMAGETYPE_3D) {
                    _SG_VALIDATE(img_desc->slice == 0, VALIDATE_VIEWDESC_IMAGE_3D_SLICE);
                }
            } else if (tex_desc) {
                SOKOL_ASSERT(img);
                // NOTE: it doesn't matter here if the mip/slice count is default-zero!
                int max_mip_level = tex_desc->mip_levels.base + tex_desc->mip_levels.count;
                int max_slice = tex_desc->slices.base + tex_desc->slices.count;
                _SG_VALIDATE((tex_desc->mip_levels.base >= 0) && (max_mip_level <= img->cmn.num_mipmaps), VALIDATE_VIEWDESC_TEXTURE_MIPLEVELS);
                if (img->cmn.type == SG_IMAGETYPE_2D) {
                    _SG_VALIDATE((tex_desc->slices.base == 0) && (max_slice <= 1), VALIDATE_VIEWDESC_TEXTURE_2D_SLICES);
                } else if (img->cmn.type == SG_IMAGETYPE_CUBE) {
                    _SG_VALIDATE((tex_desc->slices.base == 0) && (max_slice <= 1), VALIDATE_VIEWDESC_TEXTURE_CUBEMAP_SLICES);
                } else if (img->cmn.type == SG_IMAGETYPE_ARRAY) {
                    _SG_VALIDATE((tex_desc->slices.base >= 0) && (max_slice <= img->cmn.num_slices), VALIDATE_VIEWDESC_TEXTURE_ARRAY_SLICES);
                } else if (img->cmn.type == SG_IMAGETYPE_3D) {
                    _SG_VALIDATE((tex_desc->slices.base == 0) && (max_slice <= 1), VALIDATE_VIEWDESC_TEXTURE_3D_SLICES);
                }
            }
        }
        return _sg_validate_end();
    #endif
}

_SOKOL_PRIVATE bool _sg_validate_begin_pass(const sg_pass* pass) {
    #if !defined(SOKOL_DEBUG)
        _SOKOL_UNUSED(pass);
        return true;
    #else
        if (_sg.desc.disable_validation) {
            return true;
        }
        const bool is_compute_pass = pass->compute;
        const bool is_swapchain_pass = !is_compute_pass && _sg_attachments_empty(&pass->attachments);
        const bool is_offscreen_pass = !(is_compute_pass || is_swapchain_pass);
        _sg_validate_begin();
        _SG_VALIDATE(pass->_start_canary == 0, VALIDATE_BEGINPASS_CANARY);
        _SG_VALIDATE(pass->_end_canary == 0, VALIDATE_BEGINPASS_CANARY);
        if (is_compute_pass) {
            _SG_VALIDATE(_sg_attachments_empty(&pass->attachments), VALIDATE_BEGINPASS_COMPUTEPASS_EXPECT_NO_ATTACHMENTS);
        } else if (is_swapchain_pass) {
            // this is a swapchain pass
            _SG_VALIDATE(pass->swapchain.width > 0, VALIDATE_BEGINPASS_SWAPCHAIN_EXPECT_WIDTH);
            _SG_VALIDATE(pass->swapchain.height > 0, VALIDATE_BEGINPASS_SWAPCHAIN_EXPECT_HEIGHT);
            _SG_VALIDATE(pass->swapchain.sample_count > 0, VALIDATE_BEGINPASS_SWAPCHAIN_EXPECT_SAMPLECOUNT);
            _SG_VALIDATE(pass->swapchain.color_format > SG_PIXELFORMAT_NONE, VALIDATE_BEGINPASS_SWAPCHAIN_EXPECT_COLORFORMAT);
            // NOTE: depth buffer is optional, so depth_format is allowed to be invalid
            // NOTE: the GL framebuffer handle may actually be 0
            #if defined(SOKOL_METAL)
                _SG_VALIDATE(pass->swapchain.metal.current_drawable != 0, VALIDATE_BEGINPASS_SWAPCHAIN_METAL_EXPECT_CURRENTDRAWABLE);
                if (pass->swapchain.depth_format == SG_PIXELFORMAT_NONE) {
                    _SG_VALIDATE(pass->swapchain.metal.depth_stencil_texture == 0, VALIDATE_BEGINPASS_SWAPCHAIN_METAL_EXPECT_DEPTHSTENCILTEXTURE_NOTSET);
                } else {
                    _SG_VALIDATE(pass->swapchain.metal.depth_stencil_texture != 0, VALIDATE_BEGINPASS_SWAPCHAIN_METAL_EXPECT_DEPTHSTENCILTEXTURE);
                }
                if (pass->swapchain.sample_count > 1) {
                    _SG_VALIDATE(pass->swapchain.metal.msaa_color_texture != 0, VALIDATE_BEGINPASS_SWAPCHAIN_METAL_EXPECT_MSAACOLORTEXTURE);
                } else {
                    _SG_VALIDATE(pass->swapchain.metal.msaa_color_texture == 0, VALIDATE_BEGINPASS_SWAPCHAIN_METAL_EXPECT_MSAACOLORTEXTURE_NOTSET);
                }                if (pass->swapchain.depth_format == SG_PIXELFORMAT_NONE) {
                    _SG_VALIDATE(pass->swapchain.d3d11.depth_stencil_view == 0, VALIDATE_BEGINPASS_SWAPCHAIN_D3D11_EXPECT_DEPTHSTENCILVIEW_NOTSET);
                } else {
                    _SG_VALIDATE(pass->swapchain.d3d11.depth_stencil_view != 0, VALIDATE_BEGINPASS_SWAPCHAIN_D3D11_EXPECT_DEPTHSTENCILVIEW);
                }
                if (pass->swapchain.sample_count > 1) {
                    _SG_VALIDATE(pass->swapchain.d3d11.resolve_view != 0, VALIDATE_BEGINPASS_SWAPCHAIN_D3D11_EXPECT_RESOLVEVIEW);
                } else {
                    _SG_VALIDATE(pass->swapchain.d3d11.resolve_view == 0, VALIDATE_BEGINPASS_SWAPCHAIN_D3D11_EXPECT_RESOLVEVIEW_NOTSET);
                }                if (pass->swapchain.depth_format == SG_PIXELFORMAT_NONE) {
                    _SG_VALIDATE(pass->swapchain.wgpu.depth_stencil_view == 0, VALIDATE_BEGINPASS_SWAPCHAIN_WGPU_EXPECT_DEPTHSTENCILVIEW_NOTSET);
                } else {
                    _SG_VALIDATE(pass->swapchain.wgpu.depth_stencil_view != 0, VALIDATE_BEGINPASS_SWAPCHAIN_WGPU_EXPECT_DEPTHSTENCILVIEW);
                }
                if (pass->swapchain.sample_count > 1) {
                    _SG_VALIDATE(pass->swapchain.wgpu.resolve_view != 0, VALIDATE_BEGINPASS_SWAPCHAIN_WGPU_EXPECT_RESOLVEVIEW);
                } else {
                    _SG_VALIDATE(pass->swapchain.wgpu.resolve_view == 0, VALIDATE_BEGINPASS_SWAPCHAIN_WGPU_EXPECT_RESOLVEVIEW_NOTSET);
                }
            #endif
        } else {
            // this is an 'offscreen pass'
            bool has_color_atts = false;
            bool has_depth_stencil_atts = false;
            bool atts_cont = true;
            int color_width = -1, color_height = -1, color_sample_count = -1;
            for (int att_index = 0; att_index < SG_MAX_COLOR_ATTACHMENTS; att_index++) {
                if (pass->attachments.colors[att_index].id == SG_INVALID_ID) {
                    atts_cont = false;
                    continue;
                }
                has_color_atts = true;
                _SG_VALIDATE(atts_cont, VALIDATE_BEGINPASS_COLORATTACHMENTVIEWS_CONTINUOUS);
                const _sg_view_t* view = _sg_lookup_view(pass->attachments.colors[att_index].id);
                // the view object must be alive
                _SG_VALIDATE(view != 0, VALIDATE_BEGINPASS_COLORATTACHMENTVIEW_ALIVE);
                if (view) {
                    // the view object must be in valid state
                    _SG_VALIDATE(view->slot.state == SG_RESOURCESTATE_VALID, VALIDATE_BEGINPASS_COLORATTACHMENTVIEW_VALID);
                    if (view->slot.state == SG_RESOURCESTATE_VALID) {
                        // the view object must be a color attachment view
                        _SG_VALIDATE(view->cmn.type == SG_VIEWTYPE_COLORATTACHMENT, VALIDATE_BEGINPASS_COLORATTACHMENTVIEW_TYPE);
                        // the view's image object must be alive and valid
                        const _sg_image_t* img = _sg_image_ref_ptr_or_null(&view->cmn.img.ref);
                        _SG_VALIDATE(img, VALIDATE_BEGINPASS_COLORATTACHMENTVIEW_IMAGE_ALIVE);
                        if (img) {
                            _SG_VALIDATE(img->slot.state == SG_RESOURCESTATE_VALID, VALIDATE_BEGINPASS_COLORATTACHMENTVIEW_IMAGE_VALID);
                            if (img->slot.state == SG_RESOURCESTATE_VALID) {
                                if (color_width == -1) {
                                    color_width = _sg_image_view_dim(view).width;
                                    color_height = _sg_image_view_dim(view).height;
                                    color_sample_count = img->cmn.sample_count;
                                } else {
                                    _SG_VALIDATE(color_width == _sg_image_view_dim(view).width, VALIDATE_BEGINPASS_COLORATTACHMENTVIEW_SIZES);
                                    _SG_VALIDATE(color_height == _sg_image_view_dim(view).height, VALIDATE_BEGINPASS_COLORATTACHMENTVIEW_SIZES);
                                    _SG_VALIDATE(color_sample_count == img->cmn.sample_count, VALIDATE_BEGINPASS_COLORATTACHMENTVIEW_SAMPLECOUNTS);
                                }
                            }
                        }
                    }
                }
            }
            // check resolve views
            for (int att_index = 0; att_index < SG_MAX_COLOR_ATTACHMENTS; att_index++) {
                if (pass->attachments.resolves[att_index].id == SG_INVALID_ID) {
                    continue;
                }
                _SG_VALIDATE(pass->attachments.colors[att_index].id != SG_INVALID_ID, VALIDATE_BEGINPASS_RESOLVEATTACHMENTVIEW_NO_COLORATTACHMENTVIEW);
                const _sg_view_t* view = _sg_lookup_view(pass->attachments.resolves[att_index].id);
                // the view object must be alive
                _SG_VALIDATE(view != 0, VALIDATE_BEGINPASS_RESOLVEATTACHMENTVIEW_ALIVE);
                if (view) {
                    // the view object must be in valid state
                    _SG_VALIDATE(view->slot.state == SG_RESOURCESTATE_VALID, VALIDATE_BEGINPASS_RESOLVEATTACHMENTVIEW_VALID);
                    if (view->slot.state == SG_RESOURCESTATE_VALID) {
                        // the view object must be a resolve attachment view
                        _SG_VALIDATE(view->cmn.type == SG_VIEWTYPE_RESOLVEATTACHMENT, VALIDATE_BEGINPASS_RESOLVEATTACHMENTVIEW_TYPE);
                        // the view's image object must be alive and valid
                        const _sg_image_t* img = _sg_image_ref_ptr_or_null(&view->cmn.img.ref);
                        _SG_VALIDATE(img, VALIDATE_BEGINPASS_RESOLVEATTACHMENTVIEW_IMAGE_ALIVE);
                        if (img) {
                            _SG_VALIDATE(img->slot.state == SG_RESOURCESTATE_VALID, VALIDATE_BEGINPASS_RESOLVEATTACHMENTVIEW_IMAGE_VALID);
                            if (img->slot.state == SG_RESOURCESTATE_VALID) {
                                if (color_width != -1) {
                                    _SG_VALIDATE(color_width == _sg_image_view_dim(view).width, VALIDATE_BEGINPASS_RESOLVEATTACHMENTVIEW_SIZES);
                                    _SG_VALIDATE(color_height == _sg_image_view_dim(view).height, VALIDATE_BEGINPASS_RESOLVEATTACHMENTVIEW_SIZES);
                                }
                            }
                        }
                    }
                }
            }
            // check depth-stencil view
            if (pass->attachments.depth_stencil.id != SG_INVALID_ID) {
                has_depth_stencil_atts = true;
                const _sg_view_t* view = _sg_lookup_view(pass->attachments.depth_stencil.id);
                // the view object must be valid
                _SG_VALIDATE(view != 0, VALIDATE_BEGINPASS_DEPTHSTENCILATTACHMENTVIEW_ALIVE);
                if (view) {
                    // the view object must be in valid state
                    _SG_VALIDATE(view->slot.state == SG_RESOURCESTATE_VALID, VALIDATE_BEGINPASS_DEPTHSTENCILATTACHMENTVIEW_VALID);
                    if (view->slot.state == SG_RESOURCESTATE_VALID) {
                        // the view object must be a depth stencil attachment view
                        _SG_VALIDATE(view->cmn.type == SG_VIEWTYPE_DEPTHSTENCILATTACHMENT, VALIDATE_BEGINPASS_DEPTHSTENCILATTACHMENTVIEW_TYPE);
                        // the view's image object must be alive and valid
                        const _sg_image_t* img = _sg_image_ref_ptr_or_null(&view->cmn.img.ref);
                        _SG_VALIDATE(img, VALIDATE_BEGINPASS_DEPTHSTENCILATTACHMENTVIEW_IMAGE_ALIVE);
                        if (img) {
                            _SG_VALIDATE(img->slot.state == SG_RESOURCESTATE_VALID, VALIDATE_BEGINPASS_DEPTHSTENCILATTACHMENTVIEW_IMAGE_VALID);
                            if (img->slot.state == SG_RESOURCESTATE_VALID) {
                                if (color_width != -1) {
                                    _SG_VALIDATE(color_width == _sg_image_view_dim(view).width, VALIDATE_BEGINPASS_DEPTHSTENCILATTACHMENTVIEW_SIZES);
                                    _SG_VALIDATE(color_height == _sg_image_view_dim(view).height, VALIDATE_BEGINPASS_DEPTHSTENCILATTACHMENTVIEW_SIZES);
                                    _SG_VALIDATE(color_sample_count == img->cmn.sample_count, VALIDATE_BEGINPASS_DEPTHSTENCILATTACHMENTVIEW_SAMPLECOUNT);
                                }
                            }
                        }
                    }
                }
            }
            // must have at least color- or depth-stencil-attachments
            _SG_VALIDATE(has_color_atts || has_depth_stencil_atts, VALIDATE_BEGINPASS_ATTACHMENTS_EXPECTED);
        }
        if (is_compute_pass || is_offscreen_pass) {
            _SG_VALIDATE(pass->swapchain.width == 0, VALIDATE_BEGINPASS_SWAPCHAIN_EXPECT_WIDTH_NOTSET);
            _SG_VALIDATE(pass->swapchain.height == 0, VALIDATE_BEGINPASS_SWAPCHAIN_EXPECT_HEIGHT_NOTSET);
            _SG_VALIDATE(pass->swapchain.sample_count == 0, VALIDATE_BEGINPASS_SWAPCHAIN_EXPECT_SAMPLECOUNT_NOTSET);
            _SG_VALIDATE(pass->swapchain.color_format == _SG_PIXELFORMAT_DEFAULT, VALIDATE_BEGINPASS_SWAPCHAIN_EXPECT_COLORFORMAT_NOTSET);
            _SG_VALIDATE(pass->swapchain.depth_format == _SG_PIXELFORMAT_DEFAULT, VALIDATE_BEGINPASS_SWAPCHAIN_EXPECT_DEPTHFORMAT_NOTSET);
            #if defined(SOKOL_METAL)
                _SG_VALIDATE(pass->swapchain.metal.current_drawable == 0, VALIDATE_BEGINPASS_SWAPCHAIN_METAL_EXPECT_CURRENTDRAWABLE_NOTSET);
                _SG_VALIDATE(pass->swapchain.metal.depth_stencil_texture == 0, VALIDATE_BEGINPASS_SWAPCHAIN_METAL_EXPECT_DEPTHSTENCILTEXTURE_NOTSET);
                _SG_VALIDATE(pass->swapchain.metal.msaa_color_texture == 0, VALIDATE_BEGINPASS_SWAPCHAIN_METAL_EXPECT_MSAACOLORTEXTURE_NOTSET);                _SG_VALIDATE(pass->swapchain.d3d11.depth_stencil_view == 0, VALIDATE_BEGINPASS_SWAPCHAIN_D3D11_EXPECT_DEPTHSTENCILVIEW_NOTSET);
                _SG_VALIDATE(pass->swapchain.d3d11.resolve_view == 0, VALIDATE_BEGINPASS_SWAPCHAIN_D3D11_EXPECT_RESOLVEVIEW_NOTSET);                _SG_VALIDATE(pass->swapchain.wgpu.depth_stencil_view == 0, VALIDATE_BEGINPASS_SWAPCHAIN_WGPU_EXPECT_DEPTHSTENCILVIEW_NOTSET);
                _SG_VALIDATE(pass->swapchain.wgpu.resolve_view == 0, VALIDATE_BEGINPASS_SWAPCHAIN_WGPU_EXPECT_RESOLVEVIEW_NOTSET);
    #elif defined(SOKOL_METAL) || defined(SOKOL_D3D11)
        res.environment.defaults.color_format = _sg_def(res.environment.defaults.color_format, SG_PIXELFORMAT_BGRA8);
    #else
        res.environment.defaults.color_format = _sg_def(res.environment.defaults.color_format, SG_PIXELFORMAT_RGBA8);
    #endif
    res.environment.defaults.depth_format = _sg_def(res.environment.defaults.depth_format, SG_PIXELFORMAT_DEPTH_STENCIL);
    res.environment.defaults.sample_count = _sg_def(res.environment.defaults.sample_count, 1);
    res.buffer_pool_size = _sg_def(res.buffer_pool_size, _SG_DEFAULT_BUFFER_POOL_SIZE);
    res.image_pool_size = _sg_def(res.image_pool_size, _SG_DEFAULT_IMAGE_POOL_SIZE);
    res.sampler_pool_size = _sg_def(res.sampler_pool_size, _SG_DEFAULT_SAMPLER_POOL_SIZE);
    res.shader_pool_size = _sg_def(res.shader_pool_size, _SG_DEFAULT_SHADER_POOL_SIZE);
    res.pipeline_pool_size = _sg_def(res.pipeline_pool_size, _SG_DEFAULT_PIPELINE_POOL_SIZE);
    res.view_pool_size = _sg_def(res.view_pool_size, _SG_DEFAULT_VIEW_POOL_SIZE);
    res.uniform_buffer_size = _sg_def(res.uniform_buffer_size, _SG_DEFAULT_UB_SIZE);
    res.max_commit_listeners = _sg_def(res.max_commit_listeners, _SG_DEFAULT_MAX_COMMIT_LISTENERS);
    res.wgpu_bindgroups_cache_size = _sg_def(res.wgpu_bindgroups_cache_size, _SG_DEFAULT_WGPU_BINDGROUP_CACHE_SIZE);
    return res;
}

_SOKOL_PRIVATE sg_pass _sg_pass_defaults(const sg_pass* pass) {
    sg_pass res = *pass;
    if (!res.compute) {
        if (_sg_attachments_empty(&pass->attachments)) {
            // this is a swapchain-pass
            res.swapchain.sample_count = _sg_def(res.swapchain.sample_count, _sg.desc.environment.defaults.sample_count);
            res.swapchain.color_format = _sg_def(res.swapchain.color_format, _sg.desc.environment.defaults.color_format);
            res.swapchain.depth_format = _sg_def(res.swapchain.depth_format, _sg.desc.environment.defaults.depth_format);
        }
        res.action = _sg_pass_action_defaults(&res.action);
    }
    return res;
}

_SOKOL_PRIVATE void _sg_discard_all_resources(void) {
    /*  this is a bit dumb since it loops over all pool slots to
        find the occupied slots, on the other hand it is only ever
        executed at shutdown
        NOTE: ONLY EXECUTE THIS AT SHUTDOWN
              ...because the free queues will not be reset
              and the resource slots not be cleared!
    */
    for (int i = 1; i < _sg.pools.buffer_pool.size; i++) {
        sg_resource_state state = _sg.pools.buffers[i].slot.state;
        if ((state == SG_RESOURCESTATE_VALID) || (state == SG_RESOURCESTATE_FAILED)) {
            _sg_discard_buffer(&_sg.pools.buffers[i]);
        }
    }
    for (int i = 1; i < _sg.pools.image_pool.size; i++) {
        sg_resource_state state = _sg.pools.images[i].slot.state;
        if ((state == SG_RESOURCESTATE_VALID) || (state == SG_RESOURCESTATE_FAILED)) {
            _sg_discard_image(&_sg.pools.images[i]);
        }
    }
    for (int i = 1; i < _sg.pools.sampler_pool.size; i++) {
        sg_resource_state state = _sg.pools.samplers[i].slot.state;
        if ((state == SG_RESOURCESTATE_VALID) || (state == SG_RESOURCESTATE_FAILED)) {
            _sg_discard_sampler(&_sg.pools.samplers[i]);
        }
    }
    for (int i = 1; i < _sg.pools.shader_pool.size; i++) {
        sg_resource_state state = _sg.pools.shaders[i].slot.state;
        if ((state == SG_RESOURCESTATE_VALID) || (state == SG_RESOURCESTATE_FAILED)) {
            _sg_discard_shader(&_sg.pools.shaders[i]);
        }
    }
    for (int i = 1; i < _sg.pools.pipeline_pool.size; i++) {
        sg_resource_state state = _sg.pools.pipelines[i].slot.state;
        if ((state == SG_RESOURCESTATE_VALID) || (state == SG_RESOURCESTATE_FAILED)) {
            _sg_discard_pipeline(&_sg.pools.pipelines[i]);
        }
    }
    for (int i = 1; i < _sg.pools.view_pool.size; i++) {
        sg_resource_state state = _sg.pools.views[i].slot.state;
        if ((state == SG_RESOURCESTATE_VALID) || (state == SG_RESOURCESTATE_FAILED)) {
            _sg_discard_view(&_sg.pools.views[i]);
        }
    }
}

// ██████  ██    ██ ██████  ██      ██  ██████
// ██   ██ ██    ██ ██   ██ ██      ██ ██
// ██████  ██    ██ ██████  ██      ██ ██
// ██      ██    ██ ██   ██ ██      ██ ██
// ██       ██████  ██████  ███████ ██  ██████
//
// >>public
SOKOL_API_IMPL void sg_setup(const sg_desc* desc) {
    SOKOL_ASSERT(desc);
    SOKOL_ASSERT((desc->_start_canary == 0) && (desc->_end_canary == 0));
    SOKOL_ASSERT((desc->allocator.alloc_fn && desc->allocator.free_fn) || (!desc->allocator.alloc_fn && !desc->allocator.free_fn));
    _SG_CLEAR_ARC_STRUCT(_sg_state_t, _sg);
    _sg.desc = _sg_desc_defaults(desc);
    _sg_setup_pools(&_sg.pools, &_sg.desc);
    _sg_setup_commit_listeners(&_sg.desc);
    _sg.frame_index = 1;
    _sg.stats_enabled = true;
    _sg_setup_backend(&_sg.desc);
    _sg.valid = true;
}

SOKOL_API_IMPL void sg_shutdown(void) {
    _sg_discard_all_resources();
    _sg_discard_backend();
    _sg_discard_commit_listeners();
    _sg_discard_pools(&_sg.pools);
    _SG_CLEAR_ARC_STRUCT(_sg_state_t, _sg);
}

SOKOL_API_IMPL bool sg_isvalid(void) {
    return _sg.valid;
}

SOKOL_API_IMPL sg_desc sg_query_desc(void) {
    SOKOL_ASSERT(_sg.valid);
    return _sg.desc;
}

SOKOL_API_IMPL sg_backend sg_query_backend(void) {
    SOKOL_ASSERT(_sg.valid);
    return _sg.backend;
}

SOKOL_API_IMPL sg_features sg_query_features(void) {
    SOKOL_ASSERT(_sg.valid);
    return _sg.features;
}

SOKOL_API_IMPL sg_limits sg_query_limits(void) {
    SOKOL_ASSERT(_sg.valid);
    return _sg.limits;
}

SOKOL_API_IMPL sg_pixelformat_info sg_query_pixelformat(sg_pixel_format fmt) {
    SOKOL_ASSERT(_sg.valid);
    int fmt_index = (int) fmt;
    SOKOL_ASSERT((fmt_index > SG_PIXELFORMAT_NONE) && (fmt_index < _SG_PIXELFORMAT_NUM));
    const _sg_pixelformat_info_t* src = &_sg.formats[fmt_index];
    sg_pixelformat_info res;
    _sg_clear(&res, sizeof(res));
    res.sample = src->sample;
    res.filter = src->filter;
    res.render = src->render;
    res.blend = src->blend;
    res.msaa = src->msaa;
    res.depth = src->depth;
    res.compressed = _sg_is_compressed_pixel_format(fmt);
    res.read = src->read;
    res.write = src->write;
    if (!res.compressed) {
        res.bytes_per_pixel = _sg_pixelformat_bytesize(fmt);
    }
    return res;
}

SOKOL_API_IMPL int sg_query_row_pitch(sg_pixel_format fmt, int width, int row_align_bytes) {
    SOKOL_ASSERT(_sg.valid);
    SOKOL_ASSERT(width > 0);
    SOKOL_ASSERT((row_align_bytes > 0) && _sg_ispow2(row_align_bytes));
    SOKOL_ASSERT(((int)fmt > SG_PIXELFORMAT_NONE) && ((int)fmt < _SG_PIXELFORMAT_NUM));
    return _sg_row_pitch(fmt, width, row_align_bytes);
}

SOKOL_API_IMPL int sg_query_surface_pitch(sg_pixel_format fmt, int width, int height, int row_align_bytes) {
    SOKOL_ASSERT(_sg.valid);
    SOKOL_ASSERT((width > 0) && (height > 0));
    SOKOL_ASSERT((row_align_bytes > 0) && _sg_ispow2(row_align_bytes));
    SOKOL_ASSERT(((int)fmt > SG_PIXELFORMAT_NONE) && ((int)fmt < _SG_PIXELFORMAT_NUM));
    return _sg_surface_pitch(fmt, width, height, row_align_bytes);
}

SOKOL_API_IMPL sg_frame_stats sg_query_frame_stats(void) {
    SOKOL_ASSERT(_sg.valid);
    return _sg.prev_stats;
}

SOKOL_API_IMPL sg_trace_hooks sg_install_trace_hooks(const sg_trace_hooks* trace_hooks) {
    SOKOL_ASSERT(_sg.valid);
    SOKOL_ASSERT(trace_hooks);
    _SOKOL_UNUSED(trace_hooks);
    #if defined(SOKOL_TRACE_HOOKS)
        sg_trace_hooks old_hooks = _sg.hooks;
        _sg.hooks = *trace_hooks;
    #else
        static sg_trace_hooks old_hooks;
        _SG_WARN(TRACE_HOOKS_NOT_ENABLED);
    #endif
    return old_hooks;
}

SOKOL_API_IMPL sg_buffer sg_alloc_buffer(void) {
    SOKOL_ASSERT(_sg.valid);
    sg_buffer res = _sg_alloc_buffer();
    _SG_TRACE_ARGS(alloc_buffer, res);
    return res;
}

SOKOL_API_IMPL sg_image sg_alloc_image(void) {
    SOKOL_ASSERT(_sg.valid);
    sg_image res = _sg_alloc_image();
    _SG_TRACE_ARGS(alloc_image, res);
    return res;
}

SOKOL_API_IMPL sg_sampler sg_alloc_sampler(void) {
    SOKOL_ASSERT(_sg.valid);
    sg_sampler res = _sg_alloc_sampler();
    _SG_TRACE_ARGS(alloc_sampler, res);
    return res;
}

SOKOL_API_IMPL sg_shader sg_alloc_shader(void) {
    SOKOL_ASSERT(_sg.valid);
    sg_shader res = _sg_alloc_shader();
    _SG_TRACE_ARGS(alloc_shader, res);
    return res;
}

SOKOL_API_IMPL sg_pipeline sg_alloc_pipeline(void) {
    SOKOL_ASSERT(_sg.valid);
    sg_pipeline res = _sg_alloc_pipeline();
    _SG_TRACE_ARGS(alloc_pipeline, res);
    return res;
}

SOKOL_API_IMPL sg_view sg_alloc_view(void) {
    SOKOL_ASSERT(_sg.valid);
    sg_view res = _sg_alloc_view();
    _SG_TRACE_ARGS(alloc_view, res);
    return res;
}

SOKOL_API_IMPL void sg_dealloc_buffer(sg_buffer buf_id) {
    SOKOL_ASSERT(_sg.valid);
    _sg_buffer_t* buf = _sg_lookup_buffer(buf_id.id);
    if (buf) {
        if (buf->slot.state == SG_RESOURCESTATE_ALLOC) {
            _sg_dealloc_buffer(buf);
        } else {
            _SG_ERROR(DEALLOC_BUFFER_INVALID_STATE);
        }
    }
    _SG_TRACE_ARGS(dealloc_buffer, buf_id);
}

SOKOL_API_IMPL void sg_dealloc_image(sg_image img_id) {
    SOKOL_ASSERT(_sg.valid);
    _sg_image_t* img = _sg_lookup_image(img_id.id);
    if (img) {
        if (img->slot.state == SG_RESOURCESTATE_ALLOC) {
            _sg_dealloc_image(img);
        } else {
            _SG_ERROR(DEALLOC_IMAGE_INVALID_STATE);
        }
    }
    _SG_TRACE_ARGS(dealloc_image, img_id);
}

SOKOL_API_IMPL void sg_dealloc_sampler(sg_sampler smp_id) {
    SOKOL_ASSERT(_sg.valid);
    _sg_sampler_t* smp = _sg_lookup_sampler(smp_id.id);
    if (smp) {
        if (smp->slot.state == SG_RESOURCESTATE_ALLOC) {
            _sg_dealloc_sampler(smp);
        } else {
            _SG_ERROR(DEALLOC_SAMPLER_INVALID_STATE);
        }
    }
    _SG_TRACE_ARGS(dealloc_sampler, smp_id);
}

SOKOL_API_IMPL void sg_dealloc_shader(sg_shader shd_id) {
    SOKOL_ASSERT(_sg.valid);
    _sg_shader_t* shd = _sg_lookup_shader(shd_id.id);
    if (shd) {
        if (shd->slot.state == SG_RESOURCESTATE_ALLOC) {
            _sg_dealloc_shader(shd);
        } else {
            _SG_ERROR(DEALLOC_SHADER_INVALID_STATE);
        }
    }
    _SG_TRACE_ARGS(dealloc_shader, shd_id);
}

SOKOL_API_IMPL void sg_dealloc_pipeline(sg_pipeline pip_id) {
    SOKOL_ASSERT(_sg.valid);
    _sg_pipeline_t* pip = _sg_lookup_pipeline(pip_id.id);
    if (pip) {
        if (pip->slot.state == SG_RESOURCESTATE_ALLOC) {
            _sg_dealloc_pipeline(pip);
        } else {
            _SG_ERROR(DEALLOC_PIPELINE_INVALID_STATE);
        }
    }
    _SG_TRACE_ARGS(dealloc_pipeline, pip_id);
}

SOKOL_API_IMPL void sg_dealloc_view(sg_view view_id) {
    SOKOL_ASSERT(_sg.valid);
    _sg_view_t* view = _sg_lookup_view(view_id.id);
    if (view) {
        if (view->slot.state == SG_RESOURCESTATE_ALLOC) {
            _sg_dealloc_view(view);
        } else {
            _SG_ERROR(DEALLOC_VIEW_INVALID_STATE);
        }
    }
    _SG_TRACE_ARGS(dealloc_view, view_id);
}

SOKOL_API_IMPL void sg_init_buffer(sg_buffer buf_id, const sg_buffer_desc* desc) {
    SOKOL_ASSERT(_sg.valid);
    sg_buffer_desc desc_def = _sg_buffer_desc_defaults(desc);
    _sg_buffer_t* buf = _sg_lookup_buffer(buf_id.id);
    if (buf) {
        if (buf->slot.state == SG_RESOURCESTATE_ALLOC) {
            _sg_init_buffer(buf, &desc_def);
            SOKOL_ASSERT((buf->slot.state == SG_RESOURCESTATE_VALID) || (buf->slot.state == SG_RESOURCESTATE_FAILED));
        } else {
            _SG_ERROR(INIT_BUFFER_INVALID_STATE);
        }
    }
    _SG_TRACE_ARGS(init_buffer, buf_id, &desc_def);
}

SOKOL_API_IMPL void sg_init_image(sg_image img_id, const sg_image_desc* desc) {
    SOKOL_ASSERT(_sg.valid);
    sg_image_desc desc_def = _sg_image_desc_defaults(desc);
    _sg_image_t* img = _sg_lookup_image(img_id.id);
    if (img) {
        if (img->slot.state == SG_RESOURCESTATE_ALLOC) {
            _sg_init_image(img, &desc_def);
            SOKOL_ASSERT((img->slot.state == SG_RESOURCESTATE_VALID) || (img->slot.state == SG_RESOURCESTATE_FAILED));
        } else {
            _SG_ERROR(INIT_IMAGE_INVALID_STATE);
        }
    }
    _SG_TRACE_ARGS(init_image, img_id, &desc_def);
}

SOKOL_API_IMPL void sg_init_sampler(sg_sampler smp_id, const sg_sampler_desc* desc) {
    SOKOL_ASSERT(_sg.valid);
    sg_sampler_desc desc_def = _sg_sampler_desc_defaults(desc);
    _sg_sampler_t* smp = _sg_lookup_sampler(smp_id.id);
    if (smp) {
        if (smp->slot.state == SG_RESOURCESTATE_ALLOC) {
            _sg_init_sampler(smp, &desc_def);
            SOKOL_ASSERT((smp->slot.state == SG_RESOURCESTATE_VALID) || (smp->slot.state == SG_RESOURCESTATE_FAILED));
        } else {
            _SG_ERROR(INIT_SAMPLER_INVALID_STATE);
        }
    }
    _SG_TRACE_ARGS(init_sampler, smp_id, &desc_def);
}

SOKOL_API_IMPL void sg_init_shader(sg_shader shd_id, const sg_shader_desc* desc) {
    SOKOL_ASSERT(_sg.valid);
    sg_shader_desc desc_def = _sg_shader_desc_defaults(desc);
    _sg_shader_t* shd = _sg_lookup_shader(shd_id.id);
    if (shd) {
        if (shd->slot.state == SG_RESOURCESTATE_ALLOC) {
            _sg_init_shader(shd, &desc_def);
            SOKOL_ASSERT((shd->slot.state == SG_RESOURCESTATE_VALID) || (shd->slot.state == SG_RESOURCESTATE_FAILED));
        } else {
            _SG_ERROR(INIT_SHADER_INVALID_STATE);
        }
    }
    _SG_TRACE_ARGS(init_shader, shd_id, &desc_def);
}

SOKOL_API_IMPL void sg_init_pipeline(sg_pipeline pip_id, const sg_pipeline_desc* desc) {
    SOKOL_ASSERT(_sg.valid);
    sg_pipeline_desc desc_def = _sg_pipeline_desc_defaults(desc);
    _sg_pipeline_t* pip = _sg_lookup_pipeline(pip_id.id);
    if (pip) {
        if (pip->slot.state == SG_RESOURCESTATE_ALLOC) {
            _sg_init_pipeline(pip, &desc_def);
            SOKOL_ASSERT((pip->slot.state == SG_RESOURCESTATE_VALID) || (pip->slot.state == SG_RESOURCESTATE_FAILED));
        } else {
            _SG_ERROR(INIT_PIPELINE_INVALID_STATE);
        }
    }
    _SG_TRACE_ARGS(init_pipeline, pip_id, &desc_def);
}

SOKOL_API_IMPL void sg_init_view(sg_view view_id, const sg_view_desc* desc) {
    SOKOL_ASSERT(_sg.valid);
    sg_view_desc desc_def = _sg_view_desc_defaults(desc);
    _sg_view_t* view = _sg_lookup_view(view_id.id);
    if (view) {
        if (view->slot.state == SG_RESOURCESTATE_ALLOC) {
            _sg_init_view(view, &desc_def);
            SOKOL_ASSERT((view->slot.state == SG_RESOURCESTATE_VALID)
                || (view->slot.state == SG_RESOURCESTATE_FAILED)
                || (view->slot.state == SG_RESOURCESTATE_ALLOC));
        } else {
            _SG_ERROR(INIT_VIEW_INVALID_STATE);
        }
    }
    _SG_TRACE_ARGS(init_view, view_id, &desc_def);
}

SOKOL_API_IMPL void sg_uninit_buffer(sg_buffer buf_id) {
    SOKOL_ASSERT(_sg.valid);
    _sg_buffer_t* buf = _sg_lookup_buffer(buf_id.id);
    if (buf) {
        if ((buf->slot.state == SG_RESOURCESTATE_VALID) || (buf->slot.state == SG_RESOURCESTATE_FAILED)) {
            _sg_uninit_buffer(buf);
            SOKOL_ASSERT(buf->slot.state == SG_RESOURCESTATE_ALLOC);
        } else if (buf->slot.state != SG_RESOURCESTATE_ALLOC) {
            _SG_ERROR(UNINIT_BUFFER_INVALID_STATE);
        }
    }
    _SG_TRACE_ARGS(uninit_buffer, buf_id);
}

SOKOL_API_IMPL void sg_uninit_image(sg_image img_id) {
    SOKOL_ASSERT(_sg.valid);
    _sg_image_t* img = _sg_lookup_image(img_id.id);
    if (img) {
        if ((img->slot.state == SG_RESOURCESTATE_VALID) || (img->slot.state == SG_RESOURCESTATE_FAILED)) {
            _sg_uninit_image(img);
            SOKOL_ASSERT(img->slot.state == SG_RESOURCESTATE_ALLOC);
        } else if (img->slot.state != SG_RESOURCESTATE_ALLOC) {
            _SG_ERROR(UNINIT_IMAGE_INVALID_STATE);
        }
    }
    _SG_TRACE_ARGS(uninit_image, img_id);
}

SOKOL_API_IMPL void sg_uninit_sampler(sg_sampler smp_id) {
    SOKOL_ASSERT(_sg.valid);
    _sg_sampler_t* smp = _sg_lookup_sampler(smp_id.id);
    if (smp) {
        if ((smp->slot.state == SG_RESOURCESTATE_VALID) || (smp->slot.state == SG_RESOURCESTATE_FAILED)) {
            _sg_uninit_sampler(smp);
            SOKOL_ASSERT(smp->slot.state == SG_RESOURCESTATE_ALLOC);
        } else if (smp->slot.state != SG_RESOURCESTATE_ALLOC) {
            _SG_ERROR(UNINIT_SAMPLER_INVALID_STATE);
        }
    }
    _SG_TRACE_ARGS(uninit_sampler, smp_id);
}

SOKOL_API_IMPL void sg_uninit_shader(sg_shader shd_id) {
    SOKOL_ASSERT(_sg.valid);
    _sg_shader_t* shd = _sg_lookup_shader(shd_id.id);
    if (shd) {
        if ((shd->slot.state == SG_RESOURCESTATE_VALID) || (shd->slot.state == SG_RESOURCESTATE_FAILED)) {
            _sg_uninit_shader(shd);
            SOKOL_ASSERT(shd->slot.state == SG_RESOURCESTATE_ALLOC);
        } else if (shd->slot.state != SG_RESOURCESTATE_ALLOC) {
            _SG_ERROR(UNINIT_SHADER_INVALID_STATE);
        }
    }
    _SG_TRACE_ARGS(uninit_shader, shd_id);
}

SOKOL_API_IMPL void sg_uninit_pipeline(sg_pipeline pip_id) {
    SOKOL_ASSERT(_sg.valid);
    _sg_pipeline_t* pip = _sg_lookup_pipeline(pip_id.id);
    if (pip) {
        if ((pip->slot.state == SG_RESOURCESTATE_VALID) || (pip->slot.state == SG_RESOURCESTATE_FAILED)) {
            _sg_uninit_pipeline(pip);
            SOKOL_ASSERT(pip->slot.state == SG_RESOURCESTATE_ALLOC);
        } else if (pip->slot.state != SG_RESOURCESTATE_ALLOC) {
            _SG_ERROR(UNINIT_PIPELINE_INVALID_STATE);
        }
    }
    _SG_TRACE_ARGS(uninit_pipeline, pip_id);
}

SOKOL_API_IMPL void sg_uninit_view(sg_view view_id) {
    SOKOL_ASSERT(_sg.valid);
    _sg_view_t* view = _sg_lookup_view(view_id.id);
    if (view) {
        if ((view->slot.state == SG_RESOURCESTATE_VALID) || (view->slot.state == SG_RESOURCESTATE_FAILED)) {
            _sg_uninit_view(view);
            SOKOL_ASSERT(view->slot.state == SG_RESOURCESTATE_ALLOC);
        } else if (view->slot.state != SG_RESOURCESTATE_ALLOC) {
            _SG_ERROR(UNINIT_VIEW_INVALID_STATE);
        }
    }
    _SG_TRACE_ARGS(uninit_view, view_id);
}

SOKOL_API_IMPL void sg_fail_buffer(sg_buffer buf_id) {
    SOKOL_ASSERT(_sg.valid);
    _sg_buffer_t* buf = _sg_lookup_buffer(buf_id.id);
    if (buf) {
        if (buf->slot.state == SG_RESOURCESTATE_ALLOC) {
            buf->slot.state = SG_RESOURCESTATE_FAILED;
        } else {
            _SG_ERROR(FAIL_BUFFER_INVALID_STATE);
        }
    }
    _SG_TRACE_ARGS(fail_buffer, buf_id);
}

SOKOL_API_IMPL void sg_fail_image(sg_image img_id) {
    SOKOL_ASSERT(_sg.valid);
    _sg_image_t* img = _sg_lookup_image(img_id.id);
    if (img) {
        if (img->slot.state == SG_RESOURCESTATE_ALLOC) {
            img->slot.state = SG_RESOURCESTATE_FAILED;
        } else {
            _SG_ERROR(FAIL_IMAGE_INVALID_STATE);
        }
    }
    _SG_TRACE_ARGS(fail_image, img_id);
}

SOKOL_API_IMPL void sg_fail_sampler(sg_sampler smp_id) {
    SOKOL_ASSERT(_sg.valid);
    _sg_sampler_t* smp = _sg_lookup_sampler(smp_id.id);
    if (smp) {
        if (smp->slot.state == SG_RESOURCESTATE_ALLOC) {
            smp->slot.state = SG_RESOURCESTATE_FAILED;
        } else {
            _SG_ERROR(FAIL_SAMPLER_INVALID_STATE);
        }
    }
    _SG_TRACE_ARGS(fail_sampler, smp_id);
}

SOKOL_API_IMPL void sg_fail_shader(sg_shader shd_id) {
    SOKOL_ASSERT(_sg.valid);
    _sg_shader_t* shd = _sg_lookup_shader(shd_id.id);
    if (shd) {
        if (shd->slot.state == SG_RESOURCESTATE_ALLOC) {
            shd->slot.state = SG_RESOURCESTATE_FAILED;
        } else {
            _SG_ERROR(FAIL_SHADER_INVALID_STATE);
        }
    }
    _SG_TRACE_ARGS(fail_shader, shd_id);
}

SOKOL_API_IMPL void sg_fail_pipeline(sg_pipeline pip_id) {
    SOKOL_ASSERT(_sg.valid);
    _sg_pipeline_t* pip = _sg_lookup_pipeline(pip_id.id);
    if (pip) {
        if (pip->slot.state == SG_RESOURCESTATE_ALLOC) {
            pip->slot.state = SG_RESOURCESTATE_FAILED;
        } else {
            _SG_ERROR(FAIL_PIPELINE_INVALID_STATE);
        }
    }
    _SG_TRACE_ARGS(fail_pipeline, pip_id);
}

SOKOL_API_IMPL void sg_fail_view(sg_view view_id) {
    SOKOL_ASSERT(_sg.valid);
    _sg_view_t* view = _sg_lookup_view(view_id.id);
    if (view) {
        if (view->slot.state == SG_RESOURCESTATE_ALLOC) {
            view->slot.state = SG_RESOURCESTATE_FAILED;
        } else {
            _SG_ERROR(FAIL_VIEW_INVALID_STATE);
        }
    }
    _SG_TRACE_ARGS(fail_view, view_id);
}

SOKOL_API_IMPL sg_resource_state sg_query_buffer_state(sg_buffer buf_id) {
    SOKOL_ASSERT(_sg.valid);
    _sg_buffer_t* buf = _sg_lookup_buffer(buf_id.id);
    sg_resource_state res = buf ? buf->slot.state : SG_RESOURCESTATE_INVALID;
    return res;
}

SOKOL_API_IMPL sg_resource_state sg_query_image_state(sg_image img_id) {
    SOKOL_ASSERT(_sg.valid);
    _sg_image_t* img = _sg_lookup_image(img_id.id);
    sg_resource_state res = img ? img->slot.state : SG_RESOURCESTATE_INVALID;
    return res;
}

SOKOL_API_IMPL sg_resource_state sg_query_sampler_state(sg_sampler smp_id) {
    SOKOL_ASSERT(_sg.valid);
    _sg_sampler_t* smp = _sg_lookup_sampler(smp_id.id);
    sg_resource_state res = smp ? smp->slot.state : SG_RESOURCESTATE_INVALID;
    return res;
}

SOKOL_API_IMPL sg_resource_state sg_query_shader_state(sg_shader shd_id) {
    SOKOL_ASSERT(_sg.valid);
    _sg_shader_t* shd = _sg_lookup_shader(shd_id.id);
    sg_resource_state res = shd ? shd->slot.state : SG_RESOURCESTATE_INVALID;
    return res;
}

SOKOL_API_IMPL sg_resource_state sg_query_pipeline_state(sg_pipeline pip_id) {
    SOKOL_ASSERT(_sg.valid);
    _sg_pipeline_t* pip = _sg_lookup_pipeline(pip_id.id);
    sg_resource_state res = pip ? pip->slot.state : SG_RESOURCESTATE_INVALID;
    return res;
}

SOKOL_API_IMPL sg_resource_state sg_query_view_state(sg_view view_id) {
    SOKOL_ASSERT(_sg.valid);
    _sg_view_t* view = _sg_lookup_view(view_id.id);
    sg_resource_state res = view ? view->slot.state : SG_RESOURCESTATE_INVALID;
    return res;
}

SOKOL_API_IMPL sg_buffer sg_make_buffer(const sg_buffer_desc* desc) {
    SOKOL_ASSERT(_sg.valid);
    SOKOL_ASSERT(desc);
    sg_buffer_desc desc_def = _sg_buffer_desc_defaults(desc);
    sg_buffer buf_id = _sg_alloc_buffer();
    if (buf_id.id != SG_INVALID_ID) {
        _sg_buffer_t* buf = _sg_buffer_at(buf_id.id);
        SOKOL_ASSERT(buf && (buf->slot.state == SG_RESOURCESTATE_ALLOC));
        _sg_init_buffer(buf, &desc_def);
        SOKOL_ASSERT((buf->slot.state == SG_RESOURCESTATE_VALID) || (buf->slot.state == SG_RESOURCESTATE_FAILED));
    }
    _SG_TRACE_ARGS(make_buffer, &desc_def, buf_id);
    return buf_id;
}

SOKOL_API_IMPL sg_image sg_make_image(const sg_image_desc* desc) {
    SOKOL_ASSERT(_sg.valid);
    SOKOL_ASSERT(desc);
    sg_image_desc desc_def = _sg_image_desc_defaults(desc);
    sg_image img_id = _sg_alloc_image();
    if (img_id.id != SG_INVALID_ID) {
        _sg_image_t* img = _sg_image_at(img_id.id);
        SOKOL_ASSERT(img && (img->slot.state == SG_RESOURCESTATE_ALLOC));
        _sg_init_image(img, &desc_def);
        SOKOL_ASSERT((img->slot.state == SG_RESOURCESTATE_VALID) || (img->slot.state == SG_RESOURCESTATE_FAILED));
    }
    _SG_TRACE_ARGS(make_image, &desc_def, img_id);
    return img_id;
}

SOKOL_API_IMPL sg_sampler sg_make_sampler(const sg_sampler_desc* desc) {
    SOKOL_ASSERT(_sg.valid);
    SOKOL_ASSERT(desc);
    sg_sampler_desc desc_def = _sg_sampler_desc_defaults(desc);
    sg_sampler smp_id = _sg_alloc_sampler();
    if (smp_id.id != SG_INVALID_ID) {
        _sg_sampler_t* smp = _sg_sampler_at(smp_id.id);
        SOKOL_ASSERT(smp && (smp->slot.state == SG_RESOURCESTATE_ALLOC));
        _sg_init_sampler(smp, &desc_def);
        SOKOL_ASSERT((smp->slot.state == SG_RESOURCESTATE_VALID) || (smp->slot.state == SG_RESOURCESTATE_FAILED));
    }
    _SG_TRACE_ARGS(make_sampler, &desc_def, smp_id);
    return smp_id;
}

SOKOL_API_IMPL sg_shader sg_make_shader(const sg_shader_desc* desc) {
    SOKOL_ASSERT(_sg.valid);
    SOKOL_ASSERT(desc);
    sg_shader_desc desc_def = _sg_shader_desc_defaults(desc);
    sg_shader shd_id = _sg_alloc_shader();
    if (shd_id.id != SG_INVALID_ID) {
        _sg_shader_t* shd = _sg_shader_at(shd_id.id);
        SOKOL_ASSERT(shd && (shd->slot.state == SG_RESOURCESTATE_ALLOC));
        _sg_init_shader(shd, &desc_def);
        SOKOL_ASSERT((shd->slot.state == SG_RESOURCESTATE_VALID) || (shd->slot.state == SG_RESOURCESTATE_FAILED));
    }
    _SG_TRACE_ARGS(make_shader, &desc_def, shd_id);
    return shd_id;
}

SOKOL_API_IMPL sg_pipeline sg_make_pipeline(const sg_pipeline_desc* desc) {
    SOKOL_ASSERT(_sg.valid);
    SOKOL_ASSERT(desc);
    sg_pipeline_desc desc_def = _sg_pipeline_desc_defaults(desc);
    sg_pipeline pip_id = _sg_alloc_pipeline();
    if (pip_id.id != SG_INVALID_ID) {
        _sg_pipeline_t* pip = _sg_pipeline_at(pip_id.id);
        SOKOL_ASSERT(pip && (pip->slot.state == SG_RESOURCESTATE_ALLOC));
        _sg_init_pipeline(pip, &desc_def);
        SOKOL_ASSERT((pip->slot.state == SG_RESOURCESTATE_VALID) || (pip->slot.state == SG_RESOURCESTATE_FAILED));
    }
    _SG_TRACE_ARGS(make_pipeline, &desc_def, pip_id);
    return pip_id;
}

SOKOL_API_IMPL sg_view sg_make_view(const sg_view_desc* desc) {
    SOKOL_ASSERT(_sg.valid);
    SOKOL_ASSERT(desc);
    sg_view_desc desc_def = _sg_view_desc_defaults(desc);
    sg_view view_id = _sg_alloc_view();
    if (view_id.id != SG_INVALID_ID) {
        _sg_view_t* view = _sg_view_at(view_id.id);
        SOKOL_ASSERT(view && (view->slot.state == SG_RESOURCESTATE_ALLOC));
        _sg_init_view(view, &desc_def);
        SOKOL_ASSERT((view->slot.state == SG_RESOURCESTATE_VALID) || (view->slot.state == SG_RESOURCESTATE_FAILED));
    }
    _SG_TRACE_ARGS(make_view, &desc_def, view_id);
    return view_id;
}

SOKOL_API_IMPL void sg_destroy_buffer(sg_buffer buf_id) {
    SOKOL_ASSERT(_sg.valid);
    _SG_TRACE_ARGS(destroy_buffer, buf_id);
    _sg_buffer_t* buf = _sg_lookup_buffer(buf_id.id);
    if (buf) {
        if ((buf->slot.state == SG_RESOURCESTATE_VALID) || (buf->slot.state == SG_RESOURCESTATE_FAILED)) {
            _sg_uninit_buffer(buf);
            SOKOL_ASSERT(buf->slot.state == SG_RESOURCESTATE_ALLOC);
        }
        if (buf->slot.state == SG_RESOURCESTATE_ALLOC) {
            _sg_dealloc_buffer(buf);
            SOKOL_ASSERT(buf->slot.state == SG_RESOURCESTATE_INITIAL);
        }
    }
}

SOKOL_API_IMPL void sg_destroy_image(sg_image img_id) {
    SOKOL_ASSERT(_sg.valid);
    _SG_TRACE_ARGS(destroy_image, img_id);
    _sg_image_t* img = _sg_lookup_image(img_id.id);
    if (img) {
        if ((img->slot.state == SG_RESOURCESTATE_VALID) || (img->slot.state == SG_RESOURCESTATE_FAILED)) {
            _sg_uninit_image(img);
            SOKOL_ASSERT(img->slot.state == SG_RESOURCESTATE_ALLOC);
        }
        if (img->slot.state == SG_RESOURCESTATE_ALLOC) {
            _sg_dealloc_image(img);
            SOKOL_ASSERT(img->slot.state == SG_RESOURCESTATE_INITIAL);
        }
    }
}

SOKOL_API_IMPL void sg_destroy_sampler(sg_sampler smp_id) {
    SOKOL_ASSERT(_sg.valid);
    _SG_TRACE_ARGS(destroy_sampler, smp_id);
    _sg_sampler_t* smp = _sg_lookup_sampler(smp_id.id);
    if (smp) {
        if ((smp->slot.state == SG_RESOURCESTATE_VALID) || (smp->slot.state == SG_RESOURCESTATE_FAILED)) {
            _sg_uninit_sampler(smp);
            SOKOL_ASSERT(smp->slot.state == SG_RESOURCESTATE_ALLOC);
        }
        if (smp->slot.state == SG_RESOURCESTATE_ALLOC) {
            _sg_dealloc_sampler(smp);
            SOKOL_ASSERT(smp->slot.state == SG_RESOURCESTATE_INITIAL);
        }
    }
}

SOKOL_API_IMPL void sg_destroy_shader(sg_shader shd_id) {
    SOKOL_ASSERT(_sg.valid);
    _SG_TRACE_ARGS(destroy_shader, shd_id);
    _sg_shader_t* shd = _sg_lookup_shader(shd_id.id);
    if (shd) {
        if ((shd->slot.state == SG_RESOURCESTATE_VALID) || (shd->slot.state == SG_RESOURCESTATE_FAILED)) {
            _sg_uninit_shader(shd);
            SOKOL_ASSERT(shd->slot.state == SG_RESOURCESTATE_ALLOC);
        }
        if (shd->slot.state == SG_RESOURCESTATE_ALLOC) {
            _sg_dealloc_shader(shd);
            SOKOL_ASSERT(shd->slot.state == SG_RESOURCESTATE_INITIAL);
        }
    }
}

SOKOL_API_IMPL void sg_destroy_pipeline(sg_pipeline pip_id) {
    SOKOL_ASSERT(_sg.valid);
    _SG_TRACE_ARGS(destroy_pipeline, pip_id);
    _sg_pipeline_t* pip = _sg_lookup_pipeline(pip_id.id);
    if (pip) {
        if ((pip->slot.state == SG_RESOURCESTATE_VALID) || (pip->slot.state == SG_RESOURCESTATE_FAILED)) {
            _sg_uninit_pipeline(pip);
            SOKOL_ASSERT(pip->slot.state == SG_RESOURCESTATE_ALLOC);
        }
        if (pip->slot.state == SG_RESOURCESTATE_ALLOC) {
            _sg_dealloc_pipeline(pip);
            SOKOL_ASSERT(pip->slot.state == SG_RESOURCESTATE_INITIAL);
        }
    }
}

SOKOL_API_IMPL void sg_destroy_view(sg_view view_id) {
    SOKOL_ASSERT(_sg.valid);
    _SG_TRACE_ARGS(destroy_view, view_id);
    _sg_view_t* view = _sg_lookup_view(view_id.id);
    if (view) {
        if ((view->slot.state == SG_RESOURCESTATE_VALID) || (view->slot.state == SG_RESOURCESTATE_FAILED)) {
            _sg_uninit_view(view);
            SOKOL_ASSERT(view->slot.state == SG_RESOURCESTATE_ALLOC);
        }
        if (view->slot.state == SG_RESOURCESTATE_ALLOC) {
            _sg_dealloc_view(view);
            SOKOL_ASSERT(view->slot.state == SG_RESOURCESTATE_INITIAL);
        }
    }
}

SOKOL_API_IMPL void sg_begin_pass(const sg_pass* pass) {
    SOKOL_ASSERT(_sg.valid);
    SOKOL_ASSERT(!_sg.cur_pass.valid);
    SOKOL_ASSERT(!_sg.cur_pass.in_pass);
    SOKOL_ASSERT(_sg_attachments_empty(&_sg.cur_pass.atts));
    SOKOL_ASSERT(pass);
    SOKOL_ASSERT((pass->_start_canary == 0) && (pass->_end_canary == 0));
    const sg_pass pass_def = _sg_pass_defaults(pass);
    if (!_sg_validate_begin_pass(&pass_def)) {
        return;
    }
    const _sg_attachments_ptrs_t atts_ptrs = _sg_attachments_ptrs(&pass_def.attachments);
    if (!atts_ptrs.empty) {
        if (!_sg_attachments_alive(&atts_ptrs)) {
            _SG_ERROR(BEGINPASS_ATTACHMENTS_ALIVE);
            return;
        }
        _sg.cur_pass.atts = pass->attachments;
        _sg.cur_pass.dim = _sg_attachments_dim(&atts_ptrs);
    } else if (!pass_def.compute) {
        // a swapchain pass
        SOKOL_ASSERT(pass_def.swapchain.width > 0);
        SOKOL_ASSERT(pass_def.swapchain.height > 0);
        SOKOL_ASSERT(pass_def.swapchain.color_format > SG_PIXELFORMAT_NONE);
        SOKOL_ASSERT(pass_def.swapchain.sample_count > 0);
        _sg.cur_pass.dim.width = pass_def.swapchain.width;
        _sg.cur_pass.dim.height = pass_def.swapchain.height;
        _sg.cur_pass.swapchain.color_fmt = pass_def.swapchain.color_format;
        _sg.cur_pass.swapchain.depth_fmt = pass_def.swapchain.depth_format;
        _sg.cur_pass.swapchain.sample_count = pass_def.swapchain.sample_count;
    }
    _sg.cur_pass.valid = true;  // may be overruled by backend begin-pass functions
    _sg.cur_pass.in_pass = true;
    _sg.cur_pass.is_compute = pass_def.compute;
    _sg_begin_pass(&pass_def, &atts_ptrs);
    _SG_TRACE_ARGS(begin_pass, &pass_def);
}

SOKOL_API_IMPL void sg_apply_viewport(int x, int y, int width, int height, bool origin_top_left) {
    SOKOL_ASSERT(_sg.valid);
    #if defined(SOKOL_DEBUG)
    if (!_sg_validate_apply_viewport(x, y, width, height, origin_top_left)) {
        return;
    }
    #endif
    _sg_stats_add(num_apply_viewport, 1);
    if (!_sg.cur_pass.valid) {
        return;
    }
    _sg_apply_viewport(x, y, width, height, origin_top_left);
    _SG_TRACE_ARGS(apply_viewport, x, y, width, height, origin_top_left);
}

SOKOL_API_IMPL void sg_apply_viewportf(float x, float y, float width, float height, bool origin_top_left) {
    sg_apply_viewport((int)x, (int)y, (int)width, (int)height, origin_top_left);
}

SOKOL_API_IMPL void sg_apply_scissor_rect(int x, int y, int width, int height, bool origin_top_left) {
    SOKOL_ASSERT(_sg.valid);
    #if defined(SOKOL_DEBUG)
    if (!_sg_validate_apply_scissor_rect(x, y, width, height, origin_top_left)) {
        return;
    }
    #endif
    _sg_stats_add(num_apply_scissor_rect, 1);
    if (!_sg.cur_pass.valid) {
        return;
    }
    _sg_apply_scissor_rect(x, y, width, height, origin_top_left);
    _SG_TRACE_ARGS(apply_scissor_rect, x, y, width, height, origin_top_left);
}

SOKOL_API_IMPL void sg_apply_scissor_rectf(float x, float y, float width, float height, bool origin_top_left) {
    sg_apply_scissor_rect((int)x, (int)y, (int)width, (int)height, origin_top_left);
}

SOKOL_API_IMPL void sg_apply_pipeline(sg_pipeline pip_id) {
    SOKOL_ASSERT(_sg.valid);
    _sg_stats_add(num_apply_pipeline, 1);
    if (!_sg_validate_apply_pipeline(pip_id)) {
        _sg.next_draw_valid = false;
        return;
    }
    if (!_sg.cur_pass.valid) {
        return;
    }
    _sg_pipeline_t* pip = _sg_lookup_pipeline(pip_id.id);
    SOKOL_ASSERT(pip);
    _sg.cur_pip = _sg_pipeline_ref(pip);

    _sg.next_draw_valid = (SG_RESOURCESTATE_VALID == pip->slot.state);
    if (!_sg.next_draw_valid) {
        return;
    }

    _sg_apply_pipeline(pip);

    // set the expected bindings and uniform block flags
    const _sg_shader_t* shd = _sg_shader_ref_ptr(&pip->cmn.shader);
    _sg.required_bindings_and_uniforms = pip->cmn.required_bindings_and_uniforms | shd->cmn.required_bindings_and_uniforms;
    _sg.applied_bindings_and_uniforms = 0;

    _SG_TRACE_ARGS(apply_pipeline, pip_id);
}

SOKOL_API_IMPL void sg_apply_bindings(const sg_bindings* bindings) {
    SOKOL_ASSERT(_sg.valid);
    SOKOL_ASSERT(bindings);
    _sg_stats_add(num_apply_bindings, 1);
    _sg.applied_bindings_and_uniforms |= (1 << SG_MAX_UNIFORMBLOCK_BINDSLOTS);
    if (!_sg_validate_apply_bindings(bindings)) {
        _sg.next_draw_valid = false;
    }
    SOKOL_ASSERT((bindings->_start_canary == 0) && (bindings->_end_canary==0));
    if (!_sg_pipeline_ref_alive(&_sg.cur_pip)) {
        _sg.next_draw_valid = false;
    }
    if (!_sg.cur_pass.valid) {
        return;
    }
    if (!_sg.next_draw_valid) {
        return;
    }

    _sg_bindings_ptrs_t bnd;
    _sg_clear(&bnd, sizeof(bnd));
    bnd.pip = _sg_pipeline_ref_ptr(&_sg.cur_pip);
    const _sg_shader_t* shd = _sg_shader_ref_ptr(&bnd.pip->cmn.shader);
    if (!_sg.cur_pass.is_compute) {
        for (size_t i = 0; i < SG_MAX_VERTEXBUFFER_BINDSLOTS; i++) {
            if (bnd.pip->cmn.vertex_buffer_layout_active[i]) {
                SOKOL_ASSERT(bindings->vertex_buffers[i].id != SG_INVALID_ID);
                bnd.vbs[i] = _sg_lookup_buffer(bindings->vertex_buffers[i].id);
                bnd.vb_offsets[i] = bindings->vertex_buffer_offsets[i];
                _sg.next_draw_valid &= bnd.vbs[i] && (SG_RESOURCESTATE_VALID == bnd.vbs[i]->slot.state);
            }
        }
        if (bindings->index_buffer.id) {
            bnd.ib = _sg_lookup_buffer(bindings->index_buffer.id);
            bnd.ib_offset = bindings->index_buffer_offset;
            _sg.next_draw_valid &= bnd.ib && (SG_RESOURCESTATE_VALID == bnd.ib->slot.state);
        }
    }

    for (int i = 0; i < SG_MAX_VIEW_BINDSLOTS; i++) {
        if (shd->cmn.views[i].view_type != SG_VIEWTYPE_INVALID) {
            SOKOL_ASSERT(bindings->views[i].id != SG_INVALID_ID);
            bnd.views[i] = _sg_lookup_view(bindings->views[i].id);
            if (bnd.views[i]) {
                if (bnd.views[i]->cmn.type == SG_VIEWTYPE_STORAGEBUFFER) {
                    _sg.next_draw_valid &= _sg_buffer_ref_valid(&bnd.views[i]->cmn.buf.ref);
                } else {
                    _sg.next_draw_valid &= _sg_image_ref_valid(&bnd.views[i]->cmn.img.ref);
                }
            } else {
                _sg.next_draw_valid = false;
            }
        }
    }

    for (size_t i = 0; i < SG_MAX_SAMPLER_BINDSLOTS; i++) {
        if (shd->cmn.samplers[i].stage != SG_SHADERSTAGE_NONE) {
            SOKOL_ASSERT(bindings->samplers[i].id != SG_INVALID_ID);
            bnd.smps[i] = _sg_lookup_sampler(bindings->samplers[i].id);
            SOKOL_ASSERT(bnd.smps[i]);
        }
    }

    if (_sg.next_draw_valid) {
        _sg.next_draw_valid &= _sg_apply_bindings(&bnd);
        _SG_TRACE_ARGS(apply_bindings, bindings);
    }
}

SOKOL_API_IMPL void sg_apply_uniforms(int ub_slot, const sg_range* data) {
    SOKOL_ASSERT(_sg.valid);
    SOKOL_ASSERT((ub_slot >= 0) && (ub_slot < SG_MAX_UNIFORMBLOCK_BINDSLOTS));
    SOKOL_ASSERT(data && data->ptr && (data->size > 0));
    _sg_stats_add(num_apply_uniforms, 1);
    _sg_stats_add(size_apply_uniforms, (uint32_t)data->size);
    _sg.applied_bindings_and_uniforms |= 1 << ub_slot;
    if (!_sg_validate_apply_uniforms(ub_slot, data)) {
        _sg.next_draw_valid = false;
        return;
    }
    if (!_sg.cur_pass.valid) {
        return;
    }
    if (!_sg.next_draw_valid) {
        return;
    }
    _sg_apply_uniforms(ub_slot, data);
    _SG_TRACE_ARGS(apply_uniforms, ub_slot, data);
}

SOKOL_API_IMPL void sg_draw(int base_element, int num_elements, int num_instances) {
    SOKOL_ASSERT(_sg.valid);
    #if defined(SOKOL_DEBUG)
    if (!_sg_validate_draw(base_element, num_elements, num_instances)) {
        return;
    }
    #endif
    _sg_stats_add(num_draw, 1);
    if (!_sg.cur_pass.valid) {
        return;
    }
    if (!_sg.next_draw_valid) {
        return;
    }
    // skip no-op draws
    if ((0 == num_elements) || (0 == num_instances)) {
        return;
    }
    _sg_draw(base_element, num_elements, num_instances);
    _SG_TRACE_ARGS(draw, base_element, num_elements, num_instances);
}

SOKOL_API_IMPL void sg_dispatch(int num_groups_x, int num_groups_y, int num_groups_z) {
    SOKOL_ASSERT(_sg.valid);
    #if defined(SOKOL_DEBUG)
    if (!_sg_validate_dispatch(num_groups_x, num_groups_y, num_groups_z)) {
        return;
    }
    #endif
    _sg_stats_add(num_dispatch, 1);
    if (!_sg.cur_pass.valid) {
        return;
    }
    if (!_sg.next_draw_valid) {
        return;
    }
    // skip no-op dispatches
    if ((0 == num_groups_x) || (0 == num_groups_y) || (0 == num_groups_z)) {
        return;
    }
    _sg_dispatch(num_groups_x, num_groups_y, num_groups_z);
    _SG_TRACE_ARGS(dispatch, num_groups_x, num_groups_y, num_groups_z);
}

SOKOL_API_IMPL void sg_end_pass(void) {
    SOKOL_ASSERT(_sg.valid);
    SOKOL_ASSERT(_sg.cur_pass.in_pass);
    _sg_stats_add(num_passes, 1);
    // NOTE: don't exit early if !_sg.cur_pass.valid
    const _sg_attachments_ptrs_t atts_ptrs = _sg_attachments_ptrs(&_sg.cur_pass.atts);
    _sg_end_pass(&atts_ptrs);
    _sg.cur_pip = _sg_pipeline_ref(0);
    _sg_clear(&_sg.cur_pass, sizeof(_sg.cur_pass));
    _SG_TRACE_NOARGS(end_pass);
}

SOKOL_API_IMPL void sg_commit(void) {
    SOKOL_ASSERT(_sg.valid);
    SOKOL_ASSERT(!_sg.cur_pass.valid);
    SOKOL_ASSERT(!_sg.cur_pass.in_pass);
    _sg_commit();
    _sg.stats.frame_index = _sg.frame_index;
    _sg.prev_stats = _sg.stats;
    _sg_clear(&_sg.stats, sizeof(_sg.stats));
    _sg_notify_commit_listeners();
    _SG_TRACE_NOARGS(commit);
    _sg.frame_index++;
}

SOKOL_API_IMPL void sg_reset_state_cache(void) {
    SOKOL_ASSERT(_sg.valid);
    _sg_reset_state_cache();
    _SG_TRACE_NOARGS(reset_state_cache);
}

SOKOL_API_IMPL void sg_update_buffer(sg_buffer buf_id, const sg_range* data) {
    SOKOL_ASSERT(_sg.valid);
    SOKOL_ASSERT(data && data->ptr && (data->size > 0));
    _sg_stats_add(num_update_buffer, 1);
    _sg_stats_add(size_update_buffer, (uint32_t)data->size);
    _sg_buffer_t* buf = _sg_lookup_buffer(buf_id.id);
    if ((data->size > 0) && buf && (buf->slot.state == SG_RESOURCESTATE_VALID)) {
        if (_sg_validate_update_buffer(buf, data)) {
            SOKOL_ASSERT(data->size <= (size_t)buf->cmn.size);
            // only one update allowed per buffer and frame
            SOKOL_ASSERT(buf->cmn.update_frame_index != _sg.frame_index);
            // update and append on same buffer in same frame not allowed
            SOKOL_ASSERT(buf->cmn.append_frame_index != _sg.frame_index);
            _sg_update_buffer(buf, data);
            buf->cmn.update_frame_index = _sg.frame_index;
        }
    }
    _SG_TRACE_ARGS(update_buffer, buf_id, data);
}

SOKOL_API_IMPL int sg_append_buffer(sg_buffer buf_id, const sg_range* data) {
    SOKOL_ASSERT(_sg.valid);
    SOKOL_ASSERT(data && data->ptr);
    _sg_stats_add(num_append_buffer, 1);
    _sg_stats_add(size_append_buffer, (uint32_t)data->size);
    _sg_buffer_t* buf = _sg_lookup_buffer(buf_id.id);
    int result;
    if (buf) {
        // rewind append cursor in a new frame
        if (buf->cmn.append_frame_index != _sg.frame_index) {
            buf->cmn.append_pos = 0;
            buf->cmn.append_overflow = false;
        }
        if (((size_t)buf->cmn.append_pos + data->size) > (size_t)buf->cmn.size) {
            buf->cmn.append_overflow = true;
        }
        const int start_pos = buf->cmn.append_pos;
        // NOTE: the multiple-of-4 requirement for the buffer offset is coming
        // from WebGPU, but we want identical behaviour between backends
        SOKOL_ASSERT(_sg_multiple_u64((uint64_t)start_pos, 4));
        if (buf->slot.state == SG_RESOURCESTATE_VALID) {
            if (_sg_validate_append_buffer(buf, data)) {
                if (!buf->cmn.append_overflow && (data->size > 0)) {
                    // update and append on same buffer in same frame not allowed
                    SOKOL_ASSERT(buf->cmn.update_frame_index != _sg.frame_index);
                    _sg_append_buffer(buf, data, buf->cmn.append_frame_index != _sg.frame_index);
                    buf->cmn.append_pos += (int) _sg_roundup_u64(data->size, 4);
                    buf->cmn.append_frame_index = _sg.frame_index;
                }
            }
        }
        result = start_pos;
    } else {
        // FIXME: should we return -1 here?
        result = 0;
    }
    _SG_TRACE_ARGS(append_buffer, buf_id, data, result);
    return result;
}

SOKOL_API_IMPL bool sg_query_buffer_overflow(sg_buffer buf_id) {
    SOKOL_ASSERT(_sg.valid);
    _sg_buffer_t* buf = _sg_lookup_buffer(buf_id.id);
    bool result = buf ? buf->cmn.append_overflow : false;
    return result;
}

SOKOL_API_IMPL bool sg_query_buffer_will_overflow(sg_buffer buf_id, size_t size) {
    SOKOL_ASSERT(_sg.valid);
    _sg_buffer_t* buf = _sg_lookup_buffer(buf_id.id);
    bool result = false;
    if (buf) {
        int append_pos = buf->cmn.append_pos;
        // rewind append cursor in a new frame
        if (buf->cmn.append_frame_index != _sg.frame_index) {
            append_pos = 0;
        }
        if ((append_pos + _sg_roundup((int)size, 4)) > buf->cmn.size) {
            result = true;
        }
    }
    return result;
}

SOKOL_API_IMPL void sg_update_image(sg_image img_id, const sg_image_data* data) {
    SOKOL_ASSERT(_sg.valid);
    _sg_stats_add(num_update_image, 1);
    for (int face_index = 0; face_index < SG_CUBEFACE_NUM; face_index++) {
        for (int mip_index = 0; mip_index < SG_MAX_MIPMAPS; mip_index++) {
            if (data->subimage[face_index][mip_index].size == 0) {
                break;
            }
            _sg_stats_add(size_update_image, (uint32_t)data->subimage[face_index][mip_index].size);
        }
    }
    _sg_image_t* img = _sg_lookup_image(img_id.id);
    if (img && img->slot.state == SG_RESOURCESTATE_VALID) {
        if (_sg_validate_update_image(img, data)) {
            SOKOL_ASSERT(img->cmn.upd_frame_index != _sg.frame_index);
            _sg_update_image(img, data);
            img->cmn.upd_frame_index = _sg.frame_index;
        }
    }
    _SG_TRACE_ARGS(update_image, img_id, data);
}

SOKOL_API_IMPL void sg_push_debug_group(const char* name) {
    SOKOL_ASSERT(_sg.valid);
    SOKOL_ASSERT(name);
    _sg_push_debug_group(name);
    _SG_TRACE_ARGS(push_debug_group, name);
}

SOKOL_API_IMPL void sg_pop_debug_group(void) {
    SOKOL_ASSERT(_sg.valid);
    _sg_pop_debug_group();
    _SG_TRACE_NOARGS(pop_debug_group);
}

SOKOL_API_IMPL bool sg_add_commit_listener(sg_commit_listener listener) {
    SOKOL_ASSERT(_sg.valid);
    return _sg_add_commit_listener(&listener);
}

SOKOL_API_IMPL bool sg_remove_commit_listener(sg_commit_listener listener) {
    SOKOL_ASSERT(_sg.valid);
    return _sg_remove_commit_listener(&listener);
}

SOKOL_API_IMPL void sg_enable_frame_stats(void) {
    SOKOL_ASSERT(_sg.valid);
    _sg.stats_enabled = true;
}

SOKOL_API_IMPL void sg_disable_frame_stats(void) {
    SOKOL_ASSERT(_sg.valid);
    _sg.stats_enabled = false;
}

SOKOL_API_IMPL bool sg_frame_stats_enabled(void) {
    return _sg.stats_enabled;
}

SOKOL_API_IMPL sg_buffer_info sg_query_buffer_info(sg_buffer buf_id) {
    SOKOL_ASSERT(_sg.valid);
    sg_buffer_info info;
    _sg_clear(&info, sizeof(info));
    const _sg_buffer_t* buf = _sg_lookup_buffer(buf_id.id);
    if (buf) {
        info.slot.state = buf->slot.state;
        info.slot.res_id = buf->slot.id;
        info.slot.uninit_count = buf->slot.uninit_count;
        info.update_frame_index = buf->cmn.update_frame_index;
        info.append_frame_index = buf->cmn.append_frame_index;
        info.append_pos = buf->cmn.append_pos;
        info.append_overflow = buf->cmn.append_overflow;
        #if defined(SOKOL_D3D11)
        info.num_slots = 1;
        info.active_slot = 0;
        #else
        info.num_slots = buf->cmn.num_slots;
        info.active_slot = buf->cmn.active_slot;
        #endif
    }
    return info;
}

SOKOL_API_IMPL sg_image_info sg_query_image_info(sg_image img_id) {
    SOKOL_ASSERT(_sg.valid);
    sg_image_info info;
    _sg_clear(&info, sizeof(info));
    const _sg_image_t* img = _sg_lookup_image(img_id.id);
    if (img) {
        info.slot.state = img->slot.state;
        info.slot.res_id = img->slot.id;
        info.slot.uninit_count = img->slot.uninit_count;
        info.upd_frame_index = img->cmn.upd_frame_index;
        #if defined(SOKOL_D3D11)
        info.num_slots = 1;
        info.active_slot = 0;
        #else
        info.num_slots = img->cmn.num_slots;
        info.active_slot = img->cmn.active_slot;
        #endif
    }
    return info;
}

SOKOL_API_IMPL sg_sampler_info sg_query_sampler_info(sg_sampler smp_id) {
    SOKOL_ASSERT(_sg.valid);
    sg_sampler_info info;
    _sg_clear(&info, sizeof(info));
    const _sg_sampler_t* smp = _sg_lookup_sampler(smp_id.id);
    if (smp) {
        info.slot.state = smp->slot.state;
        info.slot.res_id = smp->slot.id;
        info.slot.uninit_count = smp->slot.uninit_count;
    }
    return info;
}

SOKOL_API_IMPL sg_shader_info sg_query_shader_info(sg_shader shd_id) {
    SOKOL_ASSERT(_sg.valid);
    sg_shader_info info;
    _sg_clear(&info, sizeof(info));
    const _sg_shader_t* shd = _sg_lookup_shader(shd_id.id);
    if (shd) {
        info.slot.state = shd->slot.state;
        info.slot.res_id = shd->slot.id;
        info.slot.uninit_count = shd->slot.uninit_count;
    }
    return info;
}

SOKOL_API_IMPL sg_pipeline_info sg_query_pipeline_info(sg_pipeline pip_id) {
    SOKOL_ASSERT(_sg.valid);
    sg_pipeline_info info;
    _sg_clear(&info, sizeof(info));
    const _sg_pipeline_t* pip = _sg_lookup_pipeline(pip_id.id);
    if (pip) {
        info.slot.state = pip->slot.state;
        info.slot.res_id = pip->slot.id;
        info.slot.uninit_count = pip->slot.uninit_count;
    }
    return info;
}

SOKOL_API_IMPL sg_view_info sg_query_view_info(sg_view view_id) {
    SOKOL_ASSERT(_sg.valid);
    sg_view_info info;
    _sg_clear(&info, sizeof(info));
    const _sg_view_t* view = _sg_lookup_view(view_id.id);
    if (view) {
        info.slot.state = view->slot.state;
        info.slot.res_id = view->slot.id;
        info.slot.uninit_count = view->slot.uninit_count;
    }
    return info;
}

SOKOL_API_IMPL sg_buffer_desc sg_query_buffer_desc(sg_buffer buf_id) {
    SOKOL_ASSERT(_sg.valid);
    sg_buffer_desc desc;
    _sg_clear(&desc, sizeof(desc));
    const _sg_buffer_t* buf = _sg_lookup_buffer(buf_id.id);
    if (buf) {
        desc.size = (size_t)buf->cmn.size;
        desc.usage = buf->cmn.usage;
    }
    return desc;
}

SOKOL_API_IMPL size_t sg_query_buffer_size(sg_buffer buf_id) {
    SOKOL_ASSERT(_sg.valid);
    const _sg_buffer_t* buf = _sg_lookup_buffer(buf_id.id);
    if (buf) {
        return (size_t)buf->cmn.size;
    }
    return 0;
}

SOKOL_API_IMPL sg_buffer_usage sg_query_buffer_usage(sg_buffer buf_id) {
    SOKOL_ASSERT(_sg.valid);
    sg_buffer_usage usg;
    _sg_clear(&usg, sizeof(usg));
    const _sg_buffer_t* buf = _sg_lookup_buffer(buf_id.id);
    if (buf) {
        usg = buf->cmn.usage;
    }
    return usg;
}

SOKOL_API_IMPL sg_image_desc sg_query_image_desc(sg_image img_id) {
    SOKOL_ASSERT(_sg.valid);
    sg_image_desc desc;
    _sg_clear(&desc, sizeof(desc));
    const _sg_image_t* img = _sg_lookup_image(img_id.id);
    if (img) {
        desc.type = img->cmn.type;
        desc.width = img->cmn.width;
        desc.height = img->cmn.height;
        desc.num_slices = img->cmn.num_slices;
        desc.num_mipmaps = img->cmn.num_mipmaps;
        desc.usage = img->cmn.usage;
        desc.pixel_format = img->cmn.pixel_format;
        desc.sample_count = img->cmn.sample_count;
    }
    return desc;
}

SOKOL_API_IMPL sg_image_type sg_query_image_type(sg_image img_id) {
    SOKOL_ASSERT(_sg.valid);
    const _sg_image_t* img = _sg_lookup_image(img_id.id);
    if (img) {
        return img->cmn.type;
    }
    return _SG_IMAGETYPE_DEFAULT;
}

SOKOL_API_IMPL int sg_query_image_width(sg_image img_id) {
    SOKOL_ASSERT(_sg.valid);
    const _sg_image_t* img = _sg_lookup_image(img_id.id);
    if (img) {
        return img->cmn.width;
    }
    return 0;
}

SOKOL_API_IMPL int sg_query_image_height(sg_image img_id) {
    SOKOL_ASSERT(_sg.valid);
    const _sg_image_t* img = _sg_lookup_image(img_id.id);
    if (img) {
        return img->cmn.height;
    }
    return 0;
}

SOKOL_API_IMPL int sg_query_image_num_slices(sg_image img_id) {
    SOKOL_ASSERT(_sg.valid);
    const _sg_image_t* img = _sg_lookup_image(img_id.id);
    if (img) {
        return img->cmn.num_slices;
    }
    return 0;
}

SOKOL_API_IMPL int sg_query_image_num_mipmaps(sg_image img_id) {
    SOKOL_ASSERT(_sg.valid);
    const _sg_image_t* img = _sg_lookup_image(img_id.id);
    if (img) {
        return img->cmn.num_mipmaps;
    }
    return 0;
}

SOKOL_API_IMPL sg_pixel_format sg_query_image_pixelformat(sg_image img_id) {
    SOKOL_ASSERT(_sg.valid);
    const _sg_image_t* img = _sg_lookup_image(img_id.id);
    if (img) {
        return img->cmn.pixel_format;
    }
    return _SG_PIXELFORMAT_DEFAULT;
}

SOKOL_API_IMPL sg_image_usage sg_query_image_usage(sg_image img_id) {
    SOKOL_ASSERT(_sg.valid);
    sg_image_usage usg;
    _sg_clear(&usg, sizeof(usg));
    const _sg_image_t* img = _sg_lookup_image(img_id.id);
    if (img) {
        usg = img->cmn.usage;
    }
    return usg;
}

SOKOL_API_IMPL int sg_query_image_sample_count(sg_image img_id) {
    SOKOL_ASSERT(_sg.valid);
    const _sg_image_t* img = _sg_lookup_image(img_id.id);
    if (img) {
        return img->cmn.sample_count;
    }
    return 0;
}

SOKOL_API_IMPL sg_view_type sg_query_view_type(sg_view view_id) {
    SOKOL_ASSERT(_sg.valid);
    const _sg_view_t* view = _sg_lookup_view(view_id.id);
    if (view) {
        return view->cmn.type;
    } else {
        return SG_VIEWTYPE_INVALID;
    }
}

// NOTE: may return SG_INVALID_ID if view invalid or view not an image view
SOKOL_API_IMPL sg_image sg_query_view_image(sg_view view_id) {
    SOKOL_ASSERT(_sg.valid);
    sg_image img; _sg_clear(&img, sizeof(img));
    const _sg_view_t* view = _sg_lookup_view(view_id.id);
    if (view) {
        img.id = view->cmn.img.ref.sref.id;
    }
    return img;
}

// NOTE: may return SG_INVALID_ID if view invalid or view not a buffer view
SOKOL_API_IMPL sg_buffer sg_query_view_buffer(sg_view view_id) {
    SOKOL_ASSERT(_sg.valid);
    sg_buffer buf; _sg_clear(&buf, sizeof(buf));
    const _sg_view_t* view = _sg_lookup_view(view_id.id);
    if (view) {
        buf.id = view->cmn.buf.ref.sref.id;
    }
    return buf;
}

SOKOL_API_IMPL sg_sampler_desc sg_query_sampler_desc(sg_sampler smp_id) {
    SOKOL_ASSERT(_sg.valid);
    sg_sampler_desc desc;
    _sg_clear(&desc, sizeof(desc));
    const _sg_sampler_t* smp = _sg_lookup_sampler(smp_id.id);
    if (smp) {
        desc.min_filter = smp->cmn.min_filter;
        desc.mag_filter = smp->cmn.mag_filter;
        desc.mipmap_filter = smp->cmn.mipmap_filter;
        desc.wrap_u = smp->cmn.wrap_u;
        desc.wrap_v = smp->cmn.wrap_v;
        desc.wrap_w = smp->cmn.wrap_w;
        desc.min_lod = smp->cmn.min_lod;
        desc.max_lod = smp->cmn.max_lod;
        desc.border_color = smp->cmn.border_color;
        desc.compare = smp->cmn.compare;
        desc.max_anisotropy = smp->cmn.max_anisotropy;
    }
    return desc;
}

SOKOL_API_IMPL sg_shader_desc sg_query_shader_desc(sg_shader shd_id) {
    SOKOL_ASSERT(_sg.valid);
    sg_shader_desc desc;
    _sg_clear(&desc, sizeof(desc));
    const _sg_shader_t* shd = _sg_lookup_shader(shd_id.id);
    if (shd) {
        for (size_t ub_idx = 0; ub_idx < SG_MAX_UNIFORMBLOCK_BINDSLOTS; ub_idx++) {
            sg_shader_uniform_block* ub_desc = &desc.uniform_blocks[ub_idx];
            const _sg_shader_uniform_block_t* ub = &shd->cmn.uniform_blocks[ub_idx];
            ub_desc->stage = ub->stage;
            ub_desc->size = ub->size;
        }
        for (size_t view_idx = 0; view_idx < SG_MAX_VIEW_BINDSLOTS; view_idx++) {
            const _sg_shader_view_t* view = &shd->cmn.views[view_idx];
            if (view->view_type == SG_VIEWTYPE_TEXTURE) {
                sg_shader_texture_view* tex_desc = &desc.views[view_idx].texture;
                tex_desc->stage = view->stage;
                tex_desc->image_type = view->image_type;
                tex_desc->sample_type = view->sample_type;
                tex_desc->multisampled = view->multisampled;
            } else if (shd->cmn.views[view_idx].view_type == SG_VIEWTYPE_STORAGEBUFFER) {
                sg_shader_storage_buffer_view* sbuf_desc = &desc.views[view_idx].storage_buffer;
                sbuf_desc->stage = view->stage;
                sbuf_desc->readonly = view->sbuf_readonly;
            } else if (shd->cmn.views[view_idx].view_type == SG_VIEWTYPE_STORAGEIMAGE) {
                sg_shader_storage_image_view* simg_desc = &desc.views[view_idx].storage_image;
                simg_desc->stage = view->stage;
                simg_desc->access_format = view->access_format;
                simg_desc->image_type = view->image_type;
                simg_desc->writeonly = view->simg_writeonly;
            }
        }
        for (size_t smp_idx = 0; smp_idx < SG_MAX_SAMPLER_BINDSLOTS; smp_idx++) {
            sg_shader_sampler* smp_desc = &desc.samplers[smp_idx];
            const _sg_shader_sampler_t* smp = &shd->cmn.samplers[smp_idx];
            smp_desc->stage = smp->stage;
            smp_desc->sampler_type = smp->sampler_type;
        }
        for (size_t tex_smp_idx = 0; tex_smp_idx < SG_MAX_TEXTURE_SAMPLER_PAIRS; tex_smp_idx++) {
            sg_shader_texture_sampler_pair* tex_smp_desc = &desc.texture_sampler_pairs[tex_smp_idx];
            const _sg_shader_texture_sampler_t* tex_smp = &shd->cmn.texture_samplers[tex_smp_idx];
            tex_smp_desc->stage = tex_smp->stage;
            tex_smp_desc->view_slot = tex_smp->view_slot;
            tex_smp_desc->sampler_slot = tex_smp->sampler_slot;
        }
    }
    return desc;
}

SOKOL_API_IMPL sg_pipeline_desc sg_query_pipeline_desc(sg_pipeline pip_id) {
    SOKOL_ASSERT(_sg.valid);
    sg_pipeline_desc desc;
    _sg_clear(&desc, sizeof(desc));
    const _sg_pipeline_t* pip = _sg_lookup_pipeline(pip_id.id);
    if (pip) {
        desc.compute = pip->cmn.is_compute;
        desc.shader.id = pip->cmn.shader.sref.id;
        desc.layout = pip->cmn.layout;
        desc.depth = pip->cmn.depth;
        desc.stencil = pip->cmn.stencil;
        desc.color_count = pip->cmn.color_count;
        for (int i = 0; i < pip->cmn.color_count; i++) {
            desc.colors[i] = pip->cmn.colors[i];
        }
        desc.primitive_type = pip->cmn.primitive_type;
        desc.index_type = pip->cmn.index_type;
        desc.cull_mode = pip->cmn.cull_mode;
        desc.face_winding = pip->cmn.face_winding;
        desc.sample_count = pip->cmn.sample_count;
        desc.blend_color = pip->cmn.blend_color;
        desc.alpha_to_coverage_enabled = pip->cmn.alpha_to_coverage_enabled;
    }
    return desc;
}

SOKOL_API_IMPL sg_view_desc sg_query_view_desc(sg_view view_id) {
    SOKOL_ASSERT(_sg.valid);
    sg_view_desc desc;
    _sg_clear(&desc, sizeof(desc));
    const _sg_view_t* view = _sg_lookup_view(view_id.id);
    if (view) {
        switch (view->cmn.type) {
            case SG_VIEWTYPE_STORAGEBUFFER:
                desc.storage_buffer.buffer.id = view->cmn.buf.ref.sref.id;
                desc.storage_buffer.offset = view->cmn.buf.offset;
                break;
            case SG_VIEWTYPE_STORAGEIMAGE:
                desc.storage_image.image.id = view->cmn.img.ref.sref.id;
                desc.storage_image.mip_level = view->cmn.img.mip_level;
                desc.storage_image.slice = view->cmn.img.slice;
                break;
            case SG_VIEWTYPE_TEXTURE:
                desc.texture.image.id = view->cmn.img.ref.sref.id;
                desc.texture.mip_levels.base = view->cmn.img.mip_level;
                desc.texture.mip_levels.count = view->cmn.img.mip_level_count;
                desc.texture.slices.base = view->cmn.img.slice;
                desc.texture.slices.count = view->cmn.img.slice_count;
                break;
            case SG_VIEWTYPE_COLORATTACHMENT:
                desc.color_attachment.image.id = view->cmn.img.ref.sref.id;
                desc.color_attachment.mip_level = view->cmn.img.mip_level;
                desc.color_attachment.slice = view->cmn.img.slice;
                break;
            case SG_VIEWTYPE_RESOLVEATTACHMENT:
                desc.resolve_attachment.image.id = view->cmn.img.ref.sref.id;
                desc.resolve_attachment.mip_level = view->cmn.img.mip_level;
                desc.resolve_attachment.slice = view->cmn.img.slice;
                break;
            case SG_VIEWTYPE_DEPTHSTENCILATTACHMENT:
                desc.depth_stencil_attachment.image.id = view->cmn.img.ref.sref.id;
                desc.depth_stencil_attachment.mip_level = view->cmn.img.mip_level;
                desc.depth_stencil_attachment.slice = view->cmn.img.slice;
                break;
            default:
                SOKOL_UNREACHABLE;
        }
    }
    return desc;
}

SOKOL_API_IMPL sg_buffer_desc sg_query_buffer_defaults(const sg_buffer_desc* desc) {
    SOKOL_ASSERT(_sg.valid && desc);
    return _sg_buffer_desc_defaults(desc);
}

SOKOL_API_IMPL sg_image_desc sg_query_image_defaults(const sg_image_desc* desc) {
    SOKOL_ASSERT(_sg.valid && desc);
    return _sg_image_desc_defaults(desc);
}

SOKOL_API_IMPL sg_sampler_desc sg_query_sampler_defaults(const sg_sampler_desc* desc) {
    SOKOL_ASSERT(_sg.valid && desc);
    return _sg_sampler_desc_defaults(desc);
}

SOKOL_API_IMPL sg_shader_desc sg_query_shader_defaults(const sg_shader_desc* desc) {
    SOKOL_ASSERT(_sg.valid && desc);
    return _sg_shader_desc_defaults(desc);
}

SOKOL_API_IMPL sg_pipeline_desc sg_query_pipeline_defaults(const sg_pipeline_desc* desc) {
    SOKOL_ASSERT(_sg.valid && desc);
    return _sg_pipeline_desc_defaults(desc);
}

SOKOL_API_IMPL sg_view_desc sg_query_view_defaults(const sg_view_desc* desc) {
    SOKOL_ASSERT(_sg.valid && desc);
    return _sg_view_desc_defaults(desc);
}

SOKOL_API_IMPL const void* sg_d3d11_device(void) {
    #if defined(SOKOL_D3D11)
        return (const void*) _sg.d3d11.dev;
    #else
        return 0;
    #endif
}

SOKOL_API_IMPL const void* sg_d3d11_device_context(void) {
    #if defined(SOKOL_D3D11)
        return (const void*) _sg.d3d11.ctx;
    #else
        return 0;
    #endif
}

SOKOL_API_IMPL sg_d3d11_buffer_info sg_d3d11_query_buffer_info(sg_buffer buf_id) {
    SOKOL_ASSERT(_sg.valid);
    sg_d3d11_buffer_info res;
    _sg_clear(&res, sizeof(res));
    #if defined(SOKOL_D3D11)
        const _sg_buffer_t* buf = _sg_lookup_buffer(buf_id.id);
        if (buf) {
            res.buf = (const void*) buf->d3d11.buf;
        }
    #else
        _SOKOL_UNUSED(buf_id);
    #endif
    return res;
}

SOKOL_API_IMPL sg_d3d11_image_info sg_d3d11_query_image_info(sg_image img_id) {
    SOKOL_ASSERT(_sg.valid);
    sg_d3d11_image_info res;
    _sg_clear(&res, sizeof(res));
    #if defined(SOKOL_D3D11)
        const _sg_image_t* img = _sg_lookup_image(img_id.id);
        if (img) {
            res.tex2d = (const void*) img->d3d11.tex2d;
            res.tex3d = (const void*) img->d3d11.tex3d;
            res.res = (const void*) img->d3d11.res;
        }
    #else
        _SOKOL_UNUSED(img_id);
    #endif
    return res;
}

SOKOL_API_IMPL sg_d3d11_sampler_info sg_d3d11_query_sampler_info(sg_sampler smp_id) {
    SOKOL_ASSERT(_sg.valid);
    sg_d3d11_sampler_info res;
    _sg_clear(&res, sizeof(res));
    #if defined(SOKOL_D3D11)
        const _sg_sampler_t* smp = _sg_lookup_sampler(smp_id.id);
        if (smp) {
            res.smp = (const void*) smp->d3d11.smp;
        }
    #else
        _SOKOL_UNUSED(smp_id);
    #endif
    return res;
}

SOKOL_API_IMPL sg_d3d11_shader_info sg_d3d11_query_shader_info(sg_shader shd_id) {
    SOKOL_ASSERT(_sg.valid);
    sg_d3d11_shader_info res;
    _sg_clear(&res, sizeof(res));
    #if defined(SOKOL_D3D11)
        const _sg_shader_t* shd = _sg_lookup_shader(shd_id.id);
        if (shd) {
            for (size_t i = 0; i < SG_MAX_UNIFORMBLOCK_BINDSLOTS; i++) {
                res.cbufs[i] = (const void*) shd->d3d11.all_cbufs[i];
            }
            res.vs = (const void*) shd->d3d11.vs;
            res.fs = (const void*) shd->d3d11.fs;
        }
    #else
        _SOKOL_UNUSED(shd_id);
    #endif
    return res;
}

SOKOL_API_IMPL sg_d3d11_pipeline_info sg_d3d11_query_pipeline_info(sg_pipeline pip_id) {
    SOKOL_ASSERT(_sg.valid);
    sg_d3d11_pipeline_info res;
    _sg_clear(&res, sizeof(res));
    #if defined(SOKOL_D3D11)
        const _sg_pipeline_t* pip = _sg_lookup_pipeline(pip_id.id);
        if (pip) {
            res.il = (const void*) pip->d3d11.il;
            res.rs = (const void*) pip->d3d11.rs;
            res.dss = (const void*) pip->d3d11.dss;
            res.bs = (const void*) pip->d3d11.bs;
        }
    #else
        _SOKOL_UNUSED(pip_id);
    #endif
    return res;
}

SOKOL_API_IMPL sg_d3d11_view_info sg_d3d11_query_view_info(sg_view view_id) {
    SOKOL_ASSERT(_sg.valid);
    sg_d3d11_view_info res;
    _sg_clear(&res, sizeof(res));
    #if defined(SOKOL_D3D11)
        const _sg_view_t* view = _sg_lookup_view(view_id.id);
        res.srv = (const void*) view->d3d11.srv;
        res.uav = (const void*) view->d3d11.uav;
        res.rtv = (const void*) view->d3d11.rtv;
        res.dsv = (const void*) view->d3d11.dsv;
    #else
        _SOKOL_UNUSED(view_id);
    #endif
    return res;
}

SOKOL_API_IMPL const void* sg_mtl_device(void) {
    #if defined(SOKOL_METAL)
        if (nil != _sg.mtl.device) {
            return (__bridge const void*) _sg.mtl.device;
        } else {
            return 0;
        }
    #else
        return 0;
    #endif
}

SOKOL_API_IMPL const void* sg_mtl_render_command_encoder(void) {
    #if defined(SOKOL_METAL)
        if (nil != _sg.mtl.render_cmd_encoder) {
            return (__bridge const void*) _sg.mtl.render_cmd_encoder;
        } else {
            return 0;
        }
    #else
        return 0;
    #endif
}

SOKOL_API_IMPL const void* sg_mtl_compute_command_encoder(void) {
    #if defined(SOKOL_METAL)
        if (nil != _sg.mtl.compute_cmd_encoder) {
            return (__bridge const void*) _sg.mtl.compute_cmd_encoder;
        } else {
            return 0;
        }
    #else
        return 0;
    #endif
}

SOKOL_API_IMPL sg_mtl_buffer_info sg_mtl_query_buffer_info(sg_buffer buf_id) {
    SOKOL_ASSERT(_sg.valid);
    sg_mtl_buffer_info res;
    _sg_clear(&res, sizeof(res));
    #if defined(SOKOL_METAL)
        const _sg_buffer_t* buf = _sg_lookup_buffer(buf_id.id);
        if (buf) {
            for (int i = 0; i < SG_NUM_INFLIGHT_FRAMES; i++) {
                if (buf->mtl.buf[i] != 0) {
                    res.buf[i] = (__bridge void*) _sg_mtl_id(buf->mtl.buf[i]);
                }
            }
            res.active_slot = buf->cmn.active_slot;
        }
    #else
        _SOKOL_UNUSED(buf_id);
    #endif
    return res;
}

SOKOL_API_IMPL sg_mtl_image_info sg_mtl_query_image_info(sg_image img_id) {
    SOKOL_ASSERT(_sg.valid);
    sg_mtl_image_info res;
    _sg_clear(&res, sizeof(res));
    #if defined(SOKOL_METAL)
        const _sg_image_t* img = _sg_lookup_image(img_id.id);
        if (img) {
            for (int i = 0; i < SG_NUM_INFLIGHT_FRAMES; i++) {
                if (img->mtl.tex[i] != 0) {
                    res.tex[i] = (__bridge void*) _sg_mtl_id(img->mtl.tex[i]);
                }
            }
            res.active_slot = img->cmn.active_slot;
        }
    #else
        _SOKOL_UNUSED(img_id);
    #endif
    return res;
}

SOKOL_API_IMPL sg_mtl_sampler_info sg_mtl_query_sampler_info(sg_sampler smp_id) {
    SOKOL_ASSERT(_sg.valid);
    sg_mtl_sampler_info res;
    _sg_clear(&res, sizeof(res));
    #if defined(SOKOL_METAL)
        const _sg_sampler_t* smp = _sg_lookup_sampler(smp_id.id);
        if (smp) {
            if (smp->mtl.sampler_state != 0) {
                res.smp = (__bridge void*) _sg_mtl_id(smp->mtl.sampler_state);
            }
        }
    #else
        _SOKOL_UNUSED(smp_id);
    #endif
    return res;
}

SOKOL_API_IMPL sg_mtl_shader_info sg_mtl_query_shader_info(sg_shader shd_id) {
    SOKOL_ASSERT(_sg.valid);
    sg_mtl_shader_info res;
    _sg_clear(&res, sizeof(res));
    #if defined(SOKOL_METAL)
        const _sg_shader_t* shd = _sg_lookup_shader(shd_id.id);
        if (shd) {
            const int vertex_lib  = shd->mtl.vertex_func.mtl_lib;
            const int vertex_func = shd->mtl.vertex_func.mtl_func;
            const int fragment_lib  = shd->mtl.fragment_func.mtl_lib;
            const int fragment_func = shd->mtl.fragment_func.mtl_func;
            if (vertex_lib != 0) {
                res.vertex_lib = (__bridge void*) _sg_mtl_id(vertex_lib);
            }
            if (fragment_lib != 0) {
                res.fragment_lib = (__bridge void*) _sg_mtl_id(fragment_lib);
            }
            if (vertex_func != 0) {
                res.vertex_func = (__bridge void*) _sg_mtl_id(vertex_func);
            }
            if (fragment_func != 0) {
                res.fragment_func = (__bridge void*) _sg_mtl_id(fragment_func);
            }
        }
    #else
        _SOKOL_UNUSED(shd_id);
    #endif
    return res;
}

SOKOL_API_IMPL sg_mtl_pipeline_info sg_mtl_query_pipeline_info(sg_pipeline pip_id) {
    SOKOL_ASSERT(_sg.valid);
    sg_mtl_pipeline_info res;
    _sg_clear(&res, sizeof(res));
    #if defined(SOKOL_METAL)
        const _sg_pipeline_t* pip = _sg_lookup_pipeline(pip_id.id);
        if (pip) {
            if (pip->mtl.rps != 0) {
                res.rps = (__bridge void*) _sg_mtl_id(pip->mtl.rps);
            }
            if (pip->mtl.dss != 0) {
                res.dss = (__bridge void*) _sg_mtl_id(pip->mtl.dss);
            }
        }
    #else
        _SOKOL_UNUSED(pip_id);
    #endif
    return res;
}

SOKOL_API_IMPL const void* sg_wgpu_device(void) {
    #if defined(SOKOL_WGPU)
        return (const void*) _sg.wgpu.dev;
    #else
        return 0;
    #endif
}

SOKOL_API_IMPL const void* sg_wgpu_queue(void) {
    #if defined(SOKOL_WGPU)
        return (const void*) _sg.wgpu.queue;
    #else
        return 0;
    #endif
}

SOKOL_API_IMPL const void* sg_wgpu_command_encoder(void) {
    #if defined(SOKOL_WGPU)
        return (const void*) _sg.wgpu.cmd_enc;
    #else
        return 0;
    #endif
}

SOKOL_API_IMPL const void* sg_wgpu_render_pass_encoder(void) {
    #if defined(SOKOL_WGPU)
        return (const void*) _sg.wgpu.rpass_enc;
    #else
        return 0;
    #endif
}

SOKOL_API_IMPL const void* sg_wgpu_compute_pass_encoder(void) {
    #if defined(SOKOL_WGPU)
        return (const void*) _sg.wgpu.cpass_enc;
    #else
        return 0;
    #endif
}

SOKOL_API_IMPL sg_wgpu_buffer_info sg_wgpu_query_buffer_info(sg_buffer buf_id) {
    SOKOL_ASSERT(_sg.valid);
    sg_wgpu_buffer_info res;
    _sg_clear(&res, sizeof(res));
    #if defined(SOKOL_WGPU)
        const _sg_buffer_t* buf = _sg_lookup_buffer(buf_id.id);
        if (buf) {
            res.buf = (const void*) buf->wgpu.buf;
        }
    #else
        _SOKOL_UNUSED(buf_id);
    #endif
    return res;
}

SOKOL_API_IMPL sg_wgpu_image_info sg_wgpu_query_image_info(sg_image img_id) {
    SOKOL_ASSERT(_sg.valid);
    sg_wgpu_image_info res;
    _sg_clear(&res, sizeof(res));
    #if defined(SOKOL_WGPU)
        const _sg_image_t* img = _sg_lookup_image(img_id.id);
        if (img) {
            res.tex = (const void*) img->wgpu.tex;
        }
    #else
        _SOKOL_UNUSED(img_id);
    #endif
    return res;
}

SOKOL_API_IMPL sg_wgpu_sampler_info sg_wgpu_query_sampler_info(sg_sampler smp_id) {
    SOKOL_ASSERT(_sg.valid);
    sg_wgpu_sampler_info res;
    _sg_clear(&res, sizeof(res));
    #if defined(SOKOL_WGPU)
        const _sg_sampler_t* smp = _sg_lookup_sampler(smp_id.id);
        if (smp) {
            res.smp = (const void*) smp->wgpu.smp;
        }
    #else
        _SOKOL_UNUSED(smp_id);
    #endif
    return res;
}

SOKOL_API_IMPL sg_wgpu_shader_info sg_wgpu_query_shader_info(sg_shader shd_id) {
    SOKOL_ASSERT(_sg.valid);
    sg_wgpu_shader_info res;
    _sg_clear(&res, sizeof(res));
    #if defined(SOKOL_WGPU)
        const _sg_shader_t* shd = _sg_lookup_shader(shd_id.id);
        if (shd) {
            res.vs_mod = (const void*) shd->wgpu.vertex_func.module;
            res.fs_mod = (const void*) shd->wgpu.fragment_func.module;
            res.bgl = (const void*) shd->wgpu.bgl_view_smp;
        }
    #else
        _SOKOL_UNUSED(shd_id);
    #endif
    return res;
}

SOKOL_API_IMPL sg_wgpu_pipeline_info sg_wgpu_query_pipeline_info(sg_pipeline pip_id) {
    SOKOL_ASSERT(_sg.valid);
    sg_wgpu_pipeline_info res;
    _sg_clear(&res, sizeof(res));
    #if defined(SOKOL_WGPU)
        const _sg_pipeline_t* pip = _sg_lookup_pipeline(pip_id.id);
        if (pip) {
            res.render_pipeline = (const void*) pip->wgpu.rpip;
            res.compute_pipeline = (const void*) pip->wgpu.cpip;
        }
    #else
        _SOKOL_UNUSED(pip_id);
    #endif
    return res;
}

SOKOL_API_IMPL sg_wgpu_view_info sg_wgpu_query_view_info(sg_view view_id) {
    SOKOL_ASSERT(_sg.valid);
    sg_wgpu_view_info res;
    _sg_clear(&res, sizeof(res));
    #if defined(SOKOL_WGPU)
        const _sg_view_t* view = _sg_lookup_view(view_id.id);
        if (view) {
            res.view = (const void*) view->wgpu.view;
        }
    #else
        _SOKOL_UNUSED(view_id);
    #endif
    return res;
}

SOKOL_API_IMPL sg_gl_buffer_info sg_gl_query_buffer_info(sg_buffer buf_id) {
    SOKOL_ASSERT(_sg.valid);
    sg_gl_buffer_info res;
    _sg_clear(&res, sizeof(res));
        if (buf) {
            for (int i = 0; i < SG_NUM_INFLIGHT_FRAMES; i++) {
                res.buf[i] = buf->gl.buf[i];
            }
            res.active_slot = buf->cmn.active_slot;
        }
    #else
        _SOKOL_UNUSED(buf_id);
    #endif
    return res;
}

SOKOL_API_IMPL sg_gl_image_info sg_gl_query_image_info(sg_image img_id) {
    SOKOL_ASSERT(_sg.valid);
    sg_gl_image_info res;
    _sg_clear(&res, sizeof(res));
        if (img) {
            for (int i = 0; i < SG_NUM_INFLIGHT_FRAMES; i++) {
                res.tex[i] = img->gl.tex[i];
            }
            res.tex_target = img->gl.target;
            res.active_slot = img->cmn.active_slot;
        }
    #else
        _SOKOL_UNUSED(img_id);
    #endif
    return res;
}

SOKOL_API_IMPL sg_gl_sampler_info sg_gl_query_sampler_info(sg_sampler smp_id) {
    SOKOL_ASSERT(_sg.valid);
    sg_gl_sampler_info res;
    _sg_clear(&res, sizeof(res));
        if (smp) {
            res.smp = smp->gl.smp;
        }
    #else
        _SOKOL_UNUSED(smp_id);
    #endif
    return res;
}

SOKOL_API_IMPL sg_gl_shader_info sg_gl_query_shader_info(sg_shader shd_id) {
    SOKOL_ASSERT(_sg.valid);
    sg_gl_shader_info res;
    _sg_clear(&res, sizeof(res));
        if (shd) {
            res.prog = shd->gl.prog;
        }
    #else
        _SOKOL_UNUSED(shd_id);
    #endif
    return res;
}

SOKOL_API_IMPL sg_gl_view_info sg_gl_query_view_info(sg_view view_id) {
    SOKOL_ASSERT(_sg.valid);
    sg_gl_view_info res;
    _sg_clear(&res, sizeof(res));
        if (view) {
            for (size_t i = 0; i < SG_NUM_INFLIGHT_FRAMES; i++) {
                res.tex_view[i] = view->gl.tex_view[i];
            }
            res.msaa_render_buffer = view->gl.msaa_render_buffer;
            res.msaa_resolve_frame_buffer = view->gl.msaa_resolve_frame_buffer;
        }
    #else
        _SOKOL_UNUSED(view_id);
    #endif
    return res;
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif // SOKOL_GFX_IMPL