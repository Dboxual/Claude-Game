// Original procedural art for The Ruins of Gloamwood vertical slice.
// No external meshes, textures, or copyrighted assets are used.

export function groundHeight(x,z){
  const rolling=Math.sin(x*.13)*.48+Math.sin(z*.09+x*.035)*.62+Math.cos(x*.055-z*.08)*.3;
  const river=-1.7*Math.exp(-Math.pow((z+13)/3.2,2));
  const courtyard=.5*Math.exp(-((x*x)/180+((z-1)*(z-1))/95));
  const boss=1.45*Math.exp(-((x*x)/250+((z-29)*(z-29))/100));
  const path=-.18*Math.exp(-Math.pow(x/4.4,2));
  return rolling+river+courtyard+boss+path;
}

export function createPalette(THREE){
  const std=(color,roughness=.9,metalness=0)=>metalness>.2?new THREE.MeshStandardMaterial({color,roughness,metalness}):new THREE.MeshLambertMaterial({color});
  return {
    soil:std(0x302b25,1),path:std(0x514433,.98),moss:std(0x39513a,1),grass:std(0x496342,1),wet:std(0x253f3b,.58),
    bark:std(0x392c25,1),barkLight:std(0x544034,1),leaf:std(0x294936,.96),leafLight:std(0x48684a,.95),deadLeaf:std(0x594b37,1),
    stone:std(0x6b7068,.94),stoneDark:std(0x454b48,1),stoneMoss:std(0x52604b,1),rubble:std(0x555850,1),
    bronze:std(0x8d7046,.38,.64),iron:std(0x343941,.38,.75),leather:std(0x51352a,.92),cloth:std(0x633549,.88),clothDark:std(0x27222e,.92),
    skin:std(0xb97d62,.86),bone:std(0xb7aa8c,.92),enemySkin:std(0x6c625a,.95),corrupt:std(0x4b2c35,.9),
    magic:new THREE.MeshStandardMaterial({color:0x516c70,emissive:0x31b9b1,emissiveIntensity:1.8,roughness:.35}),
    void:new THREE.MeshStandardMaterial({color:0x402c55,emissive:0x6f35a6,emissiveIntensity:2.1,metalness:.4,roughness:.3}),
    fire:new THREE.MeshStandardMaterial({color:0xd96a2c,emissive:0xff4a12,emissiveIntensity:3.4,roughness:.4}),
    water:new THREE.MeshPhongMaterial({color:0x315f67,shininess:55,transparent:true,opacity:.82}),
    terrain:new THREE.MeshLambertMaterial({vertexColors:true})
  };
}

function part(THREE,geo,mat,cast=true){const m=new THREE.Mesh(geo,mat);m.castShadow=cast;m.receiveShadow=true;return m}
function tapered(THREE,a,b,len,mat,segments=7){return part(THREE,new THREE.CylinderGeometry(a,b,len,segments),mat)}
function limb(THREE,len,r,mat){const g=new THREE.Group();const m=tapered(THREE,r*.82,r,len,mat,8);m.position.y=-len*.5;g.add(m);return g}

