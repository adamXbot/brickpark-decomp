/* LEGOLAND — the raw MIDI sequencer: the per-track event reader and the
 * 20 ms multimedia-timer tick that drives midiOutShortMsg.
 *
 * Reconstructed from original/legoland.exe (VC6 SP3, /O2 /Gy /Gd). Struct
 * field offsets, callee argument counts and global addresses are
 * load-bearing; names are ours. Types are declared LOCALLY (music.c owns
 * the loader's copies of these two records; the layouts agree).
 *
 * Scope AI (docs/SCOPE_AI_midi_path.md).
 *
 * ---------------------------------------------------------------------------
 * HOW THE SEQUENCER RUNS
 *
 * InitMIDIManager (lifecycle.c) arms a periodic timeSetEvent(20, 10, ...)
 * whose callback is MIDITimerTick. Every tick adds the file's `tempo` to the
 * file clock (+0x08) and runs UpdateMidiTrack over every track; a track that
 * still has events ahead reports 1, and when none does the file is marked not
 * playing. The file clock is in 1/256 ticks: the track compares its own event
 * time (in MIDI ticks) against `owner->time >> 8`.
 *
 * A track keeps: pos (+0x0c) into its MTrk data, running status (+0x10), the
 * absolute time of the next event (+0x14), active (+0x18, cleared by the
 * End-of-Track meta) and pending (+0x1a, "the delta-time before the next
 * event has not been read yet"). UpdateMidiTrack loops: read the delta if
 * pending, stop (return 1) if the event is still in the future, otherwise
 * consume one event:
 *
 *     0xff2f   End of Track          active = 0, pos++ (skip the length byte)
 *     0xf0/f7  SysEx                 read a one-byte length, skip that many
 *     0xff51   Set Tempo             tempo = tick_scale / ((b0 << 8) | b1);
 *                                    the third tempo byte is skipped unread
 *                                    [sic: the 24-bit us-per-quarter is used
 *                                    as its top 16 bits]
 *     0x8n 9n bn en   three-byte channel messages -> midiOutShortMsg
 *     0xcn           two-byte channel message   -> midiOutShortMsg
 *     0xan           polyphonic aftertouch      skipped (2 data bytes)
 *     0xdn           channel aftertouch         skipped (1 data byte)
 *     other 0xffnn   any other meta             read a one-byte length, skip
 *
 * ReadMidiEvent returns the status byte, or the running status (pos is
 * backed up so the data byte is re-read) when the byte has bit 7 clear, or
 * 0xff00 | type for a meta event. ReadMidiVarLen is the standard 7-bit
 * variable-length quantity.
 * ------------------------------------------------------------------------- */

/* ---- types --------------------------------------------------------------- */

typedef struct MidiFile MidiFile;

/* One MTrk chunk (0x1c bytes; music.c's MidiTrack). */
typedef struct MidiTrack {
    MidiFile*      owner;     /* +0x00 */
    int            length;    /* +0x04  bytes of data */
    unsigned char* data;      /* +0x08 */
    int            pos;       /* +0x0c  read cursor */
    int            running;   /* +0x10  running status */
    int            time;      /* +0x14  absolute tick of the next event */
    short          active;    /* +0x18  1 until End of Track */
    short          pending;   /* +0x1a  1 = delta-time still to be read */
} MidiTrack;

/* Loaded MIDI file (0x18 bytes; music.c's MidiFile). */
struct MidiFile {
    int            tick_scale; /* +0x00  division * 20000 */
    int            tempo;      /* +0x04  clock advance per 20 ms tick */
    unsigned int   time;       /* +0x08  file clock, ticks * 256 (unsigned: the
                                *        track compare is `jb`; music.c has int) */
    short          ntracks;    /* +0x0c */
    short          pad0e;      /* +0x0e */
    MidiTrack**    tracks;     /* +0x10 */
    short          playing;    /* +0x14 */
    short          pad16;      /* +0x16 */
};

/* ---- globals --------------------------------------------------------------- */

extern MidiFile* g_midi_current;   /* 0x007fd634  the sequence being played */
extern void*     g_midi_out;       /* 0x007fd638  HMIDIOUT */

__declspec(dllimport) unsigned int __stdcall midiOutShortMsg(void* hmo,
                                                             unsigned long msg); /* [0x4ab338] */

/* ---- the timer tick -------------------------------------------------------- */

/* audit.py resolves a __stdcall body only through its first-COMDAT fallback,
 * so this function must stay FIRST in the file. */
int UpdateMidiTrack(MidiTrack* t);

