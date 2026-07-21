#include "render/renderer.h"
#include "raymath.h"
#include <cstring>

// raylib's projection clip distances (rlgl defaults).
static constexpr float NEAR_PLANE = 0.01f;
static constexpr float FAR_PLANE = 1000.0f;

static Mesh GenGrassPatchMesh() {
    // Seven small tufts, three double-sided triangular blades each. Keeping a
    // whole patch in one mesh adds ground detail without multiplying draws per
    // blade or relying on noisy litter props.
    constexpr int TUFTS = 7;
    constexpr int BLADES = 3;
    constexpr int VERTS = TUFTS * BLADES * 6;
    float vertices[VERTS * 3] = {};
    float normals[VERTS * 3] = {};
    int cursor = 0;

    const Vector2 offsets[TUFTS] = {
        { 0.00f, 0.00f }, { 0.31f, 0.08f }, { -0.27f, 0.13f },
        { 0.16f, -0.26f }, { -0.20f, -0.28f }, { 0.40f, -0.21f },
        { -0.39f, -0.10f },
    };

    auto putVertex = [&](Vector3 p, Vector3 n) {
        vertices[cursor * 3 + 0] = p.x;
        vertices[cursor * 3 + 1] = p.y;
        vertices[cursor * 3 + 2] = p.z;
        normals[cursor * 3 + 0] = n.x;
        normals[cursor * 3 + 1] = n.y;
        normals[cursor * 3 + 2] = n.z;
        cursor++;
    };

    for (int t = 0; t < TUFTS; t++) {
        for (int b = 0; b < BLADES; b++) {
            float angle = b * PI / 3.0f + t * 0.47f;
            float width = 0.045f + 0.009f * ((t + b) % 3);
            float height = 0.27f + 0.045f * ((t * 2 + b) % 4);
            Vector3 across = { cosf(angle) * width, 0, sinf(angle) * width };
            Vector3 left = { offsets[t].x - across.x, 0, offsets[t].y - across.z };
            Vector3 right = { offsets[t].x + across.x, 0, offsets[t].y + across.z };
            float leanAngle = angle + PI * 0.5f;
            Vector3 tip = { offsets[t].x + cosf(leanAngle) * 0.035f, height,
                            offsets[t].y + sinf(leanAngle) * 0.035f };
            Vector3 n = Vector3Normalize(Vector3CrossProduct(
                Vector3Subtract(right, left), Vector3Subtract(tip, left)));
            putVertex(left, n); putVertex(right, n); putVertex(tip, n);
            n = Vector3Negate(n);
            putVertex(right, n); putVertex(left, n); putVertex(tip, n);
        }
    }

    Mesh mesh = {};
    mesh.vertexCount = cursor;
    mesh.triangleCount = cursor / 3;
    mesh.vertices = (float*)MemAlloc((size_t)cursor * 3 * sizeof(float));
    mesh.normals = (float*)MemAlloc((size_t)cursor * 3 * sizeof(float));
    memcpy(mesh.vertices, vertices, (size_t)cursor * 3 * sizeof(float));
    memcpy(mesh.normals, normals, (size_t)cursor * 3 * sizeof(float));
    UploadMesh(&mesh, false);
    return mesh;
}

// Builds a plane through `point`, flipping the normal if needed so that
// `inside` ends up on the positive side. Sign conventions can never rot.
static Vector4 MakePlane(Vector3 point, Vector3 normal, Vector3 inside) {
    normal = Vector3Normalize(normal);
    float d = -Vector3DotProduct(normal, point);
    if (Vector3DotProduct(normal, inside) + d < 0.0f) {
        normal = Vector3Negate(normal);
        d = -d;
    }
    return { normal.x, normal.y, normal.z, d };
}

