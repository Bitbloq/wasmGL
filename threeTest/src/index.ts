import * as THREE from "three";
import ThreeBSP from "./threeCSG";
import now from "performance-now";

console.log("hello world");

const init1 = now();
const sphere1 = new THREE.SphereGeometry(0.6, 10, 10);
sphere1.translate(0.2, 0, 0);
const end1 = now();

console.log("SphereGeometry: " + (end1 - init1).toFixed(3) + "ms");

const sphere2 = new THREE.SphereGeometry(0.6, 10, 10);
sphere2.translate(-0.2, 0, 0);

const init2 = now();
const sphere1Tree = new ThreeBSP(sphere1);
const end2 = now();

console.log("sphere1Tree: " + (end2 - init2).toFixed(3) + "ms");

const sphere2Tree = new ThreeBSP(sphere2);

const init3 = now();
sphere1Tree.intersect(sphere2Tree).toMesh(undefined);
const end3 = now();
console.log("intersect: " + (end3 - init3).toFixed(3) + "ms");

console.log("Total: " + (end3 - init1).toFixed(3) + "ms");
