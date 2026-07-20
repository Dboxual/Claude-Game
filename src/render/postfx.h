// Post-processing chain: the scene renders into an off-screen target at
// renderScale resolution, then bloom (bright pass + separable blur at
// quarter res), color grade, and vignette composite it to the backbuffer.
// UI always draws after, at native resolution. Every stage is scalable or
// skippable — this is the "graphics settings" workhorse.
#pragma once
#include "raylib.h"

class PostFx {
public:
    void Init(float renderScale);
    void Shutdown();

    // Recreate targets when the window size or render scale changes.
    void EnsureTargets(float renderScale);

    void BeginScene();                        // -> render into the scene RT
    void Composite(bool bloomEnabled);        // -> blit chain to backbuffer

private:
    RenderTexture2D scene = {};    // renderScale * screen
    RenderTexture2D bright = {};   // quarter of scene
    RenderTexture2D blurA = {};
    RenderTexture2D blurB = {};
    Shader brightShader = {};
    Shader blurShader = {};
    Shader compositeShader = {};
    int locBlurDir = 0;
    int locBloomStrength = 0;
    int width = 0, height = 0;
    float scale = 1.0f;
};
