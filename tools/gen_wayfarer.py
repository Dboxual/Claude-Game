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

def sphere(center, radius, joint, color, squash=1.0, rings=8, sectors=10):
    """Low-poly UV sphere rigidly bound to `joint`."""
    cx, cy, cz = center
    base = len(positions)
    for r in range(rings + 1):
        phi = np.pi * r / rings
        for s in range(sectors + 1):
            th = 2 * np.pi * s / sectors
            nx = np.sin(phi) * np.cos(th)
            ny = np.cos(phi)
            nz = np.sin(phi) * np.sin(th)
            positions.append((cx + nx * radius, cy + ny * radius * squash, cz + nz * radius))
            normals.append((nx, ny, nz))
            colors.append(color)
            joints_a.append((joint, 0, 0, 0))
            weights_a.append((1.0, 0.0, 0.0, 0.0))
    row = sectors + 1
    for r in range(rings):
        for s in range(sectors):
            a = base + r * row + s; b = base + (r + 1) * row + s
            indices.extend([a, b, a + 1, a + 1, b, b + 1])

# --- assemble the body (authored in global bind pose) -------------------------
box((0.00, 0.86, 0.00), (0.42, 0.24, 0.30), JI["hips"], C_LEATHER)
box((0.00, 1.24, 0.00), (0.46, 0.58, 0.30), JI["spine"], C_CLOTH, top_scale=0.92)
box((0.00, 1.26, 0.15), (0.34, 0.42, 0.06), JI["spine"], C_LEATHER)      # vest front
box((0.30, 1.50, 0.00), (0.24, 0.16, 0.32), JI["spine"], C_PAULD)        # pauldron L
box((-0.30, 1.50, 0.00), (0.24, 0.16, 0.32), JI["spine"], C_PAULD)       # pauldron R
sphere((0.00, 1.80, 0.00), 0.20, JI["head"], C_SKIN, squash=1.08)
sphere((0.00, 1.86, -0.03), 0.235, JI["head"], C_HOOD, squash=0.86)      # hood
box((0.00, 1.80, 0.15), (0.20, 0.10, 0.06), JI["head"], C_EYE)           # face band
# arms
box((0.28, 1.36, 0.00), (0.14, 0.36, 0.16), JI["upperArmL"], C_CLOTH)
box((0.28, 1.02, 0.02), (0.14, 0.40, 0.16), JI["lowerArmL"], C_LEATHER)
box((-0.28, 1.36, 0.00), (0.14, 0.36, 0.16), JI["upperArmR"], C_CLOTH)
box((-0.28, 1.02, 0.02), (0.14, 0.40, 0.16), JI["lowerArmR"], C_LEATHER)
# legs + boots
box((0.12, 0.62, 0.00), (0.18, 0.46, 0.22), JI["upperLegL"], C_PANTS)
box((0.12, 0.24, 0.00), (0.16, 0.40, 0.20), JI["lowerLegL"], C_PANTS)
box((0.12, 0.06, 0.04), (0.19, 0.14, 0.30), JI["lowerLegL"], C_BOOT)
box((-0.12, 0.62, 0.00), (0.18, 0.46, 0.22), JI["upperLegR"], C_PANTS)
box((-0.12, 0.24, 0.00), (0.16, 0.40, 0.20), JI["lowerLegR"], C_PANTS)
box((-0.12, 0.06, 0.04), (0.19, 0.14, 0.30), JI["lowerLegR"], C_BOOT)

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