export function createWayfarer(THREE,p){
  const g=new THREE.Group();g.name='player';
  const hips=new THREE.Group();hips.name='hips';hips.position.y=1.08;g.add(hips);
  const pelvis=part(THREE,new THREE.SphereGeometry(.42,10,7),p.clothDark);pelvis.scale.set(1,.7,.72);hips.add(pelvis);
  const torso=new THREE.Group();torso.name='torso';torso.position.y=.5;hips.add(torso);
  const tunic=part(THREE,new THREE.LatheGeometry([new THREE.Vector2(.5,0),new THREE.Vector2(.47,.22),new THREE.Vector2(.4,.72),new THREE.Vector2(.34,1.05)],10),p.cloth);tunic.position.y=-.04;tunic.scale.z=.72;torso.add(tunic);
  const belt=part(THREE,new THREE.TorusGeometry(.44,.075,6,12),p.leather);belt.rotation.x=Math.PI/2;belt.position.y=.03;torso.add(belt);
  const chest=part(THREE,new THREE.SphereGeometry(.47,10,7),p.iron);chest.scale.set(1,.82,.55);chest.position.y=.72;torso.add(chest);
  for(const side of[-1,1]){const plate=part(THREE,new THREE.SphereGeometry(.23,8,5),p.bronze);plate.scale.set(1.35,.55,.8);plate.position.set(side*.52,.76,0);torso.add(plate)}
  const neck=tapered(THREE,.12,.14,.22,p.skin,8);neck.position.y=1.2;torso.add(neck);
  const head=new THREE.Group();head.name='head';head.position.y=1.42;torso.add(head);
  const face=part(THREE,new THREE.SphereGeometry(.29,12,9),p.skin);face.scale.set(.85,1.15,.9);head.add(face);
  const hood=part(THREE,new THREE.SphereGeometry(.34,10,7,0,Math.PI*2,0,Math.PI*.62),p.clothDark);hood.position.y=.05;hood.scale.set(1,1.15,1);head.add(hood);
  const hair=part(THREE,new THREE.SphereGeometry(.3,9,6,0,Math.PI*2,0,Math.PI*.5),p.bark);hair.position.set(0,.15,-.03);head.add(hair);
  const cape=part(THREE,new THREE.PlaneGeometry(.85,1.55,3,5),p.clothDark);cape.position.set(0,.42,-.38);cape.rotation.x=-.12;torso.add(cape);
  const arms=[];for(const side of[-1,1]){const shoulder=new THREE.Group();shoulder.name=side<0?'armL':'armR';shoulder.position.set(side*.52,.78,0);torso.add(shoulder);const upper=limb(THREE,.62,.13,p.cloth);upper.rotation.z=side*.1;shoulder.add(upper);const elbow=new THREE.Group();elbow.position.set(side*.06,-.58,0);shoulder.add(elbow);const fore=limb(THREE,.58,.115,p.leather);elbow.add(fore);const hand=part(THREE,new THREE.SphereGeometry(.13,8,6),p.skin);hand.scale.y=1.25;hand.position.y=-.62;elbow.add(hand);arms.push({shoulder,elbow,hand})}
  const legs=[];for(const side of[-1,1]){const hip=new THREE.Group();hip.name=side<0?'legL':'legR';hip.position.set(side*.24,-.03,0);hips.add(hip);const thigh=limb(THREE,.72,.17,p.clothDark);hip.add(thigh);const knee=new THREE.Group();knee.position.y=-.68;hip.add(knee);const shin=limb(THREE,.65,.145,p.leather);knee.add(shin);const boot=part(THREE,new THREE.SphereGeometry(.18,8,6),p.iron);boot.scale.set(.9,.75,1.55);boot.position.set(0,-.68,.09);knee.add(boot);legs.push({hip,knee,boot})}
  const sword=new THREE.Group();sword.name='weapon';const bladeShape=new THREE.Shape();bladeShape.moveTo(0,-.09);bladeShape.lineTo(1.7,-.055);bladeShape.lineTo(2.05,0);bladeShape.lineTo(1.7,.055);bladeShape.lineTo(0,.09);const blade=part(THREE,new THREE.ExtrudeGeometry(bladeShape,{depth:.055,bevelEnabled:true,bevelSize:.025,bevelThickness:.02,bevelSegments:1}),p.iron);blade.name='blade';sword.add(blade);const fuller=part(THREE,new THREE.BoxGeometry(1.55,.035,.075),p.void,false);fuller.position.set(.85,0,.06);sword.add(fuller);const guard=part(THREE,new THREE.BoxGeometry(.16,.72,.13),p.bronze);guard.position.x=-.08;sword.add(guard);const grip=part(THREE,new THREE.CylinderGeometry(.065,.075,.55,8),p.leather);grip.rotation.z=Math.PI/2;grip.position.x=-.42;sword.add(grip);sword.position.set(.48,.05,.08);sword.rotation.set(0,-.1,-1.5);arms[1].hand.add(sword);
  const trail=part(THREE,new THREE.PlaneGeometry(2.2,.7),new THREE.MeshBasicMaterial({color:0x9d72c8,transparent:true,opacity:0,side:THREE.DoubleSide,depthWrite:false}),false);trail.name='weaponTrail';trail.position.x=1;trail.rotation.x=Math.PI/2;sword.add(trail);
  g.userData.rig={hips,torso,head,arms,legs,sword,trail,cape};g.userData.anim={hit:0,dodge:0,gather:0,death:0};return g;
}

