/* ============================================================================
   7T WEB — uniforms.js
   The single writer for FrameSignal (336 B) and DesignConfig (560 B).
   Byte layouts are the desktop structs verbatim (PORT_MAP §7: FrameSignal is
   frozen as-is; the stats[64] region carries the web fields, host-side only
   through Phases 1-2). DesignConfig boot values are the desktop outdoor
   defaults from the terrain build sheet (§8 of the sheet; mood 0).
   ========================================================================== */

function normalize3(x, y, z) {
  const l = Math.hypot(x, y, z);
  return [x / l, y / l, z / l];
}

export function makeConfig(device, buffer) {
  const ab = new ArrayBuffer(560);
  const f = new Float32Array(ab), u = new Uint32Array(ab);
  let dirty = true;

  /* boot map — offsets are bytes/4; values = desktop outdoor defaults */
  f[4] = 1.0;                       // wave_time_scale @16
  f[5] = 15.0;                      // pawn_speed @20
  f[6] = 0.005;                     // camera_sensitivity @24
  f[8] = 64.0;                      // active_cell_size @32
  u[10] = 0x7;                      // wave_enable_mask @40
  u[15] = 42;                       // world_seed @60
  [f[16], f[17], f[18]] = normalize3(0.69, -0.71, -0.14);  // sun_direction @64
  f[19] = 1.0;                      // aura_enabled @76
  f[20] = 1.0;                      // pawn_amp_scale @80
  f[23] = 0.003;                    // fog_density @92
  f[24] = 0.85; f[25] = 0.78; f[26] = 0.72;  // fog_color @96
  u[31] = 0;                        // pier_count @124 (desktop boots 3 test-rig piers; parity question, PORT_MAP)
  f[57] = 1.0;                      // mode_gol_tick_scale @228
  f[58] = 1.0;                      // mode_gol_height_scale @232
  u[61] = 0;                        // possessed_slot @244 (agents cut; slot 0 zeroed)
  for (let i = 40; i < 46; i++) f[i] = -1.0;  // band_blend_0..5 @160 (inactive sentinel)
  f[100] = 325.0;                   // veil_ring @400
  f[101] = 40.0;                    // veil_icing @404
  f[102] = 1.0;                     // veil_strength @408 (outdoor)
  f[103] = 175.0;                   // lod0_radius @412
  // palette_center/light/weight @416/@480/@544 (terrain_looks.hpp defaults)
  f.set([0.85, 0.70, 0.50, 0,  0.88, 0.58, 0.48, 0,
         0.45, 0.58, 0.38, 0,  0.82, 0.55, 0.42, 0], 104);
  f.set([0.92, 0.82, 0.65, 0,  0.95, 0.72, 0.62, 0,
         0.62, 0.72, 0.52, 0,  0.92, 0.72, 0.58, 0], 120);
  f.set([0.42, 0.28, 0.04, 0.26], 136);

  return {
    setPlacementPatchCount(n) { u[36] = n; dirty = true; },     // @144
    setLodPoint(x, z) { f[96] = x; f[97] = z; dirty = true; },  // @384
    flush() {
      if (!dirty) return;
      device.queue.writeBuffer(buffer, 0, ab);
      dirty = false;
    },
  };
}

export function makeSignal(device, buffer, palette) {
  const ab = new ArrayBuffer(336);
  const f = new Float32Array(ab);

  /* stats region sub-layout (PORT_MAP §7) — host convention, no shader reader
     yet. Wall clocks always run; beat fields hold 0 until the start gesture. */
  f[4] = 100.0;                              // bpm (stats[0].x) — desktop calibration anchor
  f.set([...palette.orb, 1], 12);            // colA @48
  f.set([...palette.sky, 1], 16);            // colB @64
  f.set([...palette.gold, 1], 20);           // colC @80
  f.set([...palette.ink, 1], 24);            // colBg @96

  return {
    frame(tSeconds, dt, aspect) {
      f[0] = tSeconds;   // t_seconds — terrain VS pulse phase (pulse_count=0 → inert)
      f[2] = dt;
      f[3] = aspect;
      device.queue.writeBuffer(buffer, 0, ab);
    },
  };
}

/* Interim fixed camera until the camera/vp family lifts (next in lift order).
   Desktop boot pose: aim (0,0,0), azimuth 0, elevation 0.4, distance 30 →
   eye ≈ (0, 11.68, 27.63). fov_y = 1.0 rad, near 0.1, far 600, [0,1] depth,
   column-major column-vector (clip = m * world). light_vp = identity
   (shadow maps are neutral 1.0 until the shadow pass lifts). */
export function writeInterimCamera(device, R) {
  const az = 0.0, el = 0.4, dist = 30.0;
  const eye = [Math.sin(az) * Math.cos(el) * dist, Math.sin(el) * dist, Math.cos(az) * Math.cos(el) * dist];
  const target = [0, 0, 0], up = [0, 1, 0];

  const sub = (a, b) => [a[0] - b[0], a[1] - b[1], a[2] - b[2]];
  const norm = (v) => { const l = Math.hypot(...v); return [v[0] / l, v[1] / l, v[2] / l]; };
  const cross = (a, b) => [a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]];
  const dot = (a, b) => a[0] * b[0] + a[1] * b[1] + a[2] * b[2];

  const fwd = norm(sub(target, eye));
  const right = norm(cross(fwd, up));
  const u2 = cross(right, fwd);
  // view, column-major
  const view = [
    right[0], u2[0], -fwd[0], 0,
    right[1], u2[1], -fwd[1], 0,
    right[2], u2[2], -fwd[2], 0,
    -dot(right, eye), -dot(u2, eye), dot(fwd, eye), 1,
  ];
  const fovY = 1.0, near = 0.1, far = 600.0;
  const aspect = R.aspect ?? (16 / 9);
  const fy = 1 / Math.tan(fovY / 2);
  const proj = [
    fy / aspect, 0, 0, 0,
    0, fy, 0, 0,
    0, 0, -far / (far - near), -1,
    0, 0, -near * far / (far - near), 0,
  ];
  // m = proj * view (column-major)
  const m = new Float32Array(32);   // [0..15] = m, [16..31] = light_vp
  for (let c = 0; c < 4; c++) for (let r = 0; r < 4; r++) {
    let s = 0;
    for (let k = 0; k < 4; k++) s += proj[k * 4 + r] * view[c * 4 + k];
    m[c * 4 + r] = s;
  }
  m[16] = m[21] = m[26] = m[31] = 1;   // light_vp = identity
  device.queue.writeBuffer(R.vp, 0, m);

  // CameraState: terrain FS reads only .pos (fog distance)
  const cam = new Float32Array(12);
  cam.set(eye, 0); cam[3] = az; cam[4] = el; cam[5] = dist;
  device.queue.writeBuffer(R.camera, 0, cam);
  return eye;
}

/* Directional light — mood 0 (MOOD_OPEN_DEFAULT). Point/spot counts stay 0. */
export function writeLights(device, R) {
  const d = new Float32Array(12);
  d.set(normalize3(0.69, -0.71, -0.14), 0);
  d.set([1.0, 0.95, 0.90], 4);
  d[7] = 0.80;   // intensity
  d[8] = 0.25;   // ambient
  device.queue.writeBuffer(R.dirLight, 0, d);
}
