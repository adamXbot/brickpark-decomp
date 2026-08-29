// LEGOLAND Data Lab — UI controller. Client-side only.
(function () {
  'use strict';

  var archives = {};     // name -> { members, bytes, dv }
  var sprites = [];      // { name, arch, member } across all loaded archives
  var levels = [];       // { name, arch, member }
  var decodeCache = {};  // "arch/off" -> canvas
  var wavs = [];         // { name, buf } speech / sfx
  var audioCtx = null;
  var curLevel = null;

  var $ = function (id) { return document.getElementById(id); };
  function el(tag, cls, txt) { var e = document.createElement(tag); if (cls) e.className = cls; if (txt != null) e.textContent = txt; return e; }

  // ---- decoder bridge (comp.js exposes window.LLComp.decode) -----------------
  function haveDecoder() { return window.LLComp && typeof window.LLComp.decode === 'function'; }

  function decodeToCanvas(arch, member) {
    var key = arch + '/' + member.offset;
    if (decodeCache[key]) return decodeCache[key];
    if (!haveDecoder()) return null;
    var a = archives[arch];
    var img;
    try {
      img = window.LLComp.decode(a.bytes.buffer, member.offset);
    } catch (e) {
      return null;
    }
    if (!img || !img.width || !img.rgba) return null;
    var cv = el('canvas');
    cv.width = img.width; cv.height = img.height;
    var ctx = cv.getContext('2d');
    var id = ctx.createImageData(img.width, img.height);
    id.data.set(img.rgba);
    ctx.putImageData(id, 0, 0);
    decodeCache[key] = cv;
    return cv;
  }

  // ---- loading ---------------------------------------------------------------
  function status(msg) { var s = $('status'); if (s) s.textContent = msg; }

  function addArchive(name, buf) {
    try {
      var parsed = window.LLRes.parse(buf);
      archives[name] = parsed;
      var bytes = parsed.bytes;
      for (var i = 0; i < parsed.members.length; i++) {
        var mem = parsed.members[i];
        var lower = mem.name.toLowerCase();
        var o = mem.offset;
        var isComp = bytes[o] === 0x43 && bytes[o + 1] === 0x4f && bytes[o + 2] === 0x4d && bytes[o + 3] === 0x50; // "COMP"
        if (lower.endsWith('.map')) {
          levels.push({ name: mem.name.replace(/\.map$/i, ''), arch: name, member: mem });
        } else if (isComp) {
          sprites.push({ name: mem.name, arch: name, member: mem });
        }
        // else: .3d models / other engine blobs — not renderable here (yet)
      }
      levels.sort(function (a, b) { return a.name.localeCompare(b.name); });
      refreshChips();
      refreshArchSelect();
      renderSprites();
      renderLevelsList();
      updateEmpty();
      $('n-sprites').textContent = sprites.length;
      $('n-levels').textContent = levels.length;
    } catch (e) {
      status('Could not read ' + name + ': ' + e.message);
    }
  }

  function handleFiles(fileList) {
    var files = Array.prototype.slice.call(fileList);
    if (!files.length) return;
    var pending = files.length;
    files.forEach(function (f) {
      var lower = f.name.toLowerCase();
      var rd = new FileReader();
      rd.onload = function () {
        if (lower.endsWith('.res')) {
          addArchive(f.name, rd.result);
        } else if (lower.endsWith('.wav')) {
          wavs.push({ name: f.name, buf: rd.result });
          $('n-audio').textContent = wavs.length;
          renderAudio();
          updateEmpty();
        } else if (lower.endsWith('.iso')) {
          status('ISO support: extract the loose .res files off the disc and drop those (client-side ISO reader is on the roadmap).');
        } else if (!lower.endsWith('.z') && !lower.match(/\.(sty|sgt|bnd|bnv|ltx|lms|lfm|tsf|tsm|ilf|odf)$/)) {
          status('Unrecognised file: ' + f.name);
        }
        if (--pending === 0 && !sprites.length && !wavs.length) status('No .res archives or .wav files found in the drop.');
      };
      rd.readAsArrayBuffer(f);
    });
  }

  function refreshChips() {
    var c = $('chips'); c.innerHTML = '';
    Object.keys(archives).forEach(function (name) {
      var chip = el('span', 'chip');
      chip.innerHTML = name + ' <b>' + archives[name].members.length + '</b>';
      c.appendChild(chip);
    });
  }

  function refreshArchSelect() {
    var sel = $('sprArch');
    var cur = sel.value;
    sel.innerHTML = '<option value="">all archives</option>';
    Object.keys(archives).forEach(function (name) {
      var o = el('option'); o.value = name; o.textContent = name; sel.appendChild(o);
    });
    sel.value = cur;
  }

  // ---- sprite grid -----------------------------------------------------------
  var gridBudget = 0;
  function renderSprites() {
    var grid = $('sprGrid');
    var q = ($('sprSearch').value || '').toLowerCase();
    var archFilter = $('sprArch').value;
    grid.innerHTML = '';
    var shown = 0, total = 0;
    var list = sprites.filter(function (s) {
      if (archFilter && s.arch !== archFilter) return false;
      if (q && s.name.toLowerCase().indexOf(q) < 0) return false;
      return true;
    });
    $('sprCount').textContent = list.length + ' sprites' +
      (haveDecoder() ? '' : ' — waiting for comp.js decoder');
    // lazy: render cells, decode on IntersectionObserver
    var io = new IntersectionObserver(function (entries) {
      entries.forEach(function (ent) {
        if (!ent.isIntersecting) return;
        var cell = ent.target;
        io.unobserve(cell);
        var s = list[+cell.dataset.i];
        var cv = decodeToCanvas(s.arch, s.member);
        var thumb = cell.querySelector('.thumb');
        if (cv) {
          thumb.appendChild(cv);
          var dim = cell.querySelector('.dim');
          if (dim) dim.textContent = ' ' + cv.width + '×' + cv.height;
        }
      });
    }, { root: document.querySelector('[data-view=sprites]'), rootMargin: '200px' });

    list.forEach(function (s, i) {
      var cell = el('div', 'cell'); cell.dataset.i = i;
      var thumb = el('div', 'thumb'); cell.appendChild(thumb);
      var cap = el('div', 'cap'); cap.textContent = s.name;
      var dim = el('span', 'dim'); cap.appendChild(dim);
      cell.appendChild(cap);
      cell.onclick = function () { openModal(s); };
      grid.appendChild(cell);
      io.observe(cell);
    });
  }

  function openModal(s) {
    var cv = decodeToCanvas(s.arch, s.member);
    var m = $('modal'), mc = $('modalCanvas');
    if (cv) {
      mc.width = cv.width; mc.height = cv.height;
      mc.getContext('2d').drawImage(cv, 0, 0);
      $('modalMeta').textContent = s.name + '  ·  ' + cv.width + '×' + cv.height + '  ·  ' + s.arch + '  ·  ' + s.member.size + ' bytes';
    } else {
      $('modalMeta').textContent = s.name + ' — decoder not available';
    }
    m.classList.add('show');
  }

  // ---- levels tab ------------------------------------------------------------
  function renderLevelsList() {
    if (!levels.length) return;
    $('levelsEmpty').style.display = 'none';
    $('levelsBody').style.display = 'block';
    var list = $('levelList'); list.innerHTML = '';
    levels.forEach(function (lv, i) {
      var row = el('div');
      row.style.cssText = 'font-family:var(--mono);font-size:12px;padding:6px 8px;border-radius:5px;cursor:pointer;color:var(--ink2)';
      row.textContent = lv.name;
      row.onmouseenter = function () { if (curLevel !== lv) row.style.background = 'var(--panel)'; };
      row.onmouseleave = function () { if (curLevel !== lv) row.style.background = ''; };
      row.onclick = function () {
        curLevel = lv;
        Array.prototype.forEach.call(list.children, function (c) { c.style.background = ''; c.style.color = 'var(--ink2)'; });
        row.style.background = 'var(--panel)'; row.style.color = 'var(--stud)';
        drawLevel(lv);
      };
      list.appendChild(row);
    });
    if (!curLevel) list.children[0].click();
  }

  // A stable, pleasant terrain-ish palette keyed by tile index.
  function tileColor(v) {
    if (v < 0) return null;
    var pal = ['#4e8f3a', '#5aa243', '#6cb356', '#c9b170', '#d8c98a', '#3f7fb5',
      '#5a97c9', '#8a8f96', '#a86b3c', '#7a5230', '#b7c25a', '#e0d59a'];
    return pal[v % pal.length];
  }

  function drawLevel(lv) {
    var m = window.LLLevel.parse(archives[lv.arch].bytes.subarray(lv.member.offset, lv.member.offset + lv.member.size));
    var head = $('levelHead');
    var classes = m.object_classes.map(function (c) { return c; }).join(' · ') || '(none)';
    head.innerHTML = '<span style="font-family:var(--head);font-weight:700;font-size:18px">' + m.name + '</span>' +
      ' <span class="muted">' + m.width + '×' + m.height + ' · ' + m.terrain + ' · ' +
      m.objects.length + ' objects · ' + m.terrain_cells + ' terrain features</span>' +
      '<div class="muted" style="margin-top:3px;font-size:11px">classes: ' + classes + '</div>';

    var cv = $('levelCanvas');
    var box = cv.parentElement.getBoundingClientRect();
    var W = Math.max(400, Math.floor(box.width)), H = Math.max(300, Math.floor(box.height));
    var dpr = window.devicePixelRatio || 1;
    cv.width = W * dpr; cv.height = H * dpr;
    var ctx = cv.getContext('2d'); ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
    ctx.clearRect(0, 0, W, H);

    var w = m.width, h = m.height;
    // fit the iso diamond (span = (w+h) wide, (w+h) tall/2) into the canvas
    var tw = Math.min((W - 40) / (w + h), (H - 40) / ((w + h) / 2));
    tw = Math.max(2, tw);
    var th = tw / 2;
    var ox = W / 2, oy = 24;

    function iso(x, y) { return [ox + (x - y) * tw / 2, oy + (x + y) * th / 2]; }

    // draw cells back-to-front (already row-major, which is a valid iso order)
    for (var y = 0; y < h; y++) {
      for (var x = 0; x < w; x++) {
        var i = y * w + x;
        var tv = m.tileGfx[i];
        var terr = m.terrainGrid[i];
        var col = tileColor(tv >= 0 ? tv : terr);
        if (col === null) col = (terr >= 0) ? tileColor(terr) : '#39562f'; // base grass
        var p = iso(x, y);
        ctx.fillStyle = col;
        ctx.beginPath();
        ctx.moveTo(p[0], p[1]);
        ctx.lineTo(p[0] + tw / 2, p[1] + th / 2);
        ctx.lineTo(p[0], p[1] + th);
        ctx.lineTo(p[0] - tw / 2, p[1] + th / 2);
        ctx.closePath();
        ctx.fill();
      }
    }
    // object markers
    var objPal = ['#ff3b30', '#ffcf3f', '#2ea3f2', '#3ec46d', '#ff7ac2', '#b07cff', '#ff9f0a'];
    m.objects.forEach(function (o) {
      var p = iso(o.x, o.y);
      ctx.fillStyle = objPal[o.cls % objPal.length];
      ctx.strokeStyle = '#0a0d13'; ctx.lineWidth = 1;
      ctx.beginPath(); ctx.arc(p[0], p[1] + th / 2, Math.max(3, tw * 0.6), 0, 6.29); ctx.fill(); ctx.stroke();
    });
  }

  // ---- audio tab -------------------------------------------------------------
  function renderAudio() {
    var body = $('audioBody');
    if (!wavs.length) return;
    if (!window.LLAdpcm) { body.textContent = 'adpcm.js not loaded.'; return; }
    body.innerHTML = '';
    var q = el('input'); q.type = 'search'; q.placeholder = 'filter…';
    q.style.marginBottom = '12px';
    var list = el('div');
    function draw() {
      list.innerHTML = '';
      var f = (q.value || '').toLowerCase();
      wavs.forEach(function (w) {
        if (f && w.name.toLowerCase().indexOf(f) < 0) return;
        var row = el('div');
        row.style.cssText = 'display:flex;align-items:center;gap:10px;padding:5px 0;border-bottom:1px solid var(--line);font-family:var(--mono);font-size:12px';
        var btn = el('button', 'btn'); btn.textContent = '▶'; btn.style.padding = '3px 10px';
        btn.onclick = function () { playWav(w, btn); };
        var nm = el('span'); nm.textContent = w.name; nm.style.flex = '1';
        row.appendChild(btn); row.appendChild(nm); list.appendChild(row);
      });
    }
    q.oninput = draw;
    body.appendChild(q); body.appendChild(list); draw();
  }

  function playWav(w, btn) {
    try {
      audioCtx = audioCtx || new (window.AudioContext || window.webkitAudioContext)();
      var dec = window.LLAdpcm.decode(w.buf);
      var abuf = audioCtx.createBuffer(dec.channels, dec.pcm[0].length, dec.sampleRate);
      for (var c = 0; c < dec.channels; c++) abuf.copyToChannel(dec.pcm[c], c);
      var src = audioCtx.createBufferSource();
      src.buffer = abuf; src.connect(audioCtx.destination); src.start();
      if (btn) { btn.textContent = '♪'; src.onended = function () { btn.textContent = '▶'; }; }
    } catch (e) {
      if (btn) btn.textContent = '✕';
      status('Could not play ' + w.name + ': ' + e.message);
    }
  }

  // ---- engine tab ------------------------------------------------------------
  var SUBSYS = [
    ['Rendering', 43, 'InstallDirectDraw · RenderSprite · AddBlokeToRenderList'],
    ['World / build', 24, 'AddObjectToMap · AddPathTile · AddRollerCoasterPath'],
    ['Visitor AI', 8, 'Add3DBlokeToList · Get_Path_Directions · SetPersonDirection'],
    ['Asset loading', 37, 'LoadSprite · LoadSourceImage · __BMPLoader'],
    ['Dynamic music', 6, 'LoadMusicStyle · LoadMusicSegment · BlendMusic'],
    ['Save / load', 14, 'SaveJailCells · AddOvSav']
  ];
  function renderEngine() {
    var s = $('subsys');
    if (s.childNodes.length) return;
    SUBSYS.forEach(function (row) {
      var d = el('div', 's');
      d.innerHTML = row[0] + ' <b>' + row[1] + '</b><br><span class="muted">' + row[2] + '</span>';
      s.appendChild(d);
    });
    // Best-effort: pull the full export list if served alongside.
    fetch('../symbols/legoland.exports.txt').then(function (r) {
      return r.ok ? r.text() : null;
    }).then(function (txt) {
      if (!txt) return;
      var g = $('exgrid');
      txt.split('\n').forEach(function (line) {
        if (!line || line[0] === '#') return;
        var p = line.split('\t');
        var fn = el('div', 'fn');
        fn.innerHTML = p[0] + ' <span class="rva">' + (p[2] || '') + '</span>';
        g.appendChild(fn);
      });
    }).catch(function () {});
  }

  // ---- tabs & events ---------------------------------------------------------
  var activeTab = 'sprites';
  function updateEmpty() {
    // The drop overlay only covers data-dependent tabs, and only until data loads.
    var dataTab = (activeTab === 'sprites' || activeTab === 'levels' || activeTab === 'audio');
    var loaded = Object.keys(archives).length > 0;
    $('empty').classList.toggle('hide', loaded || !dataTab);
  }

  function initTabs() {
    document.querySelectorAll('.tab').forEach(function (t) {
      t.onclick = function () {
        document.querySelectorAll('.tab').forEach(function (x) { x.classList.remove('active'); });
        document.querySelectorAll('.view').forEach(function (x) { x.classList.remove('active'); });
        t.classList.add('active');
        activeTab = t.dataset.tab;
        var view = document.querySelector('[data-view=' + activeTab + ']');
        if (view) view.classList.add('active');
        if (activeTab === 'engine') renderEngine();
        updateEmpty();
      };
    });
  }

  function initDnd() {
    var drop = $('drop');
    ['dragenter', 'dragover'].forEach(function (ev) {
      document.addEventListener(ev, function (e) { e.preventDefault(); drop.classList.add('hot'); });
    });
    ['dragleave', 'drop'].forEach(function (ev) {
      document.addEventListener(ev, function (e) { e.preventDefault(); if (ev === 'dragleave' && e.relatedTarget) return; drop.classList.remove('hot'); });
    });
    document.addEventListener('drop', function (e) {
      e.preventDefault();
      if (e.dataTransfer && e.dataTransfer.files) handleFiles(e.dataTransfer.files);
    });
  }

  function init() {
    initTabs();
    initDnd();
    $('loadBtn').onclick = $('loadBtn2').onclick = function () { $('file').click(); };
    $('folderBtn').onclick = $('folderBtn2').onclick = function () { $('folder').click(); };
    $('file').onchange = function () { handleFiles(this.files); };
    $('folder').onchange = function () { handleFiles(this.files); };
    $('sprSearch').oninput = renderSprites;
    $('sprArch').onchange = renderSprites;
    $('modal').onclick = function () { this.classList.remove('show'); };
    renderEngine();
  }

  if (document.readyState === 'loading') document.addEventListener('DOMContentLoaded', init);
  else init();

  // Programmatic entry point (also used to load from a future client-side ISO
  // reader, and for automated testing).
  window.LLDataLab = { addArchive: addArchive, addWav: function (name, buf) { wavs.push({ name: name, buf: buf }); $('n-audio').textContent = wavs.length; renderAudio(); updateEmpty(); } };
})();