function createRaider(THREE,p){
  const g=new THREE.Group();g.userData.kind='raider';
  const root=new THREE.Group();root.position.y=1;g.add(root);const body=part(THREE,new THREE.LatheGeometry([new THREE.Vector2(.43,0),new THREE.Vector2(.4,.25),new THREE.Vector2(.35,.62),new THREE.Vector2(.42,.92)],9),p.corrupt);body.position.y=.04;body.scale.z=.72;root.add(body);
  const mantle=part(THREE,new THREE.SphereGeometry(.48,9,6,0,Math.PI*2,0,Math.PI*.5),p.leather);mantle.position.y=.9;mantle.scale.y=.45;root.add(mantle);
  const head=new THREE.Group();head.position.y=1.27;root.add(head);const face=part(THREE,new THREE.SphereGeometry(.27,9,7),p.enemySkin);face.scale.y=1.1;head.add(face);const mask=part(THREE,new THREE.BoxGeometry(.42,.3,.12),p.bone);mask.position.set(0,.02,.22);mask.rotation.x=-.12;head.add(mask);for(const s of[-1,1]){const horn=tapered(THREE,0,.065,.38,p.bone,6);horn.position.set(s*.19,.27,0);horn.rotation.z=s*.45;head.add(horn)}
  const arms=[];for(const s of[-1,1]){const a=new THREE.Group();a.position.set(s*.42,.78,0);root.add(a);a.add(limb(THREE,.72,.12,p.enemySkin));arms.push(a)}
  const legs=[];for(const s of[-1,1]){const l=new THREE.Group();l.position.set(s*.2,.06,0);root.add(l);l.add(limb(THREE,.85,.15,p.leather));const foot=part(THREE,new THREE.SphereGeometry(.16,7,5),p.iron);foot.scale.set(.8,.65,1.4);foot.position.set(0,-.87,.08);l.add(foot);legs.push(l)}
  const axe=new THREE.Group();const shaft=tapered(THREE,.045,.055,1.3,p.bark,7);shaft.rotation.z=Math.PI/2;axe.add(shaft);const edge=part(THREE,new THREE.ConeGeometry(.32,.65,4),p.iron);edge.rotation.z=-Math.PI/2;edge.position.x=.58;axe.add(edge);axe.position.set(0,-.65,0);arms[1].add(axe);
  g.userData.rig={root,head,arms,legs,weapon:axe};return g;
}

function createGuardian(THREE,p){
  const g=new THREE.Group();g.userData.kind='guardian';
  const root=new THREE.Group();root.position.y=1.08;g.add(root);const core=part(THREE,new THREE.DodecahedronGeometry(.53,1),p.stoneDark);core.scale.set(.9,1.25,.72);core.position.y=.55;root.add(core);const chest=part(THREE,new THREE.BoxGeometry(1,.28,.45),p.stoneMoss);chest.position.y=.85;chest.rotation.z=.08;root.add(chest);
  const weak=part(THREE,new THREE.IcosahedronGeometry(.11,1),p.magic,false);weak.name='weakPoint';weak.position.set(0,.55,.48);root.add(weak);const head=new THREE.Group();head.position.y=1.35;root.add(head);const helm=part(THREE,new THREE.DodecahedronGeometry(.3,1),p.stone);helm.scale.set(.85,1,.72);head.add(helm);const slit=part(THREE,new THREE.BoxGeometry(.32,.045,.06),p.magic,false);slit.position.z=.27;head.add(slit);
  const arms=[];for(const s of[-1,1]){const a=new THREE.Group();a.position.set(s*.57,.78,0);root.add(a);const b=limb(THREE,.8,.17,p.stone);b.rotation.z=s*.12;a.add(b);arms.push(a)}const legs=[];for(const s of[-1,1]){const l=new THREE.Group();l.position.set(s*.23,.03,0);root.add(l);l.add(limb(THREE,.9,.2,p.stoneDark));legs.push(l)}
  const mace=new THREE.Group();const h=tapered(THREE,.055,.07,1.35,p.bark,7);h.rotation.z=Math.PI/2;mace.add(h);const headM=part(THREE,new THREE.DodecahedronGeometry(.34,0),p.stone);headM.position.x=.7;mace.add(headM);mace.position.y=-.72;arms[1].add(mace);g.userData.rig={root,head,arms,legs,weapon:mace,weak};return g;
}