/* timeSetEvent callback (TIME_PERIODIC, 20 ms). Advance the file clock by
 * `tempo` and update every track; when no track has anything left the file
 * stops playing. */
// FUNCTION: LEGOLAND 0x00480570
void __stdcall MIDITimerTick(unsigned int id, unsigned int msg, unsigned long user,
                             unsigned long dw1, unsigned long dw2)
{
    MidiFile* m = g_midi_current;
    int       active = 0;
    int       i;

    if (!m)
        return;
    if (m->playing != 1)
        return;
    m->time += m->tempo;
    for (i = 0; i < m->ntracks; i++)
        active |= UpdateMidiTrack(m->tracks[i]);
    if (!active)
        m->playing = 0;
}

/* ---- the event reader ---------------------------------------------------- */

/* Standard MIDI variable-length quantity: 7 bits per byte, MSB first, bit 7
 * set on every byte but the last. */
// FUNCTION: LEGOLAND 0x004802c0
int ReadMidiVarLen(MidiTrack* t)
{
    int v = 0;
    int c;

    do {
        c = t->data[t->pos++];
        v = (v << 7) | (c & 0x7f);
    } while (c & 0x80);
    return v;
}

/* The next event's status. A data byte (bit 7 clear) means running status:
 * back the cursor up and return the remembered status. 0xff is a meta event:
 * return 0xff00 | type. Anything else is a new status, remembered. */
// FUNCTION: LEGOLAND 0x004802f0
int ReadMidiEvent(MidiTrack* t)
{
    int c = t->data[t->pos++];

    if (!(c & 0x80)) {
        t->pos--;
        return t->running;
    }
    if (c == 0xff)
        return 0xff00 | t->data[t->pos++];
    t->running = c;
    return c;
}

/* Play every event of `t` whose time has come. Returns 1 while the track
 * still has an event ahead of the file clock, 0 when it is exhausted or
 * inactive.
 *
 * Levers: the outer `switch (status)` is a four-value compare chain (0xf0,
 * 0xf7, 0xff2f, 0xff51 — binary split at 0xff2f) with the channel-message
 * `switch (status & 0xf0)` as its default; the channel sends accumulate into
 * `status` itself (`status |= byte << 8`), which keeps the message in eax
 * and the cursor in ecx. The Set-Tempo bytes must be ONE accumulator
 * (`n = byte; n = (n << 8) | byte`): two named bytes `b0`/`b1`, in any
 * order or type, put the cursor copy in eax and the first byte in ecx (18
 * strict), and let VC6 hoist `t->data` above the length-skip store. The
 * SysEx/meta skip is `n = data[pos++]; while (n-- != 0) pos++;` on an
 * unsigned counter (dead dec/inc pair, LP01). */
// FUNCTION: LEGOLAND 0x00480330
int UpdateMidiTrack(MidiTrack* t)
{
    int          status;
    unsigned int n;

    if (t->pos >= t->length)
        return 0;
    if (t->active != 1)
        return 0;
    for (;;) {
        if (t->pending != 0) {
            t->time += ReadMidiVarLen(t);
            t->pending = 0;
        }
        if ((t->owner->time >> 8) < (unsigned int)t->time)
            return 1;
        t->pending = 1;
        status = ReadMidiEvent(t);
        switch (status) {
        case 0xf0:
        case 0xf7:
            n = t->data[t->pos++];
            while (n-- != 0)
                t->pos++;
            break;
        case 0xff2f:
            t->active = 0;
            t->pos++;
            return 0;
        case 0xff51:
            t->pos++;
            n = t->data[t->pos++];
            n = (n << 8) | t->data[t->pos];
            t->pos += 2;
            t->owner->tempo = (unsigned int)t->owner->tick_scale / n;
            break;
        default:
            switch (status & 0xf0) {
            case 0x80:
            case 0x90:
            case 0xb0:
            case 0xe0:
                status |= t->data[t->pos] << 8;
                t->pos++;
                status |= t->data[t->pos] << 16;
                t->pos++;
                midiOutShortMsg(g_midi_out, status);
                break;
            case 0xa0:
                t->pos += 2;
                break;
            case 0xc0:
                status |= t->data[t->pos] << 8;
                t->pos++;
                midiOutShortMsg(g_midi_out, status);
                break;
            case 0xd0:
                t->pos++;
                break;
            default:
                if ((status & 0xff00) == 0xff00) {
                    n = t->data[t->pos++];
                    while (n-- != 0)
                        t->pos++;
                }
                break;
            }
            break;
        }
    }
}
