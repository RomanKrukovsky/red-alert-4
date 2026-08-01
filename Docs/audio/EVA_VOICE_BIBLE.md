# EVA Voice Bible — Red Alert 4

**Version:** 1.0.0  
**Status:** Approved Specification  
**Author:** Game Audio Integration & Voice Direction Team  
**Target Engine:** Unreal Engine 5 MetaSounds & VoxCPM2 Pipeline  

---

## 1. Executive Summary & Vision

The **EVA (Electronic Video Assistant/Onboard AI Communications)** voiceover system in *Red Alert 4* provides high-clarity tactical situational awareness to the player during high-intensity RTS gameplay.
Key audio design goals:
* **Instant Information Transfer:** Clear, unambiguous Russian vocal delivery capable of cutting through dense combat noise (explosions, gunfire, engine roars).
* **Faction Identity:** Each faction features a unique EVA persona reflecting its ideological, technological, and aesthetic character (Soviet industrial authority, Allied high-tech precision, Coalition strategic honor, Chronolegion quantum detachment).
* **Zero Vocal Fatigue:** Carefully tuned pitch, formant structures, and EQ notches to prevent listener fatigue across multi-hour gameplay sessions.
* **Pure Clean Generation:** Synthesized dry at source via VoxCPM2 (48 kHz mono 16-bit PCM), with all environmental/radio FX, spatialization, and sidechain ducking processed dynamically in-engine via UE MetaSounds.

---

## 2. Faction EVA Profiles

### 2.1 Soviet EVA (`EVA_Soviet`)

* **Archetype:** Authoritative mature female military command commissar / AI contralto.
* **Persona:** Unyielding, icy-calm, bureaucratically precise, strictly disciplined under heavy fire. Never panics.
* **Control Instruction (VoxCPM2):**  
  > `"Authoritative mature female military command voice, low contralto, precise Russian diction, restrained emotion, firm concise delivery, calm under pressure."`

| Parameter | Specification |
|---|---|
| **Vocal Range / Pitch** | Low Contralto (160 Hz – 185 Hz fundamental pitch) |
| **Diction & Cadence** | Deliberate, crisp articulation, strict military cadence, firm stops on consonants (`t`, `k`, `r`) || **Forbidden Styles** | High-pitched squeaks, panicky shouting, casual slang, warm maternal tone, theatrical anime exaggeration |
| **Pronunciation Nuances** | Formal Soviet military Russian, clear stress on command verbs (`Construction completed`, `Base under attack`) |
#### DSP Processing Chain (MetaSounds)
1. **High-Pass Filter:** 100 Hz Butterworth 2nd order (cuts low rumble).
2. **Parametric EQ:** +2.0 dB shelf at 4.5 kHz for speech clarity; sharp -3.0 dB notch at 3.2 kHz to avoid harshness.
3. **Compression:** Optical-style compressor (Ratio 3.5:1, Attack 10ms, Release 120ms, Target Gain Reduction 3-5 dB).
4. **Vocoder / Radio Layer:** Subtle analog bandpass filter (300 Hz – 3.4 kHz) blended in parallel at **12% wet**, providing a subtle militarized radio texture.
5. **Spatialization & Bus:** 2D stereo center focus; routed to `Submix_VO_EVA_Soviet`.
6. **Sidechain Ducking:** Ducks `Submix_Music` by **-6 dB** (15ms attack, 200ms release) and `Submix_SFX_Explosions` by **-3 dB**.

---

### 2.2 Allied EVA (`EVA_Alliance`)

* **Archetype:** Next-generation tactical battle network AI (Athena / AI Commander).
* **Persona:** High-tech, intelligent, confident, modern, crisp, slightly warm yet strictly professional.
* **Control Instruction (VoxCPM2):**  
  > `"Professional female tactical AI voice, clean and intelligent, confident, modern, precise, slightly warm, medium-fast pace, excellent Russian diction."`

| Parameter | Specification |
|---|---|
| **Vocal Range / Pitch** | Crisp Mezzo-Soprano (210 Hz – 235 Hz fundamental pitch) |
| **Diction & Cadence** | Medium-fast pace, fluid technical Russian delivery, perfect clarity on acronyms and codes (`MCV`, `T3`, `GPS`) |
| **Forbidden Styles** | Monotone robotic text-to-speech, heavy regional accents, aggressive shouting, breathy hesitations |
| **Pronunciation Nuances** | Standard neutral literary Russian, modern technical terminology |

#### DSP Processing Chain (MetaSounds)
1. **High-Pass Filter:** 80 Hz 2nd order HPF.
2. **Parametric EQ:** +1.5 dB air shelf at 10 kHz; smooth neutral midrange.
3. **Multiband Compressor:** 4-band digital compressor (Ratio 2.5:1, fast attack 5ms, transparent gain leveling).
4. **Digital De-Esser:** Dynamic threshold de-esser targeting 6.8 kHz – 8.5 kHz sibilance.
5. **Spatialization & Reverb:** Wide digital stereo image with **5% wet** 40ms pre-delayed crystal room impulse response.
6. **Sidechain Ducking:** Ducks `Submix_Music` by **-6 dB** and `Submix_SFX` by **-2 dB**.

