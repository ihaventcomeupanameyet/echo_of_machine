import Image from "next/image";
import * as THREE from 'three'
import ThreeScene from "@/app/component/ThreeScene";
import PlayGround from "@/app/component/PlayGround";


const scene = new THREE.Scene();
export default function Home() {
  return (
      <ThreeScene></ThreeScene>
      //<PlayGround></PlayGround>
  );
}
