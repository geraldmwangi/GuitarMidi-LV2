"""
Guitar-Optimized MIDI Chord and Scale Analyzer
Handles guitar-specific realities: bass note detection, doubled notes,
omitted intervals (no 5th, no 3rd), open-string drones, and common
alternate tunings.
"""

from collections import Counter

NOTE_NAMES = ['C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B']

# ============================================================
# CHORD FORMULAS - ordered by how common they are on guitar
# ============================================================
CHORD_FORMULAS = {
    # Most common guitar chords first (helps tie-breaking)
    '13':                   [0, 4, 7, 10, 14, 21],

    '9':                    [0, 4, 7, 10, 14],
    'major9':               [0, 4, 7, 11, 14],
    'minor9':               [0, 3, 7, 10, 14],  
    '7#9':                  [0, 4, 7, 10, 15],  # "Hendrix chord"
    '7b9':                  [0, 4, 7, 10, 13],
    'major7':               [0, 4, 7, 11],
    'dominant7':            [0, 4, 7, 10],
    'minor7':               [0, 3, 7, 10],
    'add9':                 [0, 4, 7, 14],
    'minorAdd9':            [0, 3, 7, 14],
    'major6':               [0, 4, 7, 9],
    'minor6':               [0, 3, 7, 9],  
    'diminished7':          [0, 3, 6, 9],
    'minor7b5':             [0, 3, 6, 10],  
    'minorMajor7':          [0, 3, 7, 11],
    'sus4_7':               [0, 5, 7, 10],
    'sus2_7':               [0, 2, 7, 10],

    'augmented':            [0, 4, 8],
    'diminished':           [0, 3, 6],
    'no3_add4':             [0, 5, 7],  # ambiguous sus4/no-3rd voicing
    'major':                [0, 4, 7],
    'minor':                [0, 3, 7],
   # 'sus4':                 [0, 5, 7],
    'sus2':                 [0, 2, 7],

    'power_chord':          [0, 7],          # very common on guitar (no 3rd)
}

# Common guitar tunings (low to high string), used for open-note context
TUNINGS = {
    'standard':  [40, 45, 50, 55, 59, 64],       # E A D G B E
    'drop_d':    [38, 45, 50, 55, 59, 64],
    'drop_c':    [36, 43, 48, 53, 57, 62],
    'half_step_down': [39, 44, 49, 54, 58, 63],
    'open_g':    [38, 43, 50, 55, 59, 62],       # D G D G B D
    'open_d':    [38, 45, 50, 54, 57, 62],       # D A D F# A D
    'drop_a#':    [34, 41, 46, 51, 55, 60],
}

SCALE_FORMULAS = {
    'Major (Ionian)':          [0, 2, 4, 5, 7, 9, 11],
    'Natural Minor (Aeolian)': [0, 2, 3, 5, 7, 8, 10],
    'Harmonic Minor':          [0, 2, 3, 5, 7, 8, 11],
    'Melodic Minor':           [0, 2, 3, 5, 7, 9, 11],
    'Dorian':                  [0, 2, 3, 5, 7, 9, 10],
    'Phrygian':                [0, 1, 3, 5, 7, 8, 10],
    'Lydian':                  [0, 2, 4, 6, 7, 9, 11],
    'Mixolydian':              [0, 2, 4, 5, 7, 9, 10],
    'Locrian':                 [0, 1, 3, 5, 6, 8, 10],
    'Major Pentatonic':        [0, 2, 4, 7, 9],
    'Minor Pentatonic':        [0, 3, 5, 7, 10],
    'Blues':                   [0, 3, 5, 6, 7, 10],
}


