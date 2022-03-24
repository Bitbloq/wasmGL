import * as THREE from "three";
import ThreeBSP from "./threeCSG";
import now from "performance-now";

console.log("hello world");

const sphere1 = new THREE.SphereGeometry(0.6, 10, 10);
const sphere2 = new THREE.SphereGeometry(0.6, 10, 10);
sphere1.translate(0.2, 0, 0);
sphere2.translate(-0.2, 0, 0);

console.log("vertices: " + sphere1.vertices.length);

const init = now();
const sphere1Tree = new ThreeBSP(sphere1);
const end1 = now();
console.log("sphere1Tree: " + (end1 - init));
const sphere2Tree = new ThreeBSP(sphere2);
sphere1Tree.intersect(sphere2Tree).toMesh(undefined);
const end = now();

console.log("Time to create CSG: " + (end - init).toFixed(3) + "ms");