static Frustum BuildFrustum(const Camera3D& cam, float aspect) {
    Vector3 fw = Vector3Normalize(Vector3Subtract(cam.target, cam.position));
    Vector3 rt = Vector3Normalize(Vector3CrossProduct(fw, cam.up));
    Vector3 up = Vector3CrossProduct(rt, fw);

    float halfV = cam.fovy * DEG2RAD * 0.5f;
    float tanV = tanf(halfV);
    float tanH = tanV * aspect;

    Vector3 inside = Vector3Add(cam.position, Vector3Scale(fw, 10.0f));
    Vector3 dirL = Vector3Normalize(Vector3Subtract(fw, Vector3Scale(rt, tanH)));
    Vector3 dirR = Vector3Normalize(Vector3Add(fw, Vector3Scale(rt, tanH)));
    Vector3 dirT = Vector3Normalize(Vector3Add(fw, Vector3Scale(up, tanV)));
    Vector3 dirB = Vector3Normalize(Vector3Subtract(fw, Vector3Scale(up, tanV)));

    Frustum f;
    f.planes[0] = MakePlane(Vector3Add(cam.position, Vector3Scale(fw, NEAR_PLANE)), fw, inside);
    f.planes[1] = MakePlane(Vector3Add(cam.position, Vector3Scale(fw, FAR_PLANE)), Vector3Negate(fw), inside);
    f.planes[2] = MakePlane(cam.position, Vector3CrossProduct(up, dirL), inside);
    f.planes[3] = MakePlane(cam.position, Vector3CrossProduct(up, dirR), inside);
    f.planes[4] = MakePlane(cam.position, Vector3CrossProduct(rt, dirT), inside);
    f.planes[5] = MakePlane(cam.position, Vector3CrossProduct(rt, dirB), inside);
    return f;
}

bool Frustum::VisibleAABB(const BoundingBox& box) const {
    for (const Vector4& p : planes) {
        // Positive-vertex test: the box corner furthest along the normal.
        Vector3 v = { p.x >= 0 ? box.max.x : box.min.x,
                      p.y >= 0 ? box.max.y : box.min.y,
                      p.z >= 0 ? box.max.z : box.min.z };
        if (p.x * v.x + p.y * v.y + p.z * v.z + p.w < 0.0f) return false;
    }
    return true;
}

bool Frustum::VisibleSphere(Vector3 c, float r) const {
    for (const Vector4& p : planes)
        if (p.x * c.x + p.y * c.y + p.z * c.z + p.w < -r) return false;
    return true;
}

void Renderer::Init() {
    lighting.Init();
    sky.Init();

    cube = GenMeshCube(1.0f, 1.0f, 1.0f);
    cylinder = GenMeshCylinder(0.5f, 1.0f, 12);
    cone = GenMeshCone(0.5f, 1.0f, 12);
    sphere = GenMeshSphere(0.5f, 10, 16);
    facetedSphere = GenMeshSphere(0.5f, 5, 7);
    grassPatch = GenGrassPatchMesh();

    litMat = LoadMaterialDefault();
    litMat.shader = lighting.shader;
}

void Renderer::Shutdown() {
    UnloadMesh(cube);
    UnloadMesh(cylinder);
    UnloadMesh(cone);
    UnloadMesh(sphere);
    UnloadMesh(facetedSphere);
    UnloadMesh(grassPatch);
    litMat.shader = {};
    UnloadMaterial(litMat);
    sky.Shutdown();
    lighting.Shutdown();
}

void Renderer::BeginScene(const Camera3D& cam, float aspect, float time) {
    stats = {};
    frustum = BuildFrustum(cam, aspect);
    lighting.SetFrame(cam.position);

    BeginMode3D(cam);
    sky.Draw(cam, lighting.sunDir, time);
}

void Renderer::EndScene() {
    EndMode3D();
}

void Renderer::SetEmissive(float e) {
    lighting.SetEmissive(e);
    lastEmissive = e;
}

void Renderer::DrawLit(const Mesh& mesh, Matrix transform, Color tint, float emissive) {
    if (emissive != lastEmissive) {          // avoid a uniform upload per draw
        lighting.SetEmissive(emissive);
        lastEmissive = emissive;
    }
    litMat.maps[MATERIAL_MAP_DIFFUSE].color = tint;
    DrawMesh(mesh, litMat, transform);
    stats.drawCalls++;
}
