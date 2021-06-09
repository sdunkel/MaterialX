import * as THREE from 'three';
import { OBJLoader } from 'three/examples/jsm/loaders/OBJLoader.js';
import { RGBELoader } from 'three/examples/jsm/loaders/RGBELoader.js';
import { OrbitControls } from 'three/examples/jsm/controls/OrbitControls.js';
import { EffectComposer } from 'three/examples/jsm/postprocessing/EffectComposer.js';
import { RenderPass } from 'three/examples/jsm/postprocessing/RenderPass.js';
import { ShaderPass } from 'three/examples/jsm/postprocessing/ShaderPass.js';
import { GammaCorrectionShader } from 'three/examples/jsm/shaders/GammaCorrectionShader.js';
import { Uniform } from 'three';

let camera, scene, model, renderer, composer, controls, uniforms, mx;

let normalMat = new THREE.Matrix3();
let viewProjMat = new THREE.Matrix4();
let worldViewPos = new THREE.Vector3();

init();

/**
 * Adds a tangent BufferAttribute to the passed in geometry.
 * See MaterialXRender/Mesh.cpp
 * @param {THREE.BufferGeometry} geometry 
 */
function generateTangents(geometry) {
    let p0 = new THREE.Vector3();
    let p1 = new THREE.Vector3();
    let p2 = new THREE.Vector3();

    let n0 = new THREE.Vector3();
    let n1 = new THREE.Vector3();
    let n2 = new THREE.Vector3();

    let w0 = new THREE.Vector2();
    let w1 = new THREE.Vector2();
    let w2 = new THREE.Vector2();

    let e1 = new THREE.Vector3();
    let e2 = new THREE.Vector3();
    
    let tangent = new THREE.Vector3();
    let t0 = new THREE.Vector3();
    let t1 = new THREE.Vector3();
    let t2 = new THREE.Vector3();

    const positions = geometry.attributes.position;
    const normals = geometry.attributes.normal;
    const uvs = geometry.attributes.uv;
    const length = positions.count * positions.itemSize;

    const tangentsdata = new Float32Array(length);

    for(let i = 0; i < positions.count; i += 3) {
        const idx = i * positions.itemSize;
        const uvidx = i * uvs.itemSize;

        p0.set(positions.array[idx], positions.array[idx + 1], positions.array[idx + 2]);
        p1.set(positions.array[idx + 3], positions.array[idx + 4], positions.array[idx + 5]);
        p2.set(positions.array[idx + 6], positions.array[idx + 7], positions.array[idx + 8]);

        n0.set(normals.array[idx], normals.array[idx + 1], normals.array[idx + 2]);
        n1.set(normals.array[idx + 3], normals.array[idx + 4], normals.array[idx + 5]);
        n2.set(normals.array[idx + 6], normals.array[idx + 7], normals.array[idx + 8]);

        w0.set(uvs.array[uvidx], uvs.array[uvidx + 1]);
        w1.set(uvs.array[uvidx + 2], uvs.array[uvidx + 3]);
        w2.set(uvs.array[uvidx + 4], uvs.array[uvidx + 5]);

        // Based on Eric Lengyel at http://www.terathon.com/code/tangent.html

        e1.subVectors(p1, p0)
        e2.subVectors(p2, p0);

        const x1 = w1.x - w0.x;
        const x2 = w2.x - w0.x;
        const y1 = w1.y - w0.y;
        const y2 = w2.y - w0.y;

        const denom = x1 * y2 - x2 * y1;
        const r = denom ? (1.0 / denom) : 0.0;

        tangent.subVectors(e1.clone().multiplyScalar(y2), e2.clone().multiplyScalar(y1)).multiplyScalar(r);

        // Gram-Schmidt process
        t0.subVectors(tangent, n0.multiplyScalar(n0.dot(tangent))).normalize();
        t1.subVectors(tangent, n1.multiplyScalar(n1.dot(tangent))).normalize();
        t2.subVectors(tangent, n2.multiplyScalar(n2.dot(tangent))).normalize();

        tangentsdata[idx] = t0.x;
        tangentsdata[idx + 1] = t0.y;
        tangentsdata[idx + 2] = t0.z;
        tangentsdata[idx + 3] = t1.x;
        tangentsdata[idx + 4] = t1.y;
        tangentsdata[idx + 5] = t1.z;
        tangentsdata[idx + 6] = t2.x;
        tangentsdata[idx + 7] = t2.y;
        tangentsdata[idx + 8] = t2.z;
    }

    geometry.setAttribute('tangent', new THREE.BufferAttribute( tangentsdata, 3));
}

function toArray(value, dimension) {
  let outValue;
  if (Array.isArray(value) && value.length === dimension)
    outValue = value;
  else {
    outValue = []; 
    for(let i = 0; i < dimension; ++i)
      outValue.push(value);
  }

  return outValue;
}

