// Microsoft ADPCM (WAVE_FORMAT_ADPCM, 0x0002) decoder for the browser.
//
// LEGOLAND's Speech/*.wav are mono 22050 Hz 4-bit MS-ADPCM, which WebAudio's
// decodeAudioData cannot handle natively. This decodes a RIFF/WAVE ADPCM
// ArrayBuffer to Float32 PCM suitable for an AudioBuffer.
//
// Exposes window.LLAdpcm.decode(arrayBuffer) -> { sampleRate, channels, pcm:[Float32Array per ch] }.

(function () {
  'use strict';

  var ADAPT = [230, 230, 230, 230, 307, 409, 512, 614,
               768, 614, 512, 409, 307, 230, 230, 230];

  function clamp16(v) { return v < -32768 ? -32768 : v > 32767 ? 32767 : v; }

  function findChunk(dv, start, end, id) {
    var p = start;
    while (p + 8 <= end) {
      var cid = String.fromCharCode(dv.getUint8(p), dv.getUint8(p + 1), dv.getUint8(p + 2), dv.getUint8(p + 3));
      var sz = dv.getUint32(p + 4, true);
      if (cid === id) return { off: p + 8, size: sz };
      p += 8 + sz + (sz & 1);
    }
    return null;
  }

  function decode(buf) {
    var dv = new DataView(buf);
    if (String.fromCharCode(dv.getUint8(0), dv.getUint8(1), dv.getUint8(2), dv.getUint8(3)) !== 'RIFF')
      throw new Error('not a RIFF file');
    var end = 8 + dv.getUint32(4, true);
    var fmt = findChunk(dv, 12, end, 'fmt ');
    var data = findChunk(dv, 12, end, 'data');
    if (!fmt || !data) throw new Error('missing fmt/data');

    var tag = dv.getUint16(fmt.off, true);
    var channels = dv.getUint16(fmt.off + 2, true);
    var sampleRate = dv.getUint32(fmt.off + 4, true);
    var blockAlign = dv.getUint16(fmt.off + 12, true);

    if (tag === 0x0001) {                    // plain PCM, just in case
      var bits = dv.getUint16(fmt.off + 14, true);
      return decodePcm(dv, data, channels, sampleRate, bits);
    }
    if (tag !== 0x0002) throw new Error('unsupported WAVE format 0x' + tag.toString(16));

    var samplesPerBlock = dv.getUint16(fmt.off + 18, true);
    var numCoef = dv.getUint16(fmt.off + 20, true);
    var coef = [];
    for (var i = 0; i < numCoef; i++) {
      coef.push([dv.getInt16(fmt.off + 22 + i * 4, true), dv.getInt16(fmt.off + 24 + i * 4, true)]);
    }

    var out = [];
    for (var c = 0; c < channels; c++) out.push([]);

    var p = data.off, dataEnd = data.off + data.size;
    while (p + blockAlign <= dataEnd) {
      decodeBlock(dv, p, channels, samplesPerBlock, coef, out);
      p += blockAlign;
    }

    var pcm = out.map(function (arr) {
      var f = new Float32Array(arr.length);
      for (var k = 0; k < arr.length; k++) f[k] = arr[k] / 32768;
      return f;
    });
    return { sampleRate: sampleRate, channels: channels, pcm: pcm };
  }

  function decodeBlock(dv, p, channels, samplesPerBlock, coef, out) {
    var predictor = [], delta = [], s1 = [], s2 = [], coef1 = [], coef2 = [];
    var off = p;
    for (var c = 0; c < channels; c++) { var bp = dv.getUint8(off++); predictor[c] = bp; coef1[c] = coef[bp][0]; coef2[c] = coef[bp][1]; }
    for (c = 0; c < channels; c++) { delta[c] = dv.getInt16(off, true); off += 2; }
    for (c = 0; c < channels; c++) { s1[c] = dv.getInt16(off, true); off += 2; }
    for (c = 0; c < channels; c++) { s2[c] = dv.getInt16(off, true); off += 2; }
    // preamble: sample2 then sample1
    for (c = 0; c < channels; c++) { out[c].push(s2[c]); out[c].push(s1[c]); }

    var remaining = samplesPerBlock - 2;
    var ch = 0;
    var haveHi = false, hiNib = 0;
    for (var n = 0; n < remaining * channels; n++) {
      var nib;
      if (!haveHi) { var byte = dv.getUint8(off++); hiNib = byte & 0x0f; nib = (byte >> 4) & 0x0f; haveHi = true; }
      else { nib = hiNib; haveHi = false; }
      var err = nib >= 8 ? nib - 16 : nib;                 // signed 4-bit
      var pred = (s1[ch] * coef1[ch] + s2[ch] * coef2[ch]) >> 8;
      var val = clamp16(pred + delta[ch] * err);
      out[ch].push(val);
      var nd = (ADAPT[nib] * delta[ch]) >> 8;
      delta[ch] = nd < 16 ? 16 : nd;
      s2[ch] = s1[ch]; s1[ch] = val;
      ch = (ch + 1) % channels;
    }
  }

  function decodePcm(dv, data, channels, sampleRate, bits) {
    var out = []; for (var c = 0; c < channels; c++) out.push([]);
    var p = data.off, dataEnd = data.off + data.size, ch = 0;
    while (p + (bits >> 3) <= dataEnd) {
      var v = bits === 16 ? dv.getInt16(p, true) / 32768 : (dv.getUint8(p) - 128) / 128;
      out[ch].push(v); p += bits >> 3; ch = (ch + 1) % channels;
    }
    return { sampleRate: sampleRate, channels: channels, pcm: out.map(function (a) { return Float32Array.from(a); }) };
  }

  window.LLAdpcm = { decode: decode };
})();