function createColossus(THREE,p){
  const g=createGuardian(THREE,p);g.userData.kind='boss';const r=g.userData.rig;g.scale.set(2.15,2.35,2.15);r.root.rotation.z=-.03;
  const crown=new THREE.Group();for(let i=-2;i<=2;i++){const spike=tapered(THREE,0,.08,.55+Math.abs(i)*.09,p.stoneDark,6);spike.position.set(i*.16,.32,0);spike.rotation.z=-i*.12;crown.add(spike)}crown.position.y=.15;r.head.add(crown);
  const shoulder=part(THREE,new THREE.DodecahedronGeometry(.55,0),p.stoneMoss);shoulder.scale.set(1.5,.75,1);shoulder.position.set(-.62,.75,0);r.root.add(shoulder);
  for(const x of[-.25,.25]){const w=part(THREE,new THREE.IcosahedronGeometry(.075,1),p.magic,false);w.position.set(x,.48,.5);r.root.add(w)}return g;
}

export function createEnemy(THREE,p,kind='raider'){return kind==='boss'?createColossus(THREE,p):kind==='guardian'?createGuardian(THREE,p):createRaider(THREE,p)}

function makeTerrain(THREE,p){
  const geo=new THREE.PlaneGeometry(76,88,48,56);geo.rotateX(-Math.PI/2);const pos=geo.attributes.position,colors=[];const c=new THREE.Color();for(let i=0;i<pos.count;i++){const x=pos.getX(i),z=pos.getZ(i),h=groundHeight(x,z);pos.setY(i,h);const river=Math.abs(z+13)<3.4;const path=Math.abs(x)<3.8&&z>-32&&z<34;if(river)c.set(0x273d38);else if(path)c.set(0x574b38);else if(h>1.05)c.set(0x46513e);else c.set((i+x|0)%5===0?0x3a4937:0x30352e);c.offsetHSL((Math.sin(x*2+z)*.008),0,(Math.sin(x*.7-z)*.035));colors.push(c.r,c.g,c.b)}geo.setAttribute('color',new THREE.Float32BufferAttribute(colors,3));geo.computeVertexNormals();const m=part(THREE,geo,p.terrain,false);m.receiveShadow=true;m.name='sculptedTerrain';return m;
}

function branchTree(THREE,p,x,z,scale=1,dead=false,lean=0,variant=0){
  const g=new THREE.Group();g.position.set(x,groundHeight(x,z),z);g.rotation.z=lean;g.userData.sway=Math.random()*6;const trunk=tapered(THREE,.22*scale,.42*scale,4.5*scale,dead?p.barkLight:p.bark,9);trunk.position.y=2.15*scale;trunk.rotation.z=(variant-.5)*.025;g.add(trunk);
  for(let i=0;i<5;i++){const a=i*2.31+variant,level=(2.1+i*.48)*scale,len=(1.4+(i%2)*.45)*scale;const pivot=new THREE.Group();pivot.position.y=level;pivot.rotation.y=a;pivot.rotation.z=-.78-(i%2)*.12;const b=tapered(THREE,.055*scale,.13*scale,len,p.bark,6);b.position.y=len*.5;pivot.add(b);g.add(pivot);if(!dead){for(let j=0;j<3;j++){const crown=part(THREE,new THREE.DodecahedronGeometry((.62-j*.08)*scale,0),j===1?p.leafLight:p.leaf);crown.scale.set(1.3,.75,1);crown.rotation.set(j*.3,a*.2,j*.5);crown.position.set(Math.sin(a+j)*.25*scale,(level+len*.55+j*.2*scale),Math.cos(a+j)*.25*scale);g.add(crown)}}}
  for(let i=0;i<4;i++){const root=tapered(THREE,.035*scale,.16*scale,1.35*scale,p.bark,7);root.position.set(0,.08,i%2?.08:-.08);root.rotation.z=Math.PI/2-.1;root.rotation.y=i*Math.PI/2;g.add(root)}return g;
}

