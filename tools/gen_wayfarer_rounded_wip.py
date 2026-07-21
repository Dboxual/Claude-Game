#!/usr/bin/env python3
"""Generate the original Wayfarer character as a rigged, animated glTF (GLB).

Everything here is authored from scratch in code: geometry, skeleton, skinning,
and animation clips. No external models, textures, or motion capture are used,
so the output is 100% original and copyright-clean. This feeds Zion's real
skinned-mesh + skeletal-animation pipeline (raylib LoadModel/UpdateModelAnimation)
in place of stacked primitives.

Skinning is rigid (one joint per vertex) for reliability; a later pass can add
weight blending for bendy elbows/knees without changing the pipeline.

Run:  python3 tools/gen_wayfarer.py
Out:  assets/models/wayfarer.glb
"""
import struct
import numpy as np
import pygltflib as g

FLOAT, UBYTE, USHORT = 5126, 5121, 5123
ARRAY_BUFFER, ELEMENT_ARRAY_BUFFER = 34962, 34963

# ---------------------------------------------------------------- skeleton ----
# (name, parent, local translation relative to parent). Bind pose has no
# rotation, so a joint's global bind position is just the sum up the chain and
# its inverse-bind matrix is a pure negative translation.
JOINTS = [
    ("hips",       -1, (0.00,  0.90, 0.00)),  # 0
    ("spine",       0, (0.00,  0.34, 0.00)),  # 1  -> y 1.24
    ("head",        1, (0.00,  0.50, 0.00)),  # 2  -> y 1.74
    ("upperArmL",   1, (0.28,  0.28, 0.00)),  # 3  -> (0.28,1.52,0)
    ("lowerArmL",   3, (0.00, -0.34, 0.00)),  # 4  -> (0.28,1.18,0)
    ("upperArmR",   1, (-0.28, 0.28, 0.00)),  # 5
    ("lowerArmR",   5, (0.00, -0.34, 0.00)),  # 6
    ("upperLegL",   0, (0.12, -0.06, 0.00)),  # 7  -> (0.12,0.84,0)
    ("lowerLegL",   7, (0.00, -0.42, 0.00)),  # 8  -> (0.12,0.42,0)
    ("upperLegR",   0, (-0.12,-0.06, 0.00)),  # 9
    ("lowerLegR",   9, (0.00, -0.42, 0.00)),  # 10
]
JI = {name: i for i, (name, _, _) in enumerate(JOINTS)}

def global_positions():
    pos = [None] * len(JOINTS)
    for i, (_, parent, t) in enumerate(JOINTS):
        p = np.array(t, dtype=np.float64)
        pos[i] = p if parent < 0 else pos[parent] + p
    return pos

GPOS = global_positions()

# ------------------------------------------------------------------ colors ----
def rgb(r, gr, b):
    return (r / 255.0, gr / 255.0, b / 255.0, 1.0)

C_PANTS = rgb(46, 54, 62)
C_LEATHER = rgb(96, 66, 44)
C_CLOTH = rgb(50, 100, 108)
C_SKIN = rgb(228, 180, 142)
C_HOOD = rgb(56, 48, 68)
C_BOOT = rgb(70, 48, 34)
C_PAULD = rgb(176, 132, 74)
C_GLOVE = rgb(74, 52, 36)
C_GEM = rgb(130, 240, 224)
C_EYE = rgb(24, 28, 34)

# ------------------------------------------------------------------- geometry -
# Interleaved-by-attribute arrays we accumulate then pack.
positions, normals, colors, joints_a, weights_a, indices = [], [], [], [], [], []

FACES = [  # (normal, 4 CCW corners as +-1 signs on x,y,z)
    ((0, 0, 1),  [(-1,-1, 1), ( 1,-1, 1), ( 1, 1, 1), (-1, 1, 1)]),
    ((0, 0,-1),  [( 1,-1,-1), (-1,-1,-1), (-1, 1,-1), ( 1, 1,-1)]),
    ((1, 0, 0),  [( 1,-1, 1), ( 1,-1,-1), ( 1, 1,-1), ( 1, 1, 1)]),
    ((-1,0, 0),  [(-1,-1,-1), (-1,-1, 1), (-1, 1, 1), (-1, 1,-1)]),
    ((0, 1, 0),  [(-1, 1, 1), ( 1, 1, 1), ( 1, 1,-1), (-1, 1,-1)]),
    ((0,-1, 0),  [(-1,-1,-1), ( 1,-1,-1), ( 1,-1, 1), (-1,-1, 1)]),
]

