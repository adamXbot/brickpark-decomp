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

  // Find a member (sprite/tile) by name across ALL loaded archives.
  function findMember(name) {
    var ln = name.toLowerCase();
    for (var an in archives) {
      var a = archives[an];
      if (!a._byName) { a._byName = {}; a.members.forEach(function (mm) { a._byName[mm.name.toLowerCase()] = mm; }); }
      if (a._byName[ln]) return { arch: an, member: a._byName[ln] };
    }
    return null;
  }
  // Decode a tile/sprite by name -> canvas (cached).
  function spriteCanvas(name) {
    var key = 'n:' + name.toLowerCase();
    if (key in decodeCache) return decodeCache[key];
    var f = findMember(name);
    var cv = f ? decodeToCanvas(f.arch, f.member) : null;
    decodeCache[key] = cv || null;
    return decodeCache[key];
  }

  // Resolve a level's tile codes -> sprite names via its TSM/TSF chain in
  // Legoland.res. Returns { codeToName, groundName, path:[names] }.
  function resolveTiles(m, legoBytes) {
    if (!window.LLTiles) return null;
    var idx = window.LLTiles.indexRes(legoBytes);
    function mem(name) { var k = name.toLowerCase(); return idx[k] ? legoBytes.subarray(idx[k].off, idx[k].off + idx[k].size) : null; }
    var codeToName = {};
    var tsmB = mem(m.tsm_mapping + '.TSM');
    if (tsmB) {
      window.LLTiles.parseTSM(tsmB).forEach(function (e) {
        var tsfB = mem(e.tileset + '.TSF');
        if (tsfB) {
          var tsf = window.LLTiles.parseTSF(tsfB);
          tsf.codes.forEach(function (c, i) { if (!(c in codeToName)) codeToName[c] = tsf.images[i]; });
        }
      });
    }
    // ground = tile with the lowest code (the flat base tile)
    var codes = Object.keys(codeToName).map(Number).sort(function (a, b) { return a - b; });
    var groundName = codes.length ? codeToName[codes[0]] : null;
    // path tiles (indexed by position, code field is 0)
    var pathNames = [];
    m.path_tilesets.forEach(function (pn) {
      var tsfB = mem(pn + '.TSF') || mem('NORMPATH.TSF');
      if (tsfB) pathNames = window.LLTiles.parseTSF(tsfB).images;
    });
    return { codeToName: codeToName, groundName: groundName, path: pathNames };
  }

  // Resolve an object class -> a drawable: {kind:'lls', cv} or {kind:'csp', parts}.
  function resolveObjectSprite(legoBytes, idx, cls) {
    var k = (cls + '.ODF').toLowerCase();
    if (!idx[k]) return null;
    var odf = window.LLTiles.parseODF(legoBytes.subarray(idx[k].off, idx[k].off + idx[k].size));
    if (!odf.sprite) return null;
    if (/\.LLS$/i.test(odf.sprite)) {
      var cv = spriteCanvas(odf.sprite);
      return cv ? { kind: 'lls', cv: cv } : null;
    }
    if (/\.CSP$/i.test(odf.sprite)) {
      var ck = odf.sprite.toLowerCase();
      if (!idx[ck]) return null;
      var csp = window.LLTiles.parseCSP(legoBytes.subarray(idx[ck].off, idx[ck].off + idx[ck].size));
      var parts = [];
      csp.parts.forEach(function (pt) {
        if (!pt.image) return;
        var pc = spriteCanvas(pt.image);
        if (pc) parts.push({ cv: pc, dx: pt.dx, dy: pt.dy });
      });
      return parts.length ? { kind: 'csp', parts: parts } : null;
    }
    return null;
  }

  // Build the full-park offscreen render for a level (native tile size).
  function buildLevelOffscreen(lv) {
    var legoBytes = archives[lv.arch].bytes;
    var m = window.LLLevel.parse(legoBytes.subarray(lv.member.offset, lv.member.offset + lv.member.size));
    var res = resolveTiles(m, legoBytes);
    var haveGfx = !!(res && res.groundName && findMember(res.groundName));

    var head = $('levelHead');
    var classes = m.object_classes.join(' · ') || '(none)';
    var note = haveGfx ? '' : ' <span style="color:var(--stud)">— load Graphics2.res for tile art</span>';
    head.innerHTML = '<span style="font-family:var(--head);font-weight:700;font-size:18px">' + m.name + '</span>' +
      ' <span class="muted">' + m.width + '×' + m.height + ' · ' + m.terrain + ' · ' +
      m.objects.length + ' objects · scroll to zoom, drag to pan</span>' + note +
      '<div class="muted" style="margin-top:3px;font-size:11px">classes: ' + classes + '</div>';

    var w = m.width, h = m.height, TW = 32, TH = 16;
    var isoW = (w + h) * (TW / 2) + TW;
    var isoH = (w + h) * (TH / 2) + TH * 10;
    var off = document.createElement('canvas');
    off.width = Math.min(isoW, 16384); off.height = Math.min(isoH, 16384);
    var octx = off.getContext('2d');
    var ox = h * (TW / 2), oy = TH * 8;
    function sx(x, y) { return ox + (x - y) * (TW / 2); }
    function sy(x, y) { return oy + (x + y) * (TH / 2); }

    var groundCv = res && res.groundName ? spriteCanvas(res.groundName) : null;
    var codeCv = {};
    if (res) Object.keys(res.codeToName).forEach(function (c) { codeCv[c] = spriteCanvas(res.codeToName[c]); });

    if (!groundCv) {
      octx.fillStyle = '#2f4a2a';
      for (var yy = 0; yy < h; yy++) for (var xx = 0; xx < w; xx++) {
        var p0x = sx(xx, yy), p0y = sy(xx, yy);
        octx.beginPath(); octx.moveTo(p0x, p0y); octx.lineTo(p0x + TW / 2, p0y + TH / 2);
        octx.lineTo(p0x, p0y + TH); octx.lineTo(p0x - TW / 2, p0y + TH / 2); octx.closePath(); octx.fill();
      }
    } else {
      for (var y = 0; y < h; y++) {
        for (var x = 0; x < w; x++) {
          var v = m.tileGfx[y * w + x];
          var cv = (v >= 0 && codeCv[v]) ? codeCv[v] : groundCv;
          if (cv) octx.drawImage(cv, sx(x, y) - TW / 2, sy(x, y) + TH - cv.height);
        }
      }
    }

    // object sprites (resolved once per class), back-to-front
    var classSprite = {};
    if (window.LLTiles) {
      var idx = window.LLTiles.indexRes(legoBytes);
      m.object_classes.forEach(function (cls, ci) { classSprite[ci] = resolveObjectSprite(legoBytes, idx, cls); });
    }
    var objPal = ['#ff3b30', '#ffcf3f', '#2ea3f2', '#3ec46d', '#ff7ac2', '#b07cff', '#ff9f0a'];
    m.objects.slice().sort(function (a, b) { return (a.x + a.y) - (b.x + b.y); }).forEach(function (o) {
      var d = classSprite[o.cls];
      var px = sx(o.x, o.y), py = sy(o.x, o.y) + TH;
      if (d && d.kind === 'lls') {
        octx.drawImage(d.cv, px - d.cv.width / 2, py - d.cv.height);
      } else if (d && d.kind === 'csp') {
        d.parts.forEach(function (pt) { octx.drawImage(pt.cv, px + pt.dx, py + pt.dy - pt.cv.height); });
      } else {
        octx.fillStyle = objPal[o.cls % objPal.length];
        octx.strokeStyle = 'rgba(0,0,0,.5)'; octx.lineWidth = 1;
        octx.beginPath(); octx.arc(px, py - TH / 2, 4, 0, 6.29); octx.fill(); octx.stroke();
      }
    });
    return off;
  }

  var levelView = { off: null, zoom: 1, panX: 0, panY: 0 };

  function drawLevel(lv) {
    levelView.off = buildLevelOffscreen(lv);
    // default view: fit whole park
    var cv2 = $('levelCanvas');
    var boxr = cv2.parentElement.getBoundingClientRect();
    var W = Math.max(400, Math.floor(boxr.width)), H = Math.max(300, Math.floor(boxr.height));
    levelView.zoom = Math.min((W - 20) / levelView.off.width, (H - 20) / levelView.off.height);
    levelView.panX = (W - levelView.off.width * levelView.zoom) / 2;
    levelView.panY = (H - levelView.off.height * levelView.zoom) / 2;
    blitLevel();
  }

  function blitLevel() {
    var off = levelView.off; if (!off) return;
    var cv2 = $('levelCanvas');
    var boxr = cv2.parentElement.getBoundingClientRect();
    var W = Math.max(400, Math.floor(boxr.width)), H = Math.max(300, Math.floor(boxr.height));
    var dpr = window.devicePixelRatio || 1;
    cv2.width = W * dpr; cv2.height = H * dpr;
    var ctx = cv2.getContext('2d'); ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
    ctx.imageSmoothingEnabled = false;
    ctx.clearRect(0, 0, W, H);
    ctx.drawImage(off, levelView.panX, levelView.panY, off.width * levelView.zoom, off.height * levelView.zoom);
  }

  function initLevelView() {
    var cv = $('levelCanvas');
    cv.addEventListener('wheel', function (e) {
      if (!levelView.off) return;
      e.preventDefault();
      var r = cv.getBoundingClientRect();
      var mx = e.clientX - r.left, my = e.clientY - r.top;
      var factor = e.deltaY < 0 ? 1.15 : 1 / 1.15;
      var nz = Math.max(0.05, Math.min(8, levelView.zoom * factor));
      // zoom around cursor
      levelView.panX = mx - (mx - levelView.panX) * (nz / levelView.zoom);
      levelView.panY = my - (my - levelView.panY) * (nz / levelView.zoom);
      levelView.zoom = nz;
      blitLevel();
    }, { passive: false });
    var drag = null;
    cv.addEventListener('mousedown', function (e) { drag = { x: e.clientX, y: e.clientY, px: levelView.panX, py: levelView.panY }; });
    window.addEventListener('mousemove', function (e) {
      if (!drag) return;
      levelView.panX = drag.px + (e.clientX - drag.x);
      levelView.panY = drag.py + (e.clientY - drag.y);
      blitLevel();
    });
    window.addEventListener('mouseup', function () { drag = null; });
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
    initLevelView();
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