function stoneBlock(THREE,p,w,h,d,mat=p.stone){const b=part(THREE,new THREE.BoxGeometry(w,h,d),mat);const a=b.geometry.attributes.position;for(let i=0;i<a.count;i++){a.setXYZ(i,a.getX(i)+(Math.sin(i*12.7)*.035),a.getY(i)+(Math.sin(i*7.1)*.025),a.getZ(i)+(Math.sin(i*3.9)*.035))}b.geometry.computeVertexNormals();return b}
function wall(THREE,p,x,z,len,height,rot=0,broken=.4){const g=new THREE.Group();const courses=Math.ceil(height/.75),cols=Math.ceil(len/1.35);for(let y=0;y<courses;y++)for(let i=0;i<cols;i++){if(y>courses-2&&Math.sin(i*5.7+y)>broken)continue;const b=stoneBlock(THREE,p,1.25,.66,.72,(i+y)%5===0?p.stoneMoss:p.stone);b.position.set((i-(cols-1)/2)*1.24+(y%2)*.35,y*.67+.33,0);b.rotation.y=(Math.sin(i*9+y)*.025);g.add(b)}g.position.set(x,groundHeight(x,z),z);g.rotation.y=rot;return g}
function arch(THREE,p,x,z,rot=0,scale=1){const g=new THREE.Group();for(const s of[-1,1]){for(let y=0;y<5;y++){const b=stoneBlock(THREE,p,.85*scale,.65*scale,1*scale,y%3?p.stone:p.stoneMoss);b.position.set(s*1.5*scale,(y+.5)*.65*scale,0);g.add(b)}}for(let i=0;i<9;i++){const a=Math.PI-i*Math.PI/8,b=stoneBlock(THREE,p,.72*scale,.7*scale,1*scale,i%3?p.stone:p.stoneMoss);b.position.set(Math.cos(a)*1.5*scale,3.2*scale+Math.sin(a)*1.5*scale,0);b.rotation.z=a-Math.PI/2;g.add(b)}g.position.set(x,groundHeight(x,z),z);g.rotation.y=rot;return g}
function camp(THREE,p,x,z){const g=new THREE.Group();for(const [ox,oz,r] of[[-2,0,.25],[2,1,-.3]]){const frame=tapered(THREE,.04,.06,3.2,p.bark,7);frame.rotation.z=Math.PI/2;frame.position.set(ox,1.25,oz);g.add(frame);const cloth=part(THREE,new THREE.BufferGeometry().setFromPoints([]),p.cloth);const geo=new THREE.BufferGeometry();geo.setAttribute('position',new THREE.Float32BufferAttribute([-1.6,0,0,0,2.3,0,1.6,0,0,-1.6,0,-2,0,2.3,-2,1.6,0,-2],3));geo.setIndex([0,1,2,3,4,5]);geo.computeVertexNormals();cloth.geometry=geo;cloth.position.set(ox,0,oz);cloth.rotation.y=r;g.add(cloth)}const fire=part(THREE,new THREE.IcosahedronGeometry(.26,1),p.fire,false);fire.name='campfire';fire.position.set(0,.32,1);g.add(fire);const l=new THREE.PointLight(0xff6b32,7,9,2);l.position.set(0,1.1,1);g.add(l);g.position.set(x,groundHeight(x,z),z);return g}
function vegetationCluster(THREE,p,x,z,variant=0){const g=new THREE.Group();g.position.set(x,groundHeight(x,z),z);for(let i=0;i<5+(variant%4);i++){const blade=part(THREE,new THREE.PlaneGeometry(.16,.65+(i%3)*.14),i%4?p.grass:p.leafLight,false);blade.position.set((Math.sin(i*4.2)*.65),.3,(Math.cos(i*5.1)*.55));blade.rotation.y=i*1.7;g.add(blade)}if(variant%3===0)for(let i=0;i<3;i++){const cap=part(THREE,new THREE.SphereGeometry(.11+i*.02,7,4,0,Math.PI*2,0,Math.PI/2),i===2?p.fire:p.bone,false);cap.position.set((i-1)*.22,.14,.1);g.add(cap)}return g}
function instancedGroundCover(THREE,p){const g=new THREE.Group(),dummy=new THREE.Object3D(),clusters=[[-6,-27],[5,-23],[-10,-8],[9,-7],[-12,5],[12,2],[-6,12],[5,17],[-10,25],[9,29]],spots=[];for(const [cx,cz] of clusters)for(let i=0;i<9;i++)spots.push([cx+Math.sin(i*2.17)*2.2,cz+Math.cos(i*1.73)*1.8,i]);const grass=new THREE.InstancedMesh(new THREE.PlaneGeometry(.22,.72),p.grass,spots.length);grass.name='instancedGrass';spots.forEach(([x,z,i],n)=>{dummy.position.set(x,groundHeight(x,z)+.33,z);dummy.rotation.set(0,i*1.91,Math.sin(i)*.12);dummy.scale.set(.7+(i%4)*.16,.7+(i%3)*.18,1);dummy.updateMatrix();grass.setMatrixAt(n,dummy.matrix)});g.add(grass);const bushes=new THREE.InstancedMesh(new THREE.DodecahedronGeometry(.55,0),p.leaf,30);bushes.name='instancedBushes';for(let i=0;i<30;i++){const [cx,cz]=clusters[i%clusters.length],x=cx+Math.sin(i*4.37)*3,z=cz+Math.cos(i*2.93)*2.4;dummy.position.set(x,groundHeight(x,z)+.38,z);dummy.rotation.set(i*.1,i*1.37,i*.04);dummy.scale.set(.75+(i%3)*.25,.45+(i%4)*.12,.7+(i%5)*.1);dummy.updateMatrix();bushes.setMatrixAt(i,dummy.matrix)}g.add(bushes);const caps=new THREE.InstancedMesh(new THREE.SphereGeometry(.12,6,4,0,Math.PI*2,0,Math.PI/2),p.bone,18);caps.name='instancedMushrooms';for(let i=0;i<18;i++){const [cx,cz]=clusters[i%clusters.length],x=cx+Math.sin(i*5.2)*1.3,z=cz+Math.cos(i*3.4)*1.1;dummy.position.set(x,groundHeight(x,z)+.12,z);dummy.rotation.set(0,i,0);dummy.scale.setScalar(.65+(i%4)*.18);dummy.updateMatrix();caps.setMatrixAt(i,dummy.matrix)}g.add(caps);const branches=new THREE.InstancedMesh(new THREE.CylinderGeometry(.035,.08,1.5,6),p.barkLight,12);branches.name='instancedFallenBranches';for(let i=0;i<12;i++){const [cx,cz]=clusters[i%clusters.length],x=cx+Math.sin(i*7)*2,z=cz+Math.cos(i*4)*1.8;dummy.position.set(x,groundHeight(x,z)+.08,z);dummy.rotation.set(Math.PI/2,i*.8,.18);dummy.scale.set(1+(i%3)*.2,1,1);dummy.updateMatrix();branches.setMatrixAt(i,dummy.matrix)}g.add(branches);g.traverse(o=>{if(o.isInstancedMesh){o.instanceMatrix.setUsage(THREE.StaticDrawUsage);o.castShadow=false;o.receiveShadow=true}});return g}

