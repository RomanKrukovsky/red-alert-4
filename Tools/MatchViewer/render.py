# Copyright (c) Red Alert 4 project.
#
# Turns a match dump from RA4MatchDump into a single self-contained HTML page with
# a scrubbable timeline. No server, no dependencies -- open the file and watch.
#
#   ./build/hb/RA4MatchDump match.json && python3 Tools/MatchViewer/render.py match.json out.html

import json
import sys

PAGE = """<!doctype html>
<html lang="ru"><head><meta charset="utf-8">
<title>RA4 — матч ИИ против ИИ</title>
<style>
  :root { color-scheme: dark; }
  body { margin:0; background:#0d0f12; color:#c9d1d9;
         font:14px/1.5 ui-monospace,SFMono-Regular,Menlo,monospace; }
  .wrap { max-width:1100px; margin:0 auto; padding:18px; }
  h1 { font-size:17px; font-weight:600; margin:0 0 2px; letter-spacing:.02em; }
  .sub { color:#7d8590; font-size:12px; margin-bottom:14px; }
  .stage { display:flex; gap:16px; flex-wrap:wrap; }
  canvas { background:#14181d; border:1px solid #262c33; border-radius:6px;
           width:min(640px,100%); height:auto; image-rendering:auto; }
  .side { flex:1; min-width:240px; }
  .panel { border:1px solid #262c33; border-radius:6px; padding:12px; margin-bottom:12px; }
  .panel h2 { font-size:11px; text-transform:uppercase; letter-spacing:.09em;
              color:#7d8590; margin:0 0 8px; font-weight:600; }
  .row { display:flex; justify-content:space-between; gap:10px; padding:2px 0; }
  .red { color:#ff6b5e; } .blue { color:#5aa2ff; } .dim { color:#7d8590; }
  .bar { height:5px; border-radius:3px; background:#20262d; overflow:hidden; margin-top:3px; }
  .bar i { display:block; height:100%; }
  .controls { display:flex; align-items:center; gap:10px; margin-top:12px; flex-wrap:wrap; }
  button { background:#20262d; color:#c9d1d9; border:1px solid #333b44;
           border-radius:5px; padding:6px 13px; cursor:pointer; font:inherit; }
  button:hover { background:#2b323a; }
  input[type=range] { flex:1; min-width:200px; accent-color:#ff6b5e; }
  .legend { display:flex; gap:14px; flex-wrap:wrap; font-size:12px; color:#7d8590; margin-top:10px; }
  .legend span { display:flex; align-items:center; gap:5px; }
  .sw { width:10px; height:10px; border-radius:2px; display:inline-block; }
  .win { font-size:13px; padding:9px 12px; border-radius:5px;
         background:rgba(255,107,94,.12); border:1px solid rgba(255,107,94,.35); }
</style></head><body><div class="wrap">
<h1>Red Alert 4 — матч ИИ против ИИ</h1>
<div class="sub">Запись настоящей симуляции. Ничего не нарисовано вручную: позиции, здоровье,
кредиты и энергия взяты из детерминированного ядра, оба игрока управляются ИИ.</div>

<div class="stage">
  <canvas id="c" width="640" height="640"></canvas>
  <div class="side">
    <div class="panel"><h2>Время</h2>
      <div class="row"><span class="dim">тик</span><b id="tick">0</b></div>
      <div class="row"><span class="dim">симуляция</span><b id="time">0:00</b></div>
    </div>
    <div class="panel"><h2>СССР — агрессивный ИИ</h2>
      <div class="row"><span class="dim">кредиты</span><b class="red" id="c0">0</b></div>
      <div class="row"><span class="dim">энергия</span><b id="p0">0/0</b></div>
      <div class="bar"><i id="pb0" style="background:#ff6b5e"></i></div>
      <div class="row" style="margin-top:6px"><span class="dim">здания / юниты</span><b id="n0">0 / 0</b></div>
    </div>
    <div class="panel"><h2>Альянс — сбалансированный ИИ</h2>
      <div class="row"><span class="dim">кредиты</span><b class="blue" id="c1">0</b></div>
      <div class="row"><span class="dim">энергия</span><b id="p1">0/0</b></div>
      <div class="bar"><i id="pb1" style="background:#5aa2ff"></i></div>
      <div class="row" style="margin-top:6px"><span class="dim">здания / юниты</span><b id="n1">0 / 0</b></div>
    </div>
    <div id="winner"></div>
  </div>
</div>

<div class="controls">
  <button id="play">Пауза</button>
  <input type="range" id="scrub" min="0" value="0">
  <button id="speed">1x</button>
</div>
<div class="legend">
  <span><i class="sw" style="background:#ff6b5e"></i>СССР</span>
  <span><i class="sw" style="background:#5aa2ff"></i>Альянс</span>
  <span><i class="sw" style="background:#d8c15a"></i>руда</span>
  <span><i class="sw" style="background:#c9d1d9;border-radius:50%"></i>юнит</span>
  <span><i class="sw" style="background:#c9d1d9"></i>здание</span>
</div>
</div>
<script>
const DATA = __DATA__;
const cv = document.getElementById('c'), ctx = cv.getContext('2d');
const WORLD = DATA.mapTiles * DATA.tileUnits;
const S = cv.width / WORLD;
const COL = ['#ff6b5e', '#5aa2ff'];

let i = 0, playing = true, speed = 1;
const scrub = document.getElementById('scrub');
scrub.max = DATA.frames.length - 1;

function draw() {
  const f = DATA.frames[i];
  ctx.fillStyle = '#14181d';
  ctx.fillRect(0, 0, cv.width, cv.height);

  // Tile grid every 8 tiles, so distances on screen stay readable.
  ctx.strokeStyle = '#1c2128'; ctx.lineWidth = 1;
  for (let t = 0; t <= DATA.mapTiles; t += 8) {
    const p = t * DATA.tileUnits * S;
    ctx.beginPath(); ctx.moveTo(p, 0); ctx.lineTo(p, cv.height); ctx.stroke();
    ctx.beginPath(); ctx.moveTo(0, p); ctx.lineTo(cv.width, p); ctx.stroke();
  }

  const n = [[0,0],[0,0]];
  for (const e of f.e) {
    const [own, kind, x, y, hp, hpmax, foot] = e;
    const px = x * S, py = y * S;
    if (kind === 2) {                                   // ore
      ctx.fillStyle = '#d8c15a';
      ctx.fillRect(px - 3, py - 3, 6, 6);
      continue;
    }
    if (own < 2) n[own][kind === 1 ? 0 : 1]++;
    const col = own < 2 ? COL[own] : '#8b949e';
    const frac = hpmax > 0 ? hp / hpmax : 1;
    ctx.globalAlpha = 0.35 + 0.65 * frac;               // damage reads as fading
    ctx.fillStyle = col;
    if (kind === 1) {
      const w = Math.max(6, foot * DATA.tileUnits * S);
      ctx.fillRect(px - w / 2, py - w / 2, w, w);
      ctx.globalAlpha = 1; ctx.strokeStyle = col; ctx.lineWidth = 1;
      ctx.strokeRect(px - w / 2, py - w / 2, w, w);
    } else {
      ctx.beginPath(); ctx.arc(px, py, 3.2, 0, 6.2832); ctx.fill();
    }
    ctx.globalAlpha = 1;
  }

  document.getElementById('tick').textContent = f.t;
  const sec = Math.floor(f.t / 20);
  document.getElementById('time').textContent =
    Math.floor(sec / 60) + ':' + String(sec % 60).padStart(2, '0');
  for (let p = 0; p < 2; p++) {
    document.getElementById('c' + p).textContent = f.credits[p].toLocaleString('ru');
    const [prod, use] = f.power[p];
    document.getElementById('p' + p).textContent = prod + ' / ' + use;
    const ratio = use > 0 ? Math.min(1, prod / use) : 1;
    document.getElementById('pb' + p).style.width = (ratio * 100) + '%';
    document.getElementById('n' + p).textContent = n[p][0] + ' / ' + n[p][1];
  }
  scrub.value = i;

  if (i === DATA.frames.length - 1) {
    document.getElementById('winner').innerHTML =
      '<div class="win">Победил ' + (DATA.winner === 0 ? 'СССР' : 'Альянс') +
      ' на тике ' + DATA.finalTick + '</div>';
  } else {
    document.getElementById('winner').innerHTML = '';
  }
}

function loop() {
  if (playing) { i = (i + speed) % DATA.frames.length; draw(); }
  setTimeout(loop, 60);
}
document.getElementById('play').onclick = e => {
  playing = !playing; e.target.textContent = playing ? 'Пауза' : 'Играть';
};
document.getElementById('speed').onclick = e => {
  speed = speed === 1 ? 2 : speed === 2 ? 4 : 1; e.target.textContent = speed + 'x';
};
scrub.oninput = e => { i = +e.target.value; playing = false;
  document.getElementById('play').textContent = 'Играть'; draw(); };
draw(); loop();
</script></body></html>
"""


def main():
    src = sys.argv[1] if len(sys.argv) > 1 else "match.json"
    dst = sys.argv[2] if len(sys.argv) > 2 else "match.html"
    with open(src) as f:
        data = json.load(f)
    html = PAGE.replace("__DATA__", json.dumps(data, separators=(",", ":")))
    with open(dst, "w") as f:
        f.write(html)
    print("frames: {}, winner: player {}, -> {}".format(
        len(data["frames"]), data["winner"], dst))


main()
