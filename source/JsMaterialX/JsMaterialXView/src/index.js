import * as THREE from 'three';
import { OBJLoader } from 'three/examples/jsm/loaders/OBJLoader.js';
import { OrbitControls } from 'three/examples/jsm/controls/OrbitControls.js';
import { EffectComposer } from 'three/examples/jsm/postprocessing/EffectComposer.js';
import { RenderPass } from 'three/examples/jsm/postprocessing/RenderPass.js';
import { ShaderPass } from 'three/examples/jsm/postprocessing/ShaderPass.js';
import { GammaCorrectionShader } from 'three/examples/jsm/shaders/GammaCorrectionShader.js';

let camera, scene, model, renderer, composer, controls, uniforms;

let normalMat = new THREE.Matrix3();
let viewProjMat = new THREE.Matrix4();
let worldViewPos = new THREE.Vector3();

init();

function init() {
    let canvas = document.getElementById('webglcanvas');
    let context = canvas.getContext('webgl2');

    camera = new THREE.PerspectiveCamera(50, window.innerWidth / window.innerHeight, 1, 100);
    camera.position.set(0, 5, 5);

    // Set up scene
    scene = new THREE.Scene();
    scene.background = new THREE.Color(0x4c4c52);
    scene.background.convertSRGBToLinear();

    scene.add(new THREE.AmbientLight( 0x222222));
    const directionalLight = new THREE.DirectionalLight(new THREE.Color(1, 0.894474, 0.567234), 2.52776);
    directionalLight.position.set(1, 1, 1).normalize();
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

    Promise.all([
        new Promise(resolve => objLoader.load('shaderball.obj', resolve)),
        new Promise(resolve => fileloader.load('shader-frag.glsl', resolve)),
        new Promise(resolve => fileloader.load('shader-vert.glsl', resolve))
    ]).then(([obj, fShader, vShader]) => {
        const material = new THREE.RawShaderMaterial({
            uniforms: { 
              time: { value: 0.0 },

              burley_brdf1_weight: {value: 1.0},
              burley_brdf1_color: {value: new THREE.Vector3(0.6, 0.6, 0.6)},
              burley_brdf1_roughness: {value: 0.2},

              u_numActiveLightSources: {value: 1},
              u_lightData: {value: [ lightData ]},

              u_viewPosition: new THREE.Uniform( new THREE.Vector3() ),

              u_worldMatrix: new THREE.Uniform( new THREE.Matrix4() ),
              u_viewProjectionMatrix: new THREE.Uniform( new THREE.Matrix4() ),
              u_worldInverseTransposeMatrix: new THREE.Uniform( new THREE.Matrix4() )
            },
            vertexShader: vShader,
            fragmentShader: fShader,
        });

        obj.traverse((child) => {
            if (child.isMesh) child.material = material;
        });
        model = obj;
        scene.add(model);
    }).then(() => {
        animate();
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