class GuitarChordAnalyzer:
    def __init__(self, midi_notes, tuning='standard', assume_lowest_is_bass=True):
        """
        Args:
            midi_notes: list of MIDI note numbers, e.g. [40, 47, 52, 56, 59, 64]
                        (as would come from a guitar's actual played strings)
            tuning: name of tuning used, for open-string awareness (optional)
            assume_lowest_is_bass: guitarists almost always play the bass note
                        lowest, so we prioritize that as the root/inversion bass
        """
        if not (2 <= len(midi_notes) <= 6):
            raise ValueError("Please provide between 2 and 6 MIDI notes.")

        self.midi_notes = sorted(midi_notes)
        self.tuning = TUNINGS.get(tuning, TUNINGS['standard'])
        self.assume_lowest_is_bass = assume_lowest_is_bass

        # Pitch classes preserving duplicate count (guitarists double notes a lot,
        # e.g. open chords often double the root/5th across strings)
        self.pc_counts = Counter(n % 12 for n in self.midi_notes)
        self.pitch_classes = sorted(self.pc_counts.keys())
        self.bass_note = self.midi_notes[0]
        self.bass_pc = self.bass_note % 12

    @staticmethod
    def note_name(pc):
        return NOTE_NAMES[pc % 12]

    @staticmethod
    def midi_to_name(m):
        return f"{NOTE_NAMES[m % 12]}{m // 12 - 1}"

    # -----------------------------------------------------
    # CHORD DETECTION (guitar-aware)
    # -----------------------------------------------------
    def identify_chords(self):
        """
        Guitar-specific logic:
        1. Try the bass note (lowest played pitch) as root FIRST — this is how
           guitarists think ("that's a G chord") even in inversions.
        2. Also scan all other pitch classes as potential roots for slash-chord
           detection (e.g. C/E, D/F#).
        3. Weight/prefer common open-chord shapes (major, minor, sus2/4, power).
        """
        results = []
        pcs = set(self.pitch_classes)
        n_distinct = len(pcs)

        candidate_roots = [self.bass_pc] + [pc for pc in self.pitch_classes if pc != self.bass_pc]

        for root in candidate_roots:
            intervals = sorted((pc - root) % 12 for pc in pcs)
            interval_set = set(intervals)

            for chord_name, formula in CHORD_FORMULAS.items():
                formula_set = set(i % 12 for i in formula)

                if interval_set == formula_set:
                    is_bass_root = (root == self.bass_pc)
                    results.append({
                        'root': self.note_name(root),
                        'chord': chord_name,
                        'bass': self.note_name(self.bass_pc),
                        'is_root_position': is_bass_root,
                        'match': 'exact',
                        'label': self._format_label(root, chord_name, is_bass_root)
                    })
                elif interval_set.issubset(formula_set):
                    missing = formula_set - interval_set
                    # Guitar-friendly: flag if it's just missing the 5th (very common,
                    # since the 5th is often dropped/muted) or missing 3rd (power chord parent)
                    note = None
                    if missing == {7}:
                        note = "5th omitted (common on guitar)"
                    elif missing in ({3}, {4}):
                        note = "3rd omitted — ambiguous major/minor (could be power chord)"
                    is_bass_root = (root == self.bass_pc)
                    results.append({
                        'root': self.note_name(root),
                        'chord': chord_name,
                        'bass': self.note_name(self.bass_pc),
                        'is_root_position': is_bass_root,
                        'match': f'partial{f" - {note}" if note else ""}',
                        'label': self._format_label(root, chord_name, is_bass_root)
                    })

        # Ranking: exact + root-position (bass note = chord root) wins,
        # since that's overwhelmingly how guitar chords are voiced
        def rank(r):
            return (
                0 if r['match'] == 'exact' else 1,
                0 if r['is_root_position'] else 1,
            )
        results.sort(key=rank)
        return results

    def _format_label(self, root, chord_name, is_root_position):
        root_name = self.note_name(root)
        suffix = {
            'major': '', 'minor': 'm', 'sus4': 'sus4', 'sus2': 'sus2',
            'power_chord': '5', 'major7': 'maj7', 'dominant7': '7',
            'minor7': 'm7', 'add9': 'add9', 'minorAdd9': 'm(add9)',
            'major6': '6', 'minor6': 'm6', 'diminished': 'dim',
            'diminished7': 'dim7', 'minor7b5': 'm7b5', 'augmented': 'aug',
            'minorMajor7': 'm(maj7)', 'sus4_7': '7sus4', 'sus2_7': '7sus2',
            '9': '9', 'major9': 'maj9', 'minor9': 'm9', '13': '13',
            '7#9': '7#9', '7b9': '7b9', 'no3_add4': '(no3)add4'
        }.get(chord_name, chord_name)

        label = f"{root_name}{suffix}"
        if not is_root_position:
            label += f"/{self.note_name(self.bass_pc)}"
        return label

    # -----------------------------------------------------
    # SCALE DETECTION
    # -----------------------------------------------------
    def identify_scales(self, top_n=8):
        results = []
        pcs = set(self.pitch_classes)

        for root in range(12):
            for scale_name, formula in SCALE_FORMULAS.items():
                scale_pcs = set((root + i) % 12 for i in formula)
                if pcs.issubset(scale_pcs):
                    results.append({
                        'root': self.note_name(root),
                        'scale': scale_name,
                        'scale_size': len(scale_pcs),
                    })

        results.sort(key=lambda r: (r['scale_size'], r['root']))
        return results[:top_n]

    # -----------------------------------------------------
    # FULL REPORT
    # -----------------------------------------------------
    def analyze(self, verbose=True):
        chords = self.identify_chords()
        scales = self.identify_scales()

        best_chord = chords[0] if chords else None

        report = {
            'input_notes': [self.midi_to_name(n) for n in self.midi_notes],
            'bass_note': self.midi_to_name(self.bass_note),
            'best_guess': best_chord['label'] if best_chord else 'Unknown',
            'chords': chords,
            'scales': scales,
        }

        if verbose:
            self._print_report(report)

        return report

    def _print_report(self, report):
        print("=" * 60)
        print(f"NOTES PLAYED: {', '.join(report['input_notes'])}")
        print(f"BASS NOTE (lowest string): {report['bass_note']}")
        print(f"BEST GUESS: {report['best_guess']}")
        print("=" * 60)

        print("\n--- ALL CHORD MATCHES ---")
        exact = [c for c in report['chords'] if c['match'] == 'exact']
        partial = [c for c in report['chords'] if c['match'] != 'exact']

        if exact:
            print("Exact matches (best first):")
            for c in exact[:6]:
                marker = "  <-- root position" if c['is_root_position'] else " (inversion/slash)"
                print(f"  {c['label']:12s}{marker}")

        if partial:
            print("\nPartial / ambiguous matches:")
            for c in partial[:5]:
                print(f"  {c['label']:12s} [{c['match']}]")

        print("\n--- COMPATIBLE SCALES ---")
        for s in report['scales']:
            print(f"  {s['root']} {s['scale']} ({s['scale_size']} notes)")

        print("=" * 60)

    # -----------------------------------------------------
    # CHORD QUALITY WITHOUT ROOT
    # -----------------------------------------------------
    def get_chord_suffix(self, prefer_root_position=True, style='short'):
        """
        Returns the chord quality/suffix only, stripped of the root note name.

        Args:
            prefer_root_position: if True, only considers matches where the
                bass note is the actual chord root (ignores slash chords/
                inversions when picking the "best" suffix). Falls back to
                best available match if no root-position match exists.
            style: 'short' -> "m7", "", "sus4", "5"   (standard chord chart notation)
                   'long'  -> "minor7", "major", "sus4", "power_chord" (explicit words)

        Returns:
            dict with:
                'suffix': str, e.g. "m7" or "minor7" depending on style
                'raw_name': the internal formula key, e.g. "minor7"
                'root': the root note name this suffix is relative to
                'is_slash_chord': bool
                'full_label': the complete chord label, for reference
        """
        chords = self.identify_chords()
        if not chords:
            return {
                'suffix': None,
                'raw_name': None,
                'root': None,
                'is_slash_chord': False,
                'full_label': 'Unknown'
            }

        exact = [c for c in chords if c['match'] == 'exact']
        pool = exact if exact else chords

        if prefer_root_position:
            root_position_matches = [c for c in pool if c['is_root_position']]
            chosen = root_position_matches[0] if root_position_matches else pool[0]
        else:
            chosen = pool[0]

        # Short-form (standard chord chart shorthand)
        suffix_map_short = {
            'major': 'major', 'minor': 'minor', 'sus4': 'sus4', 'sus2': 'sus2',
            'power_chord': '5', 'major7': 'maj7', 'dominant7': '7',
            'minor7': 'm7', 'add9': 'add9', 'minorAdd9': 'm(add9)',
            'major6': '6', 'minor6': 'm6', 'diminished': 'dim',
            'diminished7': 'dim7', 'minor7b5': 'm7b5', 'augmented': 'aug',
            'minorMajor7': 'm(maj7)', 'sus4_7': '7sus4', 'sus2_7': '7sus2',
            '9': '9', 'major9': 'maj9', 'minor9': 'm9', '13': '13',
            '7#9': '7#9', '7b9': '7b9', 'no3_add4': '(no3)add4'
        }

        # Long-form (explicit, human-readable words)
        suffix_map_long = {
            'major': 'major', 'minor': 'minor', 'sus4': 'suspended 4th',
            'sus2': 'suspended 2nd', 'power_chord': 'power chord',
            'major7': 'major 7th', 'dominant7': 'dominant 7th',
            'minor7': 'minor 7th', 'add9': 'added 9th',
            'minorAdd9': 'minor added 9th', 'major6': 'major 6th',
            'minor6': 'minor 6th', 'diminished': 'diminished',
            'diminished7': 'diminished 7th', 'minor7b5': 'half-diminished 7th',
            'augmented': 'augmented', 'minorMajor7': 'minor major 7th',
            'sus4_7': 'dominant 7 sus4', 'sus2_7': 'dominant 7 sus2',
            '9': 'dominant 9th', 'major9': 'major 9th', 'minor9': 'minor 9th',
            '13': '13th', '7#9': 'dominant 7 sharp 9', '7b9': 'dominant 7 flat 9',
            'no3_add4': 'no 3rd, added 4th'
        }

        raw_name = chosen['chord']
        suffix_map = suffix_map_long if style == 'long' else suffix_map_short
        suffix = suffix_map.get(raw_name, raw_name)

        return {
            'suffix': suffix,
            'raw_name': raw_name,
            'root': chosen['root'],
            'is_slash_chord': not chosen['is_root_position'],
            'full_label': chosen['label']
        }