export function createGloamwood(THREE,p){
  const world=new THREE.Group();world.name='gloamwoodSlice';world.add(makeTerrain(THREE,p));
  // Authored forest walls and threshold; placement follows the route rather than random scatter.
  const trees=[];for(const side of[-1,1])for(let i=0;i<10;i++){const x=side*(7.5+(i%3)*2.2),z=-32+i*4.4;const t=branchTree(THREE,p,x,z,.85+(i%4)*.12,i===6,side*(i%2?.035:-.04),i);world.add(t);trees.push(t)}
  for(const [x,z,s,d,l,v] of[[-15,-28,1.4,0,.08,2],[15,-25,1.25,0,-.06,3],[-13,3,1.45,0,.1,1],[14,7,1.2,1,-.1,4],[-16,21,1.6,1,.07,0],[16,25,1.4,0,-.08,2]]){const t=branchTree(THREE,p,x,z,s,d,l,v);world.add(t);trees.push(t)}
  // Stream crossing with irregular banks and a bowed timber bridge.
  const stream=part(THREE,new THREE.PlaneGeometry(72,5,32,2),p.water,false);stream.rotation.x=-Math.PI/2;stream.position.set(0,groundHeight(0,-13)+.22,-13);world.add(stream);for(let i=-4;i<=4;i++){const plank=stoneBlock(THREE,p,1.35,.18,3.4,p.barkLight);plank.position.set(i*1.26,groundHeight(i*1.26,-13)+.65+Math.cos(i*.35)*.2,-13);plank.rotation.z=-Math.sin(i*.35)*.1;world.add(plank)}
  // Courtyard with a broken perimeter, stairs, arch, statue, and rubble.
  world.add(wall(THREE,p,-9,0,13,3.6,Math.PI/2,.25),wall(THREE,p,9,1,12,3.4,Math.PI/2,.45),wall(THREE,p,0,7,18,2.8,0,.6),arch(THREE,p,0,-5.5,0,1.25));
  for(let i=0;i<5;i++){const step=stoneBlock(THREE,p,8-i*.45,.3,1.2,p.stoneMoss);step.position.set(0,groundHeight(0,7.5)+i*.27,7.5+i*.8);world.add(step)}
  const statue=new THREE.Group(),robe=part(THREE,new THREE.LatheGeometry([new THREE.Vector2(.8,0),new THREE.Vector2(.65,1),new THREE.Vector2(.38,2.4),new THREE.Vector2(.45,2.8)],8),p.stoneMoss);statue.add(robe);const statueHead=part(THREE,new THREE.SphereGeometry(.38,9,6),p.stone);statueHead.position.y=3.15;statue.add(statueHead);statue.position.set(-5,groundHeight(-5,3),3);statue.rotation.z=.18;world.add(statue);
  for(let i=0;i<22;i++){const x=-8+(i*3.7)%16,z=-4+(i*5.1)%12;const r=part(THREE,new THREE.DodecahedronGeometry(.2+(i%4)*.11,0),i%3?p.rubble:p.stoneMoss);r.scale.set(1.6,.7,1);r.position.set(x,groundHeight(x,z)+.15,z);r.rotation.y=i;world.add(r)}
  world.add(camp(THREE,p,-9,14));
  // Temple facade and cave mouth.
  world.add(wall(THREE,p,0,24,22,5,0,.18),arch(THREE,p,0,23.2,0,1.55));const cave=part(THREE,new THREE.CircleGeometry(2.15,24,0,Math.PI),new THREE.MeshBasicMaterial({color:0x08090b}),false);cave.position.set(0,groundHeight(0,23.1)+2.15,23.05);world.add(cave);for(const x of[-5.8,5.8]){const relief=part(THREE,new THREE.LatheGeometry([new THREE.Vector2(.55,0),new THREE.Vector2(.38,1.8),new THREE.Vector2(.5,3.4)],8),p.stoneMoss);relief.position.set(x,groundHeight(x,23)+1.7,22.5);world.add(relief)}
  // Boss arena ringed by broken monoliths, with a distant crooked watch-spire.
  for(let i=0;i<11;i++){const a=i/11*Math.PI*2,x=Math.cos(a)*12,z=31+Math.sin(a)*8;const m=stoneBlock(THREE,p,1.2,3+(i%4),1.4,i%3?p.stoneDark:p.stoneMoss);m.position.set(x,groundHeight(x,z)+(3+(i%4))*.5,z);m.rotation.y=-a;m.rotation.z=(i%2-.5)*.12;world.add(m)}
  const spire=new THREE.Group();for(let y=0;y<8;y++){const tier=stoneBlock(THREE,p,5-y*.42,1.2,4-y*.28,y%2?p.stoneDark:p.stoneMoss);tier.position.y=y*1.05;tier.rotation.y=y*.12;spire.add(tier)}const crown=part(THREE,new THREE.ConeGeometry(2.1,5,5),p.stoneDark);crown.position.y=10.2;crown.rotation.y=.4;spire.add(crown);spire.position.set(-25,groundHeight(-25,39),41);spire.rotation.z=-.07;world.add(spire);
  world.add(instancedGroundCover(THREE,p));
  const dustGeo=new THREE.BufferGeometry(),pts=[];for(let i=0;i<150;i++)pts.push((Math.random()-.5)*55,Math.random()*7-1,(Math.random()-.5)*78);dustGeo.setAttribute('position',new THREE.Float32BufferAttribute(pts,3));const pollen=new THREE.Points(dustGeo,new THREE.PointsMaterial({color:0xd6c18a,size:.045,transparent:true,opacity:.45,depthWrite:false}));pollen.name='pollen';world.add(pollen);
  return {world,trees,pollen,stream};
}