def box(center, size, joint, color, top_scale=1.0):
    """Add a (optionally top-tapered) box rigidly bound to `joint`."""
    cx, cy, cz = center
    hx, hy, hz = size[0] / 2, size[1] / 2, size[2] / 2
    for normal, corners in FACES:
        base = len(positions)
        for sx, sy, sz in corners:
            ts = top_scale if sy > 0 else 1.0
            positions.append((cx + sx * hx * ts, cy + sy * hy, cz + sz * hz * ts))
            normals.append(normal)
            colors.append(color)
            joints_a.append((joint, 0, 0, 0))
            weights_a.append((1.0, 0.0, 0.0, 0.0))
        indices.extend([base, base + 1, base + 2, base, base + 2, base + 3])

def _vert(p, n, color, joint):
    positions.append(tuple(map(float, p)))
    nn = np.array(n, float)
    ln = np.linalg.norm(nn)
    normals.append(tuple(nn / ln) if ln > 1e-6 else (0.0, 1.0, 0.0))
    colors.append(color)
    joints_a.append((joint, 0, 0, 0))
    weights_a.append((1.0, 0.0, 0.0, 0.0))

def sphere(center, joint, color, r=0.2, sx=1.0, sy=1.0, sz=1.0, rings=7, sectors=12):
    """Low-poly ellipsoid rigidly bound to `joint`."""
    cx, cy, cz = center
    base = len(positions)
    for ri in range(rings + 1):
        phi = np.pi * ri / rings
        for s in range(sectors + 1):
            th = 2 * np.pi * s / sectors
            nx, ny, nz = np.sin(phi) * np.cos(th), np.cos(phi), np.sin(phi) * np.sin(th)
            _vert((cx + nx * r * sx, cy + ny * r * sy, cz + nz * r * sz),
                  (nx / sx, ny / sy, nz / sz), color, joint)
    row = sectors + 1
    for ri in range(rings):
        for s in range(sectors):
            a = base + ri * row + s; b = base + (ri + 1) * row + s
            indices.extend([a, b, a + 1, a + 1, b, b + 1])

def tube(p0, p1, r0, r1, joint, color, sides=10, squashZ=1.0):
    """Tapered rounded tube from p0 to p1, rigidly bound to `joint`. Ends are
    left open (covered by ball-joint spheres). Material is double-sided."""
    p0 = np.array(p0, float); p1 = np.array(p1, float)
    axis = p1 - p0
    L = np.linalg.norm(axis)
    if L < 1e-6:
        return
    axis /= L
    ref = np.array([0.0, 1.0, 0.0]) if abs(axis[1]) < 0.9 else np.array([1.0, 0.0, 0.0])
    u = np.cross(ref, axis); u /= np.linalg.norm(u)
    v = np.cross(axis, u)
    base = len(positions)
    for c, r in ((p0, r0), (p1, r1)):
        for s in range(sides):
            a = 2 * np.pi * s / sides
            nrm = u * np.cos(a) + v * np.sin(a)
            _vert(c + (u * np.cos(a) + v * np.sin(a) * squashZ) * r, nrm, color, joint)
    for s in range(sides):
        a = base + s; b = base + (s + 1) % sides
        c = base + sides + s; d = base + sides + (s + 1) % sides
        indices.extend([a, c, b, b, c, d])

# --- assemble the body (authored in global bind pose) -------------------------
# Rounded tubes for limbs/torso + ball-joint spheres at the pivots. Each joint
# ball is bound to the CHILD joint (whose rotation pivots exactly there), so it
# stays centered on the pivot and hides the seam when the limb bends.

