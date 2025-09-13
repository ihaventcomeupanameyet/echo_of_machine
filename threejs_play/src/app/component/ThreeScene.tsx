'use client'
import { useEffect, useRef } from "react";
import * as THREE from "three";

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
        camera.position.z = 5;

        // init renderer with your <canvas>
        const renderer = new THREE.WebGLRenderer({ canvas: canvasRef.current });
        renderer.setSize(window.innerWidth , window.innerHeight );
        renderer.setPixelRatio(window.devicePixelRatio);

        // add cube
        const geometry = new THREE.BoxGeometry();
        const material = new THREE.MeshBasicMaterial({ color: 0x00ff00 });
        const cube = new THREE.Mesh(geometry, material);
        scene.add(cube);

        // animate
        const animate = () => {
            requestAnimationFrame(animate);
            cube.rotation.x += 0.01;
            cube.rotation.y += 0.01;
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