export function createResourceNode(THREE,p,type,x,z){const g=new THREE.Group();g.position.set(x,groundHeight(x,z),z);g.userData={type,alive:true};if(type==='wood'){const stump=tapered(THREE,.32,.45,.75,p.bark,10);stump.position.y=.35;g.add(stump);for(let i=0;i<3;i++){const log=tapered(THREE,.2,.22,1.6,p.barkLight,9);log.rotation.z=Math.PI/2;log.position.set((i-1)*.25,.25+i*.2,0);g.add(log)}}else if(type==='ore'){for(let i=0;i<4;i++){const rock=part(THREE,new THREE.DodecahedronGeometry(.35+i*.06,0),i===2?p.bronze:p.stoneDark);rock.scale.set(1,1.3,.8);rock.position.set((i-1.5)*.35,.3,Math.sin(i)*.2);g.add(rock)}}else{for(let i=0;i<6;i++){const leaf=part(THREE,new THREE.PlaneGeometry(.22,.65),i===2?p.magic:p.leafLight,false);leaf.position.set(Math.sin(i)*.3,.3,Math.cos(i)*.3);leaf.rotation.set(-.35,i,0);g.add(leaf)}}return g}

export function animateHumanoid(actor,time,dt,speed,action=0){
  const r=actor.userData.rig;if(!r)return;const moving=speed>.05,run=Math.min(1,speed/6);actor.userData.walkPhase=(actor.userData.walkPhase||0)+speed*dt*2.15;const cycle=actor.userData.walkPhase,step=moving?Math.sin(cycle)*(.5+run*.35):0,bob=moving?Math.abs(Math.sin(cycle))*-.055:Math.sin(time*.002+actor.id)*.025,anim=actor.userData.anim;
  if(r.hips)r.hips.position.y=1.08+bob;if(r.root)r.root.position.y=(actor.userData.kind==='boss'?1.08:1)+bob;
  if(r.legs)for(let i=0;i<2;i++){const l=r.legs[i].hip||r.legs[i];l.rotation.x=step*(i?1:-1);if(r.legs[i].knee)r.legs[i].knee.rotation.x=Math.max(0,-step*(i?1:-1))*.65}
  if(r.arms)for(let i=0;i<2;i++){const a=r.arms[i].shoulder||r.arms[i];a.rotation.x=step*(i?-1:1)*.45}
  if(r.torso)r.torso.rotation.y=moving?Math.sin(cycle)*.035:Math.sin(time*.0015)*.018;
  if(r.head)r.head.rotation.y=Math.sin(time*.0011+actor.id)*.08;
  if(r.cape){const a=r.cape.geometry.attributes.position;for(let i=0;i<a.count;i++)a.setZ(i,Math.sin(time*.005+i)*.035+run*.08);a.needsUpdate=true}
  if(anim){anim.dodge=Math.max(0,(anim.dodge||0)-dt);anim.gather=Math.max(0,(anim.gather||0)-dt);anim.hit=Math.max(0,(anim.hit||0)-dt);if(r.torso)r.torso.rotation.x=0;if(r.hips){r.hips.rotation.z=anim.dodge?Math.sin((.45-anim.dodge)/.45*Math.PI)*-.55:0;r.hips.scale.y=anim.dodge?.72:1}if(anim.gather&&r.arms?.[1]){r.arms[1].shoulder.rotation.x=-1.05+Math.sin(anim.gather*18)*.6;r.torso.rotation.x=.22}if(anim.hit&&r.torso)r.torso.rotation.x=-Math.sin(anim.hit*15)*.18}
  if(action>0&&r.arms?.[1]){const phase=1-action/.8,wind=Math.sin(Math.min(1,phase)*Math.PI);const a=r.arms[1].shoulder||r.arms[1];a.rotation.x=-1.1+wind*2.2;a.rotation.z=-.45;if(r.torso)r.torso.rotation.y=-.35+wind*.9;if(r.trail)r.trail.material.opacity=Math.sin(phase*Math.PI)*.32}
  else if(r.trail)r.trail.material.opacity=0;
}