# Pelvis + flattened torso + chest.
sphere((0.0, 0.90, 0.0), JI["hips"], C_LEATHER, r=0.24, sy=0.72, sz=0.82)      # belt/pelvis
tube((0.0, 0.98, 0.0), (0.0, 1.5, 0.0), 0.27, 0.235, JI["spine"], C_CLOTH, squashZ=0.72)
sphere((0.0, 1.5, 0.0), JI["spine"], C_CLOTH, r=0.25, sy=0.6, sz=0.72)         # shoulders cap
sphere((0.0, 1.2, 0.16), JI["spine"], C_LEATHER, r=0.16, sx=1.1, sy=1.5, sz=0.32)  # vest front
sphere((0.0, 1.28, 0.205), JI["spine"], C_GEM, r=0.05)                          # chest gem

# Neck + head + hood + face.
tube((0.0, 1.5, 0.0), (0.0, 1.66, 0.0), 0.092, 0.086, JI["head"], C_SKIN)
sphere((0.0, 1.79, 0.0), JI["head"], C_SKIN, r=0.19, sy=1.12)                   # head
sphere((0.0, 1.84, -0.03), JI["head"], C_HOOD, r=0.235, sy=0.98, sz=1.06)       # hood back/top
sphere((0.0, 1.9, 0.02), JI["head"], C_HOOD, r=0.20, sy=0.62, sz=0.72)          # hood crown
sphere((0.0, 1.86, 0.135), JI["head"], C_HOOD, r=0.12, sx=1.4, sy=0.5, sz=0.5)  # brow brim
sphere((0.075, 1.78, 0.16), JI["head"], C_EYE, r=0.032)                         # eyes
sphere((-0.075, 1.78, 0.16), JI["head"], C_EYE, r=0.032)

# Arms: shoulder ball -> upper (sleeve) -> elbow ball -> forearm (bracer) -> glove.
for s, uA, lA in ((1, "upperArmL", "lowerArmL"), (-1, "upperArmR", "lowerArmR")):
    S = (0.30 * s, 1.52, 0.0); E = (0.31 * s, 1.16, 0.02); W = (0.32 * s, 0.82, 0.04)
    sphere(S, JI[uA], C_PAULD, r=0.15, sy=0.92)
    tube(S, E, 0.105, 0.092, JI[uA], C_CLOTH)
    sphere(E, JI[lA], C_LEATHER, r=0.093)
    tube(E, W, 0.092, 0.078, JI[lA], C_LEATHER)
    sphere(W, JI[lA], C_GLOVE, r=0.088)

# Legs: hip ball -> thigh -> knee ball -> shin -> boot.
for s, uL, lL in ((1, "upperLegL", "lowerLegL"), (-1, "upperLegR", "lowerLegR")):
    H = (0.13 * s, 0.86, 0.0); K = (0.135 * s, 0.45, 0.02); A = (0.135 * s, 0.09, 0.02)
    sphere(H, JI[uL], C_PANTS, r=0.15, sy=0.92)
    tube(H, K, 0.145, 0.115, JI[uL], C_PANTS)
    sphere(K, JI[lL], C_PANTS, r=0.113)
    tube(K, A, 0.11, 0.092, JI[lL], C_PANTS)
    sphere((0.135 * s, 0.06, 0.06), JI[lL], C_BOOT, r=0.12, sy=0.82, sz=1.7)

# ------------------------------------------------------------------- packing --
blob = bytearray()
views, accessors = [], []

def add_view(data, target=None):
    if len(blob) % 4:
        blob.extend(b"\x00" * (4 - len(blob) % 4))
    off = len(blob)
    blob.extend(data)
    views.append(g.BufferView(buffer=0, byteOffset=off, byteLength=len(data), target=target))
    return len(views) - 1

def add_accessor(view, ctype, count, atype, mn=None, mx=None, normalized=None):
    accessors.append(g.Accessor(bufferView=view, componentType=ctype, count=count,
                                type=atype, min=mn, max=mx, normalized=normalized))
    return len(accessors) - 1

def f32(seq):  # flatten floats
    a = np.array(seq, dtype=np.float32).reshape(-1)
    return a.tobytes(), a

