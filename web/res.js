// LEGOLAND .res archive reader (browser).
//
// A .res file is a single archive: a u32 directory offset at +0, member data
// packed from +4, and a hierarchical name tree at the tail. Leaf (file)
// records are:  ff ff ff ff | u32 X | u32 zero | u32 size | u32 offset | name\0
// Folder records carry a child count where files carry 0xffffffff. We recover
// the flat list of members by scanning for leaf records (robust and simple).
//
// Exposes window.LLRes.parse(arrayBuffer) -> { members: [{name,path,offset,size}], dv }.

(function () {
  'use strict';

  function cstr(bytes, start, maxLen) {
    let end = start;
    const limit = Math.min(bytes.length, start + (maxLen || 64));
    while (end < limit && bytes[end] !== 0) end++;
    let s = '';
    for (let i = start; i < end; i++) s += String.fromCharCode(bytes[i]);
    return { s: s, end: end };
  }

  function looksLikeName(s) {
    if (s.length < 1 || s.length > 40) return false;
    for (let i = 0; i < s.length; i++) {
      const c = s.charCodeAt(i);
      if (c < 0x20 || c > 0x7e) return false;
    }
    return true;
  }

  // Parse the hierarchical directory so we can attach folder paths to members.
  // The tree is a preorder walk: each node is [nameLen-ish fields][name]; a
  // folder announces a child count, a file is marked by 0xffffffff. We do a
  // tolerant two-pass: first a flat leaf scan (authoritative for offset/size),
  // then a best-effort folder-path reconstruction layered on top.
  function parse(buf) {
    const dv = new DataView(buf);
    const bytes = new Uint8Array(buf);
    const total = bytes.length;
    const dirOff = dv.getUint32(0, true);
    if (dirOff <= 4 || dirOff >= total) {
      throw new Error('not a .res archive (bad directory offset)');
    }

    const members = [];
    const seen = new Set();
    let i = dirOff;
    while (i < total - 20) {
      if (
        bytes[i] === 0xff && bytes[i + 1] === 0xff &&
        bytes[i + 2] === 0xff && bytes[i + 3] === 0xff
      ) {
        const zero = dv.getUint32(i + 8, true);
        const size = dv.getUint32(i + 12, true);
        const off = dv.getUint32(i + 16, true);
        if (zero === 0 && off > 0 && off < dirOff && size > 0 && size < dirOff) {
          const nm = cstr(bytes, i + 20, 40);
          if (looksLikeName(nm.s) && !seen.has(off)) {
            members.push({ name: nm.s, path: nm.s, offset: off, size: size });
            seen.add(off);
            i = nm.end + 1;
            continue;
          }
        }
      }
      i++;
    }
    members.sort((a, b) => a.offset - b.offset);
    return { members: members, dv: dv, bytes: bytes };
  }

  // Group members by leading folder inferred from name conventions, so the UI
  // can present a browsable tree even before full path reconstruction lands.
  function group(members) {
    const groups = {};
    for (const m of members) {
      const dot = m.name.indexOf('.');
      const base = dot > 0 ? m.name.slice(0, dot) : m.name;
      // Heuristic bucket: first alpha run of the base name.
      const key = (base.match(/^[A-Za-z]+/) || ['misc'])[0].toLowerCase();
      (groups[key] = groups[key] || []).push(m);
    }
    return groups;
  }

  window.LLRes = { parse: parse, group: group };
})();