---

### 2.3 Coalition EVA (`EVA_Coalition`)

* **Archetype:** Ceremonial & disciplined strategic defense grid controller (Empire / Eastern Coalition).
* **Persona:** Elegant, solemn, honor-bound, measured pace, imperial weight, restrained strategic authority.
* **Control Instruction (VoxCPM2):**  
  > `"Calm disciplined female strategic command voice, elegant and precise, controlled emotion, measured pace, subtle authority, clear standard Russian."`

| Parameter | Specification |
|---|---|
| **Vocal Range / Pitch** | Low-Mid Alto (175 Hz – 200 Hz fundamental pitch) |
| **Diction & Cadence** | Measured, rhythmic cadence, extended vowels, grand formal phrasing |
| **Forbidden Styles** | Rapid informal chatter, casual colloquialisms, chaotic shouting, vocal fry |
| **Pronunciation Nuances** | Formal Russian with solemn stress emphasis on unit readiness and honorific titles |

#### DSP Processing Chain (MetaSounds)
1. **High-Pass Filter:** 90 Hz 2nd order HPF.
2. **Saturation:** Subtle 2nd-harmonic tube warmth (0.5% THD drive) for rich vocal body.
3. **Opto Leveler:** Gentle leveling amplifier (Ratio 2:1, slow release 300ms).
4. **Sub-Harmonic Enhancer:** Sub-bass reinforcement between 70 Hz – 120 Hz (-18 dB wet).
5. **Convolution Reverb:** Hardened command bunker impulse response (**8% wet**).
6. **Sidechain Ducking:** Ducks `Submix_Music` by **-7 dB** and `Submix_SFX` by **-4 dB**.

---

### 2.4 Chrono EVA (`EVA_Chrono`)

* **Archetype:** Quantum temporal tactical processor / Unsanctioned AI core.
* **Persona:** Timeless androgynous quantum computer. Unsettlingly calm, exact micro-pauses, detached observer.
* **Control Instruction (VoxCPM2):**  
  > `"Androgynous timeless command voice, calm and unsettling, extremely precise, restrained emotion, deliberate micro-pauses, clear Russian, no baked-in audio effects."`

| Parameter | Specification |
|---|---|
| **Vocal Range / Pitch** | Androgynous Mid-Register (190 Hz – 200 Hz flat, zero pitch drift) |
| **Diction & Cadence** | Quantized timing, micro-pauses between clause boundaries, absolute steady pitch contour |
| **Forbidden Styles** | Human breath intake, emotional inflection, warmth, anger, casual pitch glides |
| **Pronunciation Nuances** | Mathematical, analytical Russian articulation without natural vocal micro-tremors |

#### DSP Processing Chain (MetaSounds)
1. **High-Pass Filter:** 120 Hz 4th order HPF.
2. **Pitch Stabilizer:** Zero-latency pitch correction forcing flat pitch center on active formants.
3. **Stereo Phase Chorus:** Dual-tap short delay (12ms / 18ms, 15% wet) creating quantum phase dispersion.
4. **Sub-Octave Synthesizer:** Transposed pitch layer (-12 semitones blended at **-18 dB**).
5. **Sidechain Ducking:** Ducks `Submix_Music` by **-8 dB** and `Submix_SFX` by **-4 dB**.

---

## 3. Pronunciation & Diction Rules

To maintain absolute voice continuity across synthesized assets, all script inputs must comply with `Content/RA4/Audio/Generated/eva_pronunciation_ru.json`.

### Key Dictionary Rules:
1. **Tech Tiers:** `T1`, `T2`, `T3` ​​expanded to `"Tier one"`, `"Tier two"`, `"Tier three"`.
2. **Acronyms:** `MCV` expanded to `"MCV"`, `EMP` to `"Electromagnetic pulse"`, `GPS` to `"GPS"`.
3. **Numbers & Quantities:** `100%` expanded to `"one hundred percent"`, `15s` expanded to `"fifteen seconds"`.
4. **Callsigns:** `Alpha-1` pronounced as `"Alpha one"`, `Bravo-2` as `"Bravo two"`.
5. **Faction Names:** `USSR` spoken as `"Soviet Union"` (or `"SS-SR"` in shorthand tactical alerts).
---

## 4. MetaSounds Integration & Dynamic Priority

All generated EVA audio files are triggered in Unreal Engine through the `MS_EVA_VoicePlayer` MetaSound asset.

```
[ Gameplay Event ] 
        │
        ▼
[ EVA Runtime System ] ──(Check Priority & Cooldown)──► [ Concurrency Group Gate ]
                                                                   │
                                                                   ▼
[ Sidechain Bus Ducking ] ◄─────────────────────────── [ MS_EVA_VoicePlayer ]
```

* **Priorities:** Defined in `eva_runtime_policy.json` (Range: 100 Strategic to 35 Production).
* **Preemption:** Higher priority events interrupt active lower priority clips with a **50ms crossfade**.
* **Cooldown Enforcement:** Prevents duplicate spam (e.g. `BASE_UNDER_ATTACK` locked for 12s per region).