pos_b, pos_a = f32(positions)
nrm_b, _ = f32(normals)
col_b, _ = f32(colors)
wt_b, _ = f32(weights_a)
joint_b = np.array(joints_a, dtype=np.uint8).reshape(-1).tobytes()
idx_arr = np.array(indices, dtype=np.uint16)
idx_b = idx_arr.tobytes()

pmin = np.array(positions, dtype=np.float32).min(axis=0).tolist()
pmax = np.array(positions, dtype=np.float32).max(axis=0).tolist()
n = len(positions)

v_pos = add_view(pos_b, ARRAY_BUFFER); a_pos = add_accessor(v_pos, FLOAT, n, "VEC3", pmin, pmax)
v_nrm = add_view(nrm_b, ARRAY_BUFFER); a_nrm = add_accessor(v_nrm, FLOAT, n, "VEC3")
v_col = add_view(col_b, ARRAY_BUFFER); a_col = add_accessor(v_col, FLOAT, n, "VEC4")
v_jnt = add_view(joint_b, ARRAY_BUFFER); a_jnt = add_accessor(v_jnt, UBYTE, n, "VEC4")
v_wt = add_view(wt_b, ARRAY_BUFFER); a_wt = add_accessor(v_wt, FLOAT, n, "VEC4")
v_idx = add_view(idx_b, ELEMENT_ARRAY_BUFFER); a_idx = add_accessor(v_idx, USHORT, len(indices), "SCALAR")

# inverse bind matrices (column-major, pure negative translation)
ibm = []
for i in range(len(JOINTS)):
    m = np.eye(4, dtype=np.float32)
    m[:3, 3] = -GPOS[i]
    ibm.append(m.T.reshape(-1))  # column-major
ibm_b = np.array(ibm, dtype=np.float32).reshape(-1).tobytes()
v_ibm = add_view(ibm_b); a_ibm = add_accessor(v_ibm, FLOAT, len(JOINTS), "MAT4")

# ---------------------------------------------------------------- animations --
def quat_x(angle):
    return (np.sin(angle / 2), 0.0, 0.0, np.cos(angle / 2))

def quat_y(angle):
    return (0.0, np.sin(angle / 2), 0.0, np.cos(angle / 2))

anim_channels, anim_samplers, anim_defs = [], [], []

def rot_track(joint, times, angfn, axis="x"):
    """Add a rotation sampler+channel for a joint from an angle function."""
    q = quat_y if axis == "y" else quat_x
    tb, _ = f32(times)
    vals = [q(angfn(t)) for t in times]
    vb, _ = f32(vals)
    vt = add_view(tb); at = add_accessor(vt, FLOAT, len(times), "SCALAR",
                                         [float(min(times))], [float(max(times))])
    vv = add_view(vb); av = add_accessor(vv, FLOAT, len(times), "VEC4")
    s = len(anim_samplers_cur)
    anim_samplers_cur.append(g.AnimationSampler(input=at, output=av, interpolation="LINEAR"))
    anim_channels_cur.append(g.AnimationChannel(sampler=s,
        target=g.AnimationChannelTarget(node=joint, path="rotation")))

def trans_track(joint, times, posfn):
    tb, _ = f32(times)
    vals = [posfn(t) for t in times]
    vb, _ = f32(vals)
    vt = add_view(tb); at = add_accessor(vt, FLOAT, len(times), "SCALAR",
                                         [float(min(times))], [float(max(times))])
    vv = add_view(vb); av = add_accessor(vv, FLOAT, len(times), "VEC3")
    s = len(anim_samplers_cur)
    anim_samplers_cur.append(g.AnimationSampler(input=at, output=av, interpolation="LINEAR"))
    anim_channels_cur.append(g.AnimationChannel(sampler=s,
        target=g.AnimationChannelTarget(node=joint, path="translation")))

animations = []

def begin(name):
    global anim_samplers_cur, anim_channels_cur
    anim_samplers_cur, anim_channels_cur = [], []
    return name

def end(name):
    animations.append(g.Animation(name=name, samplers=anim_samplers_cur, channels=anim_channels_cur))

# --- Walk: 1.0s loop, legs/arms opposed, knee/elbow bend, hip bob + spine twist
T = np.linspace(0.0, 1.0, 17).tolist()
def s1(t, ph=0.0):  # one cycle per loop
    return np.sin(2 * np.pi * (t + ph))

