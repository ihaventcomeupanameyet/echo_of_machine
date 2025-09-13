import Image from "next/image";
import * as THREE from 'three'
import ThreeScene from "@/app/component/ThreeScene";


const scene = new THREE.Scene();
export default function Home() {
  return (
      <ThreeScene></ThreeScene>
  );
}
