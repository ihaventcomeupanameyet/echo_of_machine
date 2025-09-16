'use client'
import { useEffect, useRef } from "react";
import * as THREE from "three";
import Geometries from "three/src/renderers/common/Geometries";
import {Float32BufferAttribute} from "three";



export default function ThreeScene() {
    const canvasRef = useRef<HTMLCanvasElement|undefined>(undefined);

    useEffect(() => {
        if (!canvasRef.current) return;

        // init scene/camera
        const scene = new THREE.Scene();
        const camera = new THREE.PerspectiveCamera(
            75,
            window.innerWidth / window.innerHeight,
            0.1,
            1000
        );

        const radius = 5;
        camera.position.z = radius;

        // init renderer with your <canvas>
        const renderer = new THREE.WebGLRenderer({ canvas: canvasRef.current });
        renderer.setSize(window.innerWidth , window.innerHeight );
        renderer.setPixelRatio(window.devicePixelRatio);

        // add cube
        const geometry = new THREE.SphereGeometry(2);
        const material1 = new THREE.MeshBasicMaterial({ color: 0x00ff00 });
        const cube = new THREE.Mesh(geometry, material1);
        //scene.add(cube);

        const position = geometry.attributes.position;
        let vertices = [];

        for (let i = 0; i < position.count; i++) {
            const vertex = new THREE.Vector3();
            vertex.fromBufferAttribute(position, i);
            vertices.push(vertex);
        }


        //const group = new THREE.Group();





        //part.rotation.y += 0.01;



        const part_count = 1000
        let temp = new Float32Array(part_count*3);

        for (let i = 0; i < temp.length*3;i++){
            temp[i] = (Math.random()-0.5) * 10;
            temp[i+1] = (Math.random()-0.5) * 10;
            temp[i+2] = (Math.random()-0.5) * 2.5;
        }

        const particleGeometry = new THREE.BufferGeometry;
        particleGeometry.setAttribute("position", new THREE.BufferAttribute(temp,3));

        const material = new THREE.ShaderMaterial({
            uniforms: { color: { value: new THREE.Color(0xffffff) } },
            vertexShader: `
    void main() {
      vec4 mvPosition = modelViewMatrix * vec4(position, 1.0);
      gl_PointSize = 10.0 / -mvPosition.z;
      gl_Position = projectionMatrix * modelViewMatrix * vec4(position, 1.0);
    }
  `,
            fragmentShader: `
    uniform vec3 color;
    void main() {
      vec2 uv = gl_PointCoord - vec2(0.5);
      if (dot(uv, uv) > 0.25) discard;  // outside circle radius
      gl_FragColor = vec4(color, 1.0);
    }
  `,
            transparent: true,
            sizeAttenuation: true,
        });




        //const material = new THREE.PointsMaterial({
            //size: 0.05
        //})

        const parts = new THREE.Points(particleGeometry
            , material)

        scene.add(parts);

        let march_points = null;
        let roam_points = null;
        let marching = false;
        function march(){
            if (marching) return;

            const position = geometry.attributes.position;

            const size = position.count;

            const size_part = particleGeometry.attributes.position.count;

            if ( size > size_part) return;

            // marching part
            const vertices = [];
            for (let i = 0; i < size; i++){
                const vertex = new THREE.Vector3();
                vertex.fromBufferAttribute(particleGeometry.attributes.position
                    , i);
                vertices.push(vertex.x, vertex.y, vertex.z);
            }

            const particleGeo = new THREE.BufferGeometry();
            particleGeo.setAttribute('position', new THREE.Float32BufferAttribute(vertices, 3));

            march_points = new THREE.Points(particleGeo, material);


            // roaming part
            const vertices_roam = [];
            for (let i = size; i < size_part; i++){
                const vertex = new THREE.Vector3();
                vertex.fromBufferAttribute(particleGeometry.attributes.position
                    , i);
                vertices_roam.push(vertex.x, vertex.y, vertex.z);
            }

            const particleGeo_roam = new THREE.BufferGeometry();
            particleGeo_roam.setAttribute('position', new THREE.Float32BufferAttribute(vertices_roam, 3));

            roam_points = new THREE.Points(particleGeo_roam,material);

            scene.add(march_points);
            scene.add(roam_points);
            scene.remove(parts);
            marching = true;
        }


        function morphing(){
            const position_geo = geometry.attributes.position;
            const position = march_points.geometry.attributes.position;

            const tempSource = new THREE.Vector3();
            const tempTarget = new THREE.Vector3();
            for (let i = 0; i < position_geo.count; i++){
                tempSource.fromBufferAttribute(position,i);
                tempTarget.fromBufferAttribute(position_geo,i);

                tempSource.lerp(tempTarget,0.05);

                position.setXYZ(i, tempSource.x, tempSource.y, tempSource.z);
            }
            position.needsUpdate = true;
        }
        function create(){
            const position = geometry.attributes.position;


            const vertices = [];

            for (let i = 0; i < position.count; i++) {
                const vertex = new THREE.Vector3();
                vertex.fromBufferAttribute(position, i);

                vertices.push(vertex.x, vertex.y, vertex.z);
            }

            const particleGeo = new THREE.BufferGeometry();
            particleGeo.setAttribute('position', new THREE.Float32BufferAttribute(vertices, 3));

            const particleMat = new THREE.PointsMaterial({
                color: 0xffffff,
                size: 0.05,
            });

            return new THREE.Points(particleGeo, particleMat);
        }

        const part = create();
        part.rotation.x += Math.PI/4;
        part.rotation.y += Math.PI/4;
        //scene.add(part);

        window.addEventListener('mousemove',(evt)=>{
            const x = 1 - ((2*evt.x)/window.innerWidth);
            const y = 1 - ((2*evt.y)/window.innerHeight);

            const angleX = x * Math.PI * 0.1; // left/right (±45°)
            const angleY = y * Math.PI * 0.1; // up/down (±45°)

            const theta = x * Math.PI * 0.1;
            const phi = y * Math.PI * 0.1;

            //camera.position.x = radius * Math.sin(angleX);
            //camera.position.y = radius * Math.sin(angleY);
            //camera.position.z = radius * Math.cos(angleX);

            camera.position.x = radius * Math.cos(phi) * Math.sin(theta);
            camera.position.y = radius * Math.sin(phi);
            camera.position.z = radius * Math.cos(phi) * Math.cos(theta);

            camera.lookAt(0, 0, 0);
        })


        window.addEventListener("click",march);
        // animate
        const animate = () => {
            requestAnimationFrame(animate);



            //parts.rotation.y += 0.001;

            if (marching) {
                morphing();
                roam_points.rotation.y += 0.001;
            } else {
                parts.rotation.y += 0.001;
            }
            renderer.render(scene, camera);
        };
        animate();

        // cleanup
        return () => {
            renderer.dispose();
            geometry.dispose();
            material.dispose();
        };
    }, []);

    return (<canvas ref={canvasRef}></canvas>);  // 👈 you return the <canvas>, not the renderer
}