function toThreeUniform(type, value) {
  let outValue;  
  switch(type) {
    case 'int':
    case 'uint':
    case 'float':
    case 'bool':
      outValue = value;
      break;
    case 'vec2':
    case 'ivec2':
    case 'bvec2':      
      outValue = toArray(value, 2);
      break;
    case 'vec3':
    case 'ivec3':
    case 'bvec3':
      outValue = toArray(value, 3);
      break;
    case 'vec4':
    case 'ivec4':
    case 'bvec4':
    case 'mat2':
      outValue = toArray(value, 4);
      break;
    case 'mat3':
      outValue = toArray(value, 9);
      break;
    case 'mat4':
      outValue = toArray(value, 16);
      break;
    case 'sampler2D':
      break;
    case 'samplerCube':
        break;        
    default:
      // struct
      outValue = toThreeUniform(value);
  }

  return outValue;
}

function toThreeUniforms(uniformJSON) {
  let threeUniforms = {};
  let value;
  for (const [name, description] of Object.entries(uniformJSON)) {
    threeUniforms[name] = new THREE.Uniform(toThreeUniform(description.type, description.value));
  }

  return threeUniforms;
}

function init() {
    let canvas = document.getElementById('webglcanvas');
    let context = canvas.getContext('webgl2');

    camera = new THREE.PerspectiveCamera(50, window.innerWidth / window.innerHeight, 1, 100);

    // Set up scene
    scene = new THREE.Scene();
    scene.background = new THREE.Color(0x4c4c52);
    scene.background.convertSRGBToLinear();

    scene.add(new THREE.AmbientLight( 0x222222));
    const directionalLight = new THREE.DirectionalLight(new THREE.Color(1, 0.894474, 0.567234), 2.52776);
    directionalLight.position.set(-1, 1, 1).normalize();
    scene.add(directionalLight);
    const lightData = {
      type: 1,
      direction: directionalLight.position.negate(), 
      color: new THREE.Vector3(...directionalLight.color.toArray()), 
      intensity: directionalLight.intensity
    };


    renderer = new THREE.WebGLRenderer({canvas, context});
    renderer.setPixelRatio(window.devicePixelRatio);
    renderer.setSize(window.innerWidth, window.innerHeight);

    composer = new EffectComposer( renderer );
    const renderPass = new RenderPass( scene, camera );
    composer.addPass( renderPass );
    const gammaCorrectionPass = new ShaderPass( GammaCorrectionShader );
    composer.addPass( gammaCorrectionPass );

    window.addEventListener('resize', onWindowResize);

    // controls
    controls = new OrbitControls(camera, renderer.domElement);

    // Load model and shaders
    var fileloader = new THREE.FileLoader();
    const objLoader = new OBJLoader();
    const hdrloader = new RGBELoader();

    Promise.all([

        new Promise(resolve => hdrloader.setDataType(THREE.FloatType).load('san_giuseppe_bridge_split.hdr', resolve)),
        new Promise(resolve => hdrloader.setDataType(THREE.FloatType).load('irradiance/san_giuseppe_bridge_split.hdr', resolve)),
        new Promise(resolve => objLoader.load('shaderball.obj', resolve)),
        new Promise(resolve => fileloader.load('shader-frag.glsl', resolve)),
        new Promise(resolve => fileloader.load('shader-vert.glsl', resolve)),
        new Promise(function (resolve) { MaterialX().then((module) => { resolve(module.getMaterialX()); }); }),
        new Promise(resolve => fileloader.load('material.mtlx', resolve))
    ]).then(([radianceTexture, irradianceTexture, obj, fShader, vShader, mx, mtlxMaterial]) => {
        mx = mxIn;
        let doc = mx.createDocument();
        
        let gen = new mx.EsslShaderGenerator();
        let genContext = new mx.GenContext(gen);
        let stdlib = mx.loadStandardLibraries(genContext);
        
        mx.readFromXmlString(doc, mtlxMaterial);
        doc.importLibrary(stdlib);        

        let shader = gen.generate(doc.getNodeGraphs()[0].getNodes()[0].getNamePath(), doc.getNodeGraphs()[0].getNodes()[0], genContext);

        fShader = shader.getSourceCode("pixel");
        vShader = shader.getSourceCode("vertex");
        
        console.log("uniforms: ", gen.getUniformValues(doc.getNodeGraphs()[0].getNodes()[0], genContext));
        let uniforms = {};//toThreeUniforms(JSON.parse(uniformsJSON));
        Object.assign(uniforms, { 
          time: { value: 0.0 },

          base: {value: 1.000000},
          base_color: {value: new THREE.Vector3(0.800000, 0.800000, 0.800000)},
          diffuse_roughness: {value: 0.000000},
          metalness: {value: 0.000000},
          specular: {value: 1.000000},
          specular_color: {value: new THREE.Vector3(1.000000, 1.000000, 1.000000)},
          specular_roughness: {value: 0.200000},
          specular_IOR: {value: 1.500000},
          specular_anisotropy: {value: 0.000000},
          specular_rotation: {value: 0.000000},
          transmission: {value: 0.000000},
          transmission_color: {value: new THREE.Vector3(1.000000, 1.000000, 1.000000)},
          transmission_depth: {value: 0.000000},
          transmission_scatter: {value: new THREE.Vector3(0.000000, 0.000000, 0.000000)},
          transmission_scatter_anisotropy: {value: 0.000000},
          transmission_dispersion: {value: 0.000000},
          transmission_extra_roughness: {value: 0.000000},
          subsurface: {value: 0.000000},
          subsurface_color: {value: new THREE.Vector3(1.000000, 1.000000, 1.000000)},
          subsurface_radius: {value: new THREE.Vector3(1.000000, 1.000000, 1.000000)},
          subsurface_scale: {value: 1.000000},
          subsurface_anisotropy: {value: 0.000000},
          sheen: {value: 0.000000},
          sheen_color: {value: new THREE.Vector3(1.000000, 1.000000, 1.000000)},
          sheen_roughness: {value: 0.300000},
          coat: {value: 0.000000},
          coat_color: {value: new THREE.Vector3(1.000000, 1.000000, 1.000000)},
          coat_roughness: {value: 0.100000},
          coat_anisotropy: {value: 0.000000},
          coat_rotation: {value: 0.000000},
          coat_IOR: {value: 1.500000},
          coat_affect_color: {value: 0.000000},
          coat_affect_roughness: {value: 0.000000},
          thin_film_thickness: {value: 0.000000},
          thin_film_IOR: {value: 1.500000},
          emission: {value: 0.000000},
          emission_color: {value: new THREE.Vector3(1.000000, 1.000000, 1.000000)},
          opacity: {value: new THREE.Vector3(1.000000, 1.000000, 1.000000)},
          thin_walled: {value: false},

          burley_brdf1_weight: {value: 1.0},
          burley_brdf1_color: {value: new THREE.Vector3(0.6, 0.6, 0.6)},
          burley_brdf1_roughness: {value: 0.2},

          u_numActiveLightSources: {value: 1},
          u_lightData: {value: [ lightData ]},

          u_envMatrix: {value: new THREE.Matrix4().makeRotationY(Math.PI)},
          u_envRadiance: {value: radianceTexture},
          u_envRadianceMips: {value: 1},
          u_envRadianceSamples: {value: 16},
          u_envIrradiance: {value: irradianceTexture},

          u_viewPosition: new THREE.Uniform( new THREE.Vector3() ),

          u_worldMatrix: new THREE.Uniform( new THREE.Matrix4() ),
          u_viewProjectionMatrix: new THREE.Uniform( new THREE.Matrix4() ),
          u_worldInverseTransposeMatrix: new THREE.Uniform( new THREE.Matrix4() )
        });

        // RGBELoader sets flipY to true by default
        radianceTexture.flipY = false;
        irradianceTexture.flipY = false;

        const material = new THREE.RawShaderMaterial({
            uniforms: uniforms,
            vertexShader: vShader,
            fragmentShader: fShader,
        });

        obj.traverse((child) => {
            if (child.isMesh) {
              generateTangents(child.geometry);
              child.geometry.attributes.uv_0 = child.geometry.attributes.uv
              child.material = material;
            }
        });
        model = obj;
        scene.add(model);

        const bbox = new THREE.Box3().setFromObject(model);
        const bsphere = new THREE.Sphere();
        bbox.getBoundingSphere(bsphere);

        controls.target = bsphere.center;
        camera.position.y = camera.position.z = bsphere.radius * 2.5;
        controls.update();

        camera.far = bsphere.radius * 10;
        camera.updateProjectionMatrix();

    }).then(() => {
        animate();
    }).catch(err => {
      console.error(mx.getExceptionMessage(err));
    }) 

}

function onWindowResize() {
    camera.aspect = window.innerWidth / window.innerHeight;
    camera.updateProjectionMatrix();
    renderer.setSize(window.innerWidth, window.innerHeight);
}

function animate() {  
    requestAnimationFrame(animate);
    composer.render();

    model.traverse((child) => {
      if (child.isMesh) {
        const uniforms = child.material.uniforms;
        if(uniforms) {
          uniforms.time.value = performance.now() / 1000;
          uniforms.u_viewPosition.value = camera.getWorldPosition(worldViewPos);
          uniforms.u_worldMatrix.value = child.matrixWorld;
          uniforms.u_viewProjectionMatrix.value = viewProjMat.multiplyMatrices(camera.projectionMatrix, camera.matrixWorldInverse);
          uniforms.u_worldInverseTransposeMatrix.value.setFromMatrix3(normalMat.getNormalMatrix(child.matrixWorld));
        }
      }
    });
}
