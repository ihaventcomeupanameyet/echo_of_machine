'use client'
import { useEffect, useRef } from "react";
import * as THREE from "three";
import {red} from "next/dist/lib/picocolors";

export default function PlayGround(){
    const CANVAS = useRef<HTMLCanvasElement|null>(null);



    useEffect(() => {
        const scene = new THREE.Scene();
        const camera = new THREE.PerspectiveCamera(75,
            window.innerWidth / window.innerHeight,
            0.1,
            1000 );

        camera.position.z = 5
        const renderer = new THREE.WebGLRenderer({canvas:CANVAS.current});
        renderer.setSize(window.innerWidth,window.innerHeight);
        renderer.setPixelRatio(window.devicePixelRatio);


        const cubeGeo = new THREE.BoxGeometry(1,1,1);
        const cubeMaterial = new THREE.MeshBasicMaterial({color:"red"});

        const cubeMesh = new THREE.Mesh(
            cubeGeo,
            cubeMaterial
        )

        scene.add(cubeMesh);

        renderer.render(scene,camera);


    }, []);



    return (<canvas ref={CANVAS}></canvas>);

}