begin("Walk")
SW = 0.5  # leg swing amplitude (rad ~28 deg)
rot_track(JI["upperLegL"], T, lambda t: s1(t) * SW)
rot_track(JI["upperLegR"], T, lambda t: s1(t, 0.5) * SW)
rot_track(JI["lowerLegL"], T, lambda t: -max(0.0, -s1(t)) * 0.9)     # knee bends on backswing
rot_track(JI["lowerLegR"], T, lambda t: -max(0.0, -s1(t, 0.5)) * 0.9)
rot_track(JI["upperArmL"], T, lambda t: -s1(t) * 0.42)              # arms opposite legs
rot_track(JI["upperArmR"], T, lambda t: -s1(t, 0.5) * 0.42)
rot_track(JI["lowerArmL"], T, lambda t: -0.25 - abs(s1(t)) * 0.15)
rot_track(JI["lowerArmR"], T, lambda t: -0.25 - abs(s1(t, 0.5)) * 0.15)
rot_track(JI["spine"], T, lambda t: s1(t) * 0.06, axis="y")         # counter-twist
trans_track(JI["hips"], T, lambda t: (0.0, 0.90 + abs(np.sin(2 * np.pi * t)) * 0.045, 0.0))
end("Walk")

# --- Idle: gentle 2.6s breathing loop.
TI = np.linspace(0.0, 2.6, 14).tolist()
begin("Idle")
rot_track(JI["spine"], TI, lambda t: np.sin(2 * np.pi * t / 2.6) * 0.02, axis="y")
rot_track(JI["upperArmL"], TI, lambda t: np.sin(2 * np.pi * t / 2.6) * 0.04)
rot_track(JI["upperArmR"], TI, lambda t: -np.sin(2 * np.pi * t / 2.6) * 0.04)
trans_track(JI["hips"], TI, lambda t: (0.0, 0.90 + np.sin(2 * np.pi * t / 2.6) * 0.012, 0.0))
end("Idle")

# ----------------------------------------------------------------- assemble ---
nodes = []
for i, (name, parent, t) in enumerate(JOINTS):
    children = [j for j, (_, p, _) in enumerate(JOINTS) if p == i]
    nodes.append(g.Node(name=name, translation=list(map(float, t)),
                        children=children or None))
mesh_node = len(nodes)
nodes.append(g.Node(name="WayfarerMesh", mesh=0, skin=0))
# raylib's glTF animation loader dereferences skin.joints[0]->parent, so the
# root joint MUST have a parent. Wrap the skeleton in an identity Armature node.
armature = len(nodes)
nodes.append(g.Node(name="Armature", children=[0]))

skin = g.Skin(inverseBindMatrices=a_ibm, joints=list(range(len(JOINTS))), skeleton=armature)

prim = g.Primitive(attributes=g.Attributes(POSITION=a_pos, NORMAL=a_nrm,
                    COLOR_0=a_col, JOINTS_0=a_jnt, WEIGHTS_0=a_wt), indices=a_idx, material=0)
mesh = g.Mesh(name="Wayfarer", primitives=[prim])
material = g.Material(name="WayfarerMat",
                      pbrMetallicRoughness=g.PbrMetallicRoughness(
                          baseColorFactor=[1, 1, 1, 1], metallicFactor=0.0, roughnessFactor=0.85),
                      doubleSided=True)

gltf = g.GLTF2(
    asset=g.Asset(version="2.0", generator="Zion tools/gen_wayfarer.py"),
    scenes=[g.Scene(nodes=[armature, mesh_node])], scene=0,
    nodes=nodes, meshes=[mesh], materials=[material], skins=[skin],
    accessors=accessors, bufferViews=views,
    buffers=[g.Buffer(byteLength=len(blob))],
    animations=animations,
)
gltf.set_binary_blob(bytes(blob))
out = "assets/models/wayfarer.glb"
gltf.save_binary(out)
print(f"wrote {out}: {n} verts, {len(indices)//3} tris, {len(JOINTS)} joints, "
      f"{len(animations)} anims ({', '.join(a.name for a in animations)})